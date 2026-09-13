// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Items/ShadowSlaveItemTypes.h"
#include "ShadowSlaveMemoryTypes.generated.h"

class UShadowSlaveMemoryDefinition;

/**
 * Canon Memory Ranks verified from the Shadow Slave novel.
 * Represents the soul rank of the Memory.
 * NOTE: Independent of Memory Tier.
 */
UENUM(BlueprintType)
enum class EShadowSlaveMemoryRank : uint8
{
	Dormant      UMETA(DisplayName = "Dormant"),
	Awakened     UMETA(DisplayName = "Awakened"),
	Ascended     UMETA(DisplayName = "Ascended"),
	Transcendent UMETA(DisplayName = "Transcendent"),
	Supreme      UMETA(DisplayName = "Supreme"),
	Sacred       UMETA(DisplayName = "Sacred"),
	Divine       UMETA(DisplayName = "Divine")
};

/**
 * Canon Memory Tiers (Tier 1 through Tier 7).
 * Associated with the quality/structure of the soul shards/matrix of the Memory.
 * NOTE: Independent of Memory Rank and NOT equal to enchantment count.
 */
UENUM(BlueprintType)
enum class EShadowSlaveMemoryTier : uint8
{
	Tier1 UMETA(DisplayName = "Tier I"),
	Tier2 UMETA(DisplayName = "Tier II"),
	Tier3 UMETA(DisplayName = "Tier III"),
	Tier4 UMETA(DisplayName = "Tier IV"),
	Tier5 UMETA(DisplayName = "Tier V"),
	Tier6 UMETA(DisplayName = "Tier VI"),
	Tier7 UMETA(DisplayName = "Tier VII")
};

/**
 * Technical classification categories for Memories.
 * Generic prototype taxonomy for sorting, filtering, and functional differentiation.
 * Does NOT represent or assert unverified canon lore.
 */
UENUM(BlueprintType)
enum class EShadowSlaveMemoryCategory : uint8
{
	Miscellaneous UMETA(DisplayName = "Miscellaneous"),
	Weapon        UMETA(DisplayName = "Weapon"),
	Armor         UMETA(DisplayName = "Armor"),
	Charm         UMETA(DisplayName = "Charm"),
	Relic         UMETA(DisplayName = "Relic"),
	Tool          UMETA(DisplayName = "Tool"),
	Utility       UMETA(DisplayName = "Utility")
};

/**
 * Technical gameplay-programming classification for how a Memory's effect is triggered.
 * NOTE: This is a software implementation abstraction and does NOT represent an authoritative canon novel taxonomy.
 */
UENUM(BlueprintType)
enum class EShadowSlaveMemoryActivationType : uint8
{
	Passive   UMETA(DisplayName = "Passive"),
	Triggered UMETA(DisplayName = "Triggered"),
	Active    UMETA(DisplayName = "Active"),
	Toggle    UMETA(DisplayName = "Toggle / Sustained")
};

/**
 * Generic runtime state of a Memory instance.
 */
UENUM(BlueprintType)
enum class EShadowSlaveMemoryState : uint8
{
	Dormant  UMETA(DisplayName = "Dormant"),  // Stored in soul/storage, not manifested
	Summoned UMETA(DisplayName = "Summoned"), // Manifested / equipped physically
	Active   UMETA(DisplayName = "Active"),   // Ability or effect actively executing
	Cooldown UMETA(DisplayName = "Cooldown")  // Temporarily recovering or depleted
};

/**
 * Represents static archetype enchantment data bound to a Memory Definition.
 * A Memory can contain 0 to N enchantments without an artificial maximum.
 * NOTE: Enchantment count is NOT bound 1:1 to Memory Tier.
 */
USTRUCT(BlueprintType)
struct SHADOWSLAVE_API FShadowSlaveMemoryEnchantment
{
	GENERATED_BODY()

	/** Unique internal identifier for this enchantment */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Memory|Enchantment")
	FName EnchantmentId = NAME_None;

	/** Display name shown to players in UI/logs */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Memory|Enchantment")
	FText DisplayName;

	/** Narrative or mechanical description of the enchantment */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Memory|Enchantment", meta = (MultiLine = true))
	FText Description;

	/** Optional technical effect identifier for future gameplay execution */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Memory|Enchantment")
	FName EffectIdentifier = NAME_None;

	/** Optional resource/essence requirement when invoked or sustained */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Memory|Enchantment", meta = (ClampMin = "0.0"))
	float EssenceCost = 0.0f;

	/** Optional metadata key-value pairs for future verified systems */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Memory|Enchantment")
	TMap<FName, FString> Metadata;

	FShadowSlaveMemoryEnchantment() = default;

	FShadowSlaveMemoryEnchantment(FName InId, const FText& InName, const FText& InDesc, float InCost = 0.0f)
		: EnchantmentId(InId), DisplayName(InName), Description(InDesc), EssenceCost(InCost)
	{
	}
};

/**
 * Optional consumption/sacrifice data for Memories capable of being consumed.
 * NOTE: No generic 'random +5 attribute' rule is assumed.
 * Verified effects will be populated on specific Memories when implemented.
 */
USTRUCT(BlueprintType)
struct SHADOWSLAVE_API FShadowSlaveMemoryConsumptionEffect
{
	GENERATED_BODY()

	/** Whether this Memory can be consumed/sacrificed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Memory|Consumption")
	bool bCanBeConsumed = false;

	/** Target attribute identifier (if verified to grant an attribute upon consumption) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Memory|Consumption", meta = (EditCondition = "bCanBeConsumed"))
	FName TargetAttributeName = NAME_None;

	/** Attribute modifier or magnitude granted upon consumption */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Memory|Consumption", meta = (EditCondition = "bCanBeConsumed"))
	float AttributeModifierValue = 0.0f;

	/** Narrative or UI description of the consumption effect */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Memory|Consumption", meta = (MultiLine = true, EditCondition = "bCanBeConsumed"))
	FText ConsumptionDescription;

	/** Optional metadata for future verified consumption hooks */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Memory|Consumption")
	TMap<FName, FString> ConsumptionMetadata;

	FShadowSlaveMemoryConsumptionEffect() = default;
};

/**
 * Represents an individual runtime Memory instance owned by an actor or soul storage.
 * Cleanly separates immutable Memory archetype data (UShadowSlaveMemoryDefinition)
 * from mutable, individual instance state (InstanceId, equipped state, runtime state, dynamic properties).
 * Static data (Rank, Tier, Name, Description, Enchantments, etc.) is NOT duplicated here.
 */
USTRUCT(BlueprintType)
struct SHADOWSLAVE_API FShadowSlaveMemoryInstance
{
	GENERATED_BODY()

	/** Unique identifier distinguishing this specific owned Memory instance */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Memories")
	FGuid InstanceId = FGuid::NewGuid();

	/** Static archetype data definition describing this Memory */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Memories")
	TObjectPtr<UShadowSlaveMemoryDefinition> MemoryDefinition = nullptr;

	/** Current runtime operational state of this Memory */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Memories")
	EShadowSlaveMemoryState State = EShadowSlaveMemoryState::Dormant;

	/** Whether this Memory is currently manifested or equipped on the owner */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Memories")
	bool bIsEquipped = false;

	/** Optional instance-specific dynamic properties for save-game and future state support */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Memories")
	TMap<FName, FString> DynamicProperties;

	FShadowSlaveMemoryInstance() = default;

	FShadowSlaveMemoryInstance(UShadowSlaveMemoryDefinition* InDef)
		: InstanceId(FGuid::NewGuid()), MemoryDefinition(InDef), State(EShadowSlaveMemoryState::Dormant), bIsEquipped(false)
	{
	}

	/** Returns true if this instance has a valid definition asset and valid GUID */
	bool IsValid() const
	{
		return MemoryDefinition != nullptr && InstanceId.IsValid();
	}

	/** Returns true if this Memory is currently manifested or equipped */
	bool IsEquipped() const
	{
		return bIsEquipped;
	}

	/** Safe helper to access the underlying definition's Rank */
	EShadowSlaveMemoryRank GetRank() const;

	/** Safe helper to access the underlying definition's Tier */
	EShadowSlaveMemoryTier GetTier() const;
};

/* --- Event Delegates --- */

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMemoryAddedSignature, const FShadowSlaveMemoryInstance&, MemoryInstance);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMemoryRemovedSignature, const FShadowSlaveMemoryInstance&, MemoryInstance);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMemoryDestroyedSignature, const FShadowSlaveMemoryInstance&, MemoryInstance);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMemoryEquippedSignature, const FShadowSlaveMemoryInstance&, MemoryInstance);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMemoryUnequippedSignature, const FShadowSlaveMemoryInstance&, MemoryInstance);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnMemoryStateChangedSignature, const FShadowSlaveMemoryInstance&, MemoryInstance, EShadowSlaveMemoryState, NewState, EShadowSlaveMemoryState, OldState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMemoryCollectionChangedSignature);
