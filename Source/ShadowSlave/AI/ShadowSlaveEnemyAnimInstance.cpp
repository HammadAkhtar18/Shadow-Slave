// Copyright Epic Games, Inc. All Rights Reserved.

#include "AI/ShadowSlaveEnemyAnimInstance.h"
#include "AI/ShadowSlaveEnemyCharacterBase.h"
#include "AI/ShadowSlaveAIController.h"
#include "Combat/ShadowSlaveCombatComponent.h"

UShadowSlaveEnemyAnimInstance::UShadowSlaveEnemyAnimInstance()
{
}

void UShadowSlaveEnemyAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	EnemyCharacter = Cast<AShadowSlaveEnemyCharacterBase>(TryGetPawnOwner());
}

void UShadowSlaveEnemyAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!EnemyCharacter)
	{
		EnemyCharacter = Cast<AShadowSlaveEnemyCharacterBase>(TryGetPawnOwner());
	}

	if (!EnemyCharacter)
	{
		return;
	}

	bIsStaggered = EnemyCharacter->IsStaggered();
	bIsDead = !EnemyCharacter->IsAlive();

	if (AShadowSlaveAIController* AICon = Cast<AShadowSlaveAIController>(EnemyCharacter->GetController()))
	{
		AIState = AICon->GetAIState();
		bIsChasing = (AIState == EShadowSlaveAIState::Chasing);
		bIsAttacking = (AIState == EShadowSlaveAIState::Attacking) || (CurrentCombatState == ECombatState::Attacking);
	}
	else
	{
		AIState = bIsDead ? EShadowSlaveAIState::Dead : EShadowSlaveAIState::Idle;
		bIsChasing = false;
		bIsAttacking = (CurrentCombatState == ECombatState::Attacking);
	}
}
