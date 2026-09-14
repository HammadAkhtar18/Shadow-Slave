// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Memories/ShadowSlaveMemoryTypes.h"
#include "Memories/ShadowSlaveMemoryDefinition.h"
#include "ShadowSlaveMemoryComponent.generated.h"

class UShadowSlaveInventoryComponent;

/**
 * Reusable Actor Component managing an actor's authoritative Memory collection, manifestation state, and equipment.
 * Designed purely event-driven without tick overhead.
 *
 * NOTE ON SOUL SEA:
 * This component acts as the gameplay-engineering abstraction for owned Memories.
 * It is NOT literally the metaphysical Soul Sea itself (which involves distinct progression,
 * soul cores, and soul space visualization to be addressed separately).
 */
UCLASS(ClassGroup = (ShadowSlave), meta = (BlueprintSpawnableComponent))
class SHADOWSLAVE_API UShadowSlaveMemoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UShadowSlaveMemoryComponent();

	/* --- Memory Ownership & Lifecycle --- */

	/**
	 * Explicit acquisition API: instantiates and adds a new Memory to this component based on the provided definition.
	 * Returns true if successful and outputs the fully initialized runtime instance with unique GUID and Dormant state.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Memories|Operations")
	bool AcquireMemory(UShadowSlaveMemoryDefinition* MemoryDef, FShadowSlaveMemoryInstance& OutInstance);

	/**
	 * Instantiates and adds a new Memory to this component based on the provided definition.
	 * Returns true if the Memory was successfully added, and outputs its unique InstanceId.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Memories|Operations")
	bool AddMemory(UShadowSlaveMemoryDefinition* MemoryDef, FGuid& OutInstanceId);

	/**
	 * Convenience overload for adding a Memory without capturing the InstanceId.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Memories|Operations", meta = (DisplayName = "Add Memory (Simple)"))
	bool AddMemorySimple(UShadowSlaveMemoryDefinition* MemoryDef);

	/**
	 * Adds an existing Memory instance directly.
	 * Returns true if valid and added without GUID conflict.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Memories|Operations")
	bool AddMemoryInstance(const FShadowSlaveMemoryInstance& InInstance);

	/**
	 * Removes a Memory from this component's ownership (e.g. transfer, trade, unbinding).
	 * Differentiated from physical/canon destruction (see DestroyMemory).
	 * If the Memory is currently equipped, it will be unequipped before removal.
	 * Returns true if the Memory was found and removed.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Memories|Operations")
	bool RemoveMemory(const FGuid& InstanceId);

	/**
	 * Removes the first Memory instance matching the given definition from ownership.
	 * Returns true if an instance was found and removed.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Memories|Operations")
	bool RemoveMemoryByDefinition(const UShadowSlaveMemoryDefinition* MemoryDef);

	/**
	 * Explicit canon destruction of a Memory instance (ceases to exist).
	 * Distinct from normal ownership removal; triggers destruction events.
	 * Returns true if the Memory was found and destroyed.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Memories|Operations")
	bool DestroyMemory(const FGuid& InstanceId);

	/**
	 * Explicit canon consumption of a Memory instance.
	 * Validates consumable status on definition, unequips if needed, and removes from ownership.
	 * Outputs the applied consumption effect.
	 * Returns true if successfully consumed.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Memories|Operations")
	bool ConsumeMemory(const FGuid& InstanceId, FShadowSlaveMemoryConsumptionEffect& OutEffectApplied);

	/**
	 * Empties all Memories held by this component, unequipping any active ones.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Memories|Operations")
	void ClearMemories();

	/**
	 * Dedicated persistence restoration API: replaces current owned Memories with saved instances.
	 * Preserves original instance GUIDs, equipped state, runtime state, and dynamic properties without triggering gameplay acquisition rules.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Memories|Persistence")
	void RestoreMemories(const TArray<FShadowSlaveMemoryInstance>& InInstances);

	/* --- Equipment Operations --- */

	/**
	 * Equips or manifests a Memory instance by its unique InstanceId.
	 * Respects component policy and definition exclusivity settings.
	 * Does not automatically transition technical runtime State (decoupled).
	 * Returns true if the Memory can be equipped and was successfully equipped.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Memories|Equipment")
	bool EquipMemory(const FGuid& InstanceId);

	/**
	 * Unequips or recalls a manifested Memory instance by its unique InstanceId.
	 * Does not automatically transition technical runtime State (decoupled).
	 * Returns true if the Memory was equipped and successfully unequipped.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Memories|Equipment")
	bool UnequipMemory(const FGuid& InstanceId);

	/* --- Runtime State --- */

	/**
	 * Updates the technical runtime state of an individual Memory instance.
	 * Returns true if the instance was found and state updated (or already in that state).
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Memories|Operations")
	bool SetMemoryState(const FGuid& InstanceId, EShadowSlaveMemoryState NewState);

	/**
	 * Retrieves current runtime state of a Memory instance.
	 */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Memories|Queries")
	bool GetMemoryState(const FGuid& InstanceId, EShadowSlaveMemoryState& OutState) const;

	/* --- Dynamic Instance Properties & Modification Hooks --- */

	/** Sets an instance-level dynamic property on an owned Memory, broadcasting OnMemoryModified */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Memories|Operations")
	bool SetMemoryDynamicProperty(const FGuid& InstanceId, FName Key, const FString& Value);

	/** Retrieves an instance-level dynamic property from an owned Memory */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Memories|Queries")
	bool GetMemoryDynamicProperty(const FGuid& InstanceId, FName Key, FString& OutValue) const;

	/** Removes an instance-level dynamic property from an owned Memory */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Memories|Operations")
	bool RemoveMemoryDynamicProperty(const FGuid& InstanceId, FName Key);

	/* --- Memory Queries --- */

	/** Returns true if this component contains at least one instance of MemoryDef */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Memories|Queries")
	bool HasMemory(const UShadowSlaveMemoryDefinition* MemoryDef) const;

	/** Returns true if a Memory with the given unique InstanceId is owned */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Memories|Queries")
	bool HasMemoryByInstanceId(const FGuid& InstanceId) const;

	/** Finds a Memory by its unique InstanceId; returns true and outputs the instance */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Memories|Queries")
	bool FindMemory(const FGuid& InstanceId, FShadowSlaveMemoryInstance& OutInstance) const;

	/** Finds the first Memory matching MemoryDef; returns true and outputs the instance */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Memories|Queries")
	bool FindMemoryByDefinition(const UShadowSlaveMemoryDefinition* MemoryDef, FShadowSlaveMemoryInstance& OutInstance) const;

	/** Finds all Memory instances matching the given definition (supports multiple distinct instances of same definition) */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Memories|Queries")
	TArray<FShadowSlaveMemoryInstance> FindAllMemoriesByDefinition(const UShadowSlaveMemoryDefinition* MemoryDef) const;

	/** Returns whether the Memory associated with InstanceId is currently equipped */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Memories|Queries")
	bool IsMemoryEquipped(const FGuid& InstanceId) const;

	/** Returns total number of Memories currently held */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Memories|Queries")
	int32 GetMemoryCount() const { return Memories.Num(); }

	/** Returns all Memory instances currently held */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Memories|Queries")
	const TArray<FShadowSlaveMemoryInstance>& GetMemories() const { return Memories; }

	/** Returns all Memories filtered by technical category */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Memories|Queries")
	TArray<FShadowSlaveMemoryInstance> GetMemoriesByCategory(EShadowSlaveMemoryCategory Category) const;

	/** Returns all Memories filtered by canon Rank */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Memories|Queries")
	TArray<FShadowSlaveMemoryInstance> GetMemoriesByRank(EShadowSlaveMemoryRank Rank) const;

	/** Returns all Memories filtered by canon Tier */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Memories|Queries")
	TArray<FShadowSlaveMemoryInstance> GetMemoriesByTier(EShadowSlaveMemoryTier Tier) const;

	/** Returns all Memories that have a verified/known canon Rank */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Memories|Queries")
	TArray<FShadowSlaveMemoryInstance> GetMemoriesWithKnownRank() const;

	/** Returns all Memories that have a verified/known canon Tier */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Memories|Queries")
	TArray<FShadowSlaveMemoryInstance> GetMemoriesWithKnownTier() const;

	/** Returns all Memories that are currently manifested or equipped */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Memories|Queries")
	TArray<FShadowSlaveMemoryInstance> GetEquippedMemories() const;

	/** Returns all currently equipped Memories bound to the specified equipment slot */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Memories|Queries")
	TArray<FShadowSlaveMemoryInstance> GetEquippedMemoriesBySlot(EShadowSlaveEquipmentSlot Slot) const;

	/** Finds the first equipped Memory in the designated equipment slot (returns false if none or Slot == None) */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Memories|Queries")
	bool GetEquippedMemoryInSlot(EShadowSlaveEquipmentSlot Slot, FShadowSlaveMemoryInstance& OutInstance) const;

	/** Returns whether the designated equipment slot currently has at least one equipped Memory */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Memories|Queries")
	bool IsSlotOccupied(EShadowSlaveEquipmentSlot Slot) const;

	/* --- Consumption Hooks --- */

	/** Returns true if the specified Memory is verified and configured as consumable */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Memories|Consumption")
	bool CanConsumeMemory(const FGuid& InstanceId) const;

	/** Retrieves consumption effect data for the specified Memory if consumable */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Memories|Consumption")
	bool GetMemoryConsumptionEffect(const FGuid& InstanceId, FShadowSlaveMemoryConsumptionEffect& OutEffect) const;

	/* --- Inventory Ecosystem Integration --- */

	/**
	 * Safe ownership transfer boundary: transfers an owned Memory instance into an inventory component.
	 * Requires the Memory Definition to configure a valid AssociatedItemDefinition.
	 * If the item is accepted by TargetInventory without remainder, the Memory is removed from this component,
	 * ensuring single authoritative ownership with zero duplication across systems.
	 * If no AssociatedItemDefinition exists or TargetInventory is full, transfer fails safely and Memory remains owned.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Memories|Integration")
	bool TransferToInventory(const FGuid& InstanceId, UShadowSlaveInventoryComponent* TargetInventory, int32& OutRemainder);

	/* --- Debug & Inspection --- */

	/** Outputs formatted contents of all owned Memories to the debug log */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Memories|Debug")
	void LogMemoryContents() const;

	/* --- Configuration Policies --- */

	/**
	 * When enabled, automatically unequips any previously equipped Memory occupying
	 * the same non-None EquipmentSlot when a new Memory is equipped.
	 * Individual definitions may also set bRequiresExclusiveSlot.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Memories|Policy")
	bool bEnforceUniqueSlotEquip = false;

	/* --- Event Delegates --- */

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Memories|Events")
	FOnMemoryAddedSignature OnMemoryAdded;

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Memories|Events")
	FOnMemoryRemovedSignature OnMemoryRemoved;

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Memories|Events")
	FOnMemoryDestroyedSignature OnMemoryDestroyed;

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Memories|Events")
	FOnMemoryConsumedSignature OnMemoryConsumed;

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Memories|Events")
	FOnMemoryEquippedSignature OnMemoryEquipped;

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Memories|Events")
	FOnMemoryUnequippedSignature OnMemoryUnequipped;

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Memories|Events")
	FOnMemoryTransferredSignature OnMemoryTransferred;

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Memories|Events")
	FOnMemoryModifiedSignature OnMemoryModified;

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Memories|Events")
	FOnMemoryStateChangedSignature OnMemoryStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Memories|Events")
	FOnMemoryCollectionChangedSignature OnMemoryCollectionChanged;

protected:
	/** Collection of runtime Memory instances owned authoritatively by this component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Memories|State")
	TArray<FShadowSlaveMemoryInstance> Memories;
};
