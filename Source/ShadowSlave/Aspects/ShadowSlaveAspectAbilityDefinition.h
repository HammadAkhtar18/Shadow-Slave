// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Progression/ShadowSlaveProgressionTypes.h"
#include "ShadowSlaveAspectAbilityDefinition.generated.h"

class UTexture2D;

/**
 * Data-driven primary data asset describing an immutable static Aspect Ability archetype.
 * Registered with Unreal Engine's Asset Manager via PrimaryAssetId.
 * Holds verified canon classifications, rank prerequisites, and narrative descriptions.
 * Cleanly separates immutable archetype definitions from mutable owned runtime instances.
 */
UCLASS(BlueprintType)
class SHADOWSLAVE_API UShadowSlaveAspectAbilityDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UShadowSlaveAspectAbilityDefinition();

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	/* --- Identity --- */

	/** Technical unique identifier for this Aspect Ability */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|AspectAbility|Identity")
	FName AbilityId = NAME_None;

	/** Display name shown to players in UI/status */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|AspectAbility|Identity")
	FText DisplayName;

	/** Narrative and mechanical description of the ability */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|AspectAbility|Identity", meta = (MultiLine = true))
	FText Description;

	/* --- Requirements & Parameters --- */

	/**
	 * Minimum character rank required to awaken/unlock this Aspect ability.
	 * Set to Unknown if no rank prerequisite is enforced.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|AspectAbility|Requirements")
	EShadowSlaveCharacterRank RequiredCharacterRank = EShadowSlaveCharacterRank::Unknown;

	/** Optional technical base essence cost parameter (actual consumption handled via AttributeComponent) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|AspectAbility|Parameters", meta = (ClampMin = "0.0"))
	float BaseEssenceCost = 0.0f;

	/** Returns whether this ability enforces a specific minimum character rank */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|AspectAbility|Requirements")
	bool HasRankRequirement() const { return RequiredCharacterRank != EShadowSlaveCharacterRank::Unknown; }

	/* --- Visuals & Metadata --- */

	/** Optional icon for UI display */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|AspectAbility|Visuals")
	TSoftObjectPtr<UTexture2D> Icon;

	/** Extensible metadata key-value pairs for future gameplay systems */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|AspectAbility|Metadata")
	TMap<FName, FString> Metadata;

	/** Canon research provenance or verification notes */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|AspectAbility|Metadata")
	FString CanonProvenance;
};
