// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ShadowSlaveGameModeBase.generated.h"

/**
 * Foundation Game Mode Base for Shadow Slave.
 * Configures the default player character pawn, player controller, HUD, and session bootstrap.
 */
UCLASS()
class SHADOWSLAVE_API AShadowSlaveGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
	AShadowSlaveGameModeBase();

	virtual void StartPlay() override;
};
