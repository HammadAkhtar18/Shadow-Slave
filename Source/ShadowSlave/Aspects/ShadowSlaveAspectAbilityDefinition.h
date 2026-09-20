// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Content/ShadowSlaveContentDefinition.h"
#include "Progression/ShadowSlaveProgressionTypes.h"
#include "ShadowSlaveAspectAbilityDefinition.generated.h"

class UTexture2D;

/**
 * Data-driven content definition describing an immutable static Aspect Ability archetype.
 * Specialization of UShadowSlaveContentDefinition for the generic content pipeline.
 *
 * ARCHITECTURAL PRINCIPLES:
 * 1. SPECIALIZED CONTENT DEFINITION: AbilityDefinition is a specialized static content definition
 *    deriving from UShadowSlaveContentDefinition.
 * 2. GENERIC CONTENT CLASSIFICATION: Generic ContentType identifies this asset as
 *    EShadowSlaveContentType::Ability across the content pipeline and registry.
 * 3. SINGLE AUTHORITATIVE ID: ContentId is the sole stored stable identifier. GetAbilityId() and SetAbilityId()
 *    provide backward-compatible accessors without duplicate storage or reference-member aliases.
 * 4. RUNTIME STATE SEPARATION: UShadowSlaveAspectComponent remains authoritative for runtime ability
 *    state (unlocked status, active/sustained state, dynamic instance properties).
 * 5. CHARACTER RANK AUTHORITY: UShadowSlaveProgressionComponent remains authoritative for Character Rank
 *    validation when evaluating RequiredCharacterRank prerequisites.
 * 6. ESSENCE AUTHORITY: UShadowSlaveAttributeComponent remains authoritative for Essence resource tracking
 *    and consumption when activating abilities with a BaseEssenceCost.
 * 7. CONTENT REGISTRY ROLE: UShadowSlaveContentRegistrySubsystem provides static definition lookup and
 *    discovery only; it does NOT track runtime aspect, progression, or attribute state.
 */
UCLASS(BlueprintType)
class SHADOWSLAVE_API UShadowSlaveAspectAbilityDefinition : public UShadowSlaveContentDefinition
{
	GENERATED_BODY()

public:
	UShadowSlaveAspectAbilityDefinition();

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	/**
	 * Validates definition configuration.
	 * Combines base generic content validation (valid ContentId, valid DisplayName, Version >= 1, ContentType == Ability)
	 * with Ability-specific rules (finite, non-negative BaseEssenceCost).
	 */
	virtual bool IsValidDefinition(FString* OutErrorMessage = nullptr) const override;

	/* --- Identification Compatibility Accessors --- */

	/** Compatibility accessor returning authoritative ContentId as AbilityId */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|AspectAbility|Identity")
	FName GetAbilityId() const { return ContentId; }

	/** Compatibility accessor setting authoritative ContentId as AbilityId */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|AspectAbility|Identity")
	void SetAbilityId(FName InAbilityId) { ContentId = InAbilityId; }

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
