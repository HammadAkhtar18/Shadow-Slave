// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Content/ShadowSlaveContentDefinition.h"
#include "Nightmares/ShadowSlaveNightmareTypes.h"
#include "Nightmares/ShadowSlaveNightmareObjectiveTypes.h"
#include "ShadowSlaveNightmareScenarioDefinition.generated.h"

class UWorld;

/**
 * Primary Data Asset defining a static Nightmare Scenario archetype in Shadow Slave.
 * Derived from UShadowSlaveContentDefinition as part of the generic static content pipeline.
 * Encapsulates stable identity, metadata, objectives, completion rules, and soft world references.
 *
 * ARCHITECTURAL PRINCIPLES:
 * 1. Specialized Content Definition: Inherits common metadata (ContentId, DisplayName, Description,
 *    Version, MetadataTags, ProvenanceNote) from UShadowSlaveContentDefinition.
 * 2. Single Authoritative ID: ContentId is the single authoritative stored identifier. Backwards
 *    compatibility is provided through GetScenarioId() and SetScenarioId() accessors only.
 * 3. Content Type Separation:
 *    - Generic ContentType is statically EShadowSlaveContentType::Custom, identifying this asset as
 *      a specialized scenario definition without adding ad-hoc entries to the generic enum.
 *    - Specialized Primary Asset Type is "NightmareScenario" via GetCustomPrimaryAssetType().
 * 4. Scenario-Specific Data: Owns objectives, completion rules, map/world references, and scenario metadata.
 * 5. Runtime Separation: Cleanly separates immutable scenario definitions from mutable runtime session
 *    state (managed by UShadowSlaveNightmareSubsystem and UShadowSlaveNightmareObjectiveTracker).
 * 6. Static Infrastructure: Participates in UShadowSlaveContentRegistrySubsystem for static definition
 *    lookup only; the registry does not manage active Nightmare sessions.
 *
 * NOTE: Strictly a framework content archetype.
 * Does NOT contain First Nightmare canon content or runtime mutable state.
 */
UCLASS(BlueprintType)
class SHADOWSLAVE_API UShadowSlaveNightmareScenarioDefinition : public UShadowSlaveContentDefinition
{
	GENERATED_BODY()

public:
	UShadowSlaveNightmareScenarioDefinition();

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	/**
	 * Validates definition configuration.
	 * Combines generic content definition validation with scenario objective and completion checks.
	 */
	virtual bool IsValidDefinition(FString* OutErrorMessage = nullptr) const override;

	/* --- Identity Compatibility --- */

	/**
	 * Returns the single authoritative stable content identifier (ContentId).
	 * Provided for backwards compatibility with the existing Nightmare API.
	 */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Nightmare|Identity")
	FName GetScenarioId() const { return ContentId; }

	/**
	 * Sets the single authoritative stable content identifier (ContentId).
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Nightmare|Identity")
	void SetScenarioId(FName InScenarioId) { ContentId = InScenarioId; }

	/**
	 * Returns the schema version number for backwards compatibility.
	 */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Nightmare|Identity")
	int32 GetScenarioVersion() const { return Version; }

	/**
	 * Sets the schema version number for backwards compatibility.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Nightmare|Identity")
	void SetScenarioVersion(int32 InVersion) { Version = InVersion; }

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

protected:
	virtual FName GetCustomPrimaryAssetType() const override { return TEXT("NightmareScenario"); }
};
