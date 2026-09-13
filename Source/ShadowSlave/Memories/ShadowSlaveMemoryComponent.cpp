// Copyright Epic Games, Inc. All Rights Reserved.

#include "Memories/ShadowSlaveMemoryComponent.h"
#include "Items/ShadowSlaveInventoryComponent.h"
#include "Items/ShadowSlaveItemDefinition.h"
#include "ShadowSlave.h"

UShadowSlaveMemoryComponent::UShadowSlaveMemoryComponent()
{
	// Operates event-driven; no tick overhead
	PrimaryComponentTick.bCanEverTick = false;
}

bool UShadowSlaveMemoryComponent::AddMemory(UShadowSlaveMemoryDefinition* MemoryDef, FGuid& OutInstanceId)
{
	if (!MemoryDef)
	{
		OutInstanceId.Invalidate();
		return false;
	}

	FShadowSlaveMemoryInstance NewInstance(MemoryDef);
	OutInstanceId = NewInstance.InstanceId;

	Memories.Add(NewInstance);

	OnMemoryAdded.Broadcast(NewInstance);
	OnMemoryCollectionChanged.Broadcast();
	return true;
}

bool UShadowSlaveMemoryComponent::AddMemorySimple(UShadowSlaveMemoryDefinition* MemoryDef)
{
	FGuid DummyGuid;
	return AddMemory(MemoryDef, DummyGuid);
}

bool UShadowSlaveMemoryComponent::AddMemoryInstance(const FShadowSlaveMemoryInstance& InInstance)
{
	if (!InInstance.IsValid())
	{
		return false;
	}

	// Ensure no duplicate GUID in memory storage
	for (const FShadowSlaveMemoryInstance& Existing : Memories)
	{
		if (Existing.InstanceId == InInstance.InstanceId)
		{
			return false;
		}
	}

	Memories.Add(InInstance);

	OnMemoryAdded.Broadcast(InInstance);
	OnMemoryCollectionChanged.Broadcast();
	return true;
}

bool UShadowSlaveMemoryComponent::RemoveMemory(const FGuid& InstanceId)
{
	if (!InstanceId.IsValid())
	{
		return false;
	}

	for (int32 i = 0; i < Memories.Num(); ++i)
	{
		if (Memories[i].InstanceId == InstanceId)
		{
			// If currently equipped, unequip before removing
			if (Memories[i].bIsEquipped)
			{
				UnequipMemory(InstanceId);
			}

			FShadowSlaveMemoryInstance RemovedInstance = Memories[i];
			Memories.RemoveAt(i);

			OnMemoryRemoved.Broadcast(RemovedInstance);
			OnMemoryCollectionChanged.Broadcast();
			return true;
		}
	}

	return false;
}

bool UShadowSlaveMemoryComponent::RemoveMemoryByDefinition(const UShadowSlaveMemoryDefinition* MemoryDef)
{
	if (!MemoryDef)
	{
		return false;
	}

	for (int32 i = 0; i < Memories.Num(); ++i)
	{
		if (Memories[i].MemoryDefinition == MemoryDef)
		{
			return RemoveMemory(Memories[i].InstanceId);
		}
	}

	return false;
}

bool UShadowSlaveMemoryComponent::EquipMemory(const FGuid& InstanceId)
{
	if (!InstanceId.IsValid())
	{
		return false;
	}

	for (int32 i = 0; i < Memories.Num(); ++i)
	{
		if (Memories[i].InstanceId == InstanceId)
		{
			if (!Memories[i].MemoryDefinition || !Memories[i].MemoryDefinition->bCanBeEquipped)
			{
				return false;
			}

			if (Memories[i].bIsEquipped)
			{
				return false; // Already equipped
			}

			// If this Memory occupies a distinct non-None equipment slot, unequip any existing occupant of that slot
			const EShadowSlaveEquipmentSlot TargetSlot = Memories[i].MemoryDefinition->EquipmentSlot;
			if (TargetSlot != EShadowSlaveEquipmentSlot::None)
			{
				for (int32 j = 0; j < Memories.Num(); ++j)
				{
					if (j != i && Memories[j].bIsEquipped && Memories[j].MemoryDefinition && Memories[j].MemoryDefinition->EquipmentSlot == TargetSlot)
					{
						UnequipMemory(Memories[j].InstanceId);
						break;
					}
				}
			}

			const EShadowSlaveMemoryState OldState = Memories[i].State;
			Memories[i].bIsEquipped = true;
			Memories[i].State = EShadowSlaveMemoryState::Summoned;

			OnMemoryEquipped.Broadcast(Memories[i]);
			OnMemoryStateChanged.Broadcast(Memories[i], EShadowSlaveMemoryState::Summoned, OldState);
			OnMemoryCollectionChanged.Broadcast();
			return true;
		}
	}

	return false;
}

bool UShadowSlaveMemoryComponent::UnequipMemory(const FGuid& InstanceId)
{
	if (!InstanceId.IsValid())
	{
		return false;
	}

	for (int32 i = 0; i < Memories.Num(); ++i)
	{
		if (Memories[i].InstanceId == InstanceId)
		{
			if (!Memories[i].bIsEquipped)
			{
				return false; // Not equipped
			}

			const EShadowSlaveMemoryState OldState = Memories[i].State;
			Memories[i].bIsEquipped = false;
			Memories[i].State = EShadowSlaveMemoryState::Dormant;

			OnMemoryUnequipped.Broadcast(Memories[i]);
			OnMemoryStateChanged.Broadcast(Memories[i], EShadowSlaveMemoryState::Dormant, OldState);
			OnMemoryCollectionChanged.Broadcast();
			return true;
		}
	}

	return false;
}

bool UShadowSlaveMemoryComponent::SetMemoryState(const FGuid& InstanceId, EShadowSlaveMemoryState NewState)
{
	if (!InstanceId.IsValid())
	{
		return false;
	}

	for (FShadowSlaveMemoryInstance& Instance : Memories)
	{
		if (Instance.InstanceId == InstanceId)
		{
			if (Instance.State == NewState)
			{
				return true;
			}

			const EShadowSlaveMemoryState OldState = Instance.State;
			Instance.State = NewState;

			if (NewState == EShadowSlaveMemoryState::Dormant)
			{
				Instance.bIsEquipped = false;
			}
			else if (NewState == EShadowSlaveMemoryState::Summoned)
			{
				Instance.bIsEquipped = true;
			}

			OnMemoryStateChanged.Broadcast(Instance, NewState, OldState);
			OnMemoryCollectionChanged.Broadcast();
			return true;
		}
	}

	return false;
}

void UShadowSlaveMemoryComponent::ClearMemories()
{
	if (Memories.Num() == 0)
	{
		return;
	}

	Memories.Empty();
	OnMemoryCollectionChanged.Broadcast();
}

bool UShadowSlaveMemoryComponent::HasMemory(const UShadowSlaveMemoryDefinition* MemoryDef) const
{
	if (!MemoryDef)
	{
		return false;
	}

	for (const FShadowSlaveMemoryInstance& Instance : Memories)
	{
		if (Instance.MemoryDefinition == MemoryDef)
		{
			return true;
		}
	}

	return false;
}

bool UShadowSlaveMemoryComponent::HasMemoryByInstanceId(const FGuid& InstanceId) const
{
	if (!InstanceId.IsValid())
	{
		return false;
	}

	for (const FShadowSlaveMemoryInstance& Instance : Memories)
	{
		if (Instance.InstanceId == InstanceId)
		{
			return true;
		}
	}

	return false;
}

bool UShadowSlaveMemoryComponent::FindMemory(const FGuid& InstanceId, FShadowSlaveMemoryInstance& OutInstance) const
{
	if (!InstanceId.IsValid())
	{
		return false;
	}

	for (const FShadowSlaveMemoryInstance& Instance : Memories)
	{
		if (Instance.InstanceId == InstanceId)
		{
			OutInstance = Instance;
			return true;
		}
	}

	return false;
}

bool UShadowSlaveMemoryComponent::FindMemoryByDefinition(const UShadowSlaveMemoryDefinition* MemoryDef, FShadowSlaveMemoryInstance& OutInstance) const
{
	if (!MemoryDef)
	{
		return false;
	}

	for (const FShadowSlaveMemoryInstance& Instance : Memories)
	{
		if (Instance.MemoryDefinition == MemoryDef)
		{
			OutInstance = Instance;
			return true;
		}
	}

	return false;
}

bool UShadowSlaveMemoryComponent::IsMemoryEquipped(const FGuid& InstanceId) const
{
	if (!InstanceId.IsValid())
	{
		return false;
	}

	for (const FShadowSlaveMemoryInstance& Instance : Memories)
	{
		if (Instance.InstanceId == InstanceId)
		{
			return Instance.bIsEquipped;
		}
	}

	return false;
}

TArray<FShadowSlaveMemoryInstance> UShadowSlaveMemoryComponent::GetMemoriesByCategory(EShadowSlaveMemoryCategory Category) const
{
	TArray<FShadowSlaveMemoryInstance> Filtered;
	for (const FShadowSlaveMemoryInstance& Instance : Memories)
	{
		if (Instance.MemoryDefinition && Instance.MemoryDefinition->Category == Category)
		{
			Filtered.Add(Instance);
		}
	}
	return Filtered;
}

TArray<FShadowSlaveMemoryInstance> UShadowSlaveMemoryComponent::GetEquippedMemories() const
{
	TArray<FShadowSlaveMemoryInstance> Equipped;
	for (const FShadowSlaveMemoryInstance& Instance : Memories)
	{
		if (Instance.bIsEquipped)
		{
			Equipped.Add(Instance);
		}
	}
	return Equipped;
}

bool UShadowSlaveMemoryComponent::BridgeToInventory(const FGuid& InstanceId, UShadowSlaveInventoryComponent* TargetInventory, int32& OutRemainder)
{
	OutRemainder = 0;
	if (!TargetInventory || !InstanceId.IsValid())
	{
		return false;
	}

	FShadowSlaveMemoryInstance FoundInstance;
	if (!FindMemory(InstanceId, FoundInstance) || !FoundInstance.MemoryDefinition)
	{
		return false;
	}

	UShadowSlaveItemDefinition* ItemDef = FoundInstance.MemoryDefinition->AssociatedItemDefinition.LoadSynchronous();
	if (!ItemDef)
	{
		return false;
	}

	return TargetInventory->AddItem(ItemDef, 1, OutRemainder);
}

void UShadowSlaveMemoryComponent::LogMemoryContents() const
{
	AActor* OwnerActor = GetOwner();
	const FString OwnerName = OwnerActor ? OwnerActor->GetName() : TEXT("None");

	UE_LOG(LogShadowSlave, Log, TEXT("[%s] Memory Component (%d memories stored):"), *OwnerName, Memories.Num());

	if (Memories.Num() == 0)
	{
		UE_LOG(LogShadowSlave, Log, TEXT("  (Empty)"));
		return;
	}

	for (int32 i = 0; i < Memories.Num(); ++i)
	{
		const FShadowSlaveMemoryInstance& Instance = Memories[i];
		const FString MemName = Instance.MemoryDefinition ? Instance.MemoryDefinition->DisplayName.ToString() : TEXT("Invalid");
		const FString EquippedStr = Instance.bIsEquipped ? TEXT("Equipped/Summoned") : TEXT("Dormant");
		const FString StateStr = UEnum::GetValueAsString(Instance.State);

		UE_LOG(LogShadowSlave, Log, TEXT("  Memory [%d]: %s (%s, State: %s, GUID: %s)"),
			i,
			*MemName,
			*EquippedStr,
			*StateStr,
			*Instance.InstanceId.ToString(EGuidFormats::Short)
		);
	}
}
