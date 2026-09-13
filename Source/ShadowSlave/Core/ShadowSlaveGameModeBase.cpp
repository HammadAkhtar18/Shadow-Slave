// Copyright Epic Games, Inc. All Rights Reserved.

#include "Core/ShadowSlaveGameModeBase.h"
#include "Core/ShadowSlavePlayerController.h"
#include "Characters/ShadowSlavePlayerCharacter.h"
#include "UObject/ConstructorHelpers.h"

AShadowSlaveGameModeBase::AShadowSlaveGameModeBase()
{
	// Set default pawn class to our C++ player character
	DefaultPawnClass = AShadowSlavePlayerCharacter::StaticClass();

	// Set default player controller class to our C++ player controller
	PlayerControllerClass = AShadowSlavePlayerController::StaticClass();
}
