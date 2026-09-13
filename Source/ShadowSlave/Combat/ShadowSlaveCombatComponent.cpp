// Copyright Epic Games, Inc. All Rights Reserved.

#include "Combat/ShadowSlaveCombatComponent.h"
#include "Combat/ShadowSlaveDamageableInterface.h"
#include "GameFramework/Character.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"
#include "Engine/DamageEvents.h"
#include "ShadowSlave.h"

UShadowSlaveCombatComponent::UShadowSlaveCombatComponent()
{
	// Combat component relies on event-driven timers and animation notifies; no tick needed
	PrimaryComponentTick.bCanEverTick = false;

	// Light attack configuration defaults
	LightAttackData.AttackType = EAttackType::Light;
	LightAttackData.Damage = 25.0f;
	LightAttackData.TraceRadius = 45.0f;
	LightAttackData.TraceDistance = 160.0f;
	LightAttackData.HitWindowDuration = 0.35f;
	LightAttackData.RecoveryDuration = 0.20f;

	// Heavy attack configuration defaults
	HeavyAttackData.AttackType = EAttackType::Heavy;
	HeavyAttackData.Damage = 60.0f;
	HeavyAttackData.TraceRadius = 55.0f;
	HeavyAttackData.TraceDistance = 180.0f;
	HeavyAttackData.HitWindowDuration = 0.45f;
	HeavyAttackData.RecoveryDuration = 0.35f;
}

void UShadowSlaveCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	OwningCharacter = Cast<ACharacter>(GetOwner());
}

void UShadowSlaveCombatComponent::SetCombatState(ECombatState NewState)
{
	if (CurrentCombatState == NewState)
	{
		return;
	}

	const ECombatState OldState = CurrentCombatState;
	CurrentCombatState = NewState;
	OnCombatStateChanged.Broadcast(OldState, NewState);
}

bool UShadowSlaveCombatComponent::CanPerformAttack(EAttackType AttackType) const
{
	// Cannot attack while dead, stunned, dodging, or currently in an attack swing
	if (CurrentCombatState == ECombatState::Dead ||
		CurrentCombatState == ECombatState::Stunned ||
		CurrentCombatState == ECombatState::Dodging ||
		CurrentCombatState == ECombatState::Attacking)
	{
		return false;
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
	SetCombatState(ECombatState::Attacking);
	HitActorsThisAttack.Empty();
	OnAttackExecuted.Broadcast(AttackType);

	// Play montage if configured
	bool bMontageTriggered = false;
	if (OwningCharacter.IsValid() && ActiveAttackData.AttackMontage)
	{
		if (USkeletalMeshComponent* Mesh = OwningCharacter->GetMesh())
		{
			if (UAnimInstance* AnimInstance = Mesh->GetAnimInstance())
			{
				if (AnimInstance->Montage_Play(ActiveAttackData.AttackMontage) > 0.0f)
				{
					bMontageTriggered = true;
				}
			}
		}
	}

	// Fallback path: If no montage asset is assigned, simulate hit window via timers for testing
	if (!bMontageTriggered)
	{
		OpenHitWindow();

		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(HitWindowTimerHandle, this, &UShadowSlaveCombatComponent::CloseHitWindow, ActiveAttackData.HitWindowDuration, false);
		}
	}

	return true;
}

void UShadowSlaveCombatComponent::OpenHitWindow()
{
	bHitWindowActive = true;
	HitActorsThisAttack.Empty();

	// Immediate trace on window open
	PerformMeleeTrace();

	// Recurring trace while window remains open
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(TraceLoopTimerHandle, this, &UShadowSlaveCombatComponent::PerformMeleeTrace, 0.05f, true);
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

	// Transition to recovery
	if (CurrentCombatState == ECombatState::Attacking)
	{
		SetCombatState(ECombatState::Recovering);

		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(RecoveryTimerHandle, this, &UShadowSlaveCombatComponent::OnRecoveryFinished, ActiveAttackData.RecoveryDuration, false);
		}
	}
}

void UShadowSlaveCombatComponent::PerformMeleeTrace()
{
	if (!bHitWindowActive || !OwningCharacter.IsValid())
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
		if (!HitActor || HitActor == OwningCharacter.Get())
		{
			continue;
		}

		// Prevent applying damage repeatedly to the same target during a single attack
		if (HitActorsThisAttack.Contains(HitActor))
		{
			continue;
		}

		HitActorsThisAttack.Add(HitActor);

		const FShadowSlaveDamageInfo DamageInfo(
			ActiveAttackData.Damage,
			OwningCharacter.Get(),
			OwningCharacter.Get(),
			HitResult.ImpactPoint,
			HitResult.ImpactNormal
		);

		// Route damage through interface if supported
		if (HitActor->GetClass()->ImplementsInterface(UShadowSlaveDamageableInterface::StaticClass()))
		{
			IShadowSlaveDamageableInterface::Execute_TakeDamageCustom(HitActor, DamageInfo);
		}

		// Apply standard engine damage
		HitActor->TakeDamage(
			ActiveAttackData.Damage,
			FDamageEvent(),
			OwningCharacter->GetController(),
			OwningCharacter.Get()
		);

		OnTargetHit.Broadcast(HitActor, DamageInfo);
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
	HitActorsThisAttack.Empty();
	SetCombatState(ECombatState::Dead);
}

void UShadowSlaveCombatComponent::ResetToNeutral()
{
	if (CurrentCombatState != ECombatState::Dead)
	{
		SetCombatState(ECombatState::Neutral);
	}
}
