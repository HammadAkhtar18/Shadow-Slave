// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Story/ShadowSlaveStoryTypes.h"
#include "ShadowSlaveStoryDefinition.generated.h"

/**
 * Static primary data asset describing a story chapter, narrative beat, or progression unit.
 * Pure immutable definition data; zero mutable runtime state.
 * Contains strictly zero hardcoded canon story arcs, characters, or novel events.
 */
UCLASS(BlueprintType)
class SHADOWSLAVE_API UShadowSlaveStoryDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UShadowSlaveStoryDefinition();

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	/* --- Identification & Display --- */

	/** Stable unique technical identifier for this story unit */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Story|Identity")
	FName StoryId = NAME_None;

	/** Player-facing title or chapter name */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Story|Identity")
	FText DisplayName;

	/** Descriptive narrative summary or objective briefing */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Story|Identity", meta = (MultiLine = true))
	FText Description;

	/** Content version number for compatibility and save migration */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Story|Identity", meta = (ClampMin = "1"))
	int32 Version = 1;

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

	/** Technical provenance and source attribution note for future canon alignment tracking */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Story|Provenance")
	FString ProvenanceNote;

	/* --- Validation --- */

	/** Validates definition integrity and outputs any discovered errors */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Story|Validation")
	bool ValidateDefinition(TArray<FText>& OutErrors) const;
};
