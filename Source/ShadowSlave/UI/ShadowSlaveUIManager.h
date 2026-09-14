// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UI/ShadowSlaveUITypes.h"
#include "ShadowSlaveUIManager.generated.h"

class APlayerController;
class UShadowSlaveHUDWidget;

/**
 * Game Instance Subsystem coordinating UI lifecycle and presentation in Shadow Slave.
 * Responsible for root HUD creation, viewport management, local player association,
 * and global UI event/notification routing.
 *
 * NOTE: Strictly a presentation coordinator; contains zero gameplay authority.
 */
UCLASS()
class SHADOWSLAVE_API UShadowSlaveUIManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UShadowSlaveUIManager();

	/** Creates and adds the root HUD widget to the viewport for the specified player controller */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|UI|Manager")
	UShadowSlaveHUDWidget* CreateRootHUD(APlayerController* PlayerController, TSubclassOf<UShadowSlaveHUDWidget> CustomWidgetClass = nullptr);

	/** Retrieves the active root HUD widget for the specified player controller */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|UI|Manager")
	UShadowSlaveHUDWidget* GetRootHUD(const APlayerController* PlayerController) const;

	/** Removes and destroys the root HUD widget for the specified player controller */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|UI|Manager")
	bool RemoveRootHUD(APlayerController* PlayerController);

	/** Toggles or sets HUD visibility for the specified player controller */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|UI|Manager")
	void SetHUDVisibility(APlayerController* PlayerController, bool bVisible);

	/** Sends a notification to the specified player's active HUD */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|UI|Manager")
	void SendNotification(APlayerController* PlayerController, const FShadowSlaveNotificationMessage& Message);

	/** Returns default root HUD widget class */
	TSubclassOf<UShadowSlaveHUDWidget> GetDefaultHUDWidgetClass() const { return DefaultHUDWidgetClass; }

protected:
	/** Default widget class used when no custom class is provided */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|UI|Config")
	TSubclassOf<UShadowSlaveHUDWidget> DefaultHUDWidgetClass;

	/** Active root HUD instances mapped per player controller */
	UPROPERTY(Transient)
	TMap<TWeakObjectPtr<APlayerController>, TObjectPtr<UShadowSlaveHUDWidget>> ActiveHUDs;
};
