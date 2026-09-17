// Copyright Epic Games, Inc. All Rights Reserved.

#include "Interaction/ShadowSlaveItemPickup.h"
#include "Items/ShadowSlaveInventoryComponent.h"

AShadowSlaveItemPickup::AShadowSlaveItemPickup()
{
	ItemDefinition = nullptr;
	Quantity = 1;
	InteractionPrompt = FText::FromString(TEXT("Pick Up Item"));
	InteractionPriority = 10;
}

void AShadowSlaveItemPickup::InitializeItemPickup(UShadowSlaveItemDefinition* InItemDef, int32 InQuantity)
{
	ItemDefinition = InItemDef;
	Quantity = FMath::Max(1, InQuantity);

	if (ItemDefinition)
	{
		InteractionPrompt = FText::Format(
			NSLOCTEXT("ShadowSlave", "PickUpItemFormat", "Pick Up {0}"),
			ItemDefinition->DisplayName
		);
	}
}

bool AShadowSlaveItemPickup::CanInteract_Implementation(AActor* Interactor)
{
	return Super::CanInteract_Implementation(Interactor) && ItemDefinition != nullptr && Quantity > 0;
}

FText AShadowSlaveItemPickup::GetInteractionPrompt_Implementation(AActor* Interactor)
{
	if (ItemDefinition)
	{
		if (Quantity > 1)
		{
			return FText::Format(
				NSLOCTEXT("ShadowSlave", "PickUpItemQtyFormat", "Pick Up {0} ({1})"),
				ItemDefinition->DisplayName,
				FText::AsNumber(Quantity)
			);
		}

		return FText::Format(
			NSLOCTEXT("ShadowSlave", "PickUpItemSingleFormat", "Pick Up {0}"),
			ItemDefinition->DisplayName
		);
	}

	return InteractionPrompt;
}

FShadowSlaveInteractionResult AShadowSlaveItemPickup::ExecuteInteraction(AActor* Interactor)
{
	if (!Interactor)
	{
		return FShadowSlaveInteractionResult::Failure(FText::FromString(TEXT("Invalid interactor.")), InteractionId);
	}

	if (bIsProcessingAcquisition || PickupState != EShadowSlavePickupState::Available || bIsCollected)
	{
		return FShadowSlaveInteractionResult::Failure(FText::FromString(TEXT("Pickup is not available.")), InteractionId);
	}

	if (!ItemDefinition || Quantity <= 0)
	{
		return FShadowSlaveInteractionResult::Failure(FText::FromString(TEXT("Item definition is missing or invalid.")), InteractionId);
	}

	UShadowSlaveInventoryComponent* InventoryComp = Interactor->FindComponentByClass<UShadowSlaveInventoryComponent>();
	if (!InventoryComp)
	{
		return FShadowSlaveInteractionResult::Failure(FText::FromString(TEXT("Interactor has no Inventory Component.")), InteractionId);
	}

	bIsProcessingAcquisition = true;

	int32 OutRemainder = Quantity;
	const bool bAdded = InventoryComp->AddItem(ItemDefinition, Quantity, OutRemainder);

	if (!bAdded || OutRemainder == Quantity)
	{
		// Inventory is completely full; pickup remains in the world untouched
		bIsProcessingAcquisition = false;
		return FShadowSlaveInteractionResult::Failure(FText::FromString(TEXT("Inventory is full.")), InteractionId);
	}

	if (OutRemainder > 0)
	{
		// Partial collection: update remaining quantity and keep pickup in world
		Quantity = OutRemainder;
		bIsProcessingAcquisition = false;
		return FShadowSlaveInteractionResult::Success(FName(TEXT("ItemPartiallyAcquired")));
	}

	// Full collection confirmed by inventory system; consume and remove pickup
	Quantity = 0;
	OnCollected(Interactor);
	bIsProcessingAcquisition = false;
	return FShadowSlaveInteractionResult::Success(FName(TEXT("ItemAcquired")));
}

bool AShadowSlaveItemPickup::CaptureSaveRecord_Implementation(FShadowSlaveWorldActorSaveRecord& OutRecord)
{
	if (!Super::CaptureSaveRecord_Implementation(OutRecord))
	{
		return false;
	}

	OutRecord.CustomStateData.Add(TEXT("Quantity"), FString::FromInt(Quantity));
	if (ItemDefinition)
	{
		OutRecord.CustomStateData.Add(TEXT("ItemDefinitionPath"), ItemDefinition->GetPathName());
	}
	return true;
}

bool AShadowSlaveItemPickup::RestoreSaveRecord_Implementation(const FShadowSlaveWorldActorSaveRecord& InRecord)
{
	if (!Super::RestoreSaveRecord_Implementation(InRecord))
	{
		return false;
	}

	if (const FString* PathVal = InRecord.CustomStateData.Find(TEXT("ItemDefinitionPath")))
	{
		if (!PathVal->IsEmpty())
		{
			if (UShadowSlaveItemDefinition* LoadedDef = Cast<UShadowSlaveItemDefinition>(StaticLoadObject(UShadowSlaveItemDefinition::StaticClass(), nullptr, **PathVal)))
			{
				ItemDefinition = LoadedDef;
			}
		}
	}

	if (const FString* Val = InRecord.CustomStateData.Find(TEXT("Quantity")))
	{
		Quantity = FCString::Atoi(**Val);
		if (Quantity <= 0 && PickupState == EShadowSlavePickupState::Available)
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
	}

	return true;
}
