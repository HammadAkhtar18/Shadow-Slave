// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ShadowSlaveEchoTypes.generated.h"

class UShadowSlaveEchoDefinition;

/**
 * Canon Echo Rank representing the power rank of the Echo.
 * Matches standard Nightmare Spell entity ranks.
 */
UENUM(BlueprintType)
enum class EShadowSlaveEchoRank : uint8
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
 * Classification metadata representing the entity tier / source archetype of the Echo
 * (e.g. Beast through Titan, corresponding to soul core count in source creatures).
 * NOTE: This is classification metadata only; it does not imply that the Echo runtime itself
 * is an active Nightmare Creature or that this enum is an exhaustive representation of all canon.
 */
UENUM(BlueprintType)
enum class EShadowSlaveEchoClass : uint8
{
	Unknown UMETA(DisplayName = "Unknown"),
	Beast   UMETA(DisplayName = "Beast"),   // 1 Core source archetype
	Monster UMETA(DisplayName = "Monster"), // 2 Cores source archetype
	Demon   UMETA(DisplayName = "Demon"),   // 3 Cores source archetype
	Devil   UMETA(DisplayName = "Devil"),   // 4 Cores source archetype
	Tyrant  UMETA(DisplayName = "Tyrant"),  // 5 Cores source archetype
	Terror  UMETA(DisplayName = "Terror"),  // 6 Cores source archetype
	Titan   UMETA(DisplayName = "Titan")    // 7 Cores source archetype
};

/**
 * Technical operational state of an Echo instance.
 */
UENUM(BlueprintType)
enum class EShadowSlaveEchoState : uint8
{
	Dormant   UMETA(DisplayName = "Dormant"),   // Owned, held in soul/collection, not manifested
	Summoned  UMETA(DisplayName = "Summoned"),  // Transient active manifestation state
	Destroyed UMETA(DisplayName = "Destroyed")  // Durable terminal state: destroyed/consumed, cannot be summoned
};

/**
 * Runtime instance of an individual Echo owned by an actor.
 * Possesses a unique GUID and tracks mutable runtime state (summoned status, dynamic properties).
 * Static data (Rank, Class, Name, Description, etc.) is NOT duplicated here.
 */
USTRUCT(BlueprintType)
struct SHADOWSLAVE_API FShadowSlaveEchoInstance
{
	GENERATED_BODY()

	/** Unique identifier distinguishing this specific owned Echo instance */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Echoes")
	FGuid InstanceId = FGuid::NewGuid();

	/** Static archetype data definition describing this Echo */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Echoes")
	TObjectPtr<UShadowSlaveEchoDefinition> EchoDefinition = nullptr;

	/** Operational runtime state */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Echoes")
	EShadowSlaveEchoState State = EShadowSlaveEchoState::Dormant;

	/**
	 * Transient runtime flag indicating whether this Echo is currently active/summoned.
	 * NOTE: This is strictly transient and is NEVER persisted to save files as world truth.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Echoes")
	bool bIsSummoned = false;

	/** Optional instance-specific dynamic properties for save-game, modifications, and future state support */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Echoes")
	TMap<FName, FString> DynamicProperties;

	FShadowSlaveEchoInstance() = default;

	FShadowSlaveEchoInstance(UShadowSlaveEchoDefinition* InDef)
		: InstanceId(FGuid::NewGuid()), EchoDefinition(InDef), State(EShadowSlaveEchoState::Dormant), bIsSummoned(false)
	{
	}

	/** Returns true if this instance has a valid definition asset and valid GUID */
	bool IsValid() const
	{
		return EchoDefinition != nullptr && InstanceId.IsValid();
	}
};

/* --- Echo Event Delegates --- */

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEchoAddedSignature, const FShadowSlaveEchoInstance&, Echo);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEchoRemovedSignature, const FShadowSlaveEchoInstance&, Echo);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEchoDestroyedSignature, const FShadowSlaveEchoInstance&, Echo);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEchoSummonedSignature, const FShadowSlaveEchoInstance&, Echo);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEchoDismissedSignature, const FShadowSlaveEchoInstance&, Echo);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEchoModifiedSignature, const FShadowSlaveEchoInstance&, Echo);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEchoStateChangedSignature, const FShadowSlaveEchoInstance&, Echo, EShadowSlaveEchoState, OldState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEchoCollectionChangedSignature);
