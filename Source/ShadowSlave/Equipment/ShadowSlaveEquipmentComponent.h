// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Equipment/ShadowSlaveEquipmentTypes.h"
#include "Items/ShadowSlaveItemTypes.h"
#include "Memories/ShadowSlaveMemoryTypes.h"
#include "Attributes/ShadowSlaveAttributeTypes.h"
#include "ShadowSlaveEquipmentComponent.generated.h"

class UShadowSlaveAttributeComponent;
class UShadowSlaveInventoryComponent;
class UShadowSlaveMemoryComponent;
class UShadowSlaveItemDefinition;
class UShadowSlaveMemoryDefinition;

/**
 * Reusable Actor Component managing an actor's equipped items and Memories across equipment slots.
 * Coordinates between InventoryComponent, MemoryComponent, and AttributeComponent.
 * Operates purely event-driven without tick overhead.
 * Usable by players, NPCs, and enemies.
 */
UCLASS(ClassGroup = (ShadowSlave), meta = (BlueprintSpawnableComponent))
class SHADOWSLAVE_API UShadowSlaveEquipmentComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UShadowSlaveEquipmentComponent();

	/* --- Equipment Operations --- */

	/**
	 * Equips an inventory-backed item by its unique instance GUID.
	 * If Slot is None, attempts to use the item definition's designated EquipmentSlot.
	 * Returns true if successfully equipped.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Equipment")
	bool EquipItem(const FGuid& InstanceId, EShadowSlaveEquipmentSlot Slot = EShadowSlaveEquipmentSlot::None);

	/**
	 * Convenience helper to equip an item instance struct directly.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Equipment")
	bool EquipItemInstance(const FShadowSlaveItemInstance& ItemInstance, EShadowSlaveEquipmentSlot Slot = EShadowSlaveEquipmentSlot::None);

	/**
	 * Equips a Memory instance by its unique instance GUID.
	 * If Slot is None, attempts to use the Memory definition's designated EquipmentSlot.
	 * Returns true if successfully equipped.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Equipment")
	bool EquipMemory(const FGuid& InstanceId, EShadowSlaveEquipmentSlot Slot = EShadowSlaveEquipmentSlot::None);

	/**
	 * Convenience helper to equip a Memory instance struct directly.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Equipment")
	bool EquipMemoryInstance(const FShadowSlaveMemoryInstance& MemoryInstance, EShadowSlaveEquipmentSlot Slot = EShadowSlaveEquipmentSlot::None);

	/**
	 * Unequips whatever is currently occupying the specified equipment slot.
	 * Removes associated attribute modifiers and clears slot state.
	 * Returns true if an item/Memory was equipped and unequipped.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Equipment")
	bool UnequipSlot(EShadowSlaveEquipmentSlot Slot);

	/**
	 * Unequips the specified instance by its unique GUID regardless of which slot it occupies.
	 * Returns true if found and unequipped.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Equipment")
	bool UnequipInstance(const FGuid& InstanceId);

	/**
	 * Unequips all currently equipped slots and strips all equipment-derived modifiers.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Equipment")
	void UnequipAll();

	/* --- Queries --- */

	/** Returns true if the specified equipment slot is currently occupied */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Equipment")
	bool IsSlotOccupied(EShadowSlaveEquipmentSlot Slot) const;

	/** Retrieves the descriptor for the item/Memory equipped in the given slot */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Equipment")
	bool GetEquippedItemInSlot(EShadowSlaveEquipmentSlot Slot, FShadowSlaveEquippedItem& OutEquippedItem) const;

	/** Returns true if the given instance GUID is currently equipped in any slot */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Equipment")
	bool IsInstanceEquipped(const FGuid& InstanceId) const;

	/** Returns the equipment slot occupied by the given instance GUID, or None if not equipped */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Equipment")
	EShadowSlaveEquipmentSlot GetSlotForInstance(const FGuid& InstanceId) const;

	/** Returns all currently equipped item descriptors */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Equipment")
	TArray<FShadowSlaveEquippedItem> GetAllEquippedItems() const;

	/** Retrieves the underlying Inventory Item instance for an equipped slot, if backed by inventory */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Equipment")
	bool GetEquippedItemInstance(EShadowSlaveEquipmentSlot Slot, FShadowSlaveItemInstance& OutInstance) const;

	/** Retrieves the underlying Memory instance for an equipped slot, if backed by MemoryComponent */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Equipment")
	bool GetEquippedMemoryInstance(EShadowSlaveEquipmentSlot Slot, FShadowSlaveMemoryInstance& OutInstance) const;

	/* --- Companion Component Accessors --- */

	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Equipment")
	UShadowSlaveAttributeComponent* GetAttributeComponent() const;

	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Equipment")
	UShadowSlaveInventoryComponent* GetInventoryComponent() const;

	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Equipment")
	UShadowSlaveMemoryComponent* GetMemoryComponent() const;

	/* --- Delegates --- */

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Equipment|Events")
	FOnEquipmentItemEquippedSignature OnEquipmentItemEquipped;

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Equipment|Events")
	FOnEquipmentItemUnequippedSignature OnEquipmentItemUnequipped;

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Equipment|Events")
	FOnEquipmentSlotChangedSignature OnEquipmentSlotChanged;

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Equipment|Events")
	FOnEquipmentChangedSignature OnEquipmentChanged;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Binds dynamic event listeners to owner's companion components */
	void BindToCompanionComponents();

	/** Unbinds dynamic event listeners from companion components */
	void UnbindFromCompanionComponents();

	/** Applies attribute modifiers for an equipped source instance; returns true on success */
	bool ApplyModifiersForSource(const FGuid& SourceId, const TArray<FAttributeModifier>& Modifiers);

	/** Removes all attribute modifiers originating from a source instance */
	void RemoveModifiersForSource(const FGuid& SourceId);

	/* --- Event Handlers for Companion Components --- */

	UFUNCTION()
	void HandleInventoryItemRemoved(const FShadowSlaveItemInstance& ItemInstance, int32 QuantityRemoved);

	UFUNCTION()
	void HandleInventoryChanged();

	UFUNCTION()
	void HandleMemoryRemoved(const FShadowSlaveMemoryInstance& MemoryInstance);

	UFUNCTION()
	void HandleMemoryDestroyed(const FShadowSlaveMemoryInstance& MemoryInstance);

	UFUNCTION()
	void HandleMemoryConsumed(const FShadowSlaveMemoryInstance& MemoryInstance, const FShadowSlaveMemoryConsumptionEffect& ConsumedEffect);

	UFUNCTION()
	void HandleMemoryEquippedExternally(const FShadowSlaveMemoryInstance& MemoryInstance);

	UFUNCTION()
	void HandleMemoryUnequippedExternally(const FShadowSlaveMemoryInstance& MemoryInstance);

	UFUNCTION()
	void HandleMemoryCollectionChanged();

protected:
	/** Map of slot to currently equipped item/Memory descriptor */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Equipment")
	TMap<EShadowSlaveEquipmentSlot, FShadowSlaveEquippedItem> EquippedSlots;

	/** Guard flag preventing recursive sync loops between EquipmentComponent and MemoryComponent */
	bool bIsSyncingWithMemoryComponent = false;
};
