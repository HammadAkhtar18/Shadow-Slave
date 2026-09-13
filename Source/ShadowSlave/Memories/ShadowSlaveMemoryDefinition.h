// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Memories/ShadowSlaveMemoryTypes.h"
#include "Items/ShadowSlaveItemTypes.h"
#include "ShadowSlaveMemoryDefinition.generated.h"

class UTexture2D;
class UStaticMesh;
class UShadowSlaveItemDefinition;

/**
 * Data-driven primary data asset describing immutable static Memory archetypes.
 * Registered with Unreal Engine's Asset Manager via PrimaryAssetId.
 * Holds verified canon classifications (Rank, Tier), static enchantments, and usage parameters.
 * Cleanly separates immutable Memory definitions from mutable owned runtime instances.
 */
UCLASS(BlueprintType)
class SHADOWSLAVE_API UShadowSlaveMemoryDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UShadowSlaveMemoryDefinition();

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	/* --- Identity --- */

	/** Technical internal identifier for this Memory */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Memory|Identity")
	FName MemoryId = NAME_None;

	/** Display name shown to players in UI/logs */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Memory|Identity")
	FText DisplayName;

	/** Narrative or description text */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Memory|Identity", meta = (MultiLine = true))
	FText Description;

	/* --- Canon & Technical Classification --- */

	/** Canon Memory Rank (soul rank of the Memory) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Memory|Classification")
	EShadowSlaveMemoryRank Rank = EShadowSlaveMemoryRank::Dormant;

	/** Canon Memory Tier (soul shard/matrix quality; independent of Rank & Enchantments) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Memory|Classification")
	EShadowSlaveMemoryTier Tier = EShadowSlaveMemoryTier::Tier1;

	/** Broad technical functional classification */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Memory|Classification")
	EShadowSlaveMemoryCategory Category = EShadowSlaveMemoryCategory::Miscellaneous;

	/** Equipment slot category where this Memory binds when equipped */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Memory|Classification")
	EShadowSlaveEquipmentSlot EquipmentSlot = EShadowSlaveEquipmentSlot::None;

	/** Whether equipping this Memory requires an exclusive claim over the designated EquipmentSlot */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Memory|Classification")
	bool bRequiresExclusiveSlot = false;

	/* --- Usage & Activation --- */

	/** Whether this Memory can be manifested and equipped onto the owner */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Memory|Usage")
	bool bCanBeEquipped = false;

	/** Whether this Memory possesses an actively invokable or sustained effect */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Memory|Usage")
	bool bCanBeActivated = false;

	/**
	 * Technical trigger pattern for this Memory's overall manifestation or main ability.
	 * NOTE: Software implementation concept; individual enchantments define their own triggers.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Memory|Usage", meta = (EditCondition = "bCanBeActivated"))
	EShadowSlaveMemoryActivationType ActivationType = EShadowSlaveMemoryActivationType::Passive;

	/** Optional technical base essence cost parameter */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Memory|Usage", meta = (ClampMin = "0.0"))
	float BaseEssenceCost = 0.0f;

	/* --- Enchantments --- */

	/** Static enchantment archetypes bound to this Memory (0 to N enchantments, not fixed by Tier) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Memory|Enchantments")
	TArray<FShadowSlaveMemoryEnchantment> Enchantments;

	/** Returns total number of enchantments on this Memory */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Memory|Enchantments")
	int32 GetEnchantmentCount() const { return Enchantments.Num(); }

	/* --- Consumption & Sacrifice --- */

	/** Optional consumption/sacrifice data if this Memory can be consumed */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Memory|Consumption")
	FShadowSlaveMemoryConsumptionEffect ConsumptionEffect;

	/** Returns whether this Memory definition permits consumption */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Memory|Consumption")
	bool CanBeConsumed() const { return ConsumptionEffect.bCanBeConsumed; }

	/* --- Visual Representation --- */

	/** Icon for UI rendering */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Memory|Visuals")
	TSoftObjectPtr<UTexture2D> Icon;

	/** 3D mesh for physical manifestation or inspection */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Memory|Visuals")
	TSoftObjectPtr<UStaticMesh> WorldMesh;

	/* --- Inventory Ecosystem Integration --- */

	/** Optional associated physical item definition if transferred or represented in the inventory ecosystem */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Memory|Integration")
	TSoftObjectPtr<UShadowSlaveItemDefinition> AssociatedItemDefinition;

	/** Extensible metadata key-value pairs for future verified systems */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Memory|Metadata")
	TMap<FName, FString> Metadata;

	/* --- Development Test & Canon Factories --- */

	/** Creates a generic development test memory definition for verifying lifecycle, multi-enchantment, and equipment operations */
	static UShadowSlaveMemoryDefinition* CreateTestMemoryDefinition(UObject* Outer = nullptr);

	/** Creates a canonical verified Memory definition based on verified novel data */
	static UShadowSlaveMemoryDefinition* CreateCanonMemoryDefinition(FName CanonMemoryId, UObject* Outer = nullptr);
};
