// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Characters/ShadowSlaveCharacterBase.h"
#include "AI/ShadowSlaveAITypes.h"
#include "ShadowSlaveEnemyCharacterBase.generated.h"

class AShadowSlaveAIController;
class UAnimMontage;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEnemyDiedSignature);

/**
 * Reusable base enemy character class for Nightmare Creatures.
 * Acts as the physical representation and combat executor.
 * Delegates perception, state decisions, and navigation planning to AShadowSlaveAIController.
 */
UCLASS(Abstract)
class SHADOWSLAVE_API AShadowSlaveEnemyCharacterBase : public AShadowSlaveCharacterBase
{
	GENERATED_BODY()

public:
	AShadowSlaveEnemyCharacterBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void PossessedBy(AController* NewController) override;

	/* --- Combat Actions Called by AI Controller --- */

	/** Requests execution of a melee attack via the Combat Component */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Combat|AI")
	virtual bool PerformAttack();

	/** Plays hit reaction montage and handles stagger state */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Combat|AI")
	virtual void PlayHitReaction(const FVector& HitNormal);

	/* --- Getters for AI Controller Configuration --- */

	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Combat|AI")
	float GetAttackRange() const { return AttackRange; }

	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Combat|AI")
	float GetAttackAcceptanceRadius() const { return AttackAcceptanceRadius; }

	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Combat|AI")
	float GetAttackRecoveryDuration() const { return AttackRecoveryDuration; }

	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Combat|AI")
	bool IsStaggered() const { return bIsStaggered; }

	/* --- Delegates --- */

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|AI")
	FOnEnemyDiedSignature OnEnemyDied;

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|AI")
	FOnEnemyHitReactionSignature OnEnemyHitReaction;

protected:
	virtual void BeginPlay() override;

	/** Overridden from AShadowSlaveCharacterBase to react to incoming damage */
	virtual void OnDamaged(const FShadowSlaveDamageInfo& DamageInfo) override;

	/** Overridden from AShadowSlaveCharacterBase to execute enemy death transitions */
	virtual void HandleDeath() override;

	/** Callback when stagger / hit reaction duration completes */
	virtual void OnStaggerFinished();

protected:
	/* --- AI Combat Parameters --- */

	/** Attack range in cm; enemy attacks when target is within this distance */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Combat|AI", meta = (ClampMin = "50.0"))
	float AttackRange = 160.0f;

	/** Acceptance radius in cm for navigation MoveTo requests */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Combat|AI", meta = (ClampMin = "20.0"))
	float AttackAcceptanceRadius = 120.0f;

	/** Base attack damage dealt by this enemy */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Combat|AI", meta = (ClampMin = "1.0"))
	float AttackDamage = 20.0f;

	/** Cooldown in seconds following an attack before another can be initiated */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Combat|AI", meta = (ClampMin = "0.1"))
	float AttackRecoveryDuration = 0.8f;

	/** Duration of stagger freeze when receiving a hit reaction */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Combat|AI", meta = (ClampMin = "0.0"))
	float StaggerDuration = 0.4f;

	/* --- Animation Assets --- */

	/** Animation montage played on attack */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Combat|Animation")
	TObjectPtr<UAnimMontage> AttackMontage;

	/** Animation montage played on taking damage */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Combat|Animation")
	TObjectPtr<UAnimMontage> HitReactionMontage;

	/** Animation montage played on death */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Combat|Animation")
	TObjectPtr<UAnimMontage> DeathMontage;

	/* --- State --- */

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Combat|AI")
	bool bIsStaggered = false;

	FTimerHandle StaggerTimerHandle;
};
