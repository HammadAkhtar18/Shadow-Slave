// Copyright Epic Games, Inc. All Rights Reserved.

#include "Core/ShadowSlaveGameModeBase.h"
#include "Core/ShadowSlavePlayerController.h"
#include "Characters/ShadowSlavePlayerCharacter.h"
#include "Gameplay/ShadowSlaveGameplaySubsystem.h"
#include "UI/ShadowSlaveHUD.h"
#include "Engine/GameInstance.h"
#include "UObject/ConstructorHelpers.h"
#include "ShadowSlave.h"

AShadowSlaveGameModeBase::AShadowSlaveGameModeBase()
{
	// Set default pawn class to our C++ player character
	DefaultPawnClass = AShadowSlavePlayerCharacter::StaticClass();

	// Set default player controller class to our C++ player controller
	PlayerControllerClass = AShadowSlavePlayerController::StaticClass();

	// Set default HUD class to our C++ HUD
	HUDClass = AShadowSlaveHUD::StaticClass();
}

void AShadowSlaveGameModeBase::StartPlay()
{
	Super::StartPlay();

	// Authoritative runtime session startup trigger
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UShadowSlaveGameplaySubsystem* GameplaySub = GI->GetSubsystem<UShadowSlaveGameplaySubsystem>())
		{
			const bool bStarted = GameplaySub->StartGameplaySession();
			if (!bStarted)
			{
				UE_LOG(LogShadowSlave, Warning, TEXT("AShadowSlaveGameModeBase::StartPlay - StartGameplaySession returned false."));
			}
		}
		else
		{
			UE_LOG(LogShadowSlave, Warning, TEXT("AShadowSlaveGameModeBase::StartPlay - UShadowSlaveGameplaySubsystem unavailable."));
		}
	}
}
