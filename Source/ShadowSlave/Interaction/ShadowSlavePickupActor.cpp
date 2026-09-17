// Copyright Epic Games, Inc. All Rights Reserved.

#include "Interaction/ShadowSlavePickupActor.h"
#include "Components/StaticMeshComponent.h"

AShadowSlavePickupActor::AShadowSlavePickupActor()
{
	PickupState = EShadowSlavePickupState::Available;
	bIsCollected = false;
	bDestroyOnPickup = true;
	bIsProcessingAcquisition = false;
	InteractionPrompt = FText::FromString(TEXT("Pick Up"));
	InteractionPriority = 10; // Pickups typically take priority over background geometry
}

bool AShadowSlavePickupActor::CanInteract_Implementation(AActor* Interactor)
{
	if (bIsProcessingAcquisition || PickupState != EShadowSlavePickupState::Available || bIsCollected)
	{
		return false;
	}

	return Super::CanInteract_Implementation(Interactor);
}

void AShadowSlavePickupActor::OnCollected(AActor* Interactor)
{
	if (PickupState == EShadowSlavePickupState::Consumed)
	{
		return;
	}

	PickupState = EShadowSlavePickupState::Consumed;
	bIsCollected = true;
	SetInteractionEnabled(false);

	if (StaticMeshComponent)
	{
		StaticMeshComponent->SetVisibility(false);
		StaticMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	OnPickupStateChanged.Broadcast(PickupState, Interactor);
	OnPickupCollected.Broadcast(this, Interactor);

	if (bDestroyOnPickup)
	{
		Destroy();
	}
}

bool AShadowSlavePickupActor::CaptureSaveRecord_Implementation(FShadowSlaveWorldActorSaveRecord& OutRecord)
{
	if (!Super::CaptureSaveRecord_Implementation(OutRecord))
	{
		return false;
	}

	OutRecord.CustomStateData.Add(TEXT("PickupState"), PickupState == EShadowSlavePickupState::Consumed ? TEXT("Consumed") : TEXT("Available"));
	OutRecord.CustomStateData.Add(TEXT("bIsCollected"), (PickupState == EShadowSlavePickupState::Consumed || bIsCollected) ? TEXT("1") : TEXT("0"));
	return true;
}

bool AShadowSlavePickupActor::RestoreSaveRecord_Implementation(const FShadowSlaveWorldActorSaveRecord& InRecord)
{
	if (!Super::RestoreSaveRecord_Implementation(InRecord))
	{
		return false;
	}

	bool bShouldBeConsumed = false;
	if (const FString* StateVal = InRecord.CustomStateData.Find(TEXT("PickupState")))
	{
		bShouldBeConsumed = (*StateVal == TEXT("Consumed"));
	}
	else if (const FString* Val = InRecord.CustomStateData.Find(TEXT("bIsCollected")))
	{
		bShouldBeConsumed = (*Val == TEXT("1"));
	}

	if (bShouldBeConsumed)
	{
		PickupState = EShadowSlavePickupState::Consumed;
		bIsCollected = true;
		SetInteractionEnabled(false);
		if (StaticMeshComponent)
		{
			StaticMeshComponent->SetVisibility(false);
			StaticMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
	else
	{
		PickupState = EShadowSlavePickupState::Available;
		bIsCollected = false;
		SetInteractionEnabled(true);
		if (StaticMeshComponent)
		{
			StaticMeshComponent->SetVisibility(true);
			StaticMeshComponent->SetCollisionProfileName(TEXT("BlockAllDynamic"));
		}
	}

	return true;
}
