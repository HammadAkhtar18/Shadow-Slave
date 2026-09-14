// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ShadowSlaveProgressionTypes.generated.h"

/**
 * Technical and canon-aware character rank representing the established Nightmare Spell ranks.
 * NOTE: Independent of Aspect Rank (which measures Aspect rarity/potency).
 */
UENUM(BlueprintType)
enum class EShadowSlaveCharacterRank : uint8
{
	Unknown      UMETA(DisplayName = "Unknown"),
	Dormant      UMETA(DisplayName = "Dormant"),
	Awakened     UMETA(DisplayName = "Awakened"),
	Ascended     UMETA(DisplayName = "Ascended"),
	Transcendent UMETA(DisplayName = "Transcendent"),
	Supreme      UMETA(DisplayName = "Supreme"),
	Sacred       UMETA(DisplayName = "Sacred"),
	Divine       UMETA(DisplayName = "Divine")
};

/**
 * Runtime state representing an entity's soul core configuration.
 * Supports zero cores, single-core entities (standard humans/awakened),
 * and multi-core divine aspects without hardcoding universal limits.
 */
USTRUCT(BlueprintType)
struct SHADOWSLAVE_API FShadowSlaveSoulCoreState
{
	GENERATED_BODY()

	/** Current number of formed soul cores */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Progression", meta = (ClampMin = "0"))
	int32 CurrentSoulCores = 1;

	/** Maximum number of soul cores this entity can cultivate or form */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Progression", meta = (ClampMin = "1"))
	int32 MaximumSoulCores = 1;

	FShadowSlaveSoulCoreState() = default;

	FShadowSlaveSoulCoreState(int32 InCurrent, int32 InMax)
		: CurrentSoulCores(FMath::Max(0, InCurrent)), MaximumSoulCores(FMath::Max(1, InMax))
	{
	}

	/** Returns true if this entity possesses more than one soul core */
	bool HasMultipleCores() const
	{
		return CurrentSoulCores > 1;
	}

	/** Returns whether soul core count is at maximum capacity */
	bool IsMaxCoresReached() const
	{
		return CurrentSoulCores >= MaximumSoulCores;
	}
};

/* --- Progression Event Delegates --- */

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCharacterRankChangedSignature, EShadowSlaveCharacterRank, NewRank, EShadowSlaveCharacterRank, OldRank);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSoulCoreCountChangedSignature, int32, NewCoreCount, int32, OldCoreCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMaxSoulCoresChangedSignature, int32, NewMaxCores, int32, OldMaxCores);
