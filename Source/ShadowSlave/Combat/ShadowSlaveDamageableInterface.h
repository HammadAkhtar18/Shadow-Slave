// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Combat/ShadowSlaveCombatTypes.h"
#include "ShadowSlaveDamageableInterface.generated.h"

UINTERFACE(MinimalAPI, BlueprintType)
class UShadowSlaveDamageableInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Interface for any actor in the world capable of receiving damage in the Shadow Slave combat framework.
 */
class SHADOWSLAVE_API IShadowSlaveDamageableInterface
{
	GENERATED_BODY()

public:
	/** Applies incoming combat damage to this actor. Returns actual damage taken. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ShadowSlave|Combat")
	float TakeDamageCustom(const FShadowSlaveDamageInfo& DamageInfo);

	/** Returns whether the target is alive and valid for combat targeting. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ShadowSlave|Combat")
	bool IsAlive() const;
};
