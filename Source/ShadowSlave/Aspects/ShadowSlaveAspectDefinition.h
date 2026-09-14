// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Aspects/ShadowSlaveAspectTypes.h"
#include "Aspects/ShadowSlaveAspectAbilityDefinition.h"
#include "Aspects/ShadowSlaveFlawDefinition.h"
#include "ShadowSlaveAspectDefinition.generated.h"

class UTexture2D;

/**
 * Data-driven primary data asset defining an immutable Aspect archetype in Shadow Slave.
 * Holds verified canon classification (Aspect Rank), static ability definitions, and bound Flaw definition.
 * Cleanly separates immutable static Aspect archetype data from mutable runtime character state.
 */
UCLASS(BlueprintType)
class SHADOWSLAVE_API UShadowSlaveAspectDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UShadowSlaveAspectDefinition();

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	/* --- Identity --- */

	/** Technical unique identifier for this Aspect */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Aspect|Identity")
	FName AspectId = NAME_None;

	/** Display name shown to players in UI/status (e.g. "Shadow Slave") */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Aspect|Identity")
	FText DisplayName;

	/** Narrative description of the Aspect */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Aspect|Identity", meta = (MultiLine = true))
	FText Description;

	/* --- Classification --- */

	/**
	 * Aspect Rank reflecting the intrinsic rarity/potency tier of the Aspect.
	 * NOTE: Separate from character rank (e.g. Divine aspect on a Dormant sleeper).
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Aspect|Classification")
	EShadowSlaveAspectRank AspectRank = EShadowSlaveAspectRank::Unknown;

	/** Returns whether this Aspect has a verified/known Aspect Rank */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Aspect|Classification")
	bool HasKnownAspectRank() const { return AspectRank != EShadowSlaveAspectRank::Unknown; }

	/* --- Abilities & Flaw --- */

	/** Static archetype definitions of abilities granted by this Aspect */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Aspect|Abilities")
	TArray<TObjectPtr<UShadowSlaveAspectAbilityDefinition>> AbilityDefinitions;

	/** Returns total number of static ability definitions on this Aspect */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Aspect|Abilities")
	int32 GetAbilityCount() const { return AbilityDefinitions.Num(); }

	/** Finds an ability definition by its unique AbilityId */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Aspect|Abilities")
	UShadowSlaveAspectAbilityDefinition* FindAbilityById(FName AbilityId) const;

	/** Flaw definition intrinsically bound to this Aspect */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Aspect|Flaw")
	TObjectPtr<UShadowSlaveFlawDefinition> FlawDefinition = nullptr;

	/** Returns whether this Aspect defines a bound Flaw */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Aspect|Flaw")
	bool HasFlaw() const { return FlawDefinition != nullptr; }

	/* --- Visuals & Metadata --- */

	/** Optional UI icon representing the Aspect */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Aspect|Visuals")
	TSoftObjectPtr<UTexture2D> Icon;

	/** Extensible metadata key-value pairs */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Aspect|Metadata")
	TMap<FName, FString> Metadata;

	/** Canon research provenance or verification notes */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Aspect|Metadata")
	FString CanonProvenance;
};
