// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Content/ShadowSlaveContentDefinition.h"
#include "Story/ShadowSlaveStoryTypes.h"
#include "ShadowSlaveStoryDefinition.generated.h"

/**
 * Static primary data asset describing a story chapter, narrative beat, or progression unit.
 * Derived from UShadowSlaveContentDefinition as part of the generic static content pipeline.
 * Pure immutable definition data; zero mutable runtime state.
 * Contains strictly zero hardcoded canon story arcs, characters, or novel events.
 *
 * ARCHITECTURAL PRINCIPLES:
 * 1. Specialized Content Definition: Inherits common metadata (ContentId, DisplayName, Description,
 *    Version, MetadataTags, ProvenanceNote) from UShadowSlaveContentDefinition.
 * 2. Single Authoritative ID: ContentId is the single authoritative stored identifier. Backwards
 *    compatibility is provided through GetStoryId() and SetStoryId() accessors only.
 * 3. Content Type Separation:
 *    - Generic ContentType is statically EShadowSlaveContentType::Story, identifying this asset as
 *      a story definition within the generic content pipeline.
 *    - Distinct from UShadowSlaveStoryContentDefinition (which models content entries/graph nodes within a story).
 *    - Distinct from EShadowSlaveStoryContentType (which classifies child content entries).
 * 4. Story-Specific Data: Owns prerequisite story IDs and ordered child step IDs.
 * 5. Runtime Separation: Cleanly separates immutable story definitions from mutable runtime story state
 *    (managed by UShadowSlaveStorySubsystem).
 * 6. Static Infrastructure: Participates in UShadowSlaveContentRegistrySubsystem without replacing
 *    or coupling to UShadowSlaveStorySubsystem.
 */
UCLASS(BlueprintType)
class SHADOWSLAVE_API UShadowSlaveStoryDefinition : public UShadowSlaveContentDefinition
{
	GENERATED_BODY()

public:
	UShadowSlaveStoryDefinition();

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	/**
	 * Validates definition configuration.
	 * Combines generic content definition validation with story prerequisite and step checks.
	 */
	virtual bool IsValidDefinition(FString* OutErrorMessage = nullptr) const override;

	/* --- Identity Compatibility --- */

	/**
	 * Returns the single authoritative stable content identifier (ContentId).
	 * Provided for backwards compatibility with the existing Story API.
	 */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Story|Identity")
	FName GetStoryId() const { return ContentId; }

	/**
	 * Sets the single authoritative stable content identifier (ContentId).
	 * Provided for backwards compatibility with the existing Story API.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Story|Identity")
	void SetStoryId(FName InStoryId) { ContentId = InStoryId; }

	/* --- Flow & Prerequisites --- */

	/** Story IDs that must be in Completed state before this story can become Available or Active */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Story|Flow")
	TArray<FName> PrerequisiteStoryIds;

	/** Ordered child step or beat identifiers belonging to this story */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Story|Flow")
	TArray<FName> StepIds;

	/** Extensible metadata for chapter numbering, quest tags, or zone binding */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Story|Metadata")
	TMap<FName, FString> Metadata;

	/* --- Validation --- */

	/** Validates definition integrity and outputs any discovered errors */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Story|Validation")
	bool ValidateDefinition(TArray<FText>& OutErrors) const;
};
