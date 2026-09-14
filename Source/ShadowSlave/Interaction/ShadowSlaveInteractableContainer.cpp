// Copyright Epic Games, Inc. All Rights Reserved.

#include "Interaction/ShadowSlaveInteractableContainer.h"

AShadowSlaveInteractableContainer::AShadowSlaveInteractableContainer()
{
	bIsOpen = false;
	bIsLocked = false;
	RequiredKeyId = NAME_None;
	InteractionPrompt = FText::FromString(TEXT("Open Container"));
}

FText AShadowSlaveInteractableContainer::GetInteractionPrompt_Implementation(AActor* Interactor)
{
	if (bIsLocked)
	{
		return FText::FromString(TEXT("Locked"));
	}

	return bIsOpen ? FText::FromString(TEXT("Close Container")) : FText::FromString(TEXT("Open Container"));
}

bool AShadowSlaveInteractableContainer::OpenContainer(AActor* Interactor)
{
	if (bIsLocked || bIsOpen)
	{
		return false;
	}

	bIsOpen = true;
	OnContainerStateChanged.Broadcast(bIsOpen, Interactor);
	return true;
}

bool AShadowSlaveInteractableContainer::CloseContainer(AActor* Interactor)
{
	if (!bIsOpen)
	{
		return false;
	}

	bIsOpen = false;
	OnContainerStateChanged.Broadcast(bIsOpen, Interactor);
	return true;
}

bool AShadowSlaveInteractableContainer::ToggleContainer(AActor* Interactor)
{
	if (bIsLocked)
	{
		return false;
	}

	bIsOpen = !bIsOpen;
	OnContainerStateChanged.Broadcast(bIsOpen, Interactor);
	return true;
}

bool AShadowSlaveInteractableContainer::UnlockContainer(FName InKeyId)
{
	if (!bIsLocked)
	{
		return true;
	}

	if (RequiredKeyId.IsNone() || RequiredKeyId == InKeyId)
	{
		bIsLocked = false;
		return true;
	}

	return false;
}

FShadowSlaveInteractionResult AShadowSlaveInteractableContainer::ExecuteInteraction(AActor* Interactor)
{
	if (bIsLocked)
	{
		return FShadowSlaveInteractionResult::Failure(FText::FromString(TEXT("Container is locked.")), InteractionId);
	}

	const bool bSuccess = ToggleContainer(Interactor);
	if (bSuccess)
	{
		return FShadowSlaveInteractionResult::Success(InteractionId);
	}

	return FShadowSlaveInteractionResult::Failure(FText::FromString(TEXT("Failed to interact with container.")), InteractionId);
}

bool AShadowSlaveInteractableContainer::CaptureSaveRecord_Implementation(FShadowSlaveWorldActorSaveRecord& OutRecord)
{
	if (!Super::CaptureSaveRecord_Implementation(OutRecord))
	{
		return false;
	}

	OutRecord.CustomStateData.Add(TEXT("bIsOpen"), bIsOpen ? TEXT("1") : TEXT("0"));
	OutRecord.CustomStateData.Add(TEXT("bIsLocked"), bIsLocked ? TEXT("1") : TEXT("0"));
	return true;
}

bool AShadowSlaveInteractableContainer::RestoreSaveRecord_Implementation(const FShadowSlaveWorldActorSaveRecord& InRecord)
{
	if (!Super::RestoreSaveRecord_Implementation(InRecord))
	{
		return false;
	}

	if (const FString* Val = InRecord.CustomStateData.Find(TEXT("bIsOpen")))
	{
		bIsOpen = (*Val == TEXT("1"));
	}
	if (const FString* Val = InRecord.CustomStateData.Find(TEXT("bIsLocked")))
	{
		bIsLocked = (*Val == TEXT("1"));
	}

	return true;
}
