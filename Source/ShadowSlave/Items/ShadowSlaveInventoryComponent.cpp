// Copyright Epic Games, Inc. All Rights Reserved.

#include "Items/ShadowSlaveInventoryComponent.h"
#include "ShadowSlave.h"

UShadowSlaveInventoryComponent::UShadowSlaveInventoryComponent()
{
	// Operates event-driven; no tick overhead
	PrimaryComponentTick.bCanEverTick = false;
	MaxSlots = 20;
}

void UShadowSlaveInventoryComponent::SetCapacity(int32 NewCapacity)
{
	const int32 ClampedCapacity = FMath::Max(1, NewCapacity);
	if (MaxSlots != ClampedCapacity)
	{
		MaxSlots = ClampedCapacity;
		OnInventoryChanged.Broadcast();
	}
}

bool UShadowSlaveInventoryComponent::AddItem(UShadowSlaveItemDefinition* ItemDef, int32 Quantity, int32& OutRemainder)
{
	// Edge case: invalid parameters
	if (!ItemDef || Quantity <= 0)
	{
		OutRemainder = FMath::Max(0, Quantity);
		return false;
	}

	int32 RemainingToAdd = Quantity;

	if (ItemDef->bIsStackable)
	{
		const int32 MaxStack = FMath::Max(1, ItemDef->MaxStackSize);

		// Phase 1: Try filling existing partially filled stacks
		for (FShadowSlaveItemInstance& Slot : Slots)
		{
			if (Slot.ItemDefinition == ItemDef && Slot.Quantity < MaxStack)
			{
				const int32 SpaceAvailable = MaxStack - Slot.Quantity;
				const int32 AmountToFill = FMath::Min(RemainingToAdd, SpaceAvailable);

				Slot.Quantity += AmountToFill;
				RemainingToAdd -= AmountToFill;

				OnItemAdded.Broadcast(Slot, AmountToFill);

				if (RemainingToAdd <= 0)
				{
					break;
				}
			}
		}

		// Phase 2: Create new stacks in empty slots as long as capacity permits
		while (RemainingToAdd > 0 && Slots.Num() < MaxSlots)
		{
			const int32 AmountInNewSlot = FMath::Min(RemainingToAdd, MaxStack);
			FShadowSlaveItemInstance NewInstance(ItemDef, AmountInNewSlot);

			Slots.Add(NewInstance);
			RemainingToAdd -= AmountInNewSlot;

			OnItemAdded.Broadcast(NewInstance, AmountInNewSlot);
		}
	}
	else
	{
		// Non-stackable item: each individual unit requires its own slot
		while (RemainingToAdd > 0 && Slots.Num() < MaxSlots)
		{
			FShadowSlaveItemInstance NewInstance(ItemDef, 1);

			Slots.Add(NewInstance);
			RemainingToAdd -= 1;

			OnItemAdded.Broadcast(NewInstance, 1);
		}
	}

	OutRemainder = RemainingToAdd;
	const int32 TotalAdded = Quantity - RemainingToAdd;

	if (TotalAdded > 0)
	{
		OnInventoryChanged.Broadcast();
		return true;
	}

	return false;
}

bool UShadowSlaveInventoryComponent::AddItemSimple(UShadowSlaveItemDefinition* ItemDef, int32 Quantity)
{
	int32 Remainder = 0;
	return AddItem(ItemDef, Quantity, Remainder);
}

bool UShadowSlaveInventoryComponent::RemoveItem(const UShadowSlaveItemDefinition* ItemDef, int32 Quantity)
{
	// Edge case: invalid parameters or non-existent item
	if (!ItemDef || Quantity <= 0)
	{
		return false;
	}

	// Fail safely if inventory does not contain sufficient quantity (atomic operation)
	if (GetTotalItemCount(ItemDef) < Quantity)
	{
		return false;
	}

	int32 RemainingToRemove = Quantity;

	// Iterate backwards to remove or decrement slots cleanly
	for (int32 i = Slots.Num() - 1; i >= 0; --i)
	{
		if (Slots[i].ItemDefinition == ItemDef)
		{
			const int32 AmountFromSlot = FMath::Min(RemainingToRemove, Slots[i].Quantity);
			Slots[i].Quantity -= AmountFromSlot;
			RemainingToRemove -= AmountFromSlot;

			FShadowSlaveItemInstance RemovedInfo = Slots[i];
			RemovedInfo.Quantity = AmountFromSlot;

			if (Slots[i].Quantity <= 0)
			{
				Slots.RemoveAt(i);
			}

			OnItemRemoved.Broadcast(RemovedInfo, AmountFromSlot);

			if (RemainingToRemove <= 0)
			{
				break;
			}
		}
	}

	OnInventoryChanged.Broadcast();
	return true;
}

bool UShadowSlaveInventoryComponent::RemoveItemByInstanceId(const FGuid& InstanceId, int32 Quantity)
{
	if (!InstanceId.IsValid() || Quantity <= 0)
	{
		return false;
	}

	for (int32 i = 0; i < Slots.Num(); ++i)
	{
		if (Slots[i].InstanceId == InstanceId)
		{
			// Cannot remove more than is in this specific stack
			if (Slots[i].Quantity < Quantity)
			{
				return false;
			}

			Slots[i].Quantity -= Quantity;

			FShadowSlaveItemInstance RemovedInfo = Slots[i];
			RemovedInfo.Quantity = Quantity;

			if (Slots[i].Quantity <= 0)
			{
				Slots.RemoveAt(i);
			}

			OnItemRemoved.Broadcast(RemovedInfo, Quantity);
			OnInventoryChanged.Broadcast();
			return true;
		}
	}

	return false;
}

bool UShadowSlaveInventoryComponent::HasItem(const UShadowSlaveItemDefinition* ItemDef, int32 Quantity) const
{
	if (!ItemDef || Quantity <= 0)
	{
		return false;
	}

	return GetTotalItemCount(ItemDef) >= Quantity;
}

bool UShadowSlaveInventoryComponent::FindItem(const UShadowSlaveItemDefinition* ItemDef, FShadowSlaveItemInstance& OutInstance) const
{
	if (!ItemDef)
	{
		return false;
	}

	for (const FShadowSlaveItemInstance& Slot : Slots)
	{
		if (Slot.ItemDefinition == ItemDef)
		{
			OutInstance = Slot;
			return true;
		}
	}

	return false;
}

int32 UShadowSlaveInventoryComponent::GetTotalItemCount(const UShadowSlaveItemDefinition* ItemDef) const
{
	if (!ItemDef)
	{
		return 0;
	}

	int32 Total = 0;
	for (const FShadowSlaveItemInstance& Slot : Slots)
	{
		if (Slot.ItemDefinition == ItemDef)
		{
			Total += Slot.Quantity;
		}
	}

	return Total;
}

void UShadowSlaveInventoryComponent::ClearInventory()
{
	if (Slots.Num() == 0)
	{
		return;
	}

	Slots.Empty();
	OnInventoryChanged.Broadcast();
}

TArray<FShadowSlaveItemInstance> UShadowSlaveInventoryComponent::GetItemsByType(EShadowSlaveItemType ItemType) const
{
	TArray<FShadowSlaveItemInstance> Filtered;
	for (const FShadowSlaveItemInstance& Slot : Slots)
	{
		if (Slot.ItemDefinition && Slot.ItemDefinition->ItemType == ItemType)
		{
			Filtered.Add(Slot);
		}
	}
	return Filtered;
}

void UShadowSlaveInventoryComponent::LogInventoryContents() const
{
	AActor* OwnerActor = GetOwner();
	const FString OwnerName = OwnerActor ? OwnerActor->GetName() : TEXT("None");

	UE_LOG(LogShadowSlave, Log, TEXT("[%s] Inventory (%d/%d slots occupied):"), *OwnerName, Slots.Num(), MaxSlots);

	if (Slots.Num() == 0)
	{
		UE_LOG(LogShadowSlave, Log, TEXT("  (Empty)"));
		return;
	}

	for (int32 i = 0; i < Slots.Num(); ++i)
	{
		const FShadowSlaveItemInstance& Slot = Slots[i];
		const FString ItemName = Slot.ItemDefinition ? Slot.ItemDefinition->DisplayName.ToString() : TEXT("Invalid");
		const FString Stackable = (Slot.ItemDefinition && Slot.ItemDefinition->bIsStackable) ? TEXT("Stackable") : TEXT("Single");

		UE_LOG(LogShadowSlave, Log, TEXT("  Slot [%d]: %s x%d (%s, GUID: %s)"),
			i,
			*ItemName,
			Slot.Quantity,
			*Stackable,
			*Slot.InstanceId.ToString(EGuidFormats::Short)
		);
	}
}
