// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Interaction/ShadowSlaveInteractableActor.h"
#include "ShadowSlaveInteractableContainer.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnContainerStateChangedSignature, bool, bIsOpen, AActor*, Interactor);

/**
 * Reusable foundation for interactable containers (chests, caches, trunks).
 * Manages open/closed/locked states and exposes hooks for future content/reward delegation.
 * Does NOT generate random loot or implement arbitrary loot tables.
 */
UCLASS()
class SHADOWSLAVE_API AShadowSlaveInteractableContainer : public AShadowSlaveInteractableActor
{
	GENERATED_BODY()

public:
	AShadowSlaveInteractableContainer();

	virtual FText GetInteractionPrompt_Implementation(AActor* Interactor) override;

	/** Returns whether this container is currently open */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Interaction|Container")
	bool IsOpen() const { return bIsOpen; }

	/** Returns whether this container is locked */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Interaction|Container")
	bool IsLocked() const { return bIsLocked; }

	/** Opens the container if unlocked */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Interaction|Container")
	bool OpenContainer(AActor* Interactor);

	/** Closes the container */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Interaction|Container")
	bool CloseContainer(AActor* Interactor);

	/** Toggles the open/closed state of the container */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Interaction|Container")
	bool ToggleContainer(AActor* Interactor);

	/** Unlocks the container with a key identifier */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Interaction|Container")
	bool UnlockContainer(FName InKeyId);

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Interaction|Container")
	FOnContainerStateChangedSignature OnContainerStateChanged;

protected:
	virtual FShadowSlaveInteractionResult ExecuteInteraction(AActor* Interactor) override;

	/* --- IShadowSlaveSaveableInterface Overrides --- */
	virtual bool CaptureSaveRecord_Implementation(FShadowSlaveWorldActorSaveRecord& OutRecord) override;
	virtual bool RestoreSaveRecord_Implementation(const FShadowSlaveWorldActorSaveRecord& InRecord) override;

	/** Whether the container is currently open */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Interaction|Container")
	bool bIsOpen = false;

	/** Whether the container requires unlocking before it can be opened */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Interaction|Container")
	bool bIsLocked = false;

	/** Optional key identifier required to unlock this container */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Interaction|Container")
	FName RequiredKeyId = NAME_None;
};
