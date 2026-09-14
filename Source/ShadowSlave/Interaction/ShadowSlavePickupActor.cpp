// Copyright Epic Games, Inc. All Rights Reserved.

#include "Interaction/ShadowSlavePickupActor.h"
#include "Components/StaticMeshComponent.h"

AShadowSlavePickupActor::AShadowSlavePickupActor()
{
	bIsCollected = false;
	bDestroyOnPickup = true;
	InteractionPrompt = FText::FromString(TEXT("Pick Up"));
	InteractionPriority = 10; // Pickups typically take priority over background geometry
}

bool AShadowSlavePickupActor::CanInteract_Implementation(AActor* Interactor)
{
	return !bIsCollected && Super::CanInteract_Implementation(Interactor);
}

void AShadowSlavePickupActor::OnCollected(AActor* Interactor)
{
	bIsCollected = true;
	SetInteractionEnabled(false);

	if (StaticMeshComponent)
	{
		StaticMeshComponent->SetVisibility(false);
		StaticMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

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

	OutRecord.CustomStateData.Add(TEXT("bIsCollected"), bIsCollected ? TEXT("1") : TEXT("0"));
	return true;
}

bool AShadowSlavePickupActor::RestoreSaveRecord_Implementation(const FShadowSlaveWorldActorSaveRecord& InRecord)
{
	if (!Super::RestoreSaveRecord_Implementation(InRecord))
	{
		return false;
	}

	if (const FString* Val = InRecord.CustomStateData.Find(TEXT("bIsCollected")))
	{
		bIsCollected = (*Val == TEXT("1"));
		if (bIsCollected)
		{
			SetInteractionEnabled(false);
			if (StaticMeshComponent)
			{
				StaticMeshComponent->SetVisibility(false);
				StaticMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			}
		}
	}

	return true;
}
