// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AI/ShadowSlaveEnemyCharacterBase.h"
#include "ShadowSlaveTestEnemy.generated.h"

class UStaticMeshComponent;

/**
 * Generic temporary test enemy for verifying AI detection, chasing, attacking,
 * damage receiving, staggering, and dying.
 * Not associated with any canon Shadow Slave creature.
 */
UCLASS()
class SHADOWSLAVE_API AShadowSlaveTestEnemy : public AShadowSlaveEnemyCharacterBase
{
	GENERATED_BODY()

public:
	AShadowSlaveTestEnemy(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void BeginPlay() override;
	virtual void HandleDeath() override;

protected:
	/** Optional placeholder visual mesh for testing when no skeletal mesh is assigned */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|AI|Test")
	TObjectPtr<UStaticMeshComponent> PlaceholderMesh;
};
