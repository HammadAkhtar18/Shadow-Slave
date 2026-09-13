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
 * Cleanly separates immutable Memory definitions from mutable owned runtime instances.
 */
UCLASS(BlueprintType)
class SHADOWSLAVE_API UShadowSlaveMemoryDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UShadowSlaveMemoryDefinition();

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	/* --- Identity & Categorization --- */

	/** Technical internal identifier for this Memory */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Memory|Identity")
	FName MemoryId = NAME_None;

	/** Display name shown to players in UI/logs */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Memory|Identity")
	FText DisplayName;

	/** Narrative or description text */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Memory|Identity", meta = (MultiLine = true))
	FText Description;

	/** Broad technical functional classification */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Memory|Identity")
	EShadowSlaveMemoryCategory Category = EShadowSlaveMemoryCategory::Miscellaneous;

	/* --- Equipment & Activation --- */

	/** Whether this Memory can be manifested and equipped onto the owner */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Memory|Equipment")
	bool bCanBeEquipped = false;

	/** Equipment slot category where this Memory binds when equipped */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Memory|Equipment", meta = (EditCondition = "bCanBeEquipped"))
	EShadowSlaveEquipmentSlot EquipmentSlot = EShadowSlaveEquipmentSlot::None;

	/** Whether this Memory possesses an actively invokable or sustained effect */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Memory|Activation")
	bool bCanBeActivated = false;

	/** Trigger pattern for this Memory's effect */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Memory|Activation", meta = (EditCondition = "bCanBeActivated"))
	EShadowSlaveMemoryActivationType ActivationType = EShadowSlaveMemoryActivationType::Passive;

	/** Optional base essence cost consumed upon invocation or manifestation */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Memory|Activation", meta = (ClampMin = "0.0", EditCondition = "bCanBeActivated"))
	float BaseEssenceCost = 0.0f;

	/* --- Visual Representation --- */

	/** Icon for UI rendering */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Memory|Visuals")
	TSoftObjectPtr<UTexture2D> Icon;

	/** 3D mesh for physical manifestation or inspection */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Memory|Visuals")
	TSoftObjectPtr<UStaticMesh> WorldMesh;

	/* --- Inventory & Ecosystem Integration --- */

	/** Optional associated physical item definition if mirrored in the inventory ecosystem */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Memory|Integration")
	TSoftObjectPtr<UShadowSlaveItemDefinition> AssociatedItemDefinition;

	/** Extensible metadata key-value pairs for future verified systems */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Memory|Metadata")
	TMap<FName, FString> Metadata;

	/* --- Development Test Factory --- */

	/** Creates a generic development test memory definition for verifying lifecycle operations */
	static UShadowSlaveMemoryDefinition* CreateTestMemoryDefinition(UObject* Outer = nullptr);
};
