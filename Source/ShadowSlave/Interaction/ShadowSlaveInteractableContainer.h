// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Interaction/ShadowSlaveInteractableActor.h"
#include "Items/ShadowSlaveItemTypes.h"
#include "Items/ShadowSlaveItemDefinition.h"
#include "ShadowSlaveInteractableContainer.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnContainerStateChangedSignature, bool, bIsOpen, AActor*, Interactor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnContainerLootTransferredSignature, AShadowSlaveInteractableContainer*, Container, const FShadowSlaveItemInstance&, TransferredItem, AActor*, Interactor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnContainerLootChangedSignature, AShadowSlaveInteractableContainer*, Container, int32, RemainingItemCount);

/**
 * Reusable foundation for interactable containers (chests, caches, trunks).
 * Manages open/closed/locked states and transactional loot storage and transfers.
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

	/* --- Loot & Storage Operations --- */

	/** Initializes or replaces the container's loot inventory */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Interaction|Container")
	void InitializeContainerLoot(const TArray<FShadowSlaveItemInstance>& InItems);

	/** Adds an item definition and quantity into the container's stored items */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Interaction|Container")
	void AddStoredItem(UShadowSlaveItemDefinition* ItemDef, int32 InQuantity = 1);

	/** Returns true if the container has at least one valid item stored */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Interaction|Container")
	bool HasLoot() const;

	/** Returns number of item slots currently stored in this container */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Interaction|Container")
	int32 GetStoredItemCount() const { return StoredItems.Num(); }

	/** Returns reference to all currently stored item instances */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Interaction|Container")
	const TArray<FShadowSlaveItemInstance>& GetStoredItems() const { return StoredItems; }

	/**
	 * Transactionally transfers stored items from this container to the target Interactor's InventoryComponent.
	 * Re-entrancy guarded:
	 * - Locked container returns false with 0 transfers.
	 * - Each item entry is transferred at most once.
	 * - If interactor's inventory is full for an item, that item remains in the container.
	 * - If partial quantity is accepted, the remainder is retained in the container.
	 * - Only items fully or partially accepted by inventory are removed/decremented from container.
	 * Returns true if at least one item was transferred.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Interaction|Container")
	bool TransferLootToInteractor(AActor* Interactor, TArray<FShadowSlaveItemInstance>& OutTransferredItems);

	/* --- Event Delegates --- */

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Interaction|Container")
	FOnContainerStateChangedSignature OnContainerStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Interaction|Container")
	FOnContainerLootTransferredSignature OnContainerLootTransferred;

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Interaction|Container")
	FOnContainerLootChangedSignature OnContainerLootChanged;

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

	/** Configured or remaining item instances stored inside this container */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Interaction|Container")
	TArray<FShadowSlaveItemInstance> StoredItems;

	/** When true, opening the container also automatically attempts to transfer stored loot */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Interaction|Container")
	bool bAutoLootOnOpen = false;

	/** Re-entrancy guard during loot transfer transactions */
	UPROPERTY(Transient)
	bool bIsTransferringLoot = false;
};
