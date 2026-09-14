// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Nightmares/ShadowSlaveNightmareTypes.h"
#include "Nightmares/ShadowSlaveNightmareObjectiveTypes.h"
#include "ShadowSlaveNightmareScenarioDefinition.generated.h"

class UWorld;

/**
 * Primary Data Asset defining a static Nightmare Scenario archetype in Shadow Slave.
 * Encapsulates stable identity, metadata, objectives, completion rules, and soft world references.
 *
 * NOTE: Strictly a framework content archetype.
 * Does NOT contain First Nightmare canon content or runtime mutable state.
 */
UCLASS(BlueprintType)
class SHADOWSLAVE_API UShadowSlaveNightmareScenarioDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UShadowSlaveNightmareScenarioDefinition();

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	/* --- Identity & Metadata --- */

	/** Stable programmatic identifier distinguishing this scenario (never use DisplayName as identity) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Nightmare|Identity")
	FName ScenarioId = NAME_None;

	/** Human-readable display title */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Nightmare|Identity")
	FText DisplayName;

	/** Scenario narrative premise or overview */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Nightmare|Identity", meta = (MultiLine = true))
	FText Description;

	/** Schema version number for backwards compatibility and save validation */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Nightmare|Identity", meta = (ClampMin = "1"))
	int32 ScenarioVersion = 1;

	/* --- World & Map Boundary --- */

	/**
	 * Soft object reference to the scenario level/world.
	 * Decoupled: does not force level loading or hardcode maps.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Nightmare|World")
	TSoftObjectPtr<UWorld> ScenarioMap;

	/* --- Objectives & Rules --- */

	/** Static archetype definitions for all objectives available in this scenario */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Nightmare|Objectives")
	TArray<FShadowSlaveNightmareObjectiveDefinition> Objectives;

	/** Evaluation rule for scenario completion */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Nightmare|Rules")
	EShadowSlaveScenarioCompletionRule CompletionRule = EShadowSlaveScenarioCompletionRule::AllRequiredObjectives;

	/** Extensible metadata storage for scenario parameters (difficulty tier, tags, etc.) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Nightmare|Metadata")
	TMap<FName, FString> ScenarioMetadata;

	/* --- Validation & Query Helpers --- */

	/** Validates scenario configuration integrity without requiring Unreal editor tooling */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Nightmare|Validation")
	bool ValidateScenario(FText& OutError) const;

	/** Finds an objective definition by its stable ID */
	const FShadowSlaveNightmareObjectiveDefinition* FindObjectiveDefinition(FName InObjectiveId) const;

	/** Development test factory for headless/automated validation (ZERO canon First Nightmare content) */
	static UShadowSlaveNightmareScenarioDefinition* CreateTestScenarioDefinition(UObject* Outer = nullptr);
};
