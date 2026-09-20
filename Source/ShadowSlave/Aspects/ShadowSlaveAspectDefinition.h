// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Content/ShadowSlaveContentDefinition.h"
#include "Aspects/ShadowSlaveAspectTypes.h"
#include "Aspects/ShadowSlaveAspectAbilityDefinition.h"
#include "Aspects/ShadowSlaveFlawDefinition.h"
#include "ShadowSlaveAspectDefinition.generated.h"

class UTexture2D;

/**
 * Data-driven content definition defining an immutable Aspect archetype in Shadow Slave.
 * Specialization of UShadowSlaveContentDefinition for the generic content pipeline.
 *
 * ARCHITECTURAL PRINCIPLES:
 * 1. SPECIALIZED CONTENT DEFINITION: AspectDefinition is a specialized static content definition
 *    deriving from UShadowSlaveContentDefinition.
 * 2. GENERIC CONTENT CLASSIFICATION: Generic ContentType is EShadowSlaveContentType::Custom because
 *    the generic content pipeline taxonomy currently does not define a dedicated Aspect member.
 * 3. SPECIALIZED PRIMARY ASSET TYPE: Utilizes the GetCustomPrimaryAssetType() hook to produce
 *    deterministic PrimaryAssetId with PrimaryAssetType "Aspect": FPrimaryAssetId(TEXT("Aspect"), ContentId).
 * 4. SINGLE AUTHORITATIVE ID: ContentId is the sole stored stable identifier. GetAspectId() and SetAspectId()
 *    provide backward-compatible accessors without duplicate storage or reference-member aliases.
 * 5. RUNTIME STATE SEPARATION: Aspect runtime binding, ability instances, and Flaw ownership remain
 *    authoritative with UShadowSlaveAspectComponent; static definition data remains immutable.
 * 6. CHARACTER RANK AUTHORITY: UShadowSlaveProgressionComponent remains authoritative for Character Rank.
 *    AspectRank on this definition is a separate classification from character rank.
 * 7. CONTENT REGISTRY ROLE: UShadowSlaveContentRegistrySubsystem provides static definition lookup and
 *    discovery only; it does NOT track runtime aspect, character, or ability state.
 */
UCLASS(BlueprintType)
class SHADOWSLAVE_API UShadowSlaveAspectDefinition : public UShadowSlaveContentDefinition
{
	GENERATED_BODY()

public:
	UShadowSlaveAspectDefinition();

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	/**
	 * Validates definition configuration.
	 * Combines base generic content validation (valid ContentId, valid DisplayName, Version >= 1, ContentType == Custom).
	 */
	virtual bool IsValidDefinition(FString* OutErrorMessage = nullptr) const override;

	/* --- Identification Compatibility Accessors --- */

	/** Compatibility accessor returning authoritative ContentId as AspectId */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Aspect|Identity")
	FName GetAspectId() const { return ContentId; }

	/** Compatibility accessor setting authoritative ContentId as AspectId */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Aspect|Identity")
	void SetAspectId(FName InAspectId) { ContentId = InAspectId; }

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

protected:
	/** Hook returning specialized PrimaryAssetType "Aspect" for ContentType == Custom */
	virtual FName GetCustomPrimaryAssetType() const override { return TEXT("Aspect"); }
};
