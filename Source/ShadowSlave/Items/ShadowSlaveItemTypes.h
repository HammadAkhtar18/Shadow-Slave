// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ShadowSlaveItemTypes.generated.h"

class UShadowSlaveItemDefinition;

/**
 * Generic item classification.
 * Prepared for future Shadow Slave Memories and equipment without hard-coding specific canon lore.
 */
UENUM(BlueprintType)
enum class EShadowSlaveItemType : uint8
{
	Miscellaneous UMETA(DisplayName = "Miscellaneous"),
	Consumable    UMETA(DisplayName = "Consumable"),
	Equipment     UMETA(DisplayName = "Equipment"),
	Memory        UMETA(DisplayName = "Memory"),
	Quest         UMETA(DisplayName = "Quest Item"),
	Material      UMETA(DisplayName = "Material")
};

/**
 * Generic equipment slot classification.
 * Prepared for future equipment and Memory binding without rewriting inventory architecture.
 */
UENUM(BlueprintType)
enum class EShadowSlaveEquipmentSlot : uint8
{
	None   UMETA(DisplayName = "None / Not Equippable"),
	Weapon UMETA(DisplayName = "Weapon"),
	Armor  UMETA(DisplayName = "Armor"),
	Charm  UMETA(DisplayName = "Charm"),
	Ring   UMETA(DisplayName = "Ring"),
	Relic  UMETA(DisplayName = "Relic")
};

/**
 * Represents an individual item instance owned by an inventory or actor.
 * Cleanly separates immutable item definition data (what the item is)
 * from mutable instance-specific runtime data (quantity, unique GUID, dynamic properties).
 */
USTRUCT(BlueprintType)
struct SHADOWSLAVE_API FShadowSlaveItemInstance
{
	GENERATED_BODY()

	/** Unique identifier distinguishing this specific item stack in the world */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Inventory")
	FGuid InstanceId = FGuid::NewGuid();

	/** Static data definition describing this item */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Inventory")
	TObjectPtr<UShadowSlaveItemDefinition> ItemDefinition = nullptr;

	/** Number of items contained in this stack */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Inventory", meta = (ClampMin = "1"))
	int32 Quantity = 1;

	/** Optional instance-specific dynamic properties (e.g. charges, custom state, binding info) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Inventory")
	TMap<FName, FString> DynamicProperties;

	FShadowSlaveItemInstance() = default;

	FShadowSlaveItemInstance(UShadowSlaveItemDefinition* InDef, int32 InQuantity = 1)
		: InstanceId(FGuid::NewGuid()), ItemDefinition(InDef), Quantity(InQuantity)
	{
	}

	/** Returns true if this instance has a valid definition asset and positive quantity */
	bool IsValid() const
	{
		return ItemDefinition != nullptr && Quantity > 0;
	}

	/** Checks whether this item instance can combine into a stack with another */
	bool CanStackWith(const FShadowSlaveItemInstance& Other) const;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInventoryItemAddedSignature, const FShadowSlaveItemInstance&, ItemInstance, int32, QuantityAdded);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInventoryItemRemovedSignature, const FShadowSlaveItemInstance&, ItemInstance, int32, QuantityRemoved);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryChangedSignature);
