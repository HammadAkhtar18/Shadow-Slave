// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "StatusEffects/ShadowSlaveStatusEffectTypes.h"
#include "ShadowSlaveStatusEffectDefinition.generated.h"

class UTexture2D;

/**
 * Data-driven primary data asset describing immutable static status effect / condition archetypes.
 * Registered with Unreal Engine's Asset Manager via PrimaryAssetId.
 * Holds technical duration policies, stacking rules, and classification metadata.
 * Cleanly separates immutable archetype definitions from mutable owned runtime instances.
 *
 * NOTE ON CANON:
 * This is a generic technical foundation. It does NOT implement canon-specific status effects.
 */
UCLASS(BlueprintType)
class SHADOWSLAVE_API UShadowSlaveStatusEffectDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UShadowSlaveStatusEffectDefinition();

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	/** Validates definition configuration; returns true if valid, false if malformed */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|StatusEffects|Validation")
	bool IsValidDefinition() const;

	/* --- Identity --- */

	/** Technical internal identifier for this effect */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|StatusEffects|Identity")
	FName EffectId = NAME_None;

	/** Display name shown to players in UI/status */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|StatusEffects|Identity")
	FText DisplayName;

	/** Narrative and mechanical description */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|StatusEffects|Identity", meta = (MultiLine = true))
	FText Description;

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
};
