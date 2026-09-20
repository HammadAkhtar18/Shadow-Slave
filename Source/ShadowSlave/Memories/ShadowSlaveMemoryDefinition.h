// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Content/ShadowSlaveContentDefinition.h"
#include "Memories/ShadowSlaveMemoryTypes.h"
#include "Items/ShadowSlaveItemTypes.h"
#include "Attributes/ShadowSlaveAttributeTypes.h"
#include "ShadowSlaveMemoryDefinition.generated.h"

class UTexture2D;
class UStaticMesh;
class UShadowSlaveItemDefinition;

/**
 * Data-driven primary data asset describing immutable static Memory archetypes.
 * Derived from UShadowSlaveContentDefinition as part of the generic static content pipeline.
 *
 * ARCHITECTURAL PRINCIPLES:
 * 1. Specialized Content Definition: Inherits common metadata (ContentId, DisplayName, Description,
 *    Version, MetadataTags, ProvenanceNote) from UShadowSlaveContentDefinition.
 * 2. Single Authoritative ID: ContentId is the single authoritative identifier. MemoryId is preserved
 *    as a compatibility view/accessor and direct reference alias that cannot diverge from ContentId.
 * 3. Content Type: Statically identified as EShadowSlaveContentType::Memory.
 * 4. Memory-Specific Data: Owns verified canon classifications (Rank, Tier), static enchantments,
 *    and usage parameters.
 * 5. Runtime Separation: Cleanly separates immutable Memory definitions from mutable owned runtime
 *    instances (FShadowSlaveMemoryInstance managed by UShadowSlaveMemoryComponent).
 * 6. Static Infrastructure: Participates in UShadowSlaveContentRegistrySubsystem without replacing
 *    or coupling to UShadowSlaveMemoryComponent.
 */
UCLASS(BlueprintType)
class SHADOWSLAVE_API UShadowSlaveMemoryDefinition : public UShadowSlaveContentDefinition
{
	GENERATED_BODY()

public:
	UShadowSlaveMemoryDefinition();

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	/**
	 * Validates definition configuration.
	 * Performs generic content validation (ID, Version, ContentType) and Memory-specific validation.
	 */
	virtual bool IsValidDefinition(FString* OutErrorMessage = nullptr) const override;

	/* --- Identity Compatibility --- */

	/**
	 * Returns the single authoritative stable content identifier (ContentId).
	 * Provided for backwards compatibility with the existing Memory API.
	 */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Memory|Identity")
	FName GetMemoryId() const { return ContentId; }

	/**
	 * Sets the single authoritative stable content identifier (ContentId).
	 * Provided for backwards compatibility with the existing Memory API.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Memory|Identity")
	void SetMemoryId(FName InMemoryId) { ContentId = InMemoryId; }

	/**
	 * Direct reference compatibility alias to the single authoritative ContentId.
	 * Ensures existing C++ call sites accessing MemoryId directly continue to compile
	 * and are physically guaranteed never to diverge from ContentId.
	 */
	FName& MemoryId = ContentId;

	/* --- Canon & Technical Classification --- */

	/** Canon Memory Rank (soul rank of the Memory) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Memory|Classification")
	EShadowSlaveMemoryRank Rank = EShadowSlaveMemoryRank::Unknown;

	/** Canon Memory Tier (soul shard/matrix quality; independent of Rank & Enchantments) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Memory|Classification")
	EShadowSlaveMemoryTier Tier = EShadowSlaveMemoryTier::Unknown;

	/** Broad technical functional classification */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Memory|Classification")
	EShadowSlaveMemoryCategory Category = EShadowSlaveMemoryCategory::Miscellaneous;

	/** Equipment slot category where this Memory binds when equipped */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Memory|Classification")
	EShadowSlaveEquipmentSlot EquipmentSlot = EShadowSlaveEquipmentSlot::None;

	/** Whether equipping this Memory requires an exclusive claim over the designated EquipmentSlot */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Memory|Classification")
	bool bRequiresExclusiveSlot = false;

	/** Attribute modifiers granted while this Memory is manifested/equipped */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Memory|Equipment")
	TArray<FAttributeModifier> GrantedModifiers;

	/** Returns whether this Memory specifies an actual equipment slot (i.e. EquipmentSlot != None) */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Memory|Classification")
	bool HasEquipmentSlot() const { return EquipmentSlot != EShadowSlaveEquipmentSlot::None; }

	/** Safe helper to check if this definition has a verified/known Rank */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Memory|Classification")
	bool HasKnownRank() const { return Rank != EShadowSlaveMemoryRank::Unknown; }

	/** Safe helper to check if this definition has a verified/known Tier */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Memory|Classification")
	bool HasKnownTier() const { return Tier != EShadowSlaveMemoryTier::Unknown; }

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
