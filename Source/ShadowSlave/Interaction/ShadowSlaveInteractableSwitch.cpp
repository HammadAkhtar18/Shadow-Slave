// Copyright Epic Games, Inc. All Rights Reserved.

#include "Interaction/ShadowSlaveInteractableSwitch.h"

AShadowSlaveInteractableSwitch::AShadowSlaveInteractableSwitch()
{
	bIsActivated = false;
	bCanBeDeactivated = true;
	InteractionPrompt = FText::FromString(TEXT("Activate"));
}

bool AShadowSlaveInteractableSwitch::CanInteract_Implementation(AActor* Interactor)
{
	if (!Super::CanInteract_Implementation(Interactor))
	{
		return false;
	}

	// If already activated and cannot be deactivated, cannot interact further
	if (bIsActivated && !bCanBeDeactivated)
	{
		return false;
	}

	return true;
}

FText AShadowSlaveInteractableSwitch::GetInteractionPrompt_Implementation(AActor* Interactor)
{
	if (bIsActivated)
	{
		return bCanBeDeactivated ? FText::FromString(TEXT("Deactivate")) : FText::FromString(TEXT("Activated"));
	}

	return FText::FromString(TEXT("Activate"));
}

bool AShadowSlaveInteractableSwitch::ActivateSwitch(AActor* Interactor)
{
	if (bIsActivated)
	{
		return false;
	}

	bIsActivated = true;
	OnSwitchStateChanged.Broadcast(bIsActivated, Interactor);
	ReceiveSwitchStateChanged(bIsActivated, Interactor);
	return true;
}

bool AShadowSlaveInteractableSwitch::DeactivateSwitch(AActor* Interactor)
{
	if (!bIsActivated || !bCanBeDeactivated)
	{
		return false;
	}

	bIsActivated = false;
	OnSwitchStateChanged.Broadcast(bIsActivated, Interactor);
	ReceiveSwitchStateChanged(bIsActivated, Interactor);
	return true;
}

bool AShadowSlaveInteractableSwitch::ToggleSwitch(AActor* Interactor)
{
	if (bIsActivated)
	{
		return DeactivateSwitch(Interactor);
	}
	else
	{
		return ActivateSwitch(Interactor);
	}
}

FShadowSlaveInteractionResult AShadowSlaveInteractableSwitch::ExecuteInteraction(AActor* Interactor)
{
	if (bIsActivated && !bCanBeDeactivated)
	{
		return FShadowSlaveInteractionResult::Failure(FText::FromString(TEXT("Switch cannot be deactivated.")), InteractionId);
	}

	const bool bSuccess = ToggleSwitch(Interactor);
	if (bSuccess)
	{
		return FShadowSlaveInteractionResult::Success(InteractionId);
	}

	return FShadowSlaveInteractionResult::Failure(FText::FromString(TEXT("Failed to toggle switch.")), InteractionId);
}

bool AShadowSlaveInteractableSwitch::CaptureSaveRecord_Implementation(FShadowSlaveWorldActorSaveRecord& OutRecord)
{
	if (!Super::CaptureSaveRecord_Implementation(OutRecord))
	{
		return false;
	}

	OutRecord.CustomStateData.Add(TEXT("bIsActivated"), bIsActivated ? TEXT("1") : TEXT("0"));
	return true;
}

bool AShadowSlaveInteractableSwitch::RestoreSaveRecord_Implementation(const FShadowSlaveWorldActorSaveRecord& InRecord)
{
	if (!Super::RestoreSaveRecord_Implementation(InRecord))
	{
		return false;
	}

	if (const FString* Val = InRecord.CustomStateData.Find(TEXT("bIsActivated")))
	{
		bIsActivated = (*Val == TEXT("1"));
	}

	return true;
}
