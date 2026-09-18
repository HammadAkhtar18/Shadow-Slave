// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Story/ShadowSlaveStoryContentTypes.h"
#include "ShadowSlaveStoryTypes.generated.h"

/**
 * Generic progression lifecycle states for story beats, chapters, or narrative arcs.
 * Contains strictly zero hardcoded canon names or story arcs.
 */
UENUM(BlueprintType)
enum class EShadowSlaveStoryState : uint8
{
	Unknown   UMETA(DisplayName = "Unknown"),
	Locked    UMETA(DisplayName = "Locked"),
	Available UMETA(DisplayName = "Available"),
	Active    UMETA(DisplayName = "Active"),
	Completed UMETA(DisplayName = "Completed"),
	Failed    UMETA(DisplayName = "Failed"),
	Skipped   UMETA(DisplayName = "Skipped")
};

/**
 * Mutable runtime state for an active or tracked story entry.
 */
USTRUCT(BlueprintType)
struct SHADOWSLAVE_API FShadowSlaveStoryRuntimeState
{
	GENERATED_BODY()

	/** Stable unique identifier of the story definition */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Story")
	FName StoryId = NAME_None;

	/** Current lifecycle state */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Story")
	EShadowSlaveStoryState State = EShadowSlaveStoryState::Locked;

	/** Optional identifier of current step or beat within this story */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Story")
	FName CurrentStepId = NAME_None;

	/** Extensible runtime metadata */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Story")
	TMap<FName, FString> RuntimeMetadata;

	FShadowSlaveStoryRuntimeState() = default;

	explicit FShadowSlaveStoryRuntimeState(FName InStoryId, EShadowSlaveStoryState InState = EShadowSlaveStoryState::Locked)
		: StoryId(InStoryId), State(InState)
	{
	}

	bool IsActive() const
	{
		return State == EShadowSlaveStoryState::Active;
	}

	bool IsCompleted() const
	{
		return State == EShadowSlaveStoryState::Completed;
	}

	bool IsTerminal() const
	{
		return State == EShadowSlaveStoryState::Completed ||
		       State == EShadowSlaveStoryState::Failed ||
		       State == EShadowSlaveStoryState::Skipped;
	}

	bool IsValid() const
	{
		return !StoryId.IsNone() && State != EShadowSlaveStoryState::Unknown;
	}
};

/**
 * Individual serializable record for a story unit in passive save data.
 */
USTRUCT(BlueprintType)
struct SHADOWSLAVE_API FShadowSlaveStoryRecordSaveData
{
	GENERATED_BODY()

	/** Story identifier */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Story|Save")
	FName StoryId = NAME_None;

	/** Saved lifecycle state */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Story|Save")
	EShadowSlaveStoryState State = EShadowSlaveStoryState::Locked;

	/** Saved current step identifier */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Story|Save")
	FName CurrentStepId = NAME_None;

	/** Saved runtime metadata */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Story|Save")
	TMap<FName, FString> RuntimeMetadata;

	FShadowSlaveStoryRecordSaveData() = default;

	explicit FShadowSlaveStoryRecordSaveData(const FShadowSlaveStoryRuntimeState& InState)
		: StoryId(InState.StoryId)
		, State(InState.State)
		, CurrentStepId(InState.CurrentStepId)
		, RuntimeMetadata(InState.RuntimeMetadata)
	{
	}
};

/**
 * Complete passive save snapshot for the Story Subsystem.
 * Completely free of raw UObject pointers or actor references.
 */
USTRUCT(BlueprintType)
struct SHADOWSLAVE_API FShadowSlaveStorySaveData
{
	GENERATED_BODY()

	/** Subsystem schema version for migration safety */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Story|Save")
	int32 StorySubsystemVersion = 1;

	/** Ordered list of story state records */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Story|Save")
	TArray<FShadowSlaveStoryRecordSaveData> Stories;

	/** Ordered list of story content (chapter/arc) state records */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Story|Save")
	TArray<FShadowSlaveStoryContentRecordSaveData> StoryContents;

	/** Validity flag */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Story|Save")
	bool bIsValid = false;

	FShadowSlaveStorySaveData() = default;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FOnShadowSlaveStoryStateChangedSignature,
	FName, StoryId,
	EShadowSlaveStoryState, NewState,
	EShadowSlaveStoryState, OldState
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FOnShadowSlaveStoryStepChangedSignature,
	FName, StoryId,
	FName, NewStepId,
	FName, OldStepId
);
