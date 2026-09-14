// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Save/ShadowSlaveSaveTypes.h"
#include "ShadowSlaveSaveableInterface.generated.h"

UINTERFACE(MinimalAPI, BlueprintType)
class UShadowSlaveSaveableInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Interface for world actors, interactables, and systems that opt into persistent world state.
 * Only actors that explicitly implement this interface and return a non-None persistent ID participate in saving.
 */
class SHADOWSLAVE_API IShadowSlaveSaveableInterface
{
	GENERATED_BODY()

public:
	/**
	 * Returns the stable, unique persistence identifier for this actor.
	 * Must NOT be a transient memory pointer or instance counter.
	 * Returning NAME_None disables persistence for this actor.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ShadowSlave|Save")
	FName GetPersistentSaveId() const;

	/**
	 * Captures the actor's custom state into the provided save record.
	 * Returns true if state was successfully captured.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ShadowSlave|Save")
	bool CaptureSaveRecord(FShadowSlaveWorldActorSaveRecord& OutRecord);

	/**
	 * Restores the actor's state from the provided save record.
	 * Returns true if state was successfully restored.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ShadowSlave|Save")
	bool RestoreSaveRecord(const FShadowSlaveWorldActorSaveRecord& InRecord);
};
