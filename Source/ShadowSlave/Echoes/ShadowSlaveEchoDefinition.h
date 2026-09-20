// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Content/ShadowSlaveContentDefinition.h"
#include "Echoes/ShadowSlaveEchoTypes.h"
#include "ShadowSlaveEchoDefinition.generated.h"

class APawn;
class UTexture2D;

/**
 * Data-driven primary data asset describing immutable static Echo archetypes.
 * Derived from UShadowSlaveContentDefinition as part of the generic static content pipeline.
 *
 * ARCHITECTURAL PRINCIPLES:
 * 1. Specialized Content Definition: Inherits common metadata (ContentId, DisplayName, Description,
 *    Version, MetadataTags, ProvenanceNote) from UShadowSlaveContentDefinition.
 * 2. Single Authoritative ID: ContentId is the single authoritative stored identifier. Backwards
 *    compatibility is provided through GetEchoId() and SetEchoId() accessors only.
 * 3. Content Type: Statically identified as EShadowSlaveContentType::Echo.
 * 4. Echo-Specific Data: Owns technical classifications (Rank, Class), summoning parameters, and
 *    future manifestation assets.
 * 5. Runtime Separation: Cleanly separates immutable Echo definitions from mutable owned runtime
 *    instances (FShadowSlaveEchoInstance managed by UShadowSlaveEchoComponent).
 * 6. Static Infrastructure: Participates in UShadowSlaveContentRegistrySubsystem without replacing
 *    or coupling to UShadowSlaveEchoComponent.
 */
UCLASS(BlueprintType)
class SHADOWSLAVE_API UShadowSlaveEchoDefinition : public UShadowSlaveContentDefinition
{
	GENERATED_BODY()

public:
	UShadowSlaveEchoDefinition();

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	/**
	 * Validates definition configuration.
	 * Performs generic content validation (ID, Version, ContentType) and Echo-specific validation.
	 */
	virtual bool IsValidDefinition(FString* OutErrorMessage = nullptr) const override;

	/* --- Identity Compatibility --- */

	/**
	 * Returns the single authoritative stable content identifier (ContentId).
	 * Provided for backwards compatibility with the existing Echo API.
	 */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Echo|Identity")
	FName GetEchoId() const { return ContentId; }

	/**
	 * Sets the single authoritative stable content identifier (ContentId).
	 * Provided for backwards compatibility with the existing Echo API.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Echo|Identity")
	void SetEchoId(FName InEchoId) { ContentId = InEchoId; }

	/* --- Classification --- */

	/** Canon Echo Rank (Dormant, Awakened, Ascended, etc.) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Echo|Classification")
	EShadowSlaveEchoRank Rank = EShadowSlaveEchoRank::Unknown;

	/** Canon Echo Class (Beast, Monster, Demon, Devil, Tyrant, Terror, Titan) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Echo|Classification")
	EShadowSlaveEchoClass Class = EShadowSlaveEchoClass::Unknown;

	/* --- Summoning & Resource Costs --- */

	/** Essence cost consumed immediately upon summoning this Echo */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Echo|Costs", meta = (ClampMin = "0.0"))
	float SummonEssenceCost = 0.0f;

	/* --- Future-Facing Manifestation Assets --- */

	/**
	 * Future-facing soft reference to a Pawn class for visual/physical manifestation.
	 * NOTE: At this foundation level, NO world actor is spawned, no AI is created,
	 * and this property does NOT participate in save/load restoration or runtime execution.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Echo|Assets")
	TSoftClassPtr<APawn> PawnClass;

	/** Icon for HUD and menu representation */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Echo|Assets")
	TSoftObjectPtr<UTexture2D> Icon;
};
