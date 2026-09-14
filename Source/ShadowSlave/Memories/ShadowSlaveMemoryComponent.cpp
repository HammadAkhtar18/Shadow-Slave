// Copyright Epic Games, Inc. All Rights Reserved.

#include "Memories/ShadowSlaveMemoryComponent.h"
#include "Items/ShadowSlaveInventoryComponent.h"
#include "Items/ShadowSlaveItemDefinition.h"
#include "ShadowSlave.h"

UShadowSlaveMemoryComponent::UShadowSlaveMemoryComponent()
{
	// Operates event-driven; no tick overhead
	PrimaryComponentTick.bCanEverTick = false;
	bEnforceUniqueSlotEquip = false;
}

bool UShadowSlaveMemoryComponent::AcquireMemory(UShadowSlaveMemoryDefinition* MemoryDef, FShadowSlaveMemoryInstance& OutInstance)
{
	if (!MemoryDef)
	{
		OutInstance = FShadowSlaveMemoryInstance();
		return false;
	}

	FShadowSlaveMemoryInstance NewInstance(MemoryDef);
	Memories.Add(NewInstance);
	OutInstance = NewInstance;

	OnMemoryAdded.Broadcast(NewInstance);
	OnMemoryCollectionChanged.Broadcast();
	return true;
}

bool UShadowSlaveMemoryComponent::AddMemory(UShadowSlaveMemoryDefinition* MemoryDef, FGuid& OutInstanceId)
{
	FShadowSlaveMemoryInstance NewInstance;
	if (AcquireMemory(MemoryDef, NewInstance))
	{
		OutInstanceId = NewInstance.InstanceId;
		return true;
	}

	OutInstanceId.Invalidate();
	return false;
}

bool UShadowSlaveMemoryComponent::AddMemorySimple(UShadowSlaveMemoryDefinition* MemoryDef)
{
	FShadowSlaveMemoryInstance DummyInstance;
	return AcquireMemory(MemoryDef, DummyInstance);
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
			// If currently equipped, unequip before removing ownership
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

bool UShadowSlaveMemoryComponent::DestroyMemory(const FGuid& InstanceId)
{
	if (!InstanceId.IsValid())
	{
		return false;
	}

	for (int32 i = 0; i < Memories.Num(); ++i)
	{
		if (Memories[i].InstanceId == InstanceId)
		{
			if (Memories[i].bIsEquipped)
			{
				UnequipMemory(InstanceId);
			}

			FShadowSlaveMemoryInstance DestroyedInstance = Memories[i];
			Memories.RemoveAt(i);

			OnMemoryDestroyed.Broadcast(DestroyedInstance);
			OnMemoryCollectionChanged.Broadcast();
			return true;
		}
	}

	return false;
}

bool UShadowSlaveMemoryComponent::ConsumeMemory(const FGuid& InstanceId, FShadowSlaveMemoryConsumptionEffect& OutEffectApplied)
{
	if (!InstanceId.IsValid())
	{
		return false;
	}

	for (int32 i = 0; i < Memories.Num(); ++i)
	{
		if (Memories[i].InstanceId == InstanceId)
		{
			if (!Memories[i].MemoryDefinition || !Memories[i].MemoryDefinition->CanBeConsumed())
			{
				return false;
			}

			OutEffectApplied = Memories[i].MemoryDefinition->ConsumptionEffect;

			if (Memories[i].bIsEquipped)
			{
				UnequipMemory(InstanceId);
			}

			FShadowSlaveMemoryInstance ConsumedInstance = Memories[i];
			Memories.RemoveAt(i);

			OnMemoryConsumed.Broadcast(ConsumedInstance, OutEffectApplied);
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

	for (FShadowSlaveMemoryInstance& Instance : Memories)
	{
		if (Instance.bIsEquipped)
		{
			Instance.bIsEquipped = false;
			OnMemoryUnequipped.Broadcast(Instance);
		}
	}

	Memories.Empty();
	OnMemoryCollectionChanged.Broadcast();
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

			// Check configurable slot conflict policy (component setting OR definition requirement)
			const EShadowSlaveEquipmentSlot TargetSlot = Memories[i].MemoryDefinition->EquipmentSlot;
			const bool bNewRequiresExclusivity = bEnforceUniqueSlotEquip || Memories[i].MemoryDefinition->bRequiresExclusiveSlot;

			if (TargetSlot != EShadowSlaveEquipmentSlot::None)
			{
				for (int32 j = 0; j < Memories.Num(); ++j)
				{
					if (j != i && Memories[j].bIsEquipped && Memories[j].MemoryDefinition && Memories[j].MemoryDefinition->EquipmentSlot == TargetSlot)
					{
						const bool bExistingRequiresExclusivity = bEnforceUniqueSlotEquip || Memories[j].MemoryDefinition->bRequiresExclusiveSlot;
						if (bNewRequiresExclusivity || bExistingRequiresExclusivity)
						{
							UnequipMemory(Memories[j].InstanceId);
						}
					}
				}
			}

			Memories[i].bIsEquipped = true;

			OnMemoryEquipped.Broadcast(Memories[i]);
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

			Memories[i].bIsEquipped = false;

			OnMemoryUnequipped.Broadcast(Memories[i]);
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

			OnMemoryStateChanged.Broadcast(Instance, NewState, OldState);
			return true;
		}
	}

	return false;
}

bool UShadowSlaveMemoryComponent::GetMemoryState(const FGuid& InstanceId, EShadowSlaveMemoryState& OutState) const
{
	if (!InstanceId.IsValid())
	{
		OutState = EShadowSlaveMemoryState::Dormant;
		return false;
	}

	for (const FShadowSlaveMemoryInstance& Instance : Memories)
	{
		if (Instance.InstanceId == InstanceId)
		{
			OutState = Instance.State;
			return true;
		}
	}

	OutState = EShadowSlaveMemoryState::Dormant;
	return false;
}

bool UShadowSlaveMemoryComponent::SetMemoryDynamicProperty(const FGuid& InstanceId, FName Key, const FString& Value)
{
	if (!InstanceId.IsValid() || Key.IsNone())
	{
		return false;
	}

	for (FShadowSlaveMemoryInstance& Instance : Memories)
	{
		if (Instance.InstanceId == InstanceId)
		{
			Instance.SetDynamicProperty(Key, Value);
			OnMemoryModified.Broadcast(Instance);
			return true;
		}
	}

	return false;
}

bool UShadowSlaveMemoryComponent::GetMemoryDynamicProperty(const FGuid& InstanceId, FName Key, FString& OutValue) const
{
	if (!InstanceId.IsValid() || Key.IsNone())
	{
		return false;
	}

	for (const FShadowSlaveMemoryInstance& Instance : Memories)
	{
		if (Instance.InstanceId == InstanceId)
		{
			return Instance.GetDynamicProperty(Key, OutValue);
		}
	}

	return false;
}

bool UShadowSlaveMemoryComponent::RemoveMemoryDynamicProperty(const FGuid& InstanceId, FName Key)
{
	if (!InstanceId.IsValid() || Key.IsNone())
	{
		return false;
	}

	for (FShadowSlaveMemoryInstance& Instance : Memories)
	{
		if (Instance.InstanceId == InstanceId)
		{
			if (Instance.RemoveDynamicProperty(Key))
			{
				OnMemoryModified.Broadcast(Instance);
				return true;
			}
			return false;
		}
	}

	return false;
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
		OutInstance = FShadowSlaveMemoryInstance();
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

	OutInstance = FShadowSlaveMemoryInstance();
	return false;
}

bool UShadowSlaveMemoryComponent::FindMemoryByDefinition(const UShadowSlaveMemoryDefinition* MemoryDef, FShadowSlaveMemoryInstance& OutInstance) const
{
	if (!MemoryDef)
	{
		OutInstance = FShadowSlaveMemoryInstance();
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

	OutInstance = FShadowSlaveMemoryInstance();
	return false;
}

TArray<FShadowSlaveMemoryInstance> UShadowSlaveMemoryComponent::FindAllMemoriesByDefinition(const UShadowSlaveMemoryDefinition* MemoryDef) const
{
	TArray<FShadowSlaveMemoryInstance> Matches;
	if (!MemoryDef)
	{
		return Matches;
	}

	for (const FShadowSlaveMemoryInstance& Instance : Memories)
	{
		if (Instance.MemoryDefinition == MemoryDef)
		{
			Matches.Add(Instance);
		}
	}

	return Matches;
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

TArray<FShadowSlaveMemoryInstance> UShadowSlaveMemoryComponent::GetMemoriesByRank(EShadowSlaveMemoryRank Rank) const
{
	TArray<FShadowSlaveMemoryInstance> Filtered;
	for (const FShadowSlaveMemoryInstance& Instance : Memories)
	{
		if (Instance.MemoryDefinition && Instance.MemoryDefinition->Rank == Rank)
		{
			Filtered.Add(Instance);
		}
	}
	return Filtered;
}

TArray<FShadowSlaveMemoryInstance> UShadowSlaveMemoryComponent::GetMemoriesByTier(EShadowSlaveMemoryTier Tier) const
{
	TArray<FShadowSlaveMemoryInstance> Filtered;
	for (const FShadowSlaveMemoryInstance& Instance : Memories)
	{
		if (Instance.MemoryDefinition && Instance.MemoryDefinition->Tier == Tier)
		{
			Filtered.Add(Instance);
		}
	}
	return Filtered;
}

TArray<FShadowSlaveMemoryInstance> UShadowSlaveMemoryComponent::GetMemoriesWithKnownRank() const
{
	TArray<FShadowSlaveMemoryInstance> Filtered;
	for (const FShadowSlaveMemoryInstance& Instance : Memories)
	{
		if (Instance.MemoryDefinition && Instance.MemoryDefinition->HasKnownRank())
		{
			Filtered.Add(Instance);
		}
	}
	return Filtered;
}

TArray<FShadowSlaveMemoryInstance> UShadowSlaveMemoryComponent::GetMemoriesWithKnownTier() const
{
	TArray<FShadowSlaveMemoryInstance> Filtered;
	for (const FShadowSlaveMemoryInstance& Instance : Memories)
	{
		if (Instance.MemoryDefinition && Instance.MemoryDefinition->HasKnownTier())
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

TArray<FShadowSlaveMemoryInstance> UShadowSlaveMemoryComponent::GetEquippedMemoriesBySlot(EShadowSlaveEquipmentSlot Slot) const
{
	TArray<FShadowSlaveMemoryInstance> SlotMemories;
	if (Slot == EShadowSlaveEquipmentSlot::None)
	{
		return SlotMemories;
	}

	for (const FShadowSlaveMemoryInstance& Instance : Memories)
	{
		if (Instance.bIsEquipped && Instance.MemoryDefinition && Instance.MemoryDefinition->EquipmentSlot == Slot)
		{
			SlotMemories.Add(Instance);
		}
	}
	return SlotMemories;
}

bool UShadowSlaveMemoryComponent::GetEquippedMemoryInSlot(EShadowSlaveEquipmentSlot Slot, FShadowSlaveMemoryInstance& OutInstance) const
{
	OutInstance = FShadowSlaveMemoryInstance();
	if (Slot == EShadowSlaveEquipmentSlot::None)
	{
		return false;
	}

	for (const FShadowSlaveMemoryInstance& Instance : Memories)
	{
		if (Instance.bIsEquipped && Instance.MemoryDefinition && Instance.MemoryDefinition->EquipmentSlot == Slot)
		{
			OutInstance = Instance;
			return true;
		}
	}
	return false;
}

bool UShadowSlaveMemoryComponent::IsSlotOccupied(EShadowSlaveEquipmentSlot Slot) const
{
	if (Slot == EShadowSlaveEquipmentSlot::None)
	{
		return false;
	}

	for (const FShadowSlaveMemoryInstance& Instance : Memories)
	{
		if (Instance.bIsEquipped && Instance.MemoryDefinition && Instance.MemoryDefinition->EquipmentSlot == Slot)
		{
			return true;
		}
	}
	return false;
}

bool UShadowSlaveMemoryComponent::CanConsumeMemory(const FGuid& InstanceId) const
{
	FShadowSlaveMemoryInstance FoundInstance;
	if (FindMemory(InstanceId, FoundInstance) && FoundInstance.MemoryDefinition)
	{
		return FoundInstance.MemoryDefinition->CanBeConsumed();
	}
	return false;
}

bool UShadowSlaveMemoryComponent::GetMemoryConsumptionEffect(const FGuid& InstanceId, FShadowSlaveMemoryConsumptionEffect& OutEffect) const
{
	FShadowSlaveMemoryInstance FoundInstance;
	if (FindMemory(InstanceId, FoundInstance) && FoundInstance.MemoryDefinition && FoundInstance.MemoryDefinition->CanBeConsumed())
	{
		OutEffect = FoundInstance.MemoryDefinition->ConsumptionEffect;
		return true;
	}
	return false;
}

bool UShadowSlaveMemoryComponent::TransferToInventory(const FGuid& InstanceId, UShadowSlaveInventoryComponent* TargetInventory, int32& OutRemainder)
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

	// Attempt transfer into target inventory
	const bool bAdded = TargetInventory->AddItem(ItemDef, 1, OutRemainder);
	if (bAdded && OutRemainder == 0)
	{
		// Broadcast transfer hook before removing from authoritative Memory ownership
		OnMemoryTransferred.Broadcast(FoundInstance, TargetInventory);

		// Atomically remove from MemoryComponent to maintain single authoritative ownership
		RemoveMemory(InstanceId);
		return true;
	}

	return false;
}

void UShadowSlaveMemoryComponent::LogMemoryContents() const
{
	AActor* OwnerActor = GetOwner();
	const FString OwnerName = OwnerActor ? OwnerActor->GetName() : TEXT("None");

	UE_LOG(LogShadowSlave, Log, TEXT("[%s] Memory Component (%d memories held):"), *OwnerName, Memories.Num());

	if (Memories.Num() == 0)
	{
		UE_LOG(LogShadowSlave, Log, TEXT("  (Empty)"));
		return;
	}

	for (int32 i = 0; i < Memories.Num(); ++i)
	{
		const FShadowSlaveMemoryInstance& Instance = Memories[i];
		const FString MemName = Instance.MemoryDefinition ? Instance.MemoryDefinition->DisplayName.ToString() : TEXT("Invalid");
		const FString RankStr = Instance.MemoryDefinition ? UEnum::GetValueAsString(Instance.MemoryDefinition->Rank) : TEXT("Unknown");
		const FString TierStr = Instance.MemoryDefinition ? UEnum::GetValueAsString(Instance.MemoryDefinition->Tier) : TEXT("Unknown");
		const int32 EnchantmentCount = Instance.GetTotalEnchantmentCount();
		const FString EquippedStr = Instance.bIsEquipped ? TEXT("Equipped") : TEXT("Unequipped");
		const FString StateStr = UEnum::GetValueAsString(Instance.State);

		UE_LOG(LogShadowSlave, Log, TEXT("  Memory [%d]: %s [Rank: %s, Tier: %s, Enchantments: %d] (%s, State: %s, GUID: %s)"),
			i,
			*MemName,
			*RankStr,
			*TierStr,
			EnchantmentCount,
			*EquippedStr,
			*StateStr,
			*Instance.InstanceId.ToString(EGuidFormats::Short)
		);
	}
}
