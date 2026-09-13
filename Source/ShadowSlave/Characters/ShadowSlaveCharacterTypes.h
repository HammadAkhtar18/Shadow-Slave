// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ShadowSlaveCharacterTypes.generated.h"

/**
 * Represents the current movement/gait mode of a character.
 */
UENUM(BlueprintType)
enum class EShadowSlaveGait : uint8
{
	Walk UMETA(DisplayName = "Walk"),
	Sprint UMETA(DisplayName = "Sprint")
};

/**
 * Delegate broadcast when character gait transitions (e.g. Walk <-> Sprint).
 * Used by future stamina, animation, and audio subsystems.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGaitChangedSignature, EShadowSlaveGait, OldGait, EShadowSlaveGait, NewGait);
