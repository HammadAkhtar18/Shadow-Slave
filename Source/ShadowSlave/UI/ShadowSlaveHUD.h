// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "ShadowSlaveHUD.generated.h"

class UShadowSlaveHUDWidget;

/**
 * Standard Engine HUD class for Shadow Slave.
 * Attached to APlayerController to manage local root HUD instantiation via UShadowSlaveUIManager.
 */
UCLASS()
class SHADOWSLAVE_API AShadowSlaveHUD : public AHUD
{
	GENERATED_BODY()

public:
	AShadowSlaveHUD();

	virtual void PostInitializeComponents() override;

	/** Returns active root HUD widget instance */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|UI")
	UShadowSlaveHUDWidget* GetRootHUDWidget() const;

	/** Manually notifies HUD that possessed pawn changed */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|UI")
	void NotifyPawnChanged(APawn* NewPawn);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Widget Blueprint class to spawn for root HUD */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|UI")
	TSubclassOf<UShadowSlaveHUDWidget> HUDWidgetClass;

private:
	TWeakObjectPtr<UShadowSlaveHUDWidget> RootHUDWidget;
};
