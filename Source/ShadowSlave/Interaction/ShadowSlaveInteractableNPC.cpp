// Copyright Epic Games, Inc. All Rights Reserved.

#include "Interaction/ShadowSlaveInteractableNPC.h"

AShadowSlaveInteractableNPC::AShadowSlaveInteractableNPC(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bIsInteractionEnabled = true;
	NPCId = NAME_None;
	NPCDisplayName = FText::FromString(TEXT("NPC"));
	InteractionVerb = FText::FromString(TEXT("Talk"));
	InteractionPriority = 1;
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

FShadowSlaveInteractionResult AShadowSlaveInteractableNPC::Interact_Implementation(AActor* Interactor)
{
	if (!CanInteract_Implementation(Interactor))
	{
		return FShadowSlaveInteractionResult::Failure(FText::FromString(TEXT("Cannot talk right now.")), NPCId);
	}

	OnInteractedBy(Interactor);
	OnNPCInteracted.Broadcast(Interactor, this);

	return FShadowSlaveInteractionResult::Success(NPCId);
}

int32 AShadowSlaveInteractableNPC::GetInteractionPriority_Implementation(AActor* Interactor)
{
	return InteractionPriority;
}

void AShadowSlaveInteractableNPC::OnInteractedBy_Implementation(AActor* Interactor)
{
	// Optional base logic: orient NPC towards interactor
	if (Interactor)
	{
		const FVector Direction = Interactor->GetActorLocation() - GetActorLocation();
		const FRotator TargetRotation = FRotator(0.0f, Direction.Rotation().Yaw, 0.0f);
		SetActorRotation(TargetRotation);
	}
}
