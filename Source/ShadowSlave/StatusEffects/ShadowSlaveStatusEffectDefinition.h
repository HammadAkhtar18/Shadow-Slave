// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Content/ShadowSlaveContentDefinition.h"
#include "StatusEffects/ShadowSlaveStatusEffectTypes.h"
#include "ShadowSlaveStatusEffectDefinition.generated.h"

class UTexture2D;

/**
 * Data-driven primary data asset describing immutable static status effect / condition archetypes.
 * Derived from UShadowSlaveContentDefinition as part of the generic static content pipeline.
 *
 * ARCHITECTURAL PRINCIPLES:
 * 1. Specialized Content Definition: Inherits common metadata (ContentId, DisplayName, Description,
 *    Version, MetadataTags, ProvenanceNote) from UShadowSlaveContentDefinition.
 * 2. Single Authoritative ID: ContentId is the single authoritative stored identifier. Backwards
 *    compatibility is provided through GetEffectId() and SetEffectId() accessors only.
 * 3. Content Type: Statically identified as EShadowSlaveContentType::Custom. Note that 'Custom' is
 *    a generic architectural classification and does NOT imply any canon meaning.
 * 4. Status Effect-Specific Data: Owns technical duration policies, stacking rules, polarity, and
 *    save/load persistence flags.
 * 5. Runtime Separation: Cleanly separates immutable archetype definitions from mutable owned runtime
 *    instances (FShadowSlaveStatusEffectInstance managed by UShadowSlaveStatusEffectComponent).
 * 6. Static Infrastructure: Participates in UShadowSlaveContentRegistrySubsystem without replacing
 *    or coupling to UShadowSlaveStatusEffectComponent.
 *
 * NOTE ON CANON:
 * This is a generic technical foundation. It does NOT implement canon-specific status effects.
 */
UCLASS(BlueprintType)
class SHADOWSLAVE_API UShadowSlaveStatusEffectDefinition : public UShadowSlaveContentDefinition
{
	GENERATED_BODY()

public:
	UShadowSlaveStatusEffectDefinition();

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	/**
	 * Validates definition configuration.
	 * Combines generic content validation with status effect-specific rules.
	 */
	virtual bool IsValidDefinition(FString* OutErrorMessage = nullptr) const override;

	/* --- Identity Compatibility --- */

	/**
	 * Returns the single authoritative stable content identifier (ContentId).
	 * Provided for backwards compatibility with the existing Status Effect API.
	 */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|StatusEffects|Identity")
	FName GetEffectId() const { return ContentId; }

	/**
	 * Sets the single authoritative stable content identifier (ContentId).
	 * Provided for backwards compatibility with the existing Status Effect API.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|StatusEffects|Identity")
	void SetEffectId(FName InEffectId) { ContentId = InEffectId; }

	/* --- Duration & Stacking Policies --- */

	/** How this effect handles duration and expiration */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|StatusEffects|Rules")
	EStatusEffectDurationPolicy DurationPolicy = EStatusEffectDurationPolicy::Timed;

	/** Duration in seconds (only evaluated when DurationPolicy == Timed) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|StatusEffects|Rules", meta = (ClampMin = "0.0", EditCondition = "DurationPolicy == EStatusEffectDurationPolicy::Timed"))
	float Duration = 5.0f;

	/** How this effect handles reapplication */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|StatusEffects|Rules")
	EStatusEffectStackingPolicy StackingPolicy = EStatusEffectStackingPolicy::RefreshDuration;

	/** Maximum allowed stacks (only evaluated when StackingPolicy == AddStacks) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|StatusEffects|Rules", meta = (ClampMin = "1"))
	int32 MaxStacks = 1;

	/* --- Classification & Metadata --- */

	/** Generic polarity classification (beneficial, harmful, neutral) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|StatusEffects|Classification")
	EStatusEffectPolarity Polarity = EStatusEffectPolarity::Neutral;

	/** Whether this effect is persistent across save/load sessions */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|StatusEffects|Persistence")
	bool bPersistAcrossSaveLoad = false;

	/** Optional icon for HUD/status presentation */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|StatusEffects|Assets")
	TSoftObjectPtr<UTexture2D> Icon;

	/** Optional technical tags for query and filtering */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|StatusEffects|Tags")
	TArray<FName> CustomTags;

protected:
	/** Returns the deterministic Primary Asset type name for Status Effects */
	virtual FName GetCustomPrimaryAssetType() const override { return TEXT("StatusEffect"); }
};
