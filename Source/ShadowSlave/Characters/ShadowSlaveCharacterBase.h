// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ShadowSlaveCharacterBase.generated.h"

/**
 * Base character class for all characters in Shadow Slave (player, companions, enemies).
 * Encapsulates core locomotion configuration and lifecycle hooks, keeping specialized systems modular.
 */
UCLASS(Abstract)
class SHADOWSLAVE_API AShadowSlaveCharacterBase : public ACharacter
{
	GENERATED_BODY()

public:
	AShadowSlaveCharacterBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

	/** Lifecycle hook for initializing attributes (health, soul essence, etc.) when system is added */
	virtual void InitializeAttributes();

	/** Lifecycle hook for death handling to be overridden or subscribed to by gameplay systems */
	virtual void HandleDeath();

public:
	/** Returns whether this character is currently alive */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Character")
	virtual bool IsAlive() const { return bIsAlive; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Character")
	bool bIsAlive = true;
};
