// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Items/ShadowSlaveItemTypes.h"
#include "ShadowSlaveMemoryTypes.generated.h"

class UShadowSlaveMemoryDefinition;

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
 * Technical classification for how a Memory's effect or manifestation is triggered.
 */
UENUM(BlueprintType)
enum class EShadowSlaveMemoryActivationType : uint8
{
	Passive UMETA(DisplayName = "Passive"),
	Active  UMETA(DisplayName = "Active / Triggered"),
	Toggle  UMETA(DisplayName = "Toggle / Sustained")
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
 * Represents an individual runtime Memory instance owned by an actor or soul storage.
 * Cleanly separates immutable Memory archetype data (UShadowSlaveMemoryDefinition)
 * from mutable, individual instance state (InstanceId, equipped state, runtime state, dynamic properties).
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
};

/* --- Event Delegates --- */

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMemoryAddedSignature, const FShadowSlaveMemoryInstance&, MemoryInstance);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMemoryRemovedSignature, const FShadowSlaveMemoryInstance&, MemoryInstance);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMemoryEquippedSignature, const FShadowSlaveMemoryInstance&, MemoryInstance);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMemoryUnequippedSignature, const FShadowSlaveMemoryInstance&, MemoryInstance);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnMemoryStateChangedSignature, const FShadowSlaveMemoryInstance&, MemoryInstance, EShadowSlaveMemoryState, NewState, EShadowSlaveMemoryState, OldState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMemoryCollectionChangedSignature);
