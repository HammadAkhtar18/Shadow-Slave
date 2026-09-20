// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Content/ShadowSlaveContentTypes.h"
#include "ShadowSlaveContentDefinition.generated.h"

/**
 * Base primary data asset for canonical content definitions in the Shadow Slave prototype.
 *
 * ARCHITECTURAL PRINCIPLES:
 * 1. STATIC CONTENT ARCHETYPE: Data Assets hold static authored configuration data only.
 *    They must never hold mutable runtime state (such as health, current cooldown, or active summon instance).
 * 2. STABLE IDENTIFIERS: ContentId (FName) is the authoritative identifier across systems and save data.
 *    Object paths and asset names may change during refactoring, but ContentId remains stable.
 * 3. RUNTIME SEPARATION: Runtime components (InventoryComponent, EchoComponent, StatusEffectComponent, etc.)
 *    continue to own and manage mutable runtime state.
 * 4. CANONICAL CONTENT PIPELINE: Canonical content (characters, memories, echoes, abilities, quests, stories)
 *    should eventually be authored as Data Assets inheriting from or conforming to this pipeline, replacing
 *    hard-coded C++ if/else factories.
 * 5. FUTURE MIGRATION: In Step 37, existing specialized definitions (UShadowSlaveMemoryDefinition,
 *    UShadowSlaveEchoDefinition, etc.) are kept intact to prevent API churn and avoid regression risks.
 *    Future steps can migrate specific content definitions to inherit from this base class or register
 *    adapters into the registry.
 */
UCLASS(BlueprintType)
class SHADOWSLAVE_API UShadowSlaveContentDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UShadowSlaveContentDefinition();

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	/**
	 * Validates definition configuration.
	 * Returns true if valid, false if malformed with an optional error description.
	 */
	virtual bool IsValidDefinition(FString* OutErrorMessage = nullptr) const;

	/** Blueprint-callable validation helper */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Content|Validation")
	bool ValidateDefinition(FString& OutErrorMessage) const;

	/* --- Identity & Metadata --- */

	/** Authoritative unique technical identifier for this content definition */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Content|Identity")
	FName ContentId = NAME_None;

	/** Broad generic content category */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Content|Classification")
	EShadowSlaveContentType ContentType = EShadowSlaveContentType::None;

	/** Display name shown to players in UI and logs */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Content|Identity")
	FText DisplayName;

	/** Narrative or descriptive text */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Content|Identity", meta = (MultiLine = true))
	FText Description;

	/** Content version number for compatibility and future save migration */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Content|Identity", meta = (ClampMin = "1"))
	int32 Version = 1;

	/** Native Gameplay Tags for generic categorization, querying, and filtering */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Content|Metadata")
	FGameplayTagContainer MetadataTags;

	/** Authoring provenance, novel chapter reference, or source note for tracking canon fidelity */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Content|Provenance")
	FString ProvenanceNote;

protected:
	/**
	 * Hook for derived classes with ContentType == Custom to supply a specialized PrimaryAssetType name.
	 * Keeps PrimaryAssetId construction and ContentId fallbacks centralized in this generic base.
	 */
	virtual FName GetCustomPrimaryAssetType() const { return TEXT("ContentCustom"); }
};
