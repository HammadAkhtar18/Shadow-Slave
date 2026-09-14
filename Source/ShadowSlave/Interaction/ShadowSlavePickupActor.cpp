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
