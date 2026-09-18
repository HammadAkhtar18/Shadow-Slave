// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ShadowSlaveStoryContentTypes.generated.h"

/**
 * Functional classification of generic story content elements.
 * Strictly technical classification without hardcoded canon novel concepts.
 */
UENUM(BlueprintType)
enum class EShadowSlaveStoryContentType : uint8
{
	None        UMETA(DisplayName = "None"),
	Quest       UMETA(DisplayName = "Quest"),
	Dialogue    UMETA(DisplayName = "Dialogue"),
	Nightmare   UMETA(DisplayName = "Nightmare"),
	WorldState  UMETA(DisplayName = "World State"),
	Location    UMETA(DisplayName = "Location"),
	Transition  UMETA(DisplayName = "Transition"),
	Custom      UMETA(DisplayName = "Custom / Event")
};

/**
 * Generic progression lifecycle states for story content chapters, arcs, or content entries.
 */
UENUM(BlueprintType)
enum class EShadowSlaveStoryContentState : uint8
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
 * Stable, data-driven content reference entry within a story chapter or arc.
 * Static authoring data; zero mutable runtime state.
 */
USTRUCT(BlueprintType)
struct SHADOWSLAVE_API FShadowSlaveStoryContentEntry
{
	GENERATED_BODY()

	/** Unique stable identifier for this content entry within its owning chapter or arc */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Story|Content")
	FName ContentId = NAME_None;

	/** Functional type of this content entry */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Story|Content")
	EShadowSlaveStoryContentType ContentType = EShadowSlaveStoryContentType::None;

	/** Content entry schema version */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Story|Content", meta = (ClampMin = "1"))
	int32 Version = 1;

	/** Technical reference ID (QuestId, DialogueId, Nightmare ScenarioId, WorldState Key, LocationId, etc.) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Story|Content")
	FName TargetId = NAME_None;

	/** Player-facing short display title */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Story|Content")
	FText DisplayName;

	/** Player-facing descriptive narrative briefing or notes */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Story|Content", meta = (MultiLine = true))
	FText Description;

	/** If true, failing or skipping this entry does not block overall arc completion */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Story|Content")
	bool bIsOptional = false;

	/** Prerequisite content entry IDs within this arc that must be completed before this entry can activate */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Story|Content")
	TArray<FName> PrerequisiteContentIds;

	/** Extensible metadata for content parameters, coordinates, or tags */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Story|Content")
	TMap<FName, FString> Metadata;

	/** Technical provenance and source attribution note for future canon alignment tracking */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Story|Content")
	FString ProvenanceNote;

	FShadowSlaveStoryContentEntry() = default;

	bool IsValid() const
	{
		return !ContentId.IsNone() && ContentType != EShadowSlaveStoryContentType::None;
	}
};

/**
 * Mutable runtime state for an individual content entry within an active chapter or arc.
 */
USTRUCT(BlueprintType)
struct SHADOWSLAVE_API FShadowSlaveStoryContentEntryRuntimeState
{
	GENERATED_BODY()

	/** Unique stable identifier of the content entry */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Story|Content")
	FName ContentId = NAME_None;

	/** Current lifecycle state */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Story|Content")
	EShadowSlaveStoryContentState State = EShadowSlaveStoryContentState::Locked;

	/** Extensible runtime metadata */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Story|Content")
	TMap<FName, FString> RuntimeMetadata;

	FShadowSlaveStoryContentEntryRuntimeState() = default;

	explicit FShadowSlaveStoryContentEntryRuntimeState(FName InContentId, EShadowSlaveStoryContentState InState = EShadowSlaveStoryContentState::Locked)
		: ContentId(InContentId), State(InState)
	{
	}

	bool IsActive() const { return State == EShadowSlaveStoryContentState::Active; }
	bool IsCompleted() const { return State == EShadowSlaveStoryContentState::Completed; }
	bool IsTerminal() const
	{
		return State == EShadowSlaveStoryContentState::Completed ||
		       State == EShadowSlaveStoryContentState::Failed ||
		       State == EShadowSlaveStoryContentState::Skipped;
	}
};

/**
 * Mutable runtime state for a tracked story chapter or arc.
 */
USTRUCT(BlueprintType)
struct SHADOWSLAVE_API FShadowSlaveStoryContentRuntimeState
{
	GENERATED_BODY()

	/** Unique stable identifier of the story content definition */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Story|Content")
	FName StoryContentId = NAME_None;

	/** Current overall lifecycle state */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Story|Content")
	EShadowSlaveStoryContentState State = EShadowSlaveStoryContentState::Locked;

	/** Currently active content entry ID within this chapter or arc */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Story|Content")
	FName CurrentActiveEntryId = NAME_None;

	/** Mutable runtime states for child content entries */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Story|Content")
	TMap<FName, FShadowSlaveStoryContentEntryRuntimeState> EntryStates;

	/** Extensible runtime metadata */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Story|Content")
	TMap<FName, FString> RuntimeMetadata;

	FShadowSlaveStoryContentRuntimeState() = default;

	explicit FShadowSlaveStoryContentRuntimeState(FName InStoryContentId, EShadowSlaveStoryContentState InState = EShadowSlaveStoryContentState::Locked)
		: StoryContentId(InStoryContentId), State(InState)
	{
	}

	bool IsActive() const { return State == EShadowSlaveStoryContentState::Active; }
	bool IsCompleted() const { return State == EShadowSlaveStoryContentState::Completed; }
	bool IsTerminal() const
	{
		return State == EShadowSlaveStoryContentState::Completed ||
		       State == EShadowSlaveStoryContentState::Failed ||
		       State == EShadowSlaveStoryContentState::Skipped;
	}
	bool IsValid() const { return !StoryContentId.IsNone() && State != EShadowSlaveStoryContentState::Unknown; }
};

/**
 * Individual serializable record for story content in passive save data.
 */
USTRUCT(BlueprintType)
struct SHADOWSLAVE_API FShadowSlaveStoryContentRecordSaveData
{
	GENERATED_BODY()

	/** Story content identifier */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Story|Save")
	FName StoryContentId = NAME_None;

	/** Saved content schema version */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Story|Save")
	int32 ContentVersion = 1;

	/** Overall saved lifecycle state */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Story|Save")
	EShadowSlaveStoryContentState State = EShadowSlaveStoryContentState::Locked;

	/** Saved current active entry ID */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Story|Save")
	FName CurrentActiveEntryId = NAME_None;

	/** Saved child entry states */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Story|Save")
	TMap<FName, EShadowSlaveStoryContentState> EntryStates;

	/** Saved runtime metadata */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Story|Save")
	TMap<FName, FString> RuntimeMetadata;

	FShadowSlaveStoryContentRecordSaveData() = default;

	explicit FShadowSlaveStoryContentRecordSaveData(const FShadowSlaveStoryContentRuntimeState& InState, int32 InVersion = 1)
		: StoryContentId(InState.StoryContentId)
		, ContentVersion(InVersion)
		, State(InState.State)
		, CurrentActiveEntryId(InState.CurrentActiveEntryId)
		, RuntimeMetadata(InState.RuntimeMetadata)
	{
		for (const auto& Pair : InState.EntryStates)
		{
			EntryStates.Add(Pair.Key, Pair.Value.State);
		}
	}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FOnShadowSlaveStoryContentStateChangedSignature,
	FName, StoryContentId,
	EShadowSlaveStoryContentState, NewState,
	EShadowSlaveStoryContentState, OldState
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(
	FOnShadowSlaveStoryContentEntryStateChangedSignature,
	FName, StoryContentId,
	FName, EntryId,
	EShadowSlaveStoryContentState, NewState,
	EShadowSlaveStoryContentState, OldState
);
