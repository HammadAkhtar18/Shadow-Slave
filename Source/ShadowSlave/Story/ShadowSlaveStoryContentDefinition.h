// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Story/ShadowSlaveStoryContentTypes.h"
#include "ShadowSlaveStoryContentDefinition.generated.h"

/**
 * Static primary data asset describing a chronological story chapter or arc.
 * Pure immutable authoring definition data; zero mutable runtime state.
 * Contains strictly zero hardcoded canon story arcs, characters, or novel events.
 */
UCLASS(BlueprintType)
class SHADOWSLAVE_API UShadowSlaveStoryContentDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UShadowSlaveStoryContentDefinition();

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	/* --- Identification & Display --- */

	/** Stable unique technical identifier for this story content chapter or arc */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Story|Identity")
	FName StoryContentId = NAME_None;

	/** Player-facing title of this chapter or arc */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Story|Identity")
	FText DisplayName;

	/** Player-facing narrative description or chapter overview */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Story|Identity", meta = (MultiLine = true))
	FText Description;

	/** Content version number for compatibility and save migration */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Story|Identity", meta = (ClampMin = "1"))
	int32 Version = 1;

	/* --- Content Entries (Ordered Chronology) --- */

	/**
	 * Explicit chronological list of content entries forming this story arc or chapter.
	 * Author-defined array index is the authoritative ordering. Never relies on map or object name sorting.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Story|Entries")
	TArray<FShadowSlaveStoryContentEntry> ContentEntries;

	/* --- Flow & Prerequisites --- */

	/** Story content IDs that must be in Completed state before this chapter/arc can activate */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Story|Flow")
	TArray<FName> PrerequisiteStoryContentIds;

	/* --- Optional Associated Domain Linkages --- */

	/** Optional associated quest IDs tied to this arc */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Story|Linkages")
	TArray<FName> AssociatedQuestIds;

	/** Optional associated dialogue / conversation IDs tied to this arc */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Story|Linkages")
	TArray<FName> AssociatedDialogueIds;

	/** Optional associated Nightmare Scenario IDs tied to this arc */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Story|Linkages")
	TArray<FName> AssociatedNightmareScenarioIds;

	/** Optional associated WorldState keys/flags tied to this arc */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Story|Linkages")
	TArray<FName> AssociatedWorldStateKeys;

	/* --- Metadata & Provenance --- */

	/** Extensible key-value metadata for tags, act indices, or level bindings */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Story|Metadata")
	TMap<FName, FString> Metadata;

	/** Technical provenance and source attribution note for future canon alignment tracking */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Story|Provenance")
	FString ProvenanceNote;

	/* --- Validation & Lookup Helpers --- */

	/** Validates definition integrity and outputs any discovered errors */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Story|Validation")
	bool ValidateDefinition(TArray<FText>& OutErrors) const;

	/** Finds a content entry by ContentId */
	const FShadowSlaveStoryContentEntry* FindContentEntry(FName InContentId) const;

	/** Finds the zero-based array index of a content entry by ContentId (-1 if not found) */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Story|Lookup")
	int32 FindContentEntryIndex(FName InContentId) const;

	/** Checks if this definition contains a content entry with the specified ContentId */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Story|Lookup")
	bool HasContentEntry(FName InContentId) const;
};
