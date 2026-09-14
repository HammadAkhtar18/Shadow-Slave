// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Nightmares/ShadowSlaveNightmareTypes.h"
#include "Nightmares/ShadowSlaveNightmareObjectiveTypes.h"
#include "ShadowSlaveNightmareSaveTypes.generated.h"

/**
 * Serializable snapshot of an individual Nightmare objective runtime state.
 * Contains only stable identifiers and primitives; strictly zero raw pointers.
 */
USTRUCT(BlueprintType)
struct SHADOWSLAVE_API FShadowSlaveNightmareObjectiveSaveData
{
	GENERATED_BODY()

	/** Stable identifier matching the objective definition */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Nightmare|Save")
	FName ObjectiveId = NAME_None;

	/** Objective lifecycle state */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Nightmare|Save")
	EShadowSlaveObjectiveState State = EShadowSlaveObjectiveState::NotStarted;

	/** Current progress */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Nightmare|Save")
	float CurrentProgress = 0.0f;

	/** Target progress */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Nightmare|Save")
	float TargetProgress = 1.0f;

	/** Whether this objective was required */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Nightmare|Save")
	bool bIsRequired = true;

	/** Dynamic properties */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Nightmare|Save")
	TMap<FName, FString> DynamicProperties;

	FShadowSlaveNightmareObjectiveSaveData() = default;

	FShadowSlaveNightmareObjectiveSaveData(const FShadowSlaveNightmareObjectiveRuntimeState& RuntimeState)
		: ObjectiveId(RuntimeState.ObjectiveId)
		, State(RuntimeState.State)
		, CurrentProgress(RuntimeState.CurrentProgress)
		, TargetProgress(RuntimeState.TargetProgress)
		, bIsRequired(RuntimeState.bIsRequired)
		, DynamicProperties(RuntimeState.DynamicProperties)
	{
	}
};

/**
 * Serializable snapshot of Nightmare scenario and session state.
 *
 * NOTE ON PERSISTENCE BOUNDARY & ARCHITECTURAL CONTRACT:
 * - This struct contains only stable identifiers, enum primitives, and metadata maps.
 * - Zero raw actor pointers, zero UObject pointers, zero UI widget references.
 * - Live in-scenario world restoration across level transitions requires the future level streaming
 *   and world persistence layer. When loaded without active level streaming, stable scenario metadata
 *   and objective progress are preserved while the session safely defaults to Inactive until scenario map entry.
 */
USTRUCT(BlueprintType)
struct SHADOWSLAVE_API FShadowSlaveNightmareSaveData
{
	GENERATED_BODY()

	/** Stable identifier of the active Nightmare scenario definition */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Nightmare|Save")
	FName ScenarioId = NAME_None;

	/** Schema version of the scenario definition at save time */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Nightmare|Save")
	int32 ScenarioVersion = 1;

	/** Session lifecycle state */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Nightmare|Save")
	EShadowSlaveNightmareSessionState SessionState = EShadowSlaveNightmareSessionState::Inactive;

	/** Failure reason if session failed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Nightmare|Save")
	EShadowSlaveScenarioFailureReason FailureReason = EShadowSlaveScenarioFailureReason::None;

	/** Serialized objective progress records */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Nightmare|Save")
	TArray<FShadowSlaveNightmareObjectiveSaveData> Objectives;

	/** Extensible metadata */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Nightmare|Save")
	TMap<FName, FString> ScenarioMetadata;

	/** Whether a scenario was active at the time of save */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Nightmare|Save")
	bool bIsActive = false;

	/** Validity flag */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Nightmare|Save")
	bool bIsValid = false;

	FShadowSlaveNightmareSaveData() = default;
};
