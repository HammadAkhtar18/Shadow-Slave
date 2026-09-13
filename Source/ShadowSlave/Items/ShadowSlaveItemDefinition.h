// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Items/ShadowSlaveItemTypes.h"
#include "ShadowSlaveItemDefinition.generated.h"

class UTexture2D;
class UStaticMesh;

/**
 * Data-driven primary data asset describing immutable item definitions.
 * Integrated with Unreal Engine's PrimaryAssetId system for data-driven discovery.
 */
UCLASS(BlueprintType)
class SHADOWSLAVE_API UShadowSlaveItemDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UShadowSlaveItemDefinition();

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	/* --- Identification & Categorization --- */

	/** Display name shown to players in UI/logs */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Item|Identity")
	FText DisplayName;

	/** Lore or description text */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Item|Identity", meta = (MultiLine = true))
	FText Description;

	/** Broad classification of item */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Item|Identity")
	EShadowSlaveItemType ItemType = EShadowSlaveItemType::Miscellaneous;

	/** Prepared equipment slot for future gear or Memories */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Item|Equipment")
	EShadowSlaveEquipmentSlot EquipmentSlot = EShadowSlaveEquipmentSlot::None;

	/* --- Stacking & Rules --- */

	/** Whether multiple units of this item can share a single inventory slot */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Item|Stacking")
	bool bIsStackable = false;

	/** Maximum quantity allowed per inventory slot */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Item|Stacking", meta = (ClampMin = "1", EditCondition = "bIsStackable"))
	int32 MaxStackSize = 1;

	/* --- Physical & Economic Metrics --- */

	/** Weight per unit in kilograms */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Item|Metrics", meta = (ClampMin = "0.0"))
	float Weight = 0.1f;

	/** Base economy trading value */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Item|Metrics", meta = (ClampMin = "0"))
	int32 BaseValue = 10;

	/* --- Visual Representation --- */

	/** Icon for inventory UI representation */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Item|Visuals")
	TSoftObjectPtr<UTexture2D> Icon;

	/** 3D mesh for world drops or inspection */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Item|Visuals")
	TSoftObjectPtr<UStaticMesh> WorldMesh;

	/* --- Development Test Factories --- */

	/** Creates a generic development test consumable definition (stackable) */
	static UShadowSlaveItemDefinition* CreateTestConsumableDefinition(UObject* Outer = nullptr);

	/** Creates a generic development test quest item definition (non-stackable) */
	static UShadowSlaveItemDefinition* CreateTestQuestItemDefinition(UObject* Outer = nullptr);
};
