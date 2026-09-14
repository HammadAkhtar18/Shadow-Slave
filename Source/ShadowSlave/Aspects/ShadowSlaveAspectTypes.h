// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Progression/ShadowSlaveProgressionTypes.h"
#include "ShadowSlaveAspectTypes.generated.h"

class UShadowSlaveAspectDefinition;
class UShadowSlaveAspectAbilityDefinition;
class UShadowSlaveFlawDefinition;

/**
 * Canon and technical Aspect Rank representing the rarity/potency tier of an Aspect.
 * NOTE: Independent of Character Rank (e.g. Sunny can be a Dormant sleeper with a Divine Aspect).
 * Separate enum from EShadowSlaveCharacterRank.
 */
UENUM(BlueprintType)
enum class EShadowSlaveAspectRank : uint8
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
 * Runtime state representing an individual Aspect Ability on a character.
 * Separates immutable archetype data (UShadowSlaveAspectAbilityDefinition)
 * from mutable runtime state (bIsUnlocked, bIsActive, dynamic properties).
 */
USTRUCT(BlueprintType)
struct SHADOWSLAVE_API FShadowSlaveAspectAbilityInstance
{
	GENERATED_BODY()

	/** Pointer to immutable static definition */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Aspects")
	TObjectPtr<UShadowSlaveAspectAbilityDefinition> AbilityDefinition = nullptr;

	/** Whether this ability has been unlocked/awakened for this character */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Aspects")
	bool bIsUnlocked = false;

	/** Whether this ability is currently active or sustained */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Aspects")
	bool bIsActive = false;

	/** Dynamic instance properties for state tracking without modifying static definition */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Aspects")
	TMap<FName, FString> DynamicProperties;

	FShadowSlaveAspectAbilityInstance() = default;

	FShadowSlaveAspectAbilityInstance(UShadowSlaveAspectAbilityDefinition* InDef, bool bInUnlocked = false)
		: AbilityDefinition(InDef), bIsUnlocked(bInUnlocked), bIsActive(false)
	{
	}

	bool IsValid() const
	{
		return AbilityDefinition != nullptr;
	}

	FName GetAbilityId() const;
};

/* --- Aspect Event Delegates --- */

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAspectChangedSignature, UShadowSlaveAspectDefinition*, NewAspectDef, UShadowSlaveAspectDefinition*, OldAspectDef);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAbilityUnlockedSignature, FName, AbilityId, UShadowSlaveAspectAbilityDefinition*, AbilityDef);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnFlawChangedSignature, UShadowSlaveFlawDefinition*, NewFlawDef, UShadowSlaveFlawDefinition*, OldFlawDef);
