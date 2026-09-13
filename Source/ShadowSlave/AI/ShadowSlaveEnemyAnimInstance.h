// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Characters/ShadowSlaveAnimInstance.h"
#include "AI/ShadowSlaveAITypes.h"
#include "ShadowSlaveEnemyAnimInstance.generated.h"

class AShadowSlaveEnemyCharacterBase;
class AShadowSlaveAIController;

/**
 * Animation Instance class for Nightmare Creature AI enemies.
 * Extracts locomotion, AI state, combat activity, and stagger/death triggers
 * to drive Enemy Animation Blueprints cleanly.
 */
UCLASS()
class SHADOWSLAVE_API UShadowSlaveEnemyAnimInstance : public UShadowSlaveAnimInstance
{
	GENERATED_BODY()

public:
	UShadowSlaveEnemyAnimInstance();

	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
	/** Cached reference to owning enemy character */
	UPROPERTY(BlueprintReadOnly, Category = "ShadowSlave|Animation|AI", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<AShadowSlaveEnemyCharacterBase> EnemyCharacter;

	/** Current high-level AI state (Idle, Chasing, Attacking, etc.) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Animation|AI", meta = (AllowPrivateAccess = "true"))
	EShadowSlaveAIState AIState = EShadowSlaveAIState::Idle;

	/** True if AI is currently actively chasing a target */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Animation|AI", meta = (AllowPrivateAccess = "true"))
	bool bIsChasing = false;

	/** True if AI is currently executing an attack */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Animation|AI", meta = (AllowPrivateAccess = "true"))
	bool bIsAttacking = false;

	/** True if enemy is currently staggered from taking damage */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Animation|AI", meta = (AllowPrivateAccess = "true"))
	bool bIsStaggered = false;

	/** True if enemy is dead */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Animation|AI", meta = (AllowPrivateAccess = "true"))
	bool bIsDead = false;

public:
	/** Returns cached enemy character */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Animation|AI")
	AShadowSlaveEnemyCharacterBase* GetOwningEnemyCharacter() const { return EnemyCharacter; }
};
