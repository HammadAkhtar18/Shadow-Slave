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

	if (!ItemDefinition || Quantity <= 0)
	{
		return FShadowSlaveInteractionResult::Failure(FText::FromString(TEXT("Item definition is missing or invalid.")), InteractionId);
	}

	UShadowSlaveInventoryComponent* InventoryComp = Interactor->FindComponentByClass<UShadowSlaveInventoryComponent>();
	if (!InventoryComp)
	{
		return FShadowSlaveInteractionResult::Failure(FText::FromString(TEXT("Interactor has no Inventory Component.")), InteractionId);
	}

	int32 OutRemainder = 0;
	const bool bAdded = InventoryComp->AddItem(ItemDefinition, Quantity, OutRemainder);

	if (!bAdded || OutRemainder == Quantity)
	{
		// Inventory is completely full; pickup remains in the world untouched
		return FShadowSlaveInteractionResult::Failure(FText::FromString(TEXT("Inventory is full.")), InteractionId);
	}

	if (OutRemainder > 0)
	{
		// Partial collection: update remaining quantity and keep pickup in world
		Quantity = OutRemainder;
		return FShadowSlaveInteractionResult::Success(FName(TEXT("ItemPartiallyAcquired")));
	}

	// Full collection confirmed by inventory system; consume and remove pickup
	OnCollected(Interactor);
	return FShadowSlaveInteractionResult::Success(FName(TEXT("ItemAcquired")));
}

bool AShadowSlaveItemPickup::CaptureSaveRecord_Implementation(FShadowSlaveWorldActorSaveRecord& OutRecord)
{
	if (!Super::CaptureSaveRecord_Implementation(OutRecord))
	{
		return false;
	}

	OutRecord.CustomStateData.Add(TEXT("Quantity"), FString::FromInt(Quantity));
	return true;
}

bool AShadowSlaveItemPickup::RestoreSaveRecord_Implementation(const FShadowSlaveWorldActorSaveRecord& InRecord)
{
	if (!Super::RestoreSaveRecord_Implementation(InRecord))
	{
		return false;
	}

	if (const FString* Val = InRecord.CustomStateData.Find(TEXT("Quantity")))
	{
		Quantity = FCString::Atoi(**Val);
	}

	return true;
}
