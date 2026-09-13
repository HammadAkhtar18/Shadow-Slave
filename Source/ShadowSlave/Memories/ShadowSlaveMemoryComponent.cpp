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
			const bool bCheckExclusivity = bEnforceUniqueSlotEquip || Memories[i].MemoryDefinition->bRequiresExclusiveSlot;
			const EShadowSlaveEquipmentSlot TargetSlot = Memories[i].MemoryDefinition->EquipmentSlot;

			if (bCheckExclusivity && TargetSlot != EShadowSlaveEquipmentSlot::None)
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
		const int32 EnchantmentCount = Instance.MemoryDefinition ? Instance.MemoryDefinition->GetEnchantmentCount() : 0;
		const FString EquippedStr = Instance.bIsEquipped ? TEXT("Equipped/Summoned") : TEXT("Dormant");
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
