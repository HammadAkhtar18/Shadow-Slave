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

protected:
	virtual void BeginPlay() override;

	/** Default Input Mapping Context applied on start */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;
};
