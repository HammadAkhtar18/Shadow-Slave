// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ShadowSlavePlayerController.generated.h"

class UInputMappingContext;

/**
 * Main Player Controller for Shadow Slave.
 * Manages player-level input contexts, HUD/UI hooks, and input routing.
 */
UCLASS()
class SHADOWSLAVE_API AShadowSlavePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AShadowSlavePlayerController();

	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Input")
	float GetLookSensitivityYaw() const { return LookSensitivityYaw; }

	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Input")
	float GetLookSensitivityPitch() const { return LookSensitivityPitch; }

protected:
	virtual void BeginPlay() override;

	/** Default Input Mapping Context applied on start */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	/** Horizontal look sensitivity multiplier */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Input", meta = (ClampMin = "0.1", ClampMax = "10.0"))
	float LookSensitivityYaw = 1.0f;

	/** Vertical look sensitivity multiplier */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Input", meta = (ClampMin = "0.1", ClampMax = "10.0"))
	float LookSensitivityPitch = 1.0f;
};
