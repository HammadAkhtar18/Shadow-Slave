// Copyright Epic Games, Inc. All Rights Reserved.

#include "Equipment/ShadowSlaveEquipmentComponent.h"
#include "Attributes/ShadowSlaveAttributeComponent.h"
#include "Items/ShadowSlaveInventoryComponent.h"
#include "Items/ShadowSlaveItemDefinition.h"
#include "Memories/ShadowSlaveMemoryComponent.h"
#include "Memories/ShadowSlaveMemoryDefinition.h"
#include "Core/ShadowSlaveLogChannels.h"

UShadowSlaveEquipmentComponent::UShadowSlaveEquipmentComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UShadowSlaveEquipmentComponent::BeginPlay()
{
	Super::BeginPlay();

	BindToCompanionComponents();
}

void UShadowSlaveEquipmentComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindFromCompanionComponents();

	Super::EndPlay(EndPlayReason);
}

void UShadowSlaveEquipmentComponent::BindToCompanionComponents()
{
	if (UShadowSlaveInventoryComponent* InvComp = GetInventoryComponent())
	{
		InvComp->OnItemRemoved.AddUniqueDynamic(this, &UShadowSlaveEquipmentComponent::HandleInventoryItemRemoved);
		InvComp->OnInventoryChanged.AddUniqueDynamic(this, &UShadowSlaveEquipmentComponent::HandleInventoryChanged);
	}

	if (UShadowSlaveMemoryComponent* MemComp = GetMemoryComponent())
	{
		MemComp->OnMemoryRemoved.AddUniqueDynamic(this, &UShadowSlaveEquipmentComponent::HandleMemoryRemoved);
		MemComp->OnMemoryDestroyed.AddUniqueDynamic(this, &UShadowSlaveEquipmentComponent::HandleMemoryDestroyed);
		MemComp->OnMemoryConsumed.AddUniqueDynamic(this, &UShadowSlaveEquipmentComponent::HandleMemoryConsumed);
		MemComp->OnMemoryEquipped.AddUniqueDynamic(this, &UShadowSlaveEquipmentComponent::HandleMemoryEquippedExternally);
		MemComp->OnMemoryUnequipped.AddUniqueDynamic(this, &UShadowSlaveEquipmentComponent::HandleMemoryUnequippedExternally);
		MemComp->OnMemoryCollectionChanged.AddUniqueDynamic(this, &UShadowSlaveEquipmentComponent::HandleMemoryCollectionChanged);
	}
}

void UShadowSlaveEquipmentComponent::UnbindFromCompanionComponents()
{
	if (UShadowSlaveInventoryComponent* InvComp = GetInventoryComponent())
	{
		InvComp->OnItemRemoved.RemoveDynamic(this, &UShadowSlaveEquipmentComponent::HandleInventoryItemRemoved);
		InvComp->OnInventoryChanged.RemoveDynamic(this, &UShadowSlaveEquipmentComponent::HandleInventoryChanged);
	}

	if (UShadowSlaveMemoryComponent* MemComp = GetMemoryComponent())
	{
		MemComp->OnMemoryRemoved.RemoveDynamic(this, &UShadowSlaveEquipmentComponent::HandleMemoryRemoved);
		MemComp->OnMemoryDestroyed.RemoveDynamic(this, &UShadowSlaveEquipmentComponent::HandleMemoryDestroyed);
		MemComp->OnMemoryConsumed.RemoveDynamic(this, &UShadowSlaveEquipmentComponent::HandleMemoryConsumed);
		MemComp->OnMemoryEquipped.RemoveDynamic(this, &UShadowSlaveEquipmentComponent::HandleMemoryEquippedExternally);
		MemComp->OnMemoryUnequipped.RemoveDynamic(this, &UShadowSlaveEquipmentComponent::HandleMemoryUnequippedExternally);
		MemComp->OnMemoryCollectionChanged.RemoveDynamic(this, &UShadowSlaveEquipmentComponent::HandleMemoryCollectionChanged);
	}
}

UShadowSlaveAttributeComponent* UShadowSlaveEquipmentComponent::GetAttributeComponent() const
{
	return GetOwner() ? GetOwner()->FindComponentByClass<UShadowSlaveAttributeComponent>() : nullptr;
}

UShadowSlaveInventoryComponent* UShadowSlaveEquipmentComponent::GetInventoryComponent() const
{
	return GetOwner() ? GetOwner()->FindComponentByClass<UShadowSlaveInventoryComponent>() : nullptr;
}

UShadowSlaveMemoryComponent* UShadowSlaveEquipmentComponent::GetMemoryComponent() const
{
	return GetOwner() ? GetOwner()->FindComponentByClass<UShadowSlaveMemoryComponent>() : nullptr;
}

bool UShadowSlaveEquipmentComponent::EquipItem(const FGuid& InstanceId, EShadowSlaveEquipmentSlot Slot)
{
	// 1. Validation phase (no mutation of existing equipment)
	if (!InstanceId.IsValid())
	{
		return false;
	}

	UShadowSlaveInventoryComponent* InvComp = GetInventoryComponent();
	if (!InvComp)
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("[EquipmentComponent] Cannot equip item: Owner '%s' has no InventoryComponent"), *GetNameSafe(GetOwner()));
		return false;
	}

	FShadowSlaveItemInstance ItemInstance;
	if (!InvComp->FindItemByInstanceId(InstanceId, ItemInstance) || !ItemInstance.IsValid())
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("[EquipmentComponent] Cannot equip item: InstanceId '%s' not found in inventory"), *InstanceId.ToString(EGuidFormats::Short));
		return false;
	}

	if (!ItemInstance.ItemDefinition)
	{
		return false;
	}

	const EShadowSlaveEquipmentSlot TargetSlot = (Slot != EShadowSlaveEquipmentSlot::None)
		? Slot
		: ItemInstance.ItemDefinition->EquipmentSlot;

	if (TargetSlot == EShadowSlaveEquipmentSlot::None)
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("[EquipmentComponent] Cannot equip item '%s': No valid equipment slot specified or defined"), *ItemInstance.ItemDefinition->DisplayName.ToString());
		return false;
	}

	// Validate attribute component availability if item grants modifiers
	if (ItemInstance.ItemDefinition->GrantedModifiers.Num() > 0 && !GetAttributeComponent())
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("[EquipmentComponent] Cannot equip item '%s': Item grants modifiers but owner has no AttributeComponent"), *ItemInstance.ItemDefinition->DisplayName.ToString());
		return false;
	}

	// Idempotency check: already equipped in this exact slot
	if (const FShadowSlaveEquippedItem* ExistingInTarget = EquippedSlots.Find(TargetSlot))
	{
		if (ExistingInTarget->InstanceId == InstanceId && ExistingInTarget->SourceType == EShadowSlaveEquipmentSourceType::Item)
		{
			return true;
		}
	}

	// Snapshot existing state for atomic transition & potential rollback
	const EShadowSlaveEquipmentSlot ExistingSlot = GetSlotForInstance(InstanceId);
	const bool bHasExistingSlot = (ExistingSlot != EShadowSlaveEquipmentSlot::None && ExistingSlot != TargetSlot);
	FShadowSlaveEquippedItem ExistingSlotOccupant;
	if (bHasExistingSlot)
	{
		ExistingSlotOccupant = EquippedSlots[ExistingSlot];
	}

	const bool bTargetSlotOccupied = IsSlotOccupied(TargetSlot);
	FShadowSlaveEquippedItem OldTargetOccupant;
	if (bTargetSlotOccupied)
	{
		OldTargetOccupant = EquippedSlots[TargetSlot];
	}

	// 2. Establish new equipment modifiers
	if (!ApplyModifiersForSource(InstanceId, ItemInstance.ItemDefinition->GrantedModifiers))
	{
		RemoveModifiersForSource(InstanceId);
		return false;
	}

	// 3. New equipment established successfully: commit slot changes and retire old occupants
	if (bHasExistingSlot)
	{
		EquippedSlots.Remove(ExistingSlot);
		OnEquipmentItemUnequipped.Broadcast(ExistingSlot, ExistingSlotOccupant);
		OnEquipmentSlotChanged.Broadcast(ExistingSlot);
	}

	if (bTargetSlotOccupied && OldTargetOccupant.InstanceId != InstanceId)
	{
		if (OldTargetOccupant.SourceType == EShadowSlaveEquipmentSourceType::Memory)
		{
			if (UShadowSlaveMemoryComponent* MemComp = GetMemoryComponent())
			{
				if (MemComp->IsMemoryEquipped(OldTargetOccupant.InstanceId))
				{
					TGuardValue<bool> SyncGuard(bIsSyncingWithMemoryComponent, true);
					MemComp->UnequipMemory(OldTargetOccupant.InstanceId);
				}
			}
		}

		RemoveModifiersForSource(OldTargetOccupant.InstanceId);
		OnEquipmentItemUnequipped.Broadcast(TargetSlot, OldTargetOccupant);
	}

	const FName DefId = ItemInstance.ItemDefinition->GetPrimaryAssetId().IsValid()
		? ItemInstance.ItemDefinition->GetPrimaryAssetId().PrimaryAssetName
		: ItemInstance.ItemDefinition->GetFName();

	FShadowSlaveEquippedItem NewEquipped(TargetSlot, EShadowSlaveEquipmentSourceType::Item, InstanceId, DefId);
	EquippedSlots.Add(TargetSlot, NewEquipped);

	OnEquipmentItemEquipped.Broadcast(TargetSlot, NewEquipped);
	OnEquipmentSlotChanged.Broadcast(TargetSlot);
	OnEquipmentChanged.Broadcast();

	UE_LOG(LogShadowSlave, Log, TEXT("[EquipmentComponent] Equipped Item '%s' in slot '%d' on '%s'"),
		*DefId.ToString(), static_cast<int32>(TargetSlot), *GetNameSafe(GetOwner()));

	return true;
}

bool UShadowSlaveEquipmentComponent::EquipItemInstance(const FShadowSlaveItemInstance& ItemInstance, EShadowSlaveEquipmentSlot Slot)
{
	return EquipItem(ItemInstance.InstanceId, Slot);
}

bool UShadowSlaveEquipmentComponent::EquipMemory(const FGuid& InstanceId, EShadowSlaveEquipmentSlot Slot)
{
	// 1. Validation phase (no mutation of existing equipment)
	if (!InstanceId.IsValid())
	{
		return false;
	}

	UShadowSlaveMemoryComponent* MemComp = GetMemoryComponent();
	if (!MemComp)
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("[EquipmentComponent] Cannot equip Memory: Owner '%s' has no MemoryComponent"), *GetNameSafe(GetOwner()));
		return false;
	}

	FShadowSlaveMemoryInstance MemInstance;
	if (!MemComp->FindMemory(InstanceId, MemInstance) || !MemInstance.IsValid())
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("[EquipmentComponent] Cannot equip Memory: InstanceId '%s' not found"), *InstanceId.ToString(EGuidFormats::Short));
		return false;
	}

	if (!MemInstance.MemoryDefinition || !MemInstance.MemoryDefinition->bCanBeEquipped)
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("[EquipmentComponent] Cannot equip Memory '%s': Definition does not allow equipment"), *MemInstance.MemoryDefinition->DisplayName.ToString());
		return false;
	}

	const EShadowSlaveEquipmentSlot TargetSlot = (Slot != EShadowSlaveEquipmentSlot::None)
		? Slot
		: MemInstance.MemoryDefinition->EquipmentSlot;

	if (TargetSlot == EShadowSlaveEquipmentSlot::None)
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("[EquipmentComponent] Cannot equip Memory '%s': No valid equipment slot specified or defined"), *MemInstance.MemoryDefinition->DisplayName.ToString());
		return false;
	}

	// Validate attribute component availability if Memory grants modifiers
	if (MemInstance.MemoryDefinition->GrantedModifiers.Num() > 0 && !GetAttributeComponent())
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("[EquipmentComponent] Cannot equip Memory '%s': Memory grants modifiers but owner has no AttributeComponent"), *MemInstance.MemoryDefinition->DisplayName.ToString());
		return false;
	}

	// Idempotency check: already equipped in this exact slot
	if (const FShadowSlaveEquippedItem* ExistingInTarget = EquippedSlots.Find(TargetSlot))
	{
		if (ExistingInTarget->InstanceId == InstanceId && ExistingInTarget->SourceType == EShadowSlaveEquipmentSourceType::Memory)
		{
			return true;
		}
	}

	// Snapshot existing state for atomic transition & potential rollback
	const EShadowSlaveEquipmentSlot ExistingSlot = GetSlotForInstance(InstanceId);
	const bool bHasExistingSlot = (ExistingSlot != EShadowSlaveEquipmentSlot::None && ExistingSlot != TargetSlot);
	FShadowSlaveEquippedItem ExistingSlotOccupant;
	if (bHasExistingSlot)
	{
		ExistingSlotOccupant = EquippedSlots[ExistingSlot];
	}

	const bool bTargetSlotOccupied = IsSlotOccupied(TargetSlot);
	FShadowSlaveEquippedItem OldTargetOccupant;
	bool bOldMemoryWasEquippedInMemComp = false;
	if (bTargetSlotOccupied)
	{
		OldTargetOccupant = EquippedSlots[TargetSlot];
		if (OldTargetOccupant.SourceType == EShadowSlaveEquipmentSourceType::Memory)
		{
			bOldMemoryWasEquippedInMemComp = MemComp->IsMemoryEquipped(OldTargetOccupant.InstanceId);
		}
	}

	const bool bNewMemoryAlreadyEquippedInMemComp = MemComp->IsMemoryEquipped(InstanceId);

	// 2. Synchronize with MemoryComponent
	bool bMemoryEquippedInMemCompByUs = false;
	if (!bNewMemoryAlreadyEquippedInMemComp)
	{
		TGuardValue<bool> SyncGuard(bIsSyncingWithMemoryComponent, true);
		if (!MemComp->EquipMemory(InstanceId))
		{
			UE_LOG(LogShadowSlave, Warning, TEXT("[EquipmentComponent] MemoryComponent rejected equip for Memory '%s'"), *InstanceId.ToString(EGuidFormats::Short));
			return false;
		}
		bMemoryEquippedInMemCompByUs = true;
	}

	// 3. Establish new equipment modifiers
	if (!ApplyModifiersForSource(InstanceId, MemInstance.MemoryDefinition->GrantedModifiers))
	{
		// Rollback MemoryComponent synchronization for new memory
		if (bMemoryEquippedInMemCompByUs)
		{
			TGuardValue<bool> SyncGuard(bIsSyncingWithMemoryComponent, true);
			MemComp->UnequipMemory(InstanceId);
		}

		// If MemoryComponent automatically unequipped the old memory due to slot conflict, restore it
		if (bTargetSlotOccupied && OldTargetOccupant.SourceType == EShadowSlaveEquipmentSourceType::Memory && bOldMemoryWasEquippedInMemComp && !MemComp->IsMemoryEquipped(OldTargetOccupant.InstanceId))
		{
			TGuardValue<bool> SyncGuard(bIsSyncingWithMemoryComponent, true);
			MemComp->EquipMemory(OldTargetOccupant.InstanceId);
		}

		RemoveModifiersForSource(InstanceId);
		return false;
	}

	// 4. New equipment established successfully: commit slot changes and retire old occupants
	if (bHasExistingSlot)
	{
		EquippedSlots.Remove(ExistingSlot);
		OnEquipmentItemUnequipped.Broadcast(ExistingSlot, ExistingSlotOccupant);
		OnEquipmentSlotChanged.Broadcast(ExistingSlot);
	}

	if (bTargetSlotOccupied && OldTargetOccupant.InstanceId != InstanceId)
	{
		if (OldTargetOccupant.SourceType == EShadowSlaveEquipmentSourceType::Memory)
		{
			if (MemComp->IsMemoryEquipped(OldTargetOccupant.InstanceId))
			{
				TGuardValue<bool> SyncGuard(bIsSyncingWithMemoryComponent, true);
				MemComp->UnequipMemory(OldTargetOccupant.InstanceId);
			}
		}

		RemoveModifiersForSource(OldTargetOccupant.InstanceId);
		OnEquipmentItemUnequipped.Broadcast(TargetSlot, OldTargetOccupant);
	}

	const FName DefId = MemInstance.MemoryDefinition->MemoryId != NAME_None
		? MemInstance.MemoryDefinition->MemoryId
		: MemInstance.MemoryDefinition->GetFName();

	FShadowSlaveEquippedItem NewEquipped(TargetSlot, EShadowSlaveEquipmentSourceType::Memory, InstanceId, DefId);
	EquippedSlots.Add(TargetSlot, NewEquipped);

	OnEquipmentItemEquipped.Broadcast(TargetSlot, NewEquipped);
	OnEquipmentSlotChanged.Broadcast(TargetSlot);
	OnEquipmentChanged.Broadcast();

	UE_LOG(LogShadowSlave, Log, TEXT("[EquipmentComponent] Equipped Memory '%s' in slot '%d' on '%s'"),
		*DefId.ToString(), static_cast<int32>(TargetSlot), *GetNameSafe(GetOwner()));

	return true;
}

bool UShadowSlaveEquipmentComponent::EquipMemoryInstance(const FShadowSlaveMemoryInstance& MemoryInstance, EShadowSlaveEquipmentSlot Slot)
{
	return EquipMemory(MemoryInstance.InstanceId, Slot);
}

bool UShadowSlaveEquipmentComponent::UnequipSlot(EShadowSlaveEquipmentSlot Slot)
{
	if (Slot == EShadowSlaveEquipmentSlot::None)
	{
		return false;
	}

	FShadowSlaveEquippedItem RemovedItem;
	if (!EquippedSlots.RemoveAndCopyValue(Slot, RemovedItem))
	{
		return false;
	}

	// If it was a Memory, ensure MemoryComponent reflects unequipped state
	if (RemovedItem.SourceType == EShadowSlaveEquipmentSourceType::Memory)
	{
		if (UShadowSlaveMemoryComponent* MemComp = GetMemoryComponent())
		{
			if (MemComp->IsMemoryEquipped(RemovedItem.InstanceId))
			{
				TGuardValue<bool> SyncGuard(bIsSyncingWithMemoryComponent, true);
				MemComp->UnequipMemory(RemovedItem.InstanceId);
			}
		}
	}

	// Remove all modifiers originating from this instance
	RemoveModifiersForSource(RemovedItem.InstanceId);

	// Broadcast events
	OnEquipmentItemUnequipped.Broadcast(Slot, RemovedItem);
	OnEquipmentSlotChanged.Broadcast(Slot);
	OnEquipmentChanged.Broadcast();

	UE_LOG(LogShadowSlave, Log, TEXT("[EquipmentComponent] Unequipped slot '%d' on '%s'"),
		static_cast<int32>(Slot), *GetNameSafe(GetOwner()));

	return true;
}

bool UShadowSlaveEquipmentComponent::UnequipInstance(const FGuid& InstanceId)
{
	if (!InstanceId.IsValid())
	{
		return false;
	}

	const EShadowSlaveEquipmentSlot Slot = GetSlotForInstance(InstanceId);
	if (Slot != EShadowSlaveEquipmentSlot::None)
	{
		return UnequipSlot(Slot);
	}

	// Ensure any orphaned modifiers with this SourceId are also wiped
	RemoveModifiersForSource(InstanceId);
	return false;
}

void UShadowSlaveEquipmentComponent::UnequipAll()
{
	if (EquippedSlots.Num() == 0)
	{
		return;
	}

	TArray<EShadowSlaveEquipmentSlot> OccupiedSlots;
	EquippedSlots.GetKeys(OccupiedSlots);

	for (const EShadowSlaveEquipmentSlot Slot : OccupiedSlots)
	{
		UnequipSlot(Slot);
	}
}

bool UShadowSlaveEquipmentComponent::IsSlotOccupied(EShadowSlaveEquipmentSlot Slot) const
{
	if (Slot == EShadowSlaveEquipmentSlot::None)
	{
		return false;
	}
	return EquippedSlots.Contains(Slot);
}

bool UShadowSlaveEquipmentComponent::GetEquippedItemInSlot(EShadowSlaveEquipmentSlot Slot, FShadowSlaveEquippedItem& OutEquippedItem) const
{
	if (const FShadowSlaveEquippedItem* Found = EquippedSlots.Find(Slot))
	{
		OutEquippedItem = *Found;
		return true;
	}
	return false;
}

bool UShadowSlaveEquipmentComponent::IsInstanceEquipped(const FGuid& InstanceId) const
{
	return GetSlotForInstance(InstanceId) != EShadowSlaveEquipmentSlot::None;
}

EShadowSlaveEquipmentSlot UShadowSlaveEquipmentComponent::GetSlotForInstance(const FGuid& InstanceId) const
{
	if (!InstanceId.IsValid())
	{
		return EShadowSlaveEquipmentSlot::None;
	}

	for (const auto& Pair : EquippedSlots)
	{
		if (Pair.Value.InstanceId == InstanceId)
		{
			return Pair.Key;
		}
	}

	return EShadowSlaveEquipmentSlot::None;
}

TArray<FShadowSlaveEquippedItem> UShadowSlaveEquipmentComponent::GetAllEquippedItems() const
{
	TArray<FShadowSlaveEquippedItem> Result;
	EquippedSlots.GenerateValueArray(Result);
	return Result;
}

bool UShadowSlaveEquipmentComponent::GetEquippedItemInstance(EShadowSlaveEquipmentSlot Slot, FShadowSlaveItemInstance& OutInstance) const
{
	FShadowSlaveEquippedItem Equipped;
	if (!GetEquippedItemInSlot(Slot, Equipped) || Equipped.SourceType != EShadowSlaveEquipmentSourceType::Item)
	{
		return false;
	}

	if (UShadowSlaveInventoryComponent* InvComp = GetInventoryComponent())
	{
		return InvComp->FindItemByInstanceId(Equipped.InstanceId, OutInstance);
	}

	return false;
}

bool UShadowSlaveEquipmentComponent::GetEquippedMemoryInstance(EShadowSlaveEquipmentSlot Slot, FShadowSlaveMemoryInstance& OutInstance) const
{
	FShadowSlaveEquippedItem Equipped;
	if (!GetEquippedItemInSlot(Slot, Equipped) || Equipped.SourceType != EShadowSlaveEquipmentSourceType::Memory)
	{
		return false;
	}

	if (UShadowSlaveMemoryComponent* MemComp = GetMemoryComponent())
	{
		return MemComp->FindMemory(Equipped.InstanceId, OutInstance);
	}

	return false;
}

bool UShadowSlaveEquipmentComponent::ApplyModifiersForSource(const FGuid& SourceId, const TArray<FAttributeModifier>& Modifiers)
{
	if (!SourceId.IsValid())
	{
		return false;
	}

	if (Modifiers.Num() == 0)
	{
		return true;
	}

	UShadowSlaveAttributeComponent* AttrComp = GetAttributeComponent();
	if (!AttrComp)
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("[EquipmentComponent] ApplyModifiersForSource failed: Owner '%s' has no AttributeComponent"), *GetNameSafe(GetOwner()));
		return false;
	}

	// Remove any existing modifiers with this SourceId to prevent duplicate application
	AttrComp->RemoveModifiersFromSourceId(SourceId);

	for (int32 Index = 0; Index < Modifiers.Num(); ++Index)
	{
		const FAttributeModifier& BaseMod = Modifiers[Index];

		FAttributeModifier AppliedMod = BaseMod;
		AppliedMod.SourceId = SourceId;
		AppliedMod.Source = GetOwner();

		const FString BaseNameStr = (BaseMod.ModifierId != NAME_None) ? BaseMod.ModifierId.ToString() : TEXT("EquipMod");
		AppliedMod.ModifierId = FName(*FString::Printf(TEXT("%s_%s_%d"), *BaseNameStr, *SourceId.ToString(EGuidFormats::Short), Index));

		AttrComp->AddModifier(AppliedMod);
	}

	return true;
}

void UShadowSlaveEquipmentComponent::RemoveModifiersForSource(const FGuid& SourceId)
{
	if (!SourceId.IsValid())
	{
		return;
	}

	if (UShadowSlaveAttributeComponent* AttrComp = GetAttributeComponent())
	{
		AttrComp->RemoveModifiersFromSourceId(SourceId);
	}
}

void UShadowSlaveEquipmentComponent::HandleInventoryItemRemoved(const FShadowSlaveItemInstance& ItemInstance, int32 QuantityRemoved)
{
	if (!ItemInstance.InstanceId.IsValid())
	{
		return;
	}

	if (IsInstanceEquipped(ItemInstance.InstanceId))
	{
		// Check if the item stack was fully depleted or removed from inventory
		if (UShadowSlaveInventoryComponent* InvComp = GetInventoryComponent())
		{
			FShadowSlaveItemInstance FoundInstance;
			if (!InvComp->FindItemByInstanceId(ItemInstance.InstanceId, FoundInstance) || FoundInstance.Quantity <= 0)
			{
				UnequipInstance(ItemInstance.InstanceId);
			}
		}
		else
		{
			UnequipInstance(ItemInstance.InstanceId);
		}
	}
}

void UShadowSlaveEquipmentComponent::HandleInventoryChanged()
{
	UShadowSlaveInventoryComponent* InvComp = GetInventoryComponent();
	if (!InvComp)
	{
		return;
	}

	TArray<EShadowSlaveEquipmentSlot> SlotsToUnequip;
	for (const auto& Pair : EquippedSlots)
	{
		if (Pair.Value.SourceType == EShadowSlaveEquipmentSourceType::Item)
		{
			if (!InvComp->HasItemByInstanceId(Pair.Value.InstanceId))
			{
				SlotsToUnequip.Add(Pair.Key);
			}
		}
	}

	for (const EShadowSlaveEquipmentSlot Slot : SlotsToUnequip)
	{
		UnequipSlot(Slot);
	}
}

void UShadowSlaveEquipmentComponent::HandleMemoryRemoved(const FShadowSlaveMemoryInstance& MemoryInstance)
{
	if (MemoryInstance.InstanceId.IsValid())
	{
		UnequipInstance(MemoryInstance.InstanceId);
	}
}

void UShadowSlaveEquipmentComponent::HandleMemoryDestroyed(const FShadowSlaveMemoryInstance& MemoryInstance)
{
	if (MemoryInstance.InstanceId.IsValid())
	{
		UnequipInstance(MemoryInstance.InstanceId);
	}
}

void UShadowSlaveEquipmentComponent::HandleMemoryConsumed(const FShadowSlaveMemoryInstance& MemoryInstance, const FShadowSlaveMemoryConsumptionEffect& ConsumedEffect)
{
	if (MemoryInstance.InstanceId.IsValid())
	{
		UnequipInstance(MemoryInstance.InstanceId);
	}
}

void UShadowSlaveEquipmentComponent::HandleMemoryEquippedExternally(const FShadowSlaveMemoryInstance& MemoryInstance)
{
	if (bIsSyncingWithMemoryComponent || !MemoryInstance.InstanceId.IsValid())
	{
		return;
	}

	if (!IsInstanceEquipped(MemoryInstance.InstanceId))
	{
		EquipMemory(MemoryInstance.InstanceId);
	}
}

void UShadowSlaveEquipmentComponent::HandleMemoryUnequippedExternally(const FShadowSlaveMemoryInstance& MemoryInstance)
{
	if (bIsSyncingWithMemoryComponent || !MemoryInstance.InstanceId.IsValid())
	{
		return;
	}

	if (IsInstanceEquipped(MemoryInstance.InstanceId))
	{
		UnequipInstance(MemoryInstance.InstanceId);
	}
}

void UShadowSlaveEquipmentComponent::HandleMemoryCollectionChanged()
{
	UShadowSlaveMemoryComponent* MemComp = GetMemoryComponent();
	if (!MemComp)
	{
		return;
	}

	TArray<EShadowSlaveEquipmentSlot> SlotsToUnequip;
	for (const auto& Pair : EquippedSlots)
	{
		if (Pair.Value.SourceType == EShadowSlaveEquipmentSourceType::Memory)
		{
			if (!MemComp->HasMemoryByInstanceId(Pair.Value.InstanceId))
			{
				SlotsToUnequip.Add(Pair.Key);
			}
		}
	}

	for (const EShadowSlaveEquipmentSlot Slot : SlotsToUnequip)
	{
		UnequipSlot(Slot);
	}
}
