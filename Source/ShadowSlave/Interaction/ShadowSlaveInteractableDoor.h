// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Interaction/ShadowSlaveInteractableActor.h"
#include "ShadowSlaveInteractableDoor.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDoorStateChangedSignature, bool, bIsOpen, AActor*, Interactor);

/**
 * Reusable foundation for interactable doors and gates in Shadow Slave.
 * Tracks open/closed/locked states and key requirements.
 * Provides Blueprint-implementable and C++ hooks for visual/movement animations.
 */
UCLASS()
class SHADOWSLAVE_API AShadowSlaveInteractableDoor : public AShadowSlaveInteractableActor
{
	GENERATED_BODY()

public:
	AShadowSlaveInteractableDoor();

	virtual FText GetInteractionPrompt_Implementation(AActor* Interactor) override;

	/** Returns whether this door is currently open */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Interaction|Door")
	bool IsOpen() const { return bIsOpen; }

	/** Returns whether this door is locked */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Interaction|Door")
	bool IsLocked() const { return bIsLocked; }

	/** Opens the door if unlocked */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Interaction|Door")
	bool OpenDoor(AActor* Interactor);

	/** Closes the door */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Interaction|Door")
	bool CloseDoor(AActor* Interactor);

	/** Toggles the door state */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Interaction|Door")
	bool ToggleDoor(AActor* Interactor);

	/** Unlocks the door with a matching key */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Interaction|Door")
	bool UnlockDoor(FName InKeyId);

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Interaction|Door")
	FOnDoorStateChangedSignature OnDoorStateChanged;

protected:
	virtual FShadowSlaveInteractionResult ExecuteInteraction(AActor* Interactor) override;

	/** Called when door state changes; hook for Blueprint/Animation logic */
	UFUNCTION(BlueprintImplementableEvent, Category = "ShadowSlave|Interaction|Door")
	void ReceiveDoorStateChanged(bool bNewIsOpen, AActor* Interactor);

	/** Whether the door is currently open */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Interaction|Door")
	bool bIsOpen = false;

	/** Whether the door requires unlocking before opening */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Interaction|Door")
	bool bIsLocked = false;

	/** Optional key identifier required to unlock this door */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Interaction|Door")
	FName RequiredKeyId = NAME_None;
};
