// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Content/ShadowSlaveContentDefinition.h"
#include "Gameplay/ShadowSlaveQuestTypes.h"
#include "ShadowSlaveQuestDefinition.generated.h"

/**
 * Static primary data asset defining a quest, its objectives, prerequisites, and metadata.
 * Derived from UShadowSlaveContentDefinition as part of the generic static content pipeline.
 * Pure immutable authoring definition data; zero mutable runtime progress or state.
 * Contains strictly zero hardcoded canon story arcs or novel content.
 *
 * ARCHITECTURAL PRINCIPLES:
 * 1. Specialized Content Definition: Inherits common metadata (ContentId, DisplayName, Description,
 *    Version, MetadataTags, ProvenanceNote) from UShadowSlaveContentDefinition.
 * 2. Single Authoritative ID: ContentId is the single authoritative stored identifier. Backwards
 *    compatibility is provided through GetQuestId() and SetQuestId() accessors only.
 * 3. Content Type Separation:
 *    - Generic ContentType is statically EShadowSlaveContentType::Quest, identifying this asset as
 *      a quest definition within the generic content pipeline.
 *    - Primary Asset Type is "Quest", producing FPrimaryAssetId("Quest", ContentId) via generic base.
 * 4. Quest-Specific Data: Owns ordered objectives, prerequisite quest IDs, and completion flags.
 * 5. Runtime Separation: Cleanly separates immutable quest definitions from mutable runtime quest state
 *    (managed by UShadowSlaveQuestSubsystem).
 * 6. Static Infrastructure: Participates in UShadowSlaveContentRegistrySubsystem without replacing
 *    or coupling to UShadowSlaveQuestSubsystem.
 */
UCLASS(BlueprintType)
class SHADOWSLAVE_API UShadowSlaveQuestDefinition : public UShadowSlaveContentDefinition
{
	GENERATED_BODY()

public:
	UShadowSlaveQuestDefinition();

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	/**
	 * Validates definition configuration.
	 * Combines generic content definition validation with quest objective and prerequisite checks.
	 */
	virtual bool IsValidDefinition(FString* OutErrorMessage = nullptr) const override;

	/* --- Identity Compatibility --- */

	/**
	 * Returns the single authoritative stable content identifier (ContentId).
	 * Provided for backwards compatibility with the existing Quest API.
	 */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Quest|Identity")
	FName GetQuestId() const { return ContentId; }

	/**
	 * Sets the single authoritative stable content identifier (ContentId).
	 * Provided for backwards compatibility with the existing Quest API.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Quest|Identity")
	void SetQuestId(FName InQuestId) { ContentId = InQuestId; }

	/* --- Objectives & Flow --- */

	/** Ordered collection of objectives required to advance or complete this quest */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Quest|Objectives")
	TArray<FShadowSlaveObjectiveDefinition> Objectives;

	/** Quest IDs that must be in Completed state before this quest can become Available or Active */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Quest|Flow")
	TArray<FName> PrerequisiteQuestIds;

	/** If true, the quest automatically completes when all non-optional objectives are satisfied */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Quest|Flow")
	bool bAutoCompleteWhenObjectivesComplete = true;

	/** Extensible metadata for quest categorisation, zone tags, or narrative grouping */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Quest|Metadata")
	TMap<FName, FString> Metadata;

	/* --- Queries & Validation --- */

	/** Finds an objective definition by its ObjectiveId */
	const FShadowSlaveObjectiveDefinition* FindObjective(FName ObjectiveId) const;

	/** Checks if this quest defines an objective with the specified ObjectiveId */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Quest|Objectives")
	bool HasObjective(FName ObjectiveId) const;

	/** Validates definition integrity and outputs any discovered errors */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Quest|Validation")
	bool ValidateDefinition(TArray<FText>& OutErrors) const;
};
