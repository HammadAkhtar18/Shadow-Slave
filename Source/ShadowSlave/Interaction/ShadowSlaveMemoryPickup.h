// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Interaction/ShadowSlavePickupActor.h"
#include "Memories/ShadowSlaveMemoryDefinition.h"
#include "ShadowSlaveMemoryPickup.generated.h"

/**
 * Generic world pickup actor for Memories.
 * Communicates directly with interactor's UShadowSlaveMemoryComponent to acquire Memories.
 * Does NOT own duplicate Memory instances; delegates creation and storage authoritatively to MemoryComponent.
 * If acquisition fails or interactor lacks a Memory Component, the pickup remains safely in the world.
 */
UCLASS()
class SHADOWSLAVE_API AShadowSlaveMemoryPickup : public AShadowSlavePickupActor
{
	GENERATED_BODY()

public:
	AShadowSlaveMemoryPickup();

	virtual bool CanInteract_Implementation(AActor* Interactor) override;
	virtual FText GetInteractionPrompt_Implementation(AActor* Interactor) override;

	/** Configures this pickup with a specific Memory definition */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Interaction|MemoryPickup")
	void InitializeMemoryPickup(UShadowSlaveMemoryDefinition* InMemoryDef);

	/** Returns the referenced Memory definition */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Interaction|MemoryPickup")
	UShadowSlaveMemoryDefinition* GetMemoryDefinition() const { return MemoryDefinition; }

protected:
	virtual FShadowSlaveInteractionResult ExecuteInteraction(AActor* Interactor) override;

	/** Data-driven Memory archetype referenced by this pickup */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Interaction|MemoryPickup")
	TObjectPtr<UShadowSlaveMemoryDefinition> MemoryDefinition = nullptr;
};
