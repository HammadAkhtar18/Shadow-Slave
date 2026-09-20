// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Content/ShadowSlaveContentDefinition.h"
#include "Story/ShadowSlaveStoryContentTypes.h"
#include "ShadowSlaveStoryContentDefinition.generated.h"

/**
 * Static primary data asset describing a chronological story chapter or arc.
 * Derived from UShadowSlaveContentDefinition as part of the generic static content pipeline.
 * Pure immutable authoring definition data; zero mutable runtime state.
 * Contains strictly zero hardcoded canon story arcs, characters, or novel events.
 *
 * ARCHITECTURAL PRINCIPLES:
 * 1. Specialized Content Definition: Inherits common metadata (ContentId, DisplayName, Description,
 *    Version, MetadataTags, ProvenanceNote) from UShadowSlaveContentDefinition.
 * 2. Single Authoritative ID: ContentId is the single authoritative stored identifier. Backwards
 *    compatibility is provided through GetStoryContentId() and SetStoryContentId() accessors only.
 * 3. Content Type Separation:
 *    - Generic ContentType is statically EShadowSlaveContentType::Story, identifying this asset as
 *      a story definition within the generic content pipeline.
 *    - Story-specific taxonomy (EShadowSlaveStoryContentType) remains separate and authoritative for
 *      child content entries (FShadowSlaveStoryContentEntry::ContentType: Quest, Dialogue, Nightmare,
 *      WorldState, Location, Transition, Custom).
 * 4. Story-Specific Data: Owns chronological content entries, prerequisite story content IDs, and
 *    domain linkages (associated quest, dialogue, nightmare scenario, and world state IDs).
 * 5. Runtime Separation: Cleanly separates immutable story arc definitions from mutable owned runtime
 *    states (FShadowSlaveStoryContentRuntimeState managed by UShadowSlaveStorySubsystem).
 * 6. Static Infrastructure: Participates in UShadowSlaveContentRegistrySubsystem without replacing
 *    or coupling to UShadowSlaveStorySubsystem.
 */
UCLASS(BlueprintType)
class SHADOWSLAVE_API UShadowSlaveStoryContentDefinition : public UShadowSlaveContentDefinition
{
	GENERATED_BODY()

public:
	UShadowSlaveStoryContentDefinition();

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	/**
	 * Validates definition integrity.
	 * Combines generic content definition validation with story-specific validation rules.
	 */
	virtual bool IsValidDefinition(FString* OutErrorMessage = nullptr) const override;

	/* --- Identity Compatibility --- */

	/**
	 * Returns the single authoritative stable content identifier (ContentId).
	 * Provided for backwards compatibility with the existing Story API.
	 */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Story|Identity")
	FName GetStoryContentId() const { return ContentId; }

	/**
	 * Sets the single authoritative stable content identifier (ContentId).
	 * Provided for backwards compatibility with the existing Story API.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Story|Identity")
	void SetStoryContentId(FName InStoryContentId) { ContentId = InStoryContentId; }

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

	/* --- Metadata --- */

	/** Extensible key-value metadata for tags, act indices, or level bindings */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Story|Metadata")
	TMap<FName, FString> Metadata;

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
