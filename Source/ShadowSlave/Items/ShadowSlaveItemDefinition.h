// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Content/ShadowSlaveContentDefinition.h"
#include "Items/ShadowSlaveItemTypes.h"
#include "Attributes/ShadowSlaveAttributeTypes.h"
#include "ShadowSlaveItemDefinition.generated.h"

class UTexture2D;
class UStaticMesh;

/**
 * Data-driven content definition describing immutable item definitions.
 * Specialization of UShadowSlaveContentDefinition for the generic content pipeline.
 *
 * ARCHITECTURAL PRINCIPLES:
 * 1. SPECIALIZED CONTENT DEFINITION: ItemDefinition is a specialized static content definition
 *    deriving from UShadowSlaveContentDefinition.
 * 2. GENERIC CONTENT CLASSIFICATION: Generic ContentType identifies this asset as
 *    EShadowSlaveContentType::Item across the content pipeline and registry.
 * 3. TAXONOMY SEPARATION: Any Item-specific taxonomy (EShadowSlaveItemType, EShadowSlaveEquipmentSlot)
 *    remains completely separate from the generic content pipeline type.
 * 4. SINGLE AUTHORITATIVE ID: ContentId is the sole stored identifier. GetItemId() and SetItemId()
 *    provide backward-compatible accessors without duplicate storage or reference-member aliases.
 * 5. RUNTIME STATE SEPARATION: UShadowSlaveInventoryComponent remains authoritative for runtime item
 *    ownership and state (quantities, slot layout, instance GUIDs).
 * 6. EQUIPMENT AUTHORITY: UShadowSlaveEquipmentComponent remains authoritative for equipped item
 *    state and granted modifier application.
 * 7. CONTENT REGISTRY ROLE: UShadowSlaveContentRegistrySubsystem is a static asset lookup and discovery
 *    service; it does NOT replace or manage runtime inventory/equipment state.
 */
UCLASS(BlueprintType)
class SHADOWSLAVE_API UShadowSlaveItemDefinition : public UShadowSlaveContentDefinition
{
	GENERATED_BODY()

public:
	UShadowSlaveItemDefinition();

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	/**
	 * Validates definition configuration.
	 * Combines base generic content validation (valid ContentId, valid DisplayName, Version >= 1, ContentType == Item)
	 * with Item-specific rules (stacking rules, non-negative weight and value).
	 */
	virtual bool IsValidDefinition(FString* OutErrorMessage = nullptr) const override;

	/* --- Identification Compatibility Accessors --- */

	/** Compatibility accessor returning the authoritative ContentId as ItemId */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Item|Identity")
	FName GetItemId() const { return ContentId; }

	/** Compatibility accessor setting the authoritative ContentId as ItemId */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Item|Identity")
	void SetItemId(FName InItemId) { ContentId = InItemId; }

	/* --- Identification & Categorization --- */

	/** Broad classification of item (independent of generic EShadowSlaveContentType) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Item|Identity")
	EShadowSlaveItemType ItemType = EShadowSlaveItemType::Miscellaneous;

	/** Prepared equipment slot for future gear or Memories */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Item|Equipment")
	EShadowSlaveEquipmentSlot EquipmentSlot = EShadowSlaveEquipmentSlot::None;

	/** Attribute modifiers granted while this item is equipped */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Item|Equipment")
	TArray<FAttributeModifier> GrantedModifiers;

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
