// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ShadowSlaveNightmareObjectiveTypes.generated.h"

/**
 * Generic objective type classification.
 * Technical abstractions only; no canon First Nightmare objectives.
 */
UENUM(BlueprintType)
enum class EShadowSlaveNightmareObjectiveType : uint8
{
	Unknown       UMETA(DisplayName = "Unknown"),
	ReachLocation UMETA(DisplayName = "Reach Location"),
	Interact      UMETA(DisplayName = "Interact"),
	DefeatTarget  UMETA(DisplayName = "Defeat Target"),
	Survive       UMETA(DisplayName = "Survive"),
	Collect       UMETA(DisplayName = "Collect"),
	Escort        UMETA(DisplayName = "Escort"),
	Custom        UMETA(DisplayName = "Custom")
};

/**
 * Runtime operational state of an individual objective.
 */
UENUM(BlueprintType)
enum class EShadowSlaveObjectiveState : uint8
{
	NotStarted UMETA(DisplayName = "Not Started"),
	Active     UMETA(DisplayName = "Active"),
	Completed  UMETA(DisplayName = "Completed"),
	Failed     UMETA(DisplayName = "Failed")
};

/**
 * Static archetype definition for a scenario objective.
 * Resides inside UShadowSlaveNightmareScenarioDefinition; immutable at runtime.
 */
USTRUCT(BlueprintType)
struct SHADOWSLAVE_API FShadowSlaveNightmareObjectiveDefinition
{
	GENERATED_BODY()

	/** Unique, stable identifier for this objective within the scenario */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Nightmare|Objective")
	FName ObjectiveId = NAME_None;

	/** Display name shown to players in UI/journals */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Nightmare|Objective")
	FText DisplayName;

	/** Narrative or mechanical description of what must be done */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Nightmare|Objective", meta = (MultiLine = true))
	FText Description;

	/** Technical classification of the objective */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Nightmare|Objective")
	EShadowSlaveNightmareObjectiveType ObjectiveType = EShadowSlaveNightmareObjectiveType::Unknown;

	/** Whether this objective is mandatory for scenario completion */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Nightmare|Objective")
	bool bIsRequired = true;

	/** Target count or progress threshold required to complete (e.g. 1.0 for single trigger, N for counts) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Nightmare|Objective", meta = (ClampMin = "1.0"))
	float TargetProgress = 1.0f;

	/** Optional technical metadata key-value pairs */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Nightmare|Objective")
	TMap<FName, FString> ObjectiveMetadata;

	FShadowSlaveNightmareObjectiveDefinition() = default;

	FShadowSlaveNightmareObjectiveDefinition(FName InId, const FText& InName, const FText& InDesc, EShadowSlaveNightmareObjectiveType InType = EShadowSlaveNightmareObjectiveType::Custom, bool bInRequired = true, float InTarget = 1.0f)
		: ObjectiveId(InId), DisplayName(InName), Description(InDesc), ObjectiveType(InType), bIsRequired(bInRequired), TargetProgress(InTarget)
	{
	}

	bool IsValid() const
	{
		return ObjectiveId != NAME_None && !DisplayName.IsEmpty() && TargetProgress > 0.0f;
	}
};

/**
 * Mutable runtime tracking state for an individual scenario objective.
 * Cleanly separates immutable archetype definition from dynamic player progress.
 */
USTRUCT(BlueprintType)
struct SHADOWSLAVE_API FShadowSlaveNightmareObjectiveRuntimeState
{
	GENERATED_BODY()

	/** Stable identifier matching the archetype definition */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Nightmare|Objective")
	FName ObjectiveId = NAME_None;

	/** Current lifecycle state */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Nightmare|Objective")
	EShadowSlaveObjectiveState State = EShadowSlaveObjectiveState::NotStarted;

	/** Current accumulated progress */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Nightmare|Objective")
	float CurrentProgress = 0.0f;

	/** Target progress required for completion */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Nightmare|Objective")
	float TargetProgress = 1.0f;

	/** Cached flag whether this objective is required */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Nightmare|Objective")
	bool bIsRequired = true;

	/** Dynamic instance properties for custom scripting without altering definition */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Nightmare|Objective")
	TMap<FName, FString> DynamicProperties;

	FShadowSlaveNightmareObjectiveRuntimeState() = default;

	FShadowSlaveNightmareObjectiveRuntimeState(FName InId, float InTarget = 1.0f, bool bInRequired = true)
		: ObjectiveId(InId), State(EShadowSlaveObjectiveState::NotStarted), CurrentProgress(0.0f), TargetProgress(InTarget), bIsRequired(bInRequired)
	{
	}

	float GetProgressPercent() const
	{
		return TargetProgress > 0.0f ? FMath::Clamp(CurrentProgress / TargetProgress, 0.0f, 1.0f) : 0.0f;
	}

	bool IsCompleted() const
	{
		return State == EShadowSlaveObjectiveState::Completed;
	}

	bool IsFailed() const
	{
		return State == EShadowSlaveObjectiveState::Failed;
	}

	bool IsActive() const
	{
		return State == EShadowSlaveObjectiveState::Active;
	}
};

/* --- Objective Delegates --- */

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnObjectiveStateChangedSignature, FName, ObjectiveId, EShadowSlaveObjectiveState, NewState, EShadowSlaveObjectiveState, OldState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnObjectiveProgressChangedSignature, FName, ObjectiveId, float, CurrentProgress, float, TargetProgress);
