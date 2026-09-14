// Copyright Epic Games, Inc. All Rights Reserved.

#include "Core/ShadowSlavePlayerController.h"
#include "Core/ShadowSlavePlayerCameraManager.h"
#include "UI/ShadowSlaveHUD.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "Engine/LocalPlayer.h"

AShadowSlavePlayerController::AShadowSlavePlayerController()
{
	PlayerCameraManagerClass = AShadowSlavePlayerCameraManager::StaticClass();
}

void AShadowSlavePlayerController::BeginPlay()
{
	Super::BeginPlay();

	// Add the project's default Input Mapping Context to the local player subsystem
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		if (DefaultMappingContext)
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void AShadowSlavePlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (AShadowSlaveHUD* CustomHUD = GetHUD<AShadowSlaveHUD>())
	{
		CustomHUD->NotifyPawnChanged(InPawn);
	}
}

void AShadowSlavePlayerController::AcknowledgePossession(APawn* InPawn)
{
	Super::AcknowledgePossession(InPawn);

	if (AShadowSlaveHUD* CustomHUD = GetHUD<AShadowSlaveHUD>())
	{
		CustomHUD->NotifyPawnChanged(InPawn);
	}
}
