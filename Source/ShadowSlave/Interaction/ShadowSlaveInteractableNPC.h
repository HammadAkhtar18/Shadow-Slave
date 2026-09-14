// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Characters/ShadowSlaveCharacterBase.h"
#include "Interaction/ShadowSlaveInteractableInterface.h"
#include "Interaction/ShadowSlaveInteractionTypes.h"
#include "ShadowSlaveInteractableNPC.generated.h"

class AShadowSlaveInteractableNPC;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnNPCInteractedSignature, AActor*, Interactor, AShadowSlaveInteractableNPC*, NPC);

/**
 * Reusable base character for interactable NPCs (allies, merchants, story figures).
 * Implements IShadowSlaveInteractableInterface on top of AShadowSlaveCharacterBase.
 * Serves as an interaction anchor without hardcoding dialogue trees or quest scripts.
 */
UCLASS()
class SHADOWSLAVE_API AShadowSlaveInteractableNPC : public AShadowSlaveCharacterBase, public IShadowSlaveInteractableInterface
{
	GENERATED_BODY()

public:
	AShadowSlaveInteractableNPC(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/* --- IShadowSlaveInteractableInterface Implementation --- */

	virtual bool CanInteract_Implementation(AActor* Interactor) override;
	virtual FText GetInteractionPrompt_Implementation(AActor* Interactor) override;
	virtual FShadowSlaveInteractionResult Interact_Implementation(AActor* Interactor) override;
	virtual int32 GetInteractionPriority_Implementation(AActor* Interactor) override;

	/* --- NPC Configuration --- */

	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Interaction|NPC")
	void SetInteractionEnabled(bool bEnabled) { bIsInteractionEnabled = bEnabled; }

	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Interaction|NPC")
	bool IsInteractionEnabled() const { return bIsInteractionEnabled; }

	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Interaction|NPC")
	FName GetNPCId() const { return NPCId; }

	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Interaction|NPC")
	FText GetNPCDisplayName() const { return NPCDisplayName; }

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Interaction|NPC")
	FOnNPCInteractedSignature OnNPCInteracted;

protected:
	/** Hook for Blueprint/Native logic to handle NPC interaction (e.g. turn towards player, trigger event) */
	UFUNCTION(BlueprintNativeEvent, Category = "ShadowSlave|Interaction|NPC")
	void OnInteractedBy(AActor* Interactor);
	virtual void OnInteractedBy_Implementation(AActor* Interactor);

	/** Whether this NPC can currently be interacted with */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Interaction|NPC")
	bool bIsInteractionEnabled = true;

	/** Unique identifier for this NPC */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Interaction|NPC")
	FName NPCId = NAME_None;

	/** Display name of the NPC */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Interaction|NPC")
	FText NPCDisplayName;

	/** Prompt verb displayed when targeting this NPC (e.g. "Talk", "Speak") */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Interaction|NPC")
	FText InteractionVerb;

	/** Priority used to resolve ties when multiple interactables are in range */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Interaction|NPC")
	int32 InteractionPriority = 1;
};
