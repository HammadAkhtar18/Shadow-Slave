// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Interaction/ShadowSlavePickupActor.h"
#include "Items/ShadowSlaveItemDefinition.h"
#include "ShadowSlaveItemPickup.generated.h"

/**
 * Generic world pickup actor for inventory items.
 * Communicates directly with interactor's UShadowSlaveInventoryComponent to transfer items.
 * Does NOT own duplicate inventory state; delegates storage authoritatively to InventoryComponent.
 * If inventory is full or acquisition fails, the pickup safely remains in the world.
 */
UCLASS()
class SHADOWSLAVE_API AShadowSlaveItemPickup : public AShadowSlavePickupActor
{
	GENERATED_BODY()

public:
	AShadowSlaveItemPickup();

	virtual bool CanInteract_Implementation(AActor* Interactor) override;
	virtual FText GetInteractionPrompt_Implementation(AActor* Interactor) override;

	/** Sets item definition and quantity to configure this pickup */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Interaction|ItemPickup")
	void InitializeItemPickup(UShadowSlaveItemDefinition* InItemDef, int32 InQuantity = 1);

	/** Returns the referenced item definition */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Interaction|ItemPickup")
	UShadowSlaveItemDefinition* GetItemDefinition() const { return ItemDefinition; }

	/** Returns current item quantity in this pickup */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Interaction|ItemPickup")
	int32 GetQuantity() const { return Quantity; }

protected:
	virtual FShadowSlaveInteractionResult ExecuteInteraction(AActor* Interactor) override;

	/* --- IShadowSlaveSaveableInterface Overrides --- */
	virtual bool CaptureSaveRecord_Implementation(FShadowSlaveWorldActorSaveRecord& OutRecord) override;
	virtual bool RestoreSaveRecord_Implementation(const FShadowSlaveWorldActorSaveRecord& InRecord) override;

	/** Data-driven item archetype referenced by this pickup */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Interaction|ItemPickup")
	TObjectPtr<UShadowSlaveItemDefinition> ItemDefinition = nullptr;

	/** Number of items bundled in this pickup stack */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Interaction|ItemPickup", meta = (ClampMin = "1"))
	int32 Quantity = 1;
};
