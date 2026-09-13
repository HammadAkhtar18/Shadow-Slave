// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Camera/PlayerCameraManager.h"
#include "ShadowSlavePlayerCameraManager.generated.h"

/**
 * Camera Manager for the Shadow Slave player controller.
 * Enforces comfortable third-person pitch/yaw limits and handles camera shakes/transitions.
 */
UCLASS()
class SHADOWSLAVE_API AShadowSlavePlayerCameraManager : public APlayerCameraManager
{
	GENERATED_BODY()

public:
	AShadowSlavePlayerCameraManager();

protected:
	/** Minimum view pitch limit in degrees (looking up/down bounds) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Camera")
	float DefaultPitchMin = -65.0f;

	/** Maximum view pitch limit in degrees */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Camera")
	float DefaultPitchMax = 50.0f;
};
