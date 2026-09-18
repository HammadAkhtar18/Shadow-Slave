// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ShadowSlaveQuestTypes.generated.h"

/**
 * Lifecycle states for quests in the generic quest system.
 * Contains strictly zero hardcoded canon terminology.
 */
UENUM(BlueprintType)
enum class EShadowSlaveQuestState : uint8
{
	Unknown   UMETA(DisplayName = "Unknown"),
	Locked    UMETA(DisplayName = "Locked"),
	Available UMETA(DisplayName = "Available"),
	Active    UMETA(DisplayName = "Active"),
	Completed UMETA(DisplayName = "Completed"),
	Failed    UMETA(DisplayName = "Failed"),
	Abandoned UMETA(DisplayName = "Abandoned")
};

/**
 * Lifecycle states for individual objectives within a quest.
 */
UENUM(BlueprintType)
enum class EShadowSlaveObjectiveState : uint8
{
	Unknown   UMETA(DisplayName = "Unknown"),
	Inactive  UMETA(DisplayName = "Inactive"),
	Active    UMETA(DisplayName = "Active"),
	Completed UMETA(DisplayName = "Completed"),
	Failed    UMETA(DisplayName = "Failed")
};

/**
 * Generic objective taxonomy.
 * Technical classification only; no implied hard-coded gameplay implementation in this step.
 */
UENUM(BlueprintType)
enum class EShadowSlaveObjectiveType : uint8
{
	None            UMETA(DisplayName = "None"),
	ReachLocation   UMETA(DisplayName = "Reach Location"),
	Interact        UMETA(DisplayName = "Interact"),
	TalkToCharacter UMETA(DisplayName = "Talk To Character"),
	DefeatTarget    UMETA(DisplayName = "Defeat Target"),
	CollectItem     UMETA(DisplayName = "Collect Item"),
	UseItem         UMETA(DisplayName = "Use Item"),
	Survive         UMETA(DisplayName = "Survive"),
	WorldState      UMETA(DisplayName = "World State"),
	CompleteQuest   UMETA(DisplayName = "Complete Quest")
};

/**
 * Data-driven definition of an individual quest objective.
 * Static, immutable definition asset payload; zero mutable runtime state.
 */
USTRUCT(BlueprintType)
struct SHADOWSLAVE_API FShadowSlaveObjectiveDefinition
{
	GENERATED_BODY()

	/** Unique stable identifier for this objective within its owning quest */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Quest|Objective")
	FName ObjectiveId = NAME_None;

	/** Player-facing short display title */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Quest|Objective")
	FText DisplayName;

	/** Player-facing detailed description or task briefing */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Quest|Objective", meta = (MultiLine = true))
	FText Description;

	/** Functional classification of this objective */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Quest|Objective")
	EShadowSlaveObjectiveType ObjectiveType = EShadowSlaveObjectiveType::None;

	/** Optional technical identifier of target actor, location, enemy, item, or dialogue */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Quest|Objective")
	FName TargetId = NAME_None;

	/** Required count or quantity to satisfy this objective (default 1) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Quest|Objective", meta = (ClampMin = "1"))
	int32 RequiredQuantity = 1;

	/** If true, failing or skipping this objective does not prevent quest completion */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Quest|Objective")
	bool bIsOptional = false;

	/** Extensible metadata suitable for future objective-specific parameters */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Quest|Objective")
	TMap<FName, FString> Metadata;

	bool IsValid() const
	{
		return !ObjectiveId.IsNone() && RequiredQuantity >= 1;
	}
};

/**
 * Mutable runtime state for an individual quest objective.
 */
USTRUCT(BlueprintType)
struct SHADOWSLAVE_API FShadowSlaveObjectiveRuntimeState
{
	GENERATED_BODY()

	/** Objective identifier matching its definition */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Quest|Runtime")
	FName ObjectiveId = NAME_None;

	/** Current operational lifecycle state */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Quest|Runtime")
	EShadowSlaveObjectiveState State = EShadowSlaveObjectiveState::Inactive;

	/** Current recorded progress or count */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Quest|Runtime")
	int32 CurrentQuantity = 0;

	/** Target quantity required for completion */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Quest|Runtime")
	int32 RequiredQuantity = 1;

	/** Whether this objective is optional */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Quest|Runtime")
	bool bIsOptional = false;

	/** Extensible runtime metadata */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Quest|Runtime")
	TMap<FName, FString> RuntimeMetadata;

	FShadowSlaveObjectiveRuntimeState() = default;

	explicit FShadowSlaveObjectiveRuntimeState(const FShadowSlaveObjectiveDefinition& Def)
		: ObjectiveId(Def.ObjectiveId)
		, State(EShadowSlaveObjectiveState::Inactive)
		, CurrentQuantity(0)
		, RequiredQuantity(FMath::Max(1, Def.RequiredQuantity))
		, bIsOptional(Def.bIsOptional)
		, RuntimeMetadata(Def.Metadata)
	{
	}

	bool IsActive() const { return State == EShadowSlaveObjectiveState::Active; }
	bool IsCompleted() const { return State == EShadowSlaveObjectiveState::Completed; }
	bool IsFailed() const { return State == EShadowSlaveObjectiveState::Failed; }
	bool IsValid() const { return !ObjectiveId.IsNone(); }
};

/**
 * Mutable runtime state for a tracked quest.
 */
USTRUCT(BlueprintType)
struct SHADOWSLAVE_API FShadowSlaveQuestRuntimeState
{
	GENERATED_BODY()

	/** Unique technical identifier of the quest definition */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Quest|Runtime")
	FName QuestId = NAME_None;

	/** Current quest lifecycle state */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Quest|Runtime")
	EShadowSlaveQuestState State = EShadowSlaveQuestState::Locked;

	/** Runtime objective states indexed by ObjectiveId */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Quest|Runtime")
	TMap<FName, FShadowSlaveObjectiveRuntimeState> ObjectiveStates;

	/** Extensible runtime metadata */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Quest|Runtime")
	TMap<FName, FString> RuntimeMetadata;

	FShadowSlaveQuestRuntimeState() = default;

	explicit FShadowSlaveQuestRuntimeState(FName InQuestId, EShadowSlaveQuestState InState = EShadowSlaveQuestState::Locked)
		: QuestId(InQuestId), State(InState)
	{
	}

	bool IsActive() const { return State == EShadowSlaveQuestState::Active; }
	bool IsCompleted() const { return State == EShadowSlaveQuestState::Completed; }
	bool IsFailed() const { return State == EShadowSlaveQuestState::Failed; }
	bool IsAbandoned() const { return State == EShadowSlaveQuestState::Abandoned; }

	bool IsTerminal() const
	{
		return State == EShadowSlaveQuestState::Completed ||
		       State == EShadowSlaveQuestState::Failed ||
		       State == EShadowSlaveQuestState::Abandoned;
	}

	bool IsValid() const
	{
		return !QuestId.IsNone() && State != EShadowSlaveQuestState::Unknown;
	}

	bool AreAllRequiredObjectivesComplete() const
	{
		if (ObjectiveStates.Num() == 0)
		{
			return false;
		}

		for (const auto& Pair : ObjectiveStates)
		{
			if (!Pair.Value.bIsOptional && Pair.Value.State != EShadowSlaveObjectiveState::Completed)
			{
				return false;
			}
		}
		return true;
	}

	bool HasAnyRequiredObjectiveFailed() const
	{
		for (const auto& Pair : ObjectiveStates)
		{
			if (!Pair.Value.bIsOptional && Pair.Value.State == EShadowSlaveObjectiveState::Failed)
			{
				return true;
			}
		}
		return false;
	}
};

/**
 * Serializable snapshot of an individual objective's state.
 */
USTRUCT(BlueprintType)
struct SHADOWSLAVE_API FShadowSlaveObjectiveSaveData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Quest|Save")
	FName ObjectiveId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Quest|Save")
	EShadowSlaveObjectiveState State = EShadowSlaveObjectiveState::Inactive;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Quest|Save")
	int32 CurrentQuantity = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Quest|Save")
	int32 RequiredQuantity = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Quest|Save")
	bool bIsOptional = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Quest|Save")
	TMap<FName, FString> RuntimeMetadata;

	FShadowSlaveObjectiveSaveData() = default;

	explicit FShadowSlaveObjectiveSaveData(const FShadowSlaveObjectiveRuntimeState& InState)
		: ObjectiveId(InState.ObjectiveId)
		, State(InState.State)
		, CurrentQuantity(InState.CurrentQuantity)
		, RequiredQuantity(InState.RequiredQuantity)
		, bIsOptional(InState.bIsOptional)
		, RuntimeMetadata(InState.RuntimeMetadata)
	{
	}
};

/**
 * Serializable snapshot of an individual quest's runtime state.
 */
USTRUCT(BlueprintType)
struct SHADOWSLAVE_API FShadowSlaveQuestRecordSaveData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Quest|Save")
	FName QuestId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Quest|Save")
	int32 QuestVersion = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Quest|Save")
	EShadowSlaveQuestState State = EShadowSlaveQuestState::Locked;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Quest|Save")
	TArray<FShadowSlaveObjectiveSaveData> Objectives;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Quest|Save")
	TMap<FName, FString> RuntimeMetadata;

	FShadowSlaveQuestRecordSaveData() = default;
};

/**
 * Complete passive save snapshot for the Quest Subsystem.
 */
USTRUCT(BlueprintType)
struct SHADOWSLAVE_API FShadowSlaveQuestSaveData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Quest|Save")
	int32 QuestSubsystemVersion = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Quest|Save")
	TArray<FShadowSlaveQuestRecordSaveData> Quests;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Quest|Save")
	bool bIsValid = false;

	FShadowSlaveQuestSaveData() = default;
};

/* --- Delegate Declarations --- */

/**
 * Broadcast when a quest's lifecycle state genuinely changes.
 * Parameter order follows the established project convention: (QuestId, NewState, OldState).
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FOnShadowSlaveQuestStateChangedSignature,
	FName, QuestId,
	EShadowSlaveQuestState, NewState,
	EShadowSlaveQuestState, OldState
);

/**
 * Broadcast when an objective's lifecycle state genuinely changes.
 * Parameter order follows the established project convention: (QuestId, ObjectiveId, NewState, OldState).
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(
	FOnShadowSlaveObjectiveStateChangedSignature,
	FName, QuestId,
	FName, ObjectiveId,
	EShadowSlaveObjectiveState, NewState,
	EShadowSlaveObjectiveState, OldState
);

/**
 * Broadcast when an objective's progress quantity genuinely changes.
 * Parameter order follows the established project convention: (QuestId, ObjectiveId, NewProgress, OldProgress).
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(
	FOnShadowSlaveObjectiveProgressChangedSignature,
	FName, QuestId,
	FName, ObjectiveId,
	int32, NewProgress,
	int32, OldProgress
);

/**
 * Broadcast when a quest is completed.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnShadowSlaveQuestCompletedSignature,
	FName, QuestId
);

/**
 * Broadcast when a quest is failed.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnShadowSlaveQuestFailedSignature,
	FName, QuestId
);
