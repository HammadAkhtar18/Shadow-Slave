// Copyright Epic Games, Inc. All Rights Reserved.

#include "Interaction/ShadowSlaveInteractableContainer.h"
#include "Items/ShadowSlaveInventoryComponent.h"

AShadowSlaveInteractableContainer::AShadowSlaveInteractableContainer()
{
	bIsOpen = false;
	bIsLocked = false;
	RequiredKeyId = NAME_None;
	bAutoLootOnOpen = false;
	bIsTransferringLoot = false;
	InteractionPrompt = FText::FromString(TEXT("Open Container"));
	InteractionPriority = 5;
}

FText AShadowSlaveInteractableContainer::GetInteractionPrompt_Implementation(AActor* Interactor)
{
	if (bIsLocked)
	{
		return FText::FromString(TEXT("Locked"));
	}

	if (bIsOpen)
	{
		return HasLoot() ? FText::FromString(TEXT("Take Items")) : FText::FromString(TEXT("Close Container"));
	}

	return FText::FromString(TEXT("Open Container"));
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

void AShadowSlaveInteractableContainer::InitializeContainerLoot(const TArray<FShadowSlaveItemInstance>& InItems)
{
	StoredItems = InItems;
	OnContainerLootChanged.Broadcast(this, StoredItems.Num());
}

void AShadowSlaveInteractableContainer::AddStoredItem(UShadowSlaveItemDefinition* ItemDef, int32 InQuantity)
{
	if (!ItemDef || InQuantity <= 0)
	{
		return;
	}

	FShadowSlaveItemInstance NewInstance(ItemDef, InQuantity);
	StoredItems.Add(NewInstance);
	OnContainerLootChanged.Broadcast(this, StoredItems.Num());
}

bool AShadowSlaveInteractableContainer::HasLoot() const
{
	for (const FShadowSlaveItemInstance& Item : StoredItems)
	{
		if (Item.IsValid())
		{
			return true;
		}
	}
	return false;
}

bool AShadowSlaveInteractableContainer::TransferLootToInteractor(AActor* Interactor, TArray<FShadowSlaveItemInstance>& OutTransferredItems)
{
	OutTransferredItems.Empty();

	if (bIsLocked || bIsTransferringLoot || !Interactor || !HasLoot())
	{
		return false;
	}

	UShadowSlaveInventoryComponent* InventoryComp = Interactor->FindComponentByClass<UShadowSlaveInventoryComponent>();
	if (!InventoryComp)
	{
		return false;
	}

	bIsTransferringLoot = true;

	bool bAnyTransferred = false;
	TArray<int32> IndicesToRemove;

	for (int32 Index = 0; Index < StoredItems.Num(); ++Index)
	{
		FShadowSlaveItemInstance& StoredItem = StoredItems[Index];
		if (!StoredItem.IsValid())
		{
			IndicesToRemove.Add(Index);
			continue;
		}

		int32 OutRemainder = StoredItem.Quantity;
		const bool bAdded = InventoryComp->AddItem(StoredItem.ItemDefinition, StoredItem.Quantity, OutRemainder);

		if (bAdded)
		{
			const int32 QuantityTransferred = StoredItem.Quantity - OutRemainder;
			if (QuantityTransferred > 0)
			{
				bAnyTransferred = true;

				FShadowSlaveItemInstance TransferredRecord(StoredItem.ItemDefinition, QuantityTransferred);
				TransferredRecord.InstanceId = StoredItem.InstanceId;
				TransferredRecord.DynamicProperties = StoredItem.DynamicProperties;
				OutTransferredItems.Add(TransferredRecord);

				OnContainerLootTransferred.Broadcast(this, TransferredRecord, Interactor);

				if (OutRemainder <= 0)
				{
					IndicesToRemove.Add(Index);
				}
				else
				{
					StoredItem.Quantity = OutRemainder;
				}
			}
		}
	}

	// Remove fully transferred items in reverse order
	for (int32 i = IndicesToRemove.Num() - 1; i >= 0; --i)
	{
		StoredItems.RemoveAt(IndicesToRemove[i]);
	}

	bIsTransferringLoot = false;

	if (bAnyTransferred)
	{
		OnContainerLootChanged.Broadcast(this, StoredItems.Num());
	}

	return bAnyTransferred;
}

FShadowSlaveInteractionResult AShadowSlaveInteractableContainer::ExecuteInteraction(AActor* Interactor)
{
	// 1. Check lock state and attempt unlock if interactor has matching key
	if (bIsLocked)
	{
		if (!RequiredKeyId.IsNone() && Interactor)
		{
			if (UShadowSlaveInventoryComponent* InvComp = Interactor->FindComponentByClass<UShadowSlaveInventoryComponent>())
			{
				bool bKeyFound = false;
				for (const FShadowSlaveItemInstance& Slot : InvComp->GetSlots())
				{
					if (Slot.IsValid() && Slot.ItemDefinition)
					{
						if (Slot.ItemDefinition->GetFName() == RequiredKeyId ||
							Slot.ItemDefinition->GetPrimaryAssetId().PrimaryAssetName == RequiredKeyId)
						{
							bKeyFound = true;
							break;
						}
					}
				}

				if (bKeyFound)
				{
					UnlockContainer(RequiredKeyId);
				}
			}
		}

		if (bIsLocked)
		{
			return FShadowSlaveInteractionResult::Failure(FText::FromString(TEXT("Container is locked.")), InteractionId);
		}
	}

	// 2. Closed container: interaction opens it
	if (!bIsOpen)
	{
		const bool bOpened = OpenContainer(Interactor);
		if (!bOpened)
		{
			return FShadowSlaveInteractionResult::Failure(FText::FromString(TEXT("Failed to open container.")), InteractionId);
		}

		if (bAutoLootOnOpen && HasLoot())
		{
			TArray<FShadowSlaveItemInstance> TransferredItems;
			TransferLootToInteractor(Interactor, TransferredItems);
		}

		return FShadowSlaveInteractionResult::Success(FName(TEXT("ContainerOpened")));
	}

	// 3. Open container: interaction loots if loot remains, otherwise closes container
	if (HasLoot())
	{
		TArray<FShadowSlaveItemInstance> TransferredItems;
		const bool bTransferred = TransferLootToInteractor(Interactor, TransferredItems);

		if (bTransferred)
		{
			const FName OutcomeId = StoredItems.IsEmpty() ? FName(TEXT("ContainerLootedAll")) : FName(TEXT("ContainerLootedPartial"));
			return FShadowSlaveInteractionResult::Success(OutcomeId);
		}

		// Failed transfer when loot exists means inventory cannot accept items
		return FShadowSlaveInteractionResult::Failure(FText::FromString(TEXT("Inventory is full.")), InteractionId);
	}

	// 4. Open empty container: interaction closes it
	const bool bClosed = CloseContainer(Interactor);
	if (bClosed)
	{
		return FShadowSlaveInteractionResult::Success(FName(TEXT("ContainerClosed")));
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
	if (!RequiredKeyId.IsNone())
	{
		OutRecord.CustomStateData.Add(TEXT("RequiredKeyId"), RequiredKeyId.ToString());
	}

	// Persist remaining stored loot items
	OutRecord.CustomStateData.Add(TEXT("StoredItemCount"), FString::FromInt(StoredItems.Num()));
	for (int32 i = 0; i < StoredItems.Num(); ++i)
	{
		const FShadowSlaveItemInstance& Item = StoredItems[i];
		const FString Prefix = FString::Printf(TEXT("Item_%d_"), i);

		OutRecord.CustomStateData.Add(FName(*(Prefix + TEXT("InstanceId"))), Item.InstanceId.ToString());
		OutRecord.CustomStateData.Add(FName(*(Prefix + TEXT("Quantity"))), FString::FromInt(Item.Quantity));
		if (Item.ItemDefinition)
		{
			OutRecord.CustomStateData.Add(FName(*(Prefix + TEXT("AssetPath"))), Item.ItemDefinition->GetPathName());
		}
	}

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
	if (const FString* Val = InRecord.CustomStateData.Find(TEXT("RequiredKeyId")))
	{
		RequiredKeyId = FName(**Val);
	}

	// Restore remaining stored loot items
	if (const FString* CountStr = InRecord.CustomStateData.Find(TEXT("StoredItemCount")))
	{
		const int32 Count = FCString::Atoi(**CountStr);
		StoredItems.Empty(Count);

		for (int32 i = 0; i < Count; ++i)
		{
			const FString Prefix = FString::Printf(TEXT("Item_%d_"), i);
			const FString* PathStr = InRecord.CustomStateData.Find(FName(*(Prefix + TEXT("AssetPath"))));
			const FString* QtyStr = InRecord.CustomStateData.Find(FName(*(Prefix + TEXT("Quantity"))));
			const FString* IdStr = InRecord.CustomStateData.Find(FName(*(Prefix + TEXT("InstanceId"))));

			if (PathStr && !PathStr->IsEmpty())
			{
				UShadowSlaveItemDefinition* LoadedDef = Cast<UShadowSlaveItemDefinition>(StaticLoadObject(UShadowSlaveItemDefinition::StaticClass(), nullptr, **PathStr));
				if (LoadedDef)
				{
					const int32 Qty = QtyStr ? FMath::Max(1, FCString::Atoi(**QtyStr)) : 1;
					FShadowSlaveItemInstance RestoredItem(LoadedDef, Qty);
					if (IdStr && !IdStr->IsEmpty())
					{
						FGuid ParsedGuid;
						if (FGuid::Parse(*IdStr, ParsedGuid))
						{
							RestoredItem.InstanceId = ParsedGuid;
						}
					}
					StoredItems.Add(RestoredItem);
				}
			}
		}
	}

	return true;
}
