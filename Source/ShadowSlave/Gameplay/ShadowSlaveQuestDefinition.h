// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Gameplay/ShadowSlaveQuestTypes.h"
#include "ShadowSlaveQuestDefinition.generated.h"

/**
 * Static primary data asset defining a quest, its objectives, prerequisites, and metadata.
 * Pure immutable definition data; zero mutable runtime progress or state.
 * Contains strictly zero hardcoded canon story arcs or novel content.
 */
UCLASS(BlueprintType)
class SHADOWSLAVE_API UShadowSlaveQuestDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UShadowSlaveQuestDefinition();

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	/* --- Identification & Display --- */

	/** Stable unique technical identifier for this quest */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Quest|Identity")
	FName QuestId = NAME_None;

	/** Player-facing quest title */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Quest|Identity")
	FText DisplayName;

	/** Player-facing quest briefing and narrative description */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Quest|Identity", meta = (MultiLine = true))
	FText Description;

	/** Content version number for compatibility and save migration */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Quest|Identity", meta = (ClampMin = "1"))
	int32 Version = 1;

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

	/** Technical provenance and source attribution note */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Quest|Provenance")
	FString ProvenanceNote;

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
