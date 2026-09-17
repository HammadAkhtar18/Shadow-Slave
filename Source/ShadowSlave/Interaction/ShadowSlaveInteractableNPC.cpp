// Copyright Epic Games, Inc. All Rights Reserved.

#include "Interaction/ShadowSlaveInteractableNPC.h"
#include "Dialogue/ShadowSlaveDialogueDefinition.h"
#include "Dialogue/ShadowSlaveConversationSubsystem.h"
#include "Engine/GameInstance.h"

AShadowSlaveInteractableNPC::AShadowSlaveInteractableNPC(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bIsInteractionEnabled = true;
	NPCId = NAME_None;
	NPCDisplayName = FText::FromString(TEXT("NPC"));
	InteractionVerb = FText::FromString(TEXT("Talk"));
	InteractionPriority = 1;
	DialogueDefinition = nullptr;
}

bool AShadowSlaveInteractableNPC::CanInteract_Implementation(AActor* Interactor)
{
	return bIsInteractionEnabled && IsAlive() && Interactor != nullptr && !IsPendingKillPending();
}

FText AShadowSlaveInteractableNPC::GetInteractionPrompt_Implementation(AActor* Interactor)
{
	if (!NPCDisplayName.IsEmpty())
	{
		return FText::Format(FText::FromString(TEXT("{0} ({1})")), InteractionVerb, NPCDisplayName);
	}

	return InteractionVerb;
}

void AShadowSlaveInteractableNPC::SetDialogueDefinition(UShadowSlaveDialogueDefinition* InDialogueDef)
{
	DialogueDefinition = InDialogueDef;
}

bool AShadowSlaveInteractableNPC::StartDialogue(AActor* Interactor)
{
	if (!DialogueDefinition || !Interactor)
	{
		return false;
	}

	UGameInstance* GI = GetGameInstance();
	if (!GI)
	{
		return false;
	}

	UShadowSlaveConversationSubsystem* ConvSubsystem = GI->GetSubsystem<UShadowSlaveConversationSubsystem>();
	if (!ConvSubsystem)
	{
		return false;
	}

	return ConvSubsystem->StartConversation(DialogueDefinition, this, Interactor);
}

FShadowSlaveInteractionResult AShadowSlaveInteractableNPC::Interact_Implementation(AActor* Interactor)
{
	if (!CanInteract_Implementation(Interactor))
	{
		return FShadowSlaveInteractionResult::Failure(FText::FromString(TEXT("Cannot talk right now.")), NPCId);
	}

	OnInteractedBy(Interactor);
	OnNPCInteracted.Broadcast(Interactor, this);

	// If dialogue definition is assigned, trigger dialogue conversation via conversation subsystem
	if (DialogueDefinition)
	{
		const bool bDialogueStarted = StartDialogue(Interactor);
		if (bDialogueStarted)
		{
			return FShadowSlaveInteractionResult::Success(NPCId);
		}

		return FShadowSlaveInteractionResult::Failure(FText::FromString(TEXT("Cannot start dialogue.")), NPCId);
	}

	// If no dialogue definition is assigned, fail safely per Step 20 specification
	return FShadowSlaveInteractionResult::Failure(FText::FromString(TEXT("No dialogue available.")), NPCId);
}

int32 AShadowSlaveInteractableNPC::GetInteractionPriority_Implementation(AActor* Interactor)
{
	return InteractionPriority;
}

void AShadowSlaveInteractableNPC::OnInteractedBy_Implementation(AActor* Interactor)
{
	// Orient NPC towards interactor
	if (Interactor)
	{
		const FVector Direction = Interactor->GetActorLocation() - GetActorLocation();
		const FRotator TargetRotation = FRotator(0.0f, Direction.Rotation().Yaw, 0.0f);
		SetActorRotation(TargetRotation);
	}
}
