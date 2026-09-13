// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Memories/ShadowSlaveMemoryTypes.h"
#include "Memories/ShadowSlaveMemoryDefinition.h"
#include "ShadowSlaveMemoryComponent.generated.h"

class UShadowSlaveInventoryComponent;

/**
 * Reusable Actor Component managing an actor's Memory collection, manifestation state, and equipment.
 * Designed purely event-driven without tick overhead.
 * Suitable for player characters, NPCs, or any entity capable of holding Memories.
 */
UCLASS(ClassGroup = (ShadowSlave), meta = (BlueprintSpawnableComponent))
class SHADOWSLAVE_API UShadowSlaveMemoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UShadowSlaveMemoryComponent();

	/* --- Memory Operations --- */

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
	 * Returns true if valid and added.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Memories|Operations")
	bool AddMemoryInstance(const FShadowSlaveMemoryInstance& InInstance);

	/**
	 * Removes a specific Memory instance by its unique InstanceId.
	 * If the Memory is currently equipped, it will be unequipped before removal.
	 * Returns true if the Memory was found and removed.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Memories|Operations")
	bool RemoveMemory(const FGuid& InstanceId);

	/**
	 * Removes the first Memory instance matching the given definition.
	 * Returns true if an instance was found and removed.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Memories|Operations")
	bool RemoveMemoryByDefinition(const UShadowSlaveMemoryDefinition* MemoryDef);

	/**
	 * Equips or manifests a Memory instance by its unique InstanceId.
	 * Returns true if the Memory can be equipped and was successfully equipped.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Memories|Operations")
	bool EquipMemory(const FGuid& InstanceId);

	/**
	 * Unequips or recalls a manifested Memory instance by its unique InstanceId.
	 * Returns true if the Memory was equipped and successfully unequipped.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Memories|Operations")
	bool UnequipMemory(const FGuid& InstanceId);

	/**
	 * Updates the runtime state of an individual Memory instance.
	 * Returns true if the instance was found and state updated.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Memories|Operations")
	bool SetMemoryState(const FGuid& InstanceId, EShadowSlaveMemoryState NewState);

	/**
	 * Empties all Memories held by this component, unequipping any active ones.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Memories|Operations")
	void ClearMemories();

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

	/** Returns all Memories that are currently manifested or equipped */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Memories|Queries")
	TArray<FShadowSlaveMemoryInstance> GetEquippedMemories() const;

	/* --- Inventory Ecosystem Integration --- */

	/**
	 * Bridges an owned Memory instance into an inventory component if an associated item definition is configured.
	 * Returns true if successfully transferred or mirrored.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Memories|Integration")
	bool BridgeToInventory(const FGuid& InstanceId, UShadowSlaveInventoryComponent* TargetInventory, int32& OutRemainder);

	/* --- Debug & Inspection --- */

	/** Outputs formatted contents of all owned Memories to the debug log */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Memories|Debug")
	void LogMemoryContents() const;

	/* --- Event Delegates --- */

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Memories|Events")
	FOnMemoryAddedSignature OnMemoryAdded;

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Memories|Events")
	FOnMemoryRemovedSignature OnMemoryRemoved;

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Memories|Events")
	FOnMemoryEquippedSignature OnMemoryEquipped;

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Memories|Events")
	FOnMemoryUnequippedSignature OnMemoryUnequipped;

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Memories|Events")
	FOnMemoryStateChangedSignature OnMemoryStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Memories|Events")
	FOnMemoryCollectionChangedSignature OnMemoryCollectionChanged;

protected:
	/** Collection of runtime Memory instances owned by this component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Memories|State")
	TArray<FShadowSlaveMemoryInstance> Memories;
};
