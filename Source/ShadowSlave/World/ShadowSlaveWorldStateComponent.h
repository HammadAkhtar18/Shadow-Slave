// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "World/ShadowSlaveWorldTypes.h"
#include "Save/ShadowSlaveSaveTypes.h"
#include "ShadowSlaveWorldStateComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(
	FOnShadowSlaveWorldStateChangedSignature,
	FName, Key,
	const FShadowSlaveWorldValue&, NewValue,
	const FShadowSlaveWorldValue&, OldValue,
	AActor*, OwningActor
);

/**
 * Component attachable to world actors to manage authoritative, persistent world state.
 *
 * RESPONSIBILITIES:
 * - Authoritative owner of persistent state key-values for its owning actor.
 * - Event-driven: broadcasts state changes only when values genuinely change.
 * - Integrates with the existing saveable architecture via FShadowSlaveWorldActorSaveRecord.
 * - Zero Tick overhead.
 * - Completely decoupled from Story progression.
 */
UCLASS(ClassGroup = (ShadowSlave), meta = (BlueprintSpawnableComponent))
class SHADOWSLAVE_API UShadowSlaveWorldStateComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UShadowSlaveWorldStateComponent();

	/* --- Persistent Identity --- */

	/** Returns the stable persistence identifier for this actor/component */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|WorldState")
	FName GetPersistentStateId() const { return PersistentStateId; }

	/** Sets the stable persistence identifier for this actor/component */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|WorldState")
	void SetPersistentStateId(FName InId) { PersistentStateId = InId; }

	/* --- State Modification & Access --- */

	/** Sets a generic world-state value. Emits OnWorldStateChanged only if value genuinely changes. */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|WorldState")
	bool SetStateValue(FName Key, const FShadowSlaveWorldValue& Value);

	/** Sets a boolean world-state value */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|WorldState")
	bool SetBoolValue(FName Key, bool Value);

	/** Sets an integer world-state value */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|WorldState")
	bool SetIntValue(FName Key, int32 Value);

	/** Sets a float world-state value */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|WorldState")
	bool SetFloatValue(FName Key, float Value);

	/** Sets a string world-state value */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|WorldState")
	bool SetStringValue(FName Key, const FString& Value);

	/** Sets a name world-state value */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|WorldState")
	bool SetNameValue(FName Key, FName Value);

	/** Retrieves a generic world-state value */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|WorldState")
	bool GetStateValue(FName Key, FShadowSlaveWorldValue& OutValue) const;

	/** Retrieves a boolean world-state value */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|WorldState")
	bool GetBoolValue(FName Key, bool DefaultValue = false) const;

	/** Retrieves an integer world-state value */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|WorldState")
	int32 GetIntValue(FName Key, int32 DefaultValue = 0) const;

	/** Retrieves a float world-state value */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|WorldState")
	float GetFloatValue(FName Key, float DefaultValue = 0.0f) const;

	/** Retrieves a string world-state value */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|WorldState")
	FString GetStringValue(FName Key, const FString& DefaultValue = TEXT("")) const;

	/** Retrieves a name world-state value */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|WorldState")
	FName GetNameValue(FName Key, FName DefaultValue = NAME_None) const;

	/** Returns true if a state entry exists for Key */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|WorldState")
	bool HasStateValue(FName Key) const;

	/** Removes a state entry for Key. Broadcasts OnWorldStateChanged with empty NewValue. */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|WorldState")
	bool RemoveStateValue(FName Key);

	/** Clears all runtime state entries */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|WorldState")
	void ClearState();

	/** Returns all active state keys */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|WorldState")
	TArray<FName> GetAllStateKeys() const;

	/* --- Save & Snapshot Integration --- */

	/** Captures current state into a lightweight snapshot */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|WorldState|Save")
	FShadowSlaveWorldStateSnapshot CaptureSnapshot() const;

	/** Restores state from a lightweight snapshot */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|WorldState|Save")
	bool RestoreSnapshot(const FShadowSlaveWorldStateSnapshot& InSnapshot);

	/** Captures state into an existing FShadowSlaveWorldActorSaveRecord */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|WorldState|Save")
	bool CaptureSaveRecord(FShadowSlaveWorldActorSaveRecord& OutRecord) const;

	/** Restores state from an existing FShadowSlaveWorldActorSaveRecord */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|WorldState|Save")
	bool RestoreSaveRecord(const FShadowSlaveWorldActorSaveRecord& InRecord);

	/* --- Events --- */

	/** Broadcast when any world state value on this component is modified or removed */
	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|WorldState|Events")
	FOnShadowSlaveWorldStateChangedSignature OnWorldStateChanged;

protected:
	/** Stable unique persistence identifier for this actor/component. If NAME_None, persistence is disabled. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|WorldState")
	FName PersistentStateId = NAME_None;

	/** Runtime container of persistent state variables */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|WorldState")
	TMap<FName, FShadowSlaveWorldValue> StateEntries;

	/** Extensible metadata */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|WorldState")
	TMap<FName, FString> ComponentMetadata;
};
