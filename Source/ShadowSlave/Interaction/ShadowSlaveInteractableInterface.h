// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Interaction/ShadowSlaveInteractionTypes.h"
#include "ShadowSlaveInteractableInterface.generated.h"

UINTERFACE(MinimalAPI, BlueprintType)
class UShadowSlaveInteractableInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Interface for any world object, actor, pickup, container, or NPC capable of interaction.
 * Polymorphic contract decoupling interactor (player/AI) from concrete object implementations.
 */
class SHADOWSLAVE_API IShadowSlaveInteractableInterface
{
	GENERATED_BODY()

public:
	/** Returns whether the given interactor is currently permitted to interact with this object */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ShadowSlave|Interaction")
	bool CanInteract(AActor* Interactor);

	/** Returns user-facing prompt text displayed when targeting this object (e.g. "Open", "Talk", "Pick Up") */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ShadowSlave|Interaction")
	FText GetInteractionPrompt(AActor* Interactor);

	/** Executes the interaction on behalf of the specified interactor. Returns outcome payload */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ShadowSlave|Interaction")
	FShadowSlaveInteractionResult Interact(AActor* Interactor);

	/** Returns candidate selection priority used to resolve ties when multiple interactables overlap (higher = preferred) */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ShadowSlave|Interaction")
	int32 GetInteractionPriority(AActor* Interactor);
};
