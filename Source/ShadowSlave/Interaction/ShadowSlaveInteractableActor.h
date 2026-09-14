// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/ShadowSlaveInteractableInterface.h"
#include "Interaction/ShadowSlaveInteractionTypes.h"
#include "Save/ShadowSlaveSaveableInterface.h"
#include "ShadowSlaveInteractableActor.generated.h"

class USceneComponent;
class UStaticMeshComponent;

/**
 * Reusable base actor for generic interactable world objects in Shadow Slave.
 * Implements IShadowSlaveInteractableInterface with configurable availability, prompts, and priorities.
 * Implements IShadowSlaveSaveableInterface for opt-in persistence when PersistentSaveId is specified.
 * Designed purely event-driven without tick overhead.
 */
UCLASS(Abstract)
class SHADOWSLAVE_API AShadowSlaveInteractableActor : public AActor, public IShadowSlaveInteractableInterface, public IShadowSlaveSaveableInterface
{
	GENERATED_BODY()

public:
	AShadowSlaveInteractableActor();

	/* --- IShadowSlaveInteractableInterface Implementation --- */

	virtual bool CanInteract_Implementation(AActor* Interactor) override;
	virtual FText GetInteractionPrompt_Implementation(AActor* Interactor) override;
	virtual FShadowSlaveInteractionResult Interact_Implementation(AActor* Interactor) override;
	virtual int32 GetInteractionPriority_Implementation(AActor* Interactor) override;

	/* --- IShadowSlaveSaveableInterface Implementation --- */

	virtual FName GetPersistentSaveId_Implementation() const override;
	virtual bool CaptureSaveRecord_Implementation(FShadowSlaveWorldActorSaveRecord& OutRecord) override;
	virtual bool RestoreSaveRecord_Implementation(const FShadowSlaveWorldActorSaveRecord& InRecord) override;

	/* --- Interaction Configuration --- */

	/** Enables or disables interaction on this actor */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Interaction")
	virtual void SetInteractionEnabled(bool bEnabled);

	/** Returns whether interaction is currently enabled */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Interaction")
	bool IsInteractionEnabled() const { return bIsInteractionEnabled; }

	/** Updates the interaction prompt text */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Interaction")
	void SetInteractionPrompt(const FText& NewPrompt);

	/* --- Components --- */

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Interaction", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> SceneRootComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Interaction", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> StaticMeshComponent;

	/* --- Event Delegates --- */

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Interaction|Events")
	FOnInteractedSignature OnInteracted;

protected:
	/** Core extensibility hook for derived classes to implement custom interaction behavior */
	virtual FShadowSlaveInteractionResult ExecuteInteraction(AActor* Interactor);

	/** Whether this actor can currently be interacted with */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Interaction|Config")
	bool bIsInteractionEnabled = true;

	/** User-facing prompt text displayed when targeting this actor */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Interaction|Config")
	FText InteractionPrompt;

	/** Selection priority used to resolve ties when multiple interactables are in range */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Interaction|Config")
	int32 InteractionPriority = 0;

	/** Unique identifier for this interaction */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Interaction|Config")
	FName InteractionId = NAME_None;

	/** Optional unique persistence identifier for saving/loading this actor. If NAME_None, persistence is disabled. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Save")
	FName PersistentSaveId = NAME_None;
};
