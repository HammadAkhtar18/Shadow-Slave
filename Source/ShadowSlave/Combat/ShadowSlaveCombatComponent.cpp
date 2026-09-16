// Copyright Epic Games, Inc. All Rights Reserved.

#include "Combat/ShadowSlaveCombatComponent.h"
#include "Combat/ShadowSlaveDamageableInterface.h"
#include "Attributes/ShadowSlaveAttributeComponent.h"
#include "Characters/ShadowSlaveCharacterBase.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"
#include "Engine/DamageEvents.h"
#include "ShadowSlave.h"

UShadowSlaveCombatComponent::UShadowSlaveCombatComponent()
{
	// Combat component relies on event-driven timers and animation notifies; no tick needed
	PrimaryComponentTick.bCanEverTick = false;

	// Light attack configuration defaults (prototype values for testing, not novel canon)
	LightAttackData.AttackType = EAttackType::Light;
	LightAttackData.Damage = 25.0f;
	LightAttackData.TraceRadius = 45.0f;
	LightAttackData.TraceDistance = 160.0f;
	LightAttackData.HitWindowDuration = 0.35f;
	LightAttackData.RecoveryDuration = 0.20f;
	LightAttackData.MaxHitsPerTarget = 1;

	// Heavy attack configuration defaults (prototype values for testing, not novel canon)
	HeavyAttackData.AttackType = EAttackType::Heavy;
	HeavyAttackData.Damage = 60.0f;
	HeavyAttackData.TraceRadius = 55.0f;
	HeavyAttackData.TraceDistance = 180.0f;
	HeavyAttackData.HitWindowDuration = 0.45f;
	HeavyAttackData.RecoveryDuration = 0.35f;
	HeavyAttackData.MaxHitsPerTarget = 1;

	// Generic dodge configuration defaults (prototype tuning values, not novel canon)
	DodgeData.StaminaCost = 20.0f;
	DodgeData.DodgeDuration = 0.35f;
	DodgeData.DodgeSpeed = 950.0f;
}

void UShadowSlaveCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	OwningCharacter = Cast<AShadowSlaveCharacterBase>(GetOwner());
}

bool UShadowSlaveCombatComponent::CanTransitionToState(ECombatState NewState) const
{
	if (CurrentCombatState == NewState)
	{
		return true;
	}

	// Dead is a terminal state; no transitions out of Dead are legal
	if (CurrentCombatState == ECombatState::Dead)
	{
		return false;
	}

	// Any living state can transition to Dead upon death
	if (NewState == ECombatState::Dead)
	{
		return true;
	}

	// Any living state can transition to Stunned (hit reaction / stagger interruption)
	if (NewState == ECombatState::Stunned)
	{
		return true;
	}

	switch (CurrentCombatState)
	{
	case ECombatState::Neutral:
		// From Neutral, the character can start an attack or a dodge
		return (NewState == ECombatState::Attacking || NewState == ECombatState::Dodging);

	case ECombatState::Attacking:
		// From Attacking, character can enter recovery, cancel into dodge, or cancel back to neutral
		return (NewState == ECombatState::Recovering || NewState == ECombatState::Dodging || NewState == ECombatState::Neutral);

	case ECombatState::Recovering:
		// From Recovering, character returns to Neutral when recovery elapses, or can dodge-cancel
		return (NewState == ECombatState::Neutral || NewState == ECombatState::Dodging);

	case ECombatState::Dodging:
		// From Dodging, character returns to Neutral when dodge elapses
		return (NewState == ECombatState::Neutral);

	case ECombatState::Stunned:
		// From Stunned, character recovers back to Neutral
		return (NewState == ECombatState::Neutral);

	default:
		return false;
	}
}

void UShadowSlaveCombatComponent::SetCombatState(ECombatState NewState)
{
	if (CurrentCombatState == NewState)
	{
		return;
	}

	if (bIsTransitioningState)
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveCombatComponent::SetCombatState - Recursive state transition rejected: %d -> %d on '%s'"),
			static_cast<int32>(CurrentCombatState),
			static_cast<int32>(NewState),
			OwningCharacter.IsValid() ? *OwningCharacter->GetName() : TEXT("Unknown"));
		return;
	}

	if (!CanTransitionToState(NewState))
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveCombatComponent::SetCombatState - Invalid state transition rejected: %d -> %d on '%s'"),
			static_cast<int32>(CurrentCombatState),
			static_cast<int32>(NewState),
			OwningCharacter.IsValid() ? *OwningCharacter->GetName() : TEXT("Unknown"));
		return;
	}

	bIsTransitioningState = true;

	const ECombatState OldState = CurrentCombatState;
	const bool bWasAttacking = (OldState == ECombatState::Attacking);
	const EAttackType EndedAttackType = ActiveAttackData.AttackType;
	const int32 EndedInstanceId = CurrentAttackInstanceId;

	// Determine if an active montage should be halted.
	// Normal attack transition to Recovering lets the montage follow-through play or blend out naturally.
	// Abrupt transitions (Stunned, Dodging, Dead, Neutral) immediately stop the active attack montage.
	// Abrupt interruptions during Dodging (Stunned, Dead) immediately stop the active dodge montage.
	UAnimMontage* MontageToStop = nullptr;
	if (bWasAttacking && NewState != ECombatState::Recovering)
	{
		MontageToStop = ActiveAttackData.AttackMontage.Get();
	}
	else if (OldState == ECombatState::Dodging && (NewState == ECombatState::Stunned || NewState == ECombatState::Dead))
	{
		MontageToStop = GetDodgeMontageForDirection(CurrentDodgeCardinalDirection);
	}
	else if (NewState == ECombatState::Dead && ActiveAttackData.AttackMontage)
	{
		MontageToStop = ActiveAttackData.AttackMontage.Get();
	}

	// 1. Clean up transient timers and hit window state
	if (bWasAttacking)
	{
		bHitWindowActive = false;
		bIsMontageDriven = false;

		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(TraceLoopTimerHandle);
			World->GetTimerManager().ClearTimer(HitWindowTimerHandle);
		}

		if (NewState != ECombatState::Recovering)
		{
			HitCountsThisAttack.Empty();
		}
	}
	else if (OldState == ECombatState::Recovering)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(RecoveryTimerHandle);
		}
	}
	else if (OldState == ECombatState::Dodging)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(DodgeTimerHandle);
		}
		RestoreDodgeMovement();
	}

	if (NewState == ECombatState::Dead)
	{
		bHitWindowActive = false;
		bIsMontageDriven = false;
		HitCountsThisAttack.Empty();

		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(TraceLoopTimerHandle);
			World->GetTimerManager().ClearTimer(HitWindowTimerHandle);
			World->GetTimerManager().ClearTimer(RecoveryTimerHandle);
			World->GetTimerManager().ClearTimer(DodgeTimerHandle);
		}
	}

	// 2. Commit authoritative state update FIRST, before any external callbacks or montage stops
	CurrentCombatState = NewState;

	// 3. Stop montage if requested. Any callback (e.g. HandleMontageEnded) will now see
	// CurrentCombatState == NewState (!= Attacking) and safely no-op without re-entrancy.
	if (MontageToStop && OwningCharacter.IsValid())
	{
		if (USkeletalMeshComponent* Mesh = OwningCharacter->GetMesh())
		{
			if (UAnimInstance* AnimInstance = Mesh->GetAnimInstance())
			{
				AnimInstance->Montage_Stop(0.1f, MontageToStop);
			}
		}
	}

	// 4. Broadcast state change delegate
	OnCombatStateChanged.Broadcast(OldState, NewState);

	// 5. Broadcast end delegates whenever leaving Attacking or Dodging states
	if (bWasAttacking)
	{
		OnAttackEnded.Broadcast(EndedAttackType, EndedInstanceId);
	}
	else if (OldState == ECombatState::Dodging)
	{
		OnDodgeEnded.Broadcast();
	}

	bIsTransitioningState = false;
}

bool UShadowSlaveCombatComponent::CanPerformAttack(EAttackType AttackType) const
{
	// Attack can only be initiated from Neutral state (rejects input during Attacking, Recovering, Dodging, Stunned, Dead)
	if (CurrentCombatState != ECombatState::Neutral)
	{
		return false;
	}

	if (!OwningCharacter.IsValid())
	{
		return false;
	}

	// Character must be alive
	if (!OwningCharacter->IsAlive())
	{
		return false;
	}

	// Authoritative attribute check if attribute component is attached
	if (const UShadowSlaveAttributeComponent* AttribComp = GetOwnerAttributeComponent())
	{
		if (!AttribComp->IsAlive())
		{
			return false;
		}
	}

	return true;
}

bool UShadowSlaveCombatComponent::ExecuteAttack(EAttackType AttackType)
{
	if (!CanPerformAttack(AttackType))
	{
		return false;
	}

	ActiveAttackData = GetAttackData(AttackType);
	++CurrentAttackInstanceId;
	HitCountsThisAttack.Empty();

	SetCombatState(ECombatState::Attacking);
	OnAttackExecuted.Broadcast(AttackType);
	OnAttackStarted.Broadcast(AttackType, CurrentAttackInstanceId);

	// Attempt montage playback if configured
	bIsMontageDriven = false;
	if (OwningCharacter.IsValid() && ActiveAttackData.AttackMontage)
	{
		if (USkeletalMeshComponent* Mesh = OwningCharacter->GetMesh())
		{
			if (UAnimInstance* AnimInstance = Mesh->GetAnimInstance())
			{
				if (AnimInstance->Montage_Play(ActiveAttackData.AttackMontage) > 0.0f)
				{
					bIsMontageDriven = true;

					FOnMontageEnded EndDelegate;
					EndDelegate.BindUObject(this, &UShadowSlaveCombatComponent::HandleMontageEnded);
					AnimInstance->Montage_SetEndDelegate(EndDelegate, ActiveAttackData.AttackMontage);
				}
			}
		}
	}

	// Fallback path: If montage asset is unassigned or failed to play, drive hit window via timers for testing
	if (!bIsMontageDriven)
	{
		OpenHitWindow();

		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(HitWindowTimerHandle, this, &UShadowSlaveCombatComponent::CloseHitWindow, ActiveAttackData.HitWindowDuration, false);
		}
	}

	return true;
}

void UShadowSlaveCombatComponent::CancelAttack()
{
	if (CurrentCombatState != ECombatState::Attacking)
	{
		return;
	}

	SetCombatState(ECombatState::Neutral);
}

void UShadowSlaveCombatComponent::OpenHitWindow()
{
	if (CurrentCombatState != ECombatState::Attacking)
	{
		return;
	}

	bHitWindowActive = true;

	// Immediate trace sweep upon window opening
	PerformMeleeTrace();

	// Recurring trace loop timer only if not driven by animation notifies
	if (!bIsMontageDriven)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(TraceLoopTimerHandle, this, &UShadowSlaveCombatComponent::PerformMeleeTrace, 0.05f, true);
		}
	}
}

void UShadowSlaveCombatComponent::CloseHitWindow()
{
	bHitWindowActive = false;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TraceLoopTimerHandle);
		World->GetTimerManager().ClearTimer(HitWindowTimerHandle);
	}

	// Transition to recovery if still in Attacking state
	if (CurrentCombatState == ECombatState::Attacking)
	{
		const float RecoveryTime = ActiveAttackData.RecoveryDuration;

		SetCombatState(ECombatState::Recovering);

		if (RecoveryTime > 0.0f)
		{
			if (UWorld* World = GetWorld())
			{
				World->GetTimerManager().SetTimer(RecoveryTimerHandle, this, &UShadowSlaveCombatComponent::OnRecoveryFinished, RecoveryTime, false);
			}
		}
		else
		{
			OnRecoveryFinished();
		}
	}
}

void UShadowSlaveCombatComponent::HandleMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	// Stale or unrelated callback check: must match active attack montage and component must be in Attacking state
	if (!Montage || Montage != ActiveAttackData.AttackMontage || CurrentCombatState != ECombatState::Attacking)
	{
		return;
	}

	if (bInterrupted)
	{
		CancelAttack();
	}
	else
	{
		CloseHitWindow();
	}
}

void UShadowSlaveCombatComponent::PerformMeleeTrace()
{
	if (!bHitWindowActive || CurrentCombatState != ECombatState::Attacking || !OwningCharacter.IsValid())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FVector ForwardVector = OwningCharacter->GetActorForwardVector();
	const FVector Start = OwningCharacter->GetActorLocation() + FVector(0.0f, 0.0f, 20.0f);
	const FVector End = Start + (ForwardVector * ActiveAttackData.TraceDistance);
	const float Radius = ActiveAttackData.TraceRadius;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(OwningCharacter.Get());
	QueryParams.bTraceComplex = false;

	TArray<FHitResult> OutHits;
	const bool bHit = World->SweepMultiByChannel(
		OutHits,
		Start,
		End,
		FQuat::Identity,
		MeleeTraceChannel,
		FCollisionShape::MakeSphere(Radius),
		QueryParams
	);

	if (bDrawDebugTraces)
	{
		DrawDebugSphere(World, End, Radius, 12, bHit ? FColor::Red : FColor::Green, false, DebugTraceDuration);
		DrawDebugLine(World, Start, End, FColor::Yellow, false, DebugTraceDuration, 0, 1.5f);
	}

	if (!bHit)
	{
		return;
	}

	for (const FHitResult& HitResult : OutHits)
	{
		AActor* HitActor = HitResult.GetActor();
		if (!HitActor || !IsValid(HitActor) || !CanDamageTarget(HitActor))
		{
			continue;
		}

		// Hit deduplication check: enforce configured MaxHitsPerTarget for this attack instance
		const int32 CurrentHits = HitCountsThisAttack.FindRef(HitActor);
		if (CurrentHits >= ActiveAttackData.MaxHitsPerTarget)
		{
			continue;
		}

		HitCountsThisAttack.Add(HitActor, CurrentHits + 1);

		FVector HitDir = (HitResult.ImpactPoint - Start).GetSafeNormal();
		if (HitDir.IsNearlyZero())
		{
			HitDir = ForwardVector;
		}

		const FShadowSlaveDamageInfo DamageInfo(
			ActiveAttackData.Damage,
			OwningCharacter.Get(),
			OwningCharacter.Get(),
			HitResult.ImpactPoint,
			HitResult.ImpactNormal,
			HitDir,
			CurrentAttackInstanceId
		);

		// Route damage through interface if supported
		if (HitActor->GetClass()->ImplementsInterface(UShadowSlaveDamageableInterface::StaticClass()))
		{
			IShadowSlaveDamageableInterface::Execute_TakeDamageCustom(HitActor, DamageInfo);
		}
		else
		{
			// Fallback to standard engine damage for non-interface actors
			HitActor->TakeDamage(
				ActiveAttackData.Damage,
				FDamageEvent(),
				OwningCharacter->GetController(),
				OwningCharacter.Get()
			);
		}

		OnTargetHit.Broadcast(HitActor, DamageInfo);
		OnDamageDealt.Broadcast(DamageInfo);
	}
}

void UShadowSlaveCombatComponent::OnRecoveryFinished()
{
	if (CurrentCombatState == ECombatState::Recovering)
	{
		SetCombatState(ECombatState::Neutral);
	}
}

const FShadowSlaveAttackData& UShadowSlaveCombatComponent::GetAttackData(EAttackType AttackType) const
{
	return (AttackType == EAttackType::Heavy) ? HeavyAttackData : LightAttackData;
}

void UShadowSlaveCombatComponent::HandleOwnerDeath()
{
	SetCombatState(ECombatState::Dead);
}

void UShadowSlaveCombatComponent::ResetToNeutral()
{
	if (CurrentCombatState == ECombatState::Dead)
	{
		return;
	}

	if (CurrentCombatState == ECombatState::Attacking)
	{
		CancelAttack();
	}
	else if (CurrentCombatState == ECombatState::Recovering || CurrentCombatState == ECombatState::Dodging || CurrentCombatState == ECombatState::Stunned)
	{
		SetCombatState(ECombatState::Neutral);
	}
}

bool UShadowSlaveCombatComponent::CanDamageTarget(AActor* TargetActor) const
{
	if (!TargetActor || !IsValid(TargetActor) || TargetActor == OwningCharacter.Get() || TargetActor == GetOwner())
	{
		return false;
	}

	// Don't damage actors that are already dead
	if (TargetActor->GetClass()->ImplementsInterface(UShadowSlaveDamageableInterface::StaticClass()))
	{
		if (!IShadowSlaveDamageableInterface::Execute_IsAlive(TargetActor))
		{
			return false;
		}
	}

	// Friendly fire checks
	if (!bAllowFriendlyFire && OwningCharacter.IsValid())
	{
		const bool bOwnerIsEnemy = OwningCharacter->ActorHasTag(TEXT("Enemy"));
		const bool bTargetIsEnemy = TargetActor->ActorHasTag(TEXT("Enemy"));
		if (bOwnerIsEnemy && bTargetIsEnemy)
		{
			return false;
		}

		const bool bOwnerIsPlayer = OwningCharacter->ActorHasTag(TEXT("Player")) || OwningCharacter->IsPlayerControlled();
		const bool bTargetIsPlayer = TargetActor->ActorHasTag(TEXT("Player"));
		if (bOwnerIsPlayer && bTargetIsPlayer)
		{
			return false;
		}
	}

	return true;
}

bool UShadowSlaveCombatComponent::HasHitTargetThisAttack(AActor* TargetActor) const
{
	if (!TargetActor)
	{
		return false;
	}

	return HitCountsThisAttack.FindRef(TargetActor) > 0;
}

int32 UShadowSlaveCombatComponent::GetHitCountForTargetThisAttack(AActor* TargetActor) const
{
	if (!TargetActor)
	{
		return 0;
	}

	return HitCountsThisAttack.FindRef(TargetActor);
}

UShadowSlaveAttributeComponent* UShadowSlaveCombatComponent::GetOwnerAttributeComponent() const
{
	if (OwningCharacter.IsValid())
	{
		return OwningCharacter->GetAttributeComponent();
	}

	if (AActor* OwnerActor = GetOwner())
	{
		return OwnerActor->FindComponentByClass<UShadowSlaveAttributeComponent>();
	}

	return nullptr;
}

void UShadowSlaveCombatComponent::NotifyDamageReceived(const FShadowSlaveDamageInfo& DamageInfo)
{
	OnDamageReceived.Broadcast(DamageInfo);
}

bool UShadowSlaveCombatComponent::CanPerformDodge() const
{
	// Cannot initiate dodge if currently in a state transition (re-entrancy guard)
	if (bIsTransitioningState)
	{
		return false;
	}

	// Cannot dodge if already dodging or if transition to Dodging is disallowed
	if (CurrentCombatState == ECombatState::Dodging)
	{
		return false;
	}

	if (!CanTransitionToState(ECombatState::Dodging))
	{
		return false;
	}

	if (!OwningCharacter.IsValid())
	{
		return false;
	}

	// Character must be alive
	if (!OwningCharacter->IsAlive())
	{
		return false;
	}

	// Grounded check if configured
	if (bRequireGrounded)
	{
		if (const UCharacterMovementComponent* MoveComp = OwningCharacter->GetCharacterMovement())
		{
			if (MoveComp->IsFalling())
			{
				return false;
			}
		}
	}

	// Authoritative stamina check against AttributeComponent if attached
	if (const UShadowSlaveAttributeComponent* AttribComp = GetOwnerAttributeComponent())
	{
		if (AttribComp->GetCurrentStamina() < DodgeData.StaminaCost)
		{
			return false;
		}
	}

	return true;
}

bool UShadowSlaveCombatComponent::RequestDodge(const FVector& Direction)
{
	// 1. Pre-validation: verify state transition legality, grounded, alive, re-entrancy guard, stamina sufficiency
	if (!CanPerformDodge())
	{
		OnDodgeRejected.Broadcast();
		return false;
	}

	UShadowSlaveAttributeComponent* AttribComp = GetOwnerAttributeComponent();

	// 2. Consume stamina only as part of committing the dodge.
	// If stamina consumption fails, combat state remains completely unchanged.
	bool bStaminaSpent = false;
	if (AttribComp)
	{
		if (!AttribComp->ConsumeStamina(DodgeData.StaminaCost))
		{
			OnDodgeRejected.Broadcast();
			return false;
		}
		bStaminaSpent = true;
	}

	// 3. Commit state transition to Dodging
	SetCombatState(ECombatState::Dodging);

	// 4. Verify that state entry actually succeeded!
	if (CurrentCombatState != ECombatState::Dodging)
	{
		// State transition was rejected (e.g. re-entrancy guard or external state interruption).
		// Refund spent stamina so a rejected dodge never spends stamina!
		if (bStaminaSpent && AttribComp)
		{
			AttribComp->RestoreStamina(DodgeData.StaminaCost);
		}

		OnDodgeRejected.Broadcast();
		return false;
	}

	// 5. Authoritatively in Dodging state with stamina consumed.
	// Execute movement launch, animation montage, duration timer, and OnDodgeStarted broadcast.
	ExecuteDodge(Direction);
	return true;
}

void UShadowSlaveCombatComponent::ExecuteDodge(const FVector& Direction)
{
	// Invariant: No successful dodge side effects unless CurrentCombatState == ECombatState::Dodging
	if (CurrentCombatState != ECombatState::Dodging)
	{
		return;
	}

	const FVector ResolvedDirection = ResolveDodgeDirection(Direction);
	const EDodgeDirection CardinalDirection = CalculateDodgeCardinalDirection(ResolvedDirection);

	ActiveDodgeDirection = ResolvedDirection;
	CurrentDodgeCardinalDirection = CardinalDirection;

	// Apply physical launch impulse and temporarily suppress movement input
	ApplyDodgeMovement(ResolvedDirection);

	// Play directional dodge animation montage if available
	UAnimMontage* DodgeMontage = GetDodgeMontageForDirection(CardinalDirection);
	if (DodgeMontage && OwningCharacter.IsValid())
	{
		if (USkeletalMeshComponent* Mesh = OwningCharacter->GetMesh())
		{
			if (UAnimInstance* AnimInstance = Mesh->GetAnimInstance())
			{
				AnimInstance->Montage_Play(DodgeMontage);
			}
		}
	}

	// Broadcast OnDodgeStarted while authoritatively in Dodging state
	OnDodgeStarted.Broadcast(ResolvedDirection, CardinalDirection);

	// If a listener to OnDodgeStarted caused a state transition away from Dodging, do not schedule or finish
	if (CurrentCombatState != ECombatState::Dodging)
	{
		return;
	}

	// Schedule dodge completion timer or finish immediately if non-positive duration
	if (DodgeData.DodgeDuration > 0.0f)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(DodgeTimerHandle, this, &UShadowSlaveCombatComponent::OnDodgeFinished, DodgeData.DodgeDuration, false);
		}
	}
	else
	{
		OnDodgeFinished();
	}
}

void UShadowSlaveCombatComponent::OnDodgeFinished()
{
	if (CurrentCombatState == ECombatState::Dodging)
	{
		SetCombatState(ECombatState::Neutral);
	}
}

FVector UShadowSlaveCombatComponent::ResolveDodgeDirection(const FVector& InputDirection) const
{
	// 1. Explicit valid direction passed by caller (e.g. AI or directed input)
	if (!InputDirection.IsNearlyZero())
	{
		const FVector Direction2D = FVector(InputDirection.X, InputDirection.Y, 0.0f).GetSafeNormal();
		if (!Direction2D.IsNearlyZero())
		{
			return Direction2D;
		}
	}

	if (!OwningCharacter.IsValid())
	{
		return FVector::ForwardVector;
	}

	// 2. Active movement input acceleration (e.g. player WASD / analog stick input)
	if (const UCharacterMovementComponent* MoveComp = OwningCharacter->GetCharacterMovement())
	{
		const FVector Accel = MoveComp->GetCurrentAcceleration();
		const FVector Accel2D = FVector(Accel.X, Accel.Y, 0.0f).GetSafeNormal();
		if (!Accel2D.IsNearlyZero())
		{
			return Accel2D;
		}
	}

	// 3. Current character movement velocity (preserves directional momentum)
	const FVector Velocity = OwningCharacter->GetVelocity();
	const FVector Velocity2D = FVector(Velocity.X, Velocity.Y, 0.0f).GetSafeNormal();
	if (!Velocity2D.IsNearlyZero())
	{
		return Velocity2D;
	}

	// 4. Fall back to character forward facing vector
	const FVector Forward = OwningCharacter->GetActorForwardVector();
	const FVector Forward2D = FVector(Forward.X, Forward.Y, 0.0f).GetSafeNormal();
	if (!Forward2D.IsNearlyZero())
	{
		return Forward2D;
	}

	return FVector::ForwardVector;
}

EDodgeDirection UShadowSlaveCombatComponent::CalculateDodgeCardinalDirection(const FVector& Direction) const
{
	if (!OwningCharacter.IsValid())
	{
		return EDodgeDirection::Forward;
	}

	const FVector ForwardVector = OwningCharacter->GetActorForwardVector().GetSafeNormal2D();
	const FVector RightVector = OwningCharacter->GetActorRightVector().GetSafeNormal2D();
	const FVector DodgeDir2D = Direction.GetSafeNormal2D();

	const float ForwardDot = FVector::DotProduct(ForwardVector, DodgeDir2D);
	const float RightDot = FVector::DotProduct(RightVector, DodgeDir2D);

	// 45-degree angle threshold: cos(45 deg) ~= 0.7071f
	if (ForwardDot >= 0.7071f)
	{
		return EDodgeDirection::Forward;
	}
	else if (ForwardDot <= -0.7071f)
	{
		return EDodgeDirection::Backward;
	}
	else if (RightDot > 0.0f)
	{
		return EDodgeDirection::Right;
	}
	else
	{
		return EDodgeDirection::Left;
	}
}

UAnimMontage* UShadowSlaveCombatComponent::GetDodgeMontageForDirection(EDodgeDirection Direction) const
{
	switch (Direction)
	{
	case EDodgeDirection::Forward:
		return DodgeData.DodgeForwardMontage.Get();
	case EDodgeDirection::Backward:
		return DodgeData.DodgeBackwardMontage.Get();
	case EDodgeDirection::Left:
		return DodgeData.DodgeLeftMontage.Get();
	case EDodgeDirection::Right:
		return DodgeData.DodgeRightMontage.Get();
	default:
		return DodgeData.DodgeForwardMontage.Get();
	}
}

void UShadowSlaveCombatComponent::ApplyDodgeMovement(const FVector& Direction)
{
	if (!OwningCharacter.IsValid())
	{
		return;
	}

	// Temporarily suppress movement input during dodge impulse so input doesn't counteract launch velocity
	OwningCharacter->SetMovementControlSuppressed(TEXT("Dodge"), true);
	bMovementControlSuppressedByDodge = true;

	// Launch character horizontally along dodge direction
	const FVector LaunchVelocity = Direction * DodgeData.DodgeSpeed;
	OwningCharacter->LaunchCharacter(LaunchVelocity, true, false);
}

void UShadowSlaveCombatComponent::RestoreDodgeMovement()
{
	if (bMovementControlSuppressedByDodge)
	{
		bMovementControlSuppressedByDodge = false;
		if (OwningCharacter.IsValid())
		{
			OwningCharacter->SetMovementControlSuppressed(TEXT("Dodge"), false);
		}
	}
}

