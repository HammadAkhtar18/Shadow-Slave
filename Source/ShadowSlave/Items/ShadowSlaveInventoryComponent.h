// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Items/ShadowSlaveItemTypes.h"
#include "Items/ShadowSlaveItemDefinition.h"
#include "ShadowSlaveInventoryComponent.generated.h"

/**
 * Reusable Actor Component managing an actor's inventory slots, stacking, and capacity.
 * Operates purely event-driven without tick overhead.
 * Usable by players, NPCs, future containers, and enemies.
 */
UCLASS(ClassGroup = (ShadowSlave), meta = (BlueprintSpawnableComponent))
class SHADOWSLAVE_API UShadowSlaveInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UShadowSlaveInventoryComponent();

	/* --- Capacity API --- */

	/** Returns total number of available inventory slots */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Inventory|Capacity")
	int32 GetCapacity() const { return MaxSlots; }

	/** Updates total slot capacity (clamped to at least 1) */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Inventory|Capacity")
	void SetCapacity(int32 NewCapacity);

	/** Returns number of currently occupied slots */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Inventory|Capacity")
	int32 GetUsedSlotCount() const { return Slots.Num(); }

	/** Returns number of free unoccupied slots */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Inventory|Capacity")
	int32 GetFreeSlotCount() const { return FMath::Max(0, MaxSlots - Slots.Num()); }

	/** Returns true if all inventory slots are occupied */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Inventory|Capacity")
	bool IsFull() const { return Slots.Num() >= MaxSlots; }

	/* --- Inventory Operations --- */

	/**
	 * Adds items to the inventory, respecting stackability and capacity.
	 * Returns true if at least one item was added.
	 * OutRemainder outputs any items that could not fit due to capacity.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Inventory|Operations")
	bool AddItem(UShadowSlaveItemDefinition* ItemDef, int32 Quantity, int32& OutRemainder);

	/** Blueprint convenience wrapper without OutRemainder parameter */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Inventory|Operations", meta = (DisplayName = "Add Item (Simple)"))
	bool AddItemSimple(UShadowSlaveItemDefinition* ItemDef, int32 Quantity = 1);

	/**
	 * Removes Quantity of ItemDef from the inventory.
	 * Returns true if requested quantity was completely removed.
	 * If inventory contains fewer items than Quantity, operation fails safely with zero removal.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Inventory|Operations")
	bool RemoveItem(const UShadowSlaveItemDefinition* ItemDef, int32 Quantity = 1);

	/**
	 * Removes Quantity from a specific item instance stack by GUID.
	 * Returns true if successfully removed.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Inventory|Operations")
	bool RemoveItemByInstanceId(const FGuid& InstanceId, int32 Quantity = 1);

	/** Returns whether the inventory contains at least Quantity of ItemDef */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Inventory|Queries")
	bool HasItem(const UShadowSlaveItemDefinition* ItemDef, int32 Quantity = 1) const;

	/** Finds the first slot matching ItemDef; returns true and outputs ItemInstance */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Inventory|Queries")
	bool FindItem(const UShadowSlaveItemDefinition* ItemDef, FShadowSlaveItemInstance& OutInstance) const;

	/** Finds an item instance by its unique InstanceId; returns true and outputs ItemInstance */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Inventory|Queries")
	bool FindItemByInstanceId(const FGuid& InstanceId, FShadowSlaveItemInstance& OutInstance) const;

	/** Returns true if an item instance with the given unique InstanceId exists */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Inventory|Queries")
	bool HasItemByInstanceId(const FGuid& InstanceId) const;

	/** Returns total count of ItemDef across all slots */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Inventory|Queries")
	int32 GetTotalItemCount(const UShadowSlaveItemDefinition* ItemDef) const;

	/** Empties all slots in the inventory */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Inventory|Operations")
	void ClearInventory();

	/**
	 * Dedicated persistence restoration API: replaces current inventory slots with saved instances.
	 * Preserves original instance GUIDs, quantities, and dynamic properties without triggering gameplay acquisition rules.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Inventory|Persistence")
	void RestoreInventory(const TArray<FShadowSlaveItemInstance>& InInstances, int32 InCapacity);

	/** Returns all current item slots */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Inventory|Queries")
	const TArray<FShadowSlaveItemInstance>& GetSlots() const { return Slots; }

	/** Queries items filtered by item type */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Inventory|Queries")
	TArray<FShadowSlaveItemInstance> GetItemsByType(EShadowSlaveItemType ItemType) const;

	/** Outputs formatted inventory contents to the debug log */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Inventory|Debug")
	void LogInventoryContents() const;

	/* --- Delegates --- */

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Inventory|Events")
	FOnInventoryItemAddedSignature OnItemAdded;

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Inventory|Events")
	FOnInventoryItemRemovedSignature OnItemRemoved;

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Inventory|Events")
	FOnInventoryChangedSignature OnInventoryChanged;

protected:
	/** Maximum number of inventory slots */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Inventory|Capacity", meta = (ClampMin = "1"))
	int32 MaxSlots = 20;

	/** Current inventory slots holding item instances */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Inventory|State")
	TArray<FShadowSlaveItemInstance> Slots;
};
