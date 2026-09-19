// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Echoes/ShadowSlaveEchoTypes.h"
#include "ShadowSlaveEchoDefinition.generated.h"

class APawn;
class UTexture2D;

/**
 * Data-driven primary data asset describing immutable static Echo archetypes.
 * Registered with Unreal Engine's Asset Manager via PrimaryAssetId.
 * Holds technical classifications (Rank, Class), summoning parameters, and visual/pawn references.
 * Cleanly separates immutable Echo definitions from mutable owned runtime instances.
 */
UCLASS(BlueprintType)
class SHADOWSLAVE_API UShadowSlaveEchoDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UShadowSlaveEchoDefinition();

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	/* --- Identity --- */

	/** Technical internal identifier for this Echo */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Echo|Identity")
	FName EchoId = NAME_None;

	/** Display name shown to players in UI/logs */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Echo|Identity")
	FText DisplayName;

	/** Narrative or description text */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Echo|Identity", meta = (MultiLine = true))
	FText Description;

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

	/** Essence upkeep required while this Echo remains summoned (per second) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Echo|Costs", meta = (ClampMin = "0.0"))
	float EssenceUpkeepPerSecond = 0.0f;

	/* --- Manifestation Assets --- */

	/** Optional pawn class to spawn when this Echo is manifested in the world */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Echo|Assets")
	TSoftClassPtr<APawn> PawnClass;

	/** Icon for HUD and menu representation */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Echo|Assets")
	TSoftObjectPtr<UTexture2D> Icon;
};
