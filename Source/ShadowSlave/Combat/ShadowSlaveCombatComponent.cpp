// Copyright Epic Games, Inc. All Rights Reserved.

#include "Combat/ShadowSlaveCombatComponent.h"
#include "Combat/ShadowSlaveDamageableInterface.h"
#include "Attributes/ShadowSlaveAttributeComponent.h"
#include "GameFramework/Character.h"
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
}

void UShadowSlaveCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	OwningCharacter = Cast<ACharacter>(GetOwner());
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

	if (!CanTransitionToState(NewState))
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveCombatComponent::SetCombatState - Invalid state transition rejected: %d -> %d on '%s'"),
			static_cast<int32>(CurrentCombatState),
			static_cast<int32>(NewState),
			OwningCharacter.IsValid() ? *OwningCharacter->GetName() : TEXT("Unknown"));
		return;
	}

	const ECombatState OldState = CurrentCombatState;

	// State exit hooks
	if (OldState == ECombatState::Attacking)
	{
		bHitWindowActive = false;
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(TraceLoopTimerHandle);
			World->GetTimerManager().ClearTimer(HitWindowTimerHandle);
		}

		// If leaving Attacking to Stunned, Dodging, or Neutral, halt active attack montage
		if (NewState != ECombatState::Recovering && OwningCharacter.IsValid() && ActiveAttackData.AttackMontage)
		{
			if (USkeletalMeshComponent* Mesh = OwningCharacter->GetMesh())
			{
				if (UAnimInstance* AnimInstance = Mesh->GetAnimInstance())
				{
					AnimInstance->Montage_Stop(0.1f, ActiveAttackData.AttackMontage);
				}
			}
		}
	}
	else if (OldState == ECombatState::Recovering)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(RecoveryTimerHandle);
		}
	}

	// State enter hooks
	if (NewState == ECombatState::Dead)
	{
		bHitWindowActive = false;
		HitCountsThisAttack.Empty();
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(TraceLoopTimerHandle);
			World->GetTimerManager().ClearTimer(HitWindowTimerHandle);
			World->GetTimerManager().ClearTimer(RecoveryTimerHandle);
		}

		if (OwningCharacter.IsValid() && ActiveAttackData.AttackMontage)
		{
			if (USkeletalMeshComponent* Mesh = OwningCharacter->GetMesh())
			{
				if (UAnimInstance* AnimInstance = Mesh->GetAnimInstance())
				{
					AnimInstance->Montage_Stop(0.1f, ActiveAttackData.AttackMontage);
				}
			}
		}
	}

	CurrentCombatState = NewState;
	OnCombatStateChanged.Broadcast(OldState, NewState);
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

	bHitWindowActive = false;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TraceLoopTimerHandle);
		World->GetTimerManager().ClearTimer(HitWindowTimerHandle);
		World->GetTimerManager().ClearTimer(RecoveryTimerHandle);
	}

	if (OwningCharacter.IsValid() && ActiveAttackData.AttackMontage)
	{
		if (USkeletalMeshComponent* Mesh = OwningCharacter->GetMesh())
		{
			if (UAnimInstance* AnimInstance = Mesh->GetAnimInstance())
			{
				AnimInstance->Montage_Stop(0.1f, ActiveAttackData.AttackMontage);
			}
		}
	}

	const EAttackType CancelledAttackType = ActiveAttackData.AttackType;
	const int32 CancelledInstanceId = CurrentAttackInstanceId;

	HitCountsThisAttack.Empty();
	SetCombatState(ECombatState::Neutral);
	OnAttackEnded.Broadcast(CancelledAttackType, CancelledInstanceId);
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
		const EAttackType EndedAttackType = ActiveAttackData.AttackType;
		const int32 EndedInstanceId = CurrentAttackInstanceId;
		const float RecoveryTime = ActiveAttackData.RecoveryDuration;

		SetCombatState(ECombatState::Recovering);
		OnAttackEnded.Broadcast(EndedAttackType, EndedInstanceId);

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
	if (Montage && Montage == ActiveAttackData.AttackMontage && CurrentCombatState == ECombatState::Attacking)
	{
		if (bInterrupted)
		{
			CancelAttack();
		}
		else
		{
			CloseHitWindow();
		}
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
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TraceLoopTimerHandle);
		World->GetTimerManager().ClearTimer(HitWindowTimerHandle);
		World->GetTimerManager().ClearTimer(RecoveryTimerHandle);
	}

	bHitWindowActive = false;
	HitCountsThisAttack.Empty();

	if (OwningCharacter.IsValid() && ActiveAttackData.AttackMontage)
	{
		if (USkeletalMeshComponent* Mesh = OwningCharacter->GetMesh())
		{
			if (UAnimInstance* AnimInstance = Mesh->GetAnimInstance())
			{
				AnimInstance->Montage_Stop(0.1f, ActiveAttackData.AttackMontage);
			}
		}
	}

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
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(RecoveryTimerHandle);
		}
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

