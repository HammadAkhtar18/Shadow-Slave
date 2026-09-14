// Copyright Epic Games, Inc. All Rights Reserved.

#include "Interaction/ShadowSlaveInteractableDoor.h"

AShadowSlaveInteractableDoor::AShadowSlaveInteractableDoor()
{
	bIsOpen = false;
	bIsLocked = false;
	RequiredKeyId = NAME_None;
	InteractionPrompt = FText::FromString(TEXT("Open Door"));
}

FText AShadowSlaveInteractableDoor::GetInteractionPrompt_Implementation(AActor* Interactor)
{
	if (bIsLocked)
	{
		return FText::FromString(TEXT("Locked"));
	}

	return bIsOpen ? FText::FromString(TEXT("Close Door")) : FText::FromString(TEXT("Open Door"));
}

bool AShadowSlaveInteractableDoor::OpenDoor(AActor* Interactor)
{
	if (bIsLocked || bIsOpen)
	{
		return false;
	}

	bIsOpen = true;
	OnDoorStateChanged.Broadcast(bIsOpen, Interactor);
	ReceiveDoorStateChanged(bIsOpen, Interactor);
	return true;
}

bool AShadowSlaveInteractableDoor::CloseDoor(AActor* Interactor)
{
	if (!bIsOpen)
	{
		return false;
	}

	bIsOpen = false;
	OnDoorStateChanged.Broadcast(bIsOpen, Interactor);
	ReceiveDoorStateChanged(bIsOpen, Interactor);
	return true;
}

bool AShadowSlaveInteractableDoor::ToggleDoor(AActor* Interactor)
{
	if (bIsLocked)
	{
		return false;
	}

	bIsOpen = !bIsOpen;
	OnDoorStateChanged.Broadcast(bIsOpen, Interactor);
	ReceiveDoorStateChanged(bIsOpen, Interactor);
	return true;
}

bool AShadowSlaveInteractableDoor::UnlockDoor(FName InKeyId)
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

FShadowSlaveInteractionResult AShadowSlaveInteractableDoor::ExecuteInteraction(AActor* Interactor)
{
	if (bIsLocked)
	{
		return FShadowSlaveInteractionResult::Failure(FText::FromString(TEXT("Door is locked.")), InteractionId);
	}

	const bool bSuccess = ToggleDoor(Interactor);
	if (bSuccess)
	{
		return FShadowSlaveInteractionResult::Success(InteractionId);
	}

	return FShadowSlaveInteractionResult::Failure(FText::FromString(TEXT("Failed to toggle door.")), InteractionId);
}

bool AShadowSlaveInteractableDoor::CaptureSaveRecord_Implementation(FShadowSlaveWorldActorSaveRecord& OutRecord)
{
	if (!Super::CaptureSaveRecord_Implementation(OutRecord))
	{
		return false;
	}

	OutRecord.CustomStateData.Add(TEXT("bIsOpen"), bIsOpen ? TEXT("1") : TEXT("0"));
	OutRecord.CustomStateData.Add(TEXT("bIsLocked"), bIsLocked ? TEXT("1") : TEXT("0"));
	return true;
}

bool AShadowSlaveInteractableDoor::RestoreSaveRecord_Implementation(const FShadowSlaveWorldActorSaveRecord& InRecord)
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
