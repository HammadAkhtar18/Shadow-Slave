// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Items/ShadowSlaveItemTypes.h"
#include "ShadowSlaveEquipmentTypes.generated.h"

/**
 * Origin source classification for an equipped object.
 */
UENUM(BlueprintType)
enum class EShadowSlaveEquipmentSourceType : uint8
{
	None   UMETA(DisplayName = "None"),
	Item   UMETA(DisplayName = "Inventory Item"),
	Memory UMETA(DisplayName = "Memory")
};

/**
 * Lightweight runtime descriptor representing an equipped slot binding.
 * References an authoritative instance (Inventory Item or Memory) by its unique GUID.
 * Does not duplicate or create independent copies of item/memory state.
 */
USTRUCT(BlueprintType)
struct SHADOWSLAVE_API FShadowSlaveEquippedItem
{
	GENERATED_BODY()

	/** The slot this item occupies */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Equipment")
	EShadowSlaveEquipmentSlot Slot = EShadowSlaveEquipmentSlot::None;

	/** Source origin: Inventory Item or Memory */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Equipment")
	EShadowSlaveEquipmentSourceType SourceType = EShadowSlaveEquipmentSourceType::None;

	/** Unique stable instance GUID of the equipped item or Memory */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Equipment")
	FGuid InstanceId = FGuid();

	/** Definition ID or asset name for debugging / save reference */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Equipment")
	FName DefinitionId = NAME_None;

	FShadowSlaveEquippedItem() = default;

	FShadowSlaveEquippedItem(EShadowSlaveEquipmentSlot InSlot, EShadowSlaveEquipmentSourceType InSourceType, const FGuid& InInstanceId, FName InDefId = NAME_None)
		: Slot(InSlot), SourceType(InSourceType), InstanceId(InInstanceId), DefinitionId(InDefId)
	{
	}

	bool IsValid() const
	{
		return Slot != EShadowSlaveEquipmentSlot::None &&
		       SourceType != EShadowSlaveEquipmentSourceType::None &&
		       InstanceId.IsValid();
	}

	void Reset()
	{
		Slot = EShadowSlaveEquipmentSlot::None;
		SourceType = EShadowSlaveEquipmentSourceType::None;
		InstanceId.Invalidate();
		DefinitionId = NAME_None;
	}
};

/* --- Equipment Event Delegates --- */

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEquipmentItemEquippedSignature, EShadowSlaveEquipmentSlot, Slot, const FShadowSlaveEquippedItem&, EquippedItem);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEquipmentItemUnequippedSignature, EShadowSlaveEquipmentSlot, Slot, const FShadowSlaveEquippedItem&, UnequippedItem);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEquipmentSlotChangedSignature, EShadowSlaveEquipmentSlot, Slot);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEquipmentChangedSignature);
