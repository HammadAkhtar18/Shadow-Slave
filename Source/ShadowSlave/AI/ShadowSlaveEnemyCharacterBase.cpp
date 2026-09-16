// Copyright Epic Games, Inc. All Rights Reserved.

#include "AI/ShadowSlaveEnemyCharacterBase.h"
#include "AI/ShadowSlaveAIController.h"
#include "Combat/ShadowSlaveCombatComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimMontage.h"
#include "Engine/World.h"
#include "TimerManager.h"

AShadowSlaveEnemyCharacterBase::AShadowSlaveEnemyCharacterBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Configure AI possession defaults
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	AIControllerClass = AShadowSlaveAIController::StaticClass();

	// Add identification tag for friendly-fire prevention and team classification
	Tags.Add(FName("Enemy"));

	// Default enemy locomotion speeds
	WalkSpeed = 260.0f;
	SprintSpeed = 520.0f;
	BaseMaxAcceleration = 1600.0f;
	BaseBrakingDecelerationWalking = 1800.0f;

	bUseControllerRotationYaw = false;
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->bOrientRotationToMovement = true;
	}
}

void AShadowSlaveEnemyCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	// Initialize combat component light attack data for AI attacks
	if (CombatComponent)
	{
		FShadowSlaveAttackData LightData;
		LightData.AttackType = EAttackType::Light;
		LightData.Damage = AttackDamage;
		LightData.TraceRadius = 45.0f;
		LightData.TraceDistance = AttackRange;
		LightData.HitWindowDuration = 0.35f;
		LightData.RecoveryDuration = 0.20f;
		LightData.AttackMontage = AttackMontage;
		CombatComponent->SetLightAttackData(LightData);
	}
}

void AShadowSlaveEnemyCharacterBase::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
}

bool AShadowSlaveEnemyCharacterBase::PerformAttack()
{
	if (!bIsAlive || bIsStaggered || !CombatComponent)
	{
		return false;
	}

	return CombatComponent->ExecuteAttack(EAttackType::Light);
}

void AShadowSlaveEnemyCharacterBase::OnDamaged(const FShadowSlaveDamageInfo& DamageInfo)
{
	Super::OnDamaged(DamageInfo);

	// Alert AI controller of the attacker or damage origin
	if (AShadowSlaveAIController* AICon = Cast<AShadowSlaveAIController>(GetController()))
	{
		AICon->HandleDamagedBy(DamageInfo.Attacker.Get(), DamageInfo.HitLocation);
	}

	// Trigger hit reaction if still alive
	if (DamageInfo.DamageAmount > 0.0f && IsAlive())
	{
		PlayHitReaction(DamageInfo.HitNormal);
	}
}

void AShadowSlaveEnemyCharacterBase::PlayHitReaction(const FVector& HitNormal)
{
	if (!IsAlive())
	{
		return;
	}

	bIsStaggered = true;

	if (CombatComponent)
	{
		CombatComponent->SetCombatState(ECombatState::Stunned);
	}

	float Duration = StaggerDuration;

	if (HitReactionMontage)
	{
		const float MontageDuration = PlayAnimMontage(HitReactionMontage);
		if (MontageDuration > 0.0f)
		{
			Duration = MontageDuration;
		}
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(StaggerTimerHandle, this, &AShadowSlaveEnemyCharacterBase::OnStaggerFinished, Duration, false);
	}

	OnEnemyHitReaction.Broadcast(HitNormal);
}

void AShadowSlaveEnemyCharacterBase::OnStaggerFinished()
{
	bIsStaggered = false;

	if (CombatComponent && CombatComponent->GetCombatState() == ECombatState::Stunned)
	{
		CombatComponent->ResetToNeutral();
	}
}

void AShadowSlaveEnemyCharacterBase::HandleDeath()
{
	Super::HandleDeath();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(StaggerTimerHandle);
	}

	bIsStaggered = false;

	if (DeathMontage)
	{
		PlayAnimMontage(DeathMontage);
	}

	// Ignore pawn collision channel so the corpse does not obstruct other characters
	if (GetCapsuleComponent())
	{
		GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	}

	// Invalidate AI controller upon death
	if (AShadowSlaveAIController* AICon = Cast<AShadowSlaveAIController>(GetController()))
	{
		AICon->OnPawnDeath();
	}

	OnEnemyDied.Broadcast();
}
