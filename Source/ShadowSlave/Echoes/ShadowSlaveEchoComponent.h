// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Echoes/ShadowSlaveEchoTypes.h"
#include "Echoes/ShadowSlaveEchoDefinition.h"
#include "ShadowSlaveEchoComponent.generated.h"

class UShadowSlaveAttributeComponent;

/**
 * Reusable Actor Component managing an actor's authoritative Echo collection, manifestation/summoned state, and lifecycle.
 * Designed purely event-driven without tick overhead.
 *
 * NOTE ON AUTHORITY:
 * This component is the sole authority for Echo ownership, instance GUIDs, and summoned status.
 * Resource costs (such as Essence consumption upon summoning) are delegated to UShadowSlaveAttributeComponent.
 */
UCLASS(ClassGroup = (ShadowSlave), meta = (BlueprintSpawnableComponent))
class SHADOWSLAVE_API UShadowSlaveEchoComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UShadowSlaveEchoComponent();

	/* --- Echo Ownership & Lifecycle --- */

	/**
	 * Explicit acquisition API: instantiates and adds a new Echo to this component based on the provided definition.
	 * Returns true if successful and outputs the fully initialized runtime instance with unique GUID and Dormant state.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Echoes|Operations")
	bool AcquireEcho(UShadowSlaveEchoDefinition* EchoDef, FShadowSlaveEchoInstance& OutInstance);

	/**
	 * Instantiates and adds a new Echo to this component based on the provided definition.
	 * Returns true if the Echo was successfully added, and outputs its unique InstanceId.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Echoes|Operations")
	bool AddEcho(UShadowSlaveEchoDefinition* EchoDef, FGuid& OutInstanceId);

	/**
	 * Convenience overload for adding an Echo without capturing the InstanceId.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Echoes|Operations", meta = (DisplayName = "Add Echo (Simple)"))
	bool AddEchoSimple(UShadowSlaveEchoDefinition* EchoDef);

	/**
	 * Adds an existing Echo instance directly.
	 * Returns true if valid and added without GUID conflict.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Echoes|Operations")
	bool AddEchoInstance(const FShadowSlaveEchoInstance& InInstance);

	/**
	 * Removes an Echo from this component's ownership (e.g. transfer, unbinding, trading).
	 * Differentiated from destruction (see DestroyEcho).
	 * If the Echo is currently summoned, it is automatically dismissed before removal.
	 * Returns true if the Echo was found and removed.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Echoes|Operations")
	bool RemoveEcho(const FGuid& InstanceId);

	/**
	 * Removes the first Echo instance matching the given definition from ownership.
	 * Returns true if an instance was found and removed.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Echoes|Operations")
	bool RemoveEchoByDefinition(const UShadowSlaveEchoDefinition* EchoDef);

	/**
	 * Explicit destruction of an Echo instance (e.g. slain in battle or consumed/fed into shadows).
	 * Distinct from normal ownership removal; triggers destruction events.
	 * If summoned, it is automatically dismissed before destruction.
	 * Returns true if the Echo was found and destroyed.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Echoes|Operations")
	bool DestroyEcho(const FGuid& InstanceId);

	/**
	 * Empties all Echoes held by this component, dismissing any active ones.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Echoes|Operations")
	void ClearEchoes();

	/**
	 * Dedicated persistence restoration API: replaces current owned Echoes with saved instances.
	 * Preserves original instance GUIDs, summoned state, runtime state, and dynamic properties without triggering gameplay acquisition rules.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Echoes|Persistence")
	void RestoreEchoes(const TArray<FShadowSlaveEchoInstance>& InInstances);

	/* --- Summoning & Dismissal Operations --- */

	/**
	 * Manifests/summons an Echo into reality.
	 * Validates essence availability via UShadowSlaveAttributeComponent if a summon essence cost is configured.
	 * Returns true if successfully summoned (or already summoned).
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Echoes|Operations")
	bool SummonEcho(const FGuid& InstanceId);

	/**
	 * Dismisses/recalls a summoned Echo back into dormant soul storage.
	 * Returns true if successfully dismissed (or already dormant).
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Echoes|Operations")
	bool DismissEcho(const FGuid& InstanceId);

	/**
	 * Dismisses all currently summoned Echoes back to dormant state.
	 * Returns true if any Echoes were dismissed or none were summoned.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Echoes|Operations")
	bool DismissAllEchoes();

	/* --- Runtime State --- */

	/**
	 * Updates the technical runtime state of an individual Echo instance.
	 * Returns true if the instance was found and state updated (or already in that state).
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Echoes|Operations")
	bool SetEchoState(const FGuid& InstanceId, EShadowSlaveEchoState NewState);

	/**
	 * Retrieves current runtime state of an Echo instance.
	 */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Echoes|Queries")
	bool GetEchoState(const FGuid& InstanceId, EShadowSlaveEchoState& OutState) const;

	/* --- Dynamic Instance Properties --- */

	/** Sets an instance-level dynamic property on an owned Echo, broadcasting OnEchoModified */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Echoes|Operations")
	bool SetEchoDynamicProperty(const FGuid& InstanceId, FName Key, const FString& Value);

	/** Retrieves an instance-level dynamic property from an owned Echo */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Echoes|Queries")
	bool GetEchoDynamicProperty(const FGuid& InstanceId, FName Key, FString& OutValue) const;

	/** Removes an instance-level dynamic property from an owned Echo */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Echoes|Operations")
	bool RemoveEchoDynamicProperty(const FGuid& InstanceId, FName Key);

	/* --- Echo Queries --- */

	/** Returns true if this component contains at least one instance of EchoDef */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Echoes|Queries")
	bool HasEcho(const UShadowSlaveEchoDefinition* EchoDef) const;

	/** Returns true if an Echo with the given unique InstanceId is owned */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Echoes|Queries")
	bool HasEchoByInstanceId(const FGuid& InstanceId) const;

	/** Finds an Echo by its unique InstanceId; returns true and outputs the instance */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Echoes|Queries")
	bool FindEcho(const FGuid& InstanceId, FShadowSlaveEchoInstance& OutInstance) const;

	/** Finds the first Echo matching EchoDef; returns true and outputs the instance */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Echoes|Queries")
	bool FindEchoByDefinition(const UShadowSlaveEchoDefinition* EchoDef, FShadowSlaveEchoInstance& OutInstance) const;

	/** Finds all Echo instances matching the given definition (supports multiple distinct instances of same definition) */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Echoes|Queries")
	TArray<FShadowSlaveEchoInstance> FindAllEchoesByDefinition(const UShadowSlaveEchoDefinition* EchoDef) const;

	/** Returns whether the Echo associated with InstanceId is currently summoned */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Echoes|Queries")
	bool IsEchoSummoned(const FGuid& InstanceId) const;

	/** Returns total number of Echoes currently held */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Echoes|Queries")
	int32 GetEchoCount() const { return Echoes.Num(); }

	/** Returns all Echo instances currently held */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Echoes|Queries")
	const TArray<FShadowSlaveEchoInstance>& GetEchoes() const { return Echoes; }

	/** Returns all Echo instances that are currently manifested / summoned */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Echoes|Queries")
	TArray<FShadowSlaveEchoInstance> GetSummonedEchoes() const;

	/** Returns all Echoes filtered by canon Rank */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Echoes|Queries")
	TArray<FShadowSlaveEchoInstance> GetEchoesByRank(EShadowSlaveEchoRank Rank) const;

	/** Returns all Echoes filtered by canon Class */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Echoes|Queries")
	TArray<FShadowSlaveEchoInstance> GetEchoesByClass(EShadowSlaveEchoClass Class) const;

	/* --- Event Delegates --- */

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Echoes|Events")
	FOnEchoAddedSignature OnEchoAdded;

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Echoes|Events")
	FOnEchoRemovedSignature OnEchoRemoved;

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Echoes|Events")
	FOnEchoDestroyedSignature OnEchoDestroyed;

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Echoes|Events")
	FOnEchoSummonedSignature OnEchoSummoned;

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Echoes|Events")
	FOnEchoDismissedSignature OnEchoDismissed;

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Echoes|Events")
	FOnEchoModifiedSignature OnEchoModified;

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Echoes|Events")
	FOnEchoStateChangedSignature OnEchoStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Echoes|Events")
	FOnEchoCollectionChangedSignature OnEchoCollectionChanged;

protected:
	/** Collection of runtime Echo instances owned authoritatively by this component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Echoes|State")
	TArray<FShadowSlaveEchoInstance> Echoes;

private:
	/** Re-entrancy guard preventing recursive transitions during delegate broadcasts */
	bool bIsProcessingEchoTransition = false;
};
