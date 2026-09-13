// Copyright Epic Games, Inc. All Rights Reserved.

#include "Items/ShadowSlaveItemDefinition.h"

bool FShadowSlaveItemInstance::CanStackWith(const FShadowSlaveItemInstance& Other) const
{
	if (!IsValid() || !Other.IsValid())
	{
		return false;
	}

	if (ItemDefinition != Other.ItemDefinition)
	{
		return false;
	}

	if (!ItemDefinition->bIsStackable)
	{
		return false;
	}

	// Must match dynamic instance properties to safely stack
	if (DynamicProperties.Num() != Other.DynamicProperties.Num())
	{
		return false;
	}

	for (const auto& Pair : DynamicProperties)
	{
		const FString* OtherVal = Other.DynamicProperties.Find(Pair.Key);
		if (!OtherVal || *OtherVal != Pair.Value)
		{
			return false;
		}
	}

	return true;
}

UShadowSlaveItemDefinition::UShadowSlaveItemDefinition()
{
	DisplayName = FText::FromString(TEXT("Generic Item"));
	Description = FText::FromString(TEXT("A generic item prototype."));
	ItemType = EShadowSlaveItemType::Miscellaneous;
	EquipmentSlot = EShadowSlaveEquipmentSlot::None;
	bIsStackable = false;
	MaxStackSize = 1;
	Weight = 0.1f;
	BaseValue = 10;
}

FPrimaryAssetId UShadowSlaveItemDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("Item"), GetFName());
}

UShadowSlaveItemDefinition* UShadowSlaveItemDefinition::CreateTestConsumableDefinition(UObject* Outer)
{
	UObject* EffectiveOuter = Outer ? Outer : GetTransientPackage();
	UShadowSlaveItemDefinition* TestDef = NewObject<UShadowSlaveItemDefinition>(EffectiveOuter, FName(TEXT("TestConsumableItem")));
	if (TestDef)
	{
		TestDef->DisplayName = FText::FromString(TEXT("Generic Test Draught"));
		TestDef->Description = FText::FromString(TEXT("A temporary development consumable for verifying stackable inventory operations."));
		TestDef->ItemType = EShadowSlaveItemType::Consumable;
		TestDef->EquipmentSlot = EShadowSlaveEquipmentSlot::None;
		TestDef->bIsStackable = true;
		TestDef->MaxStackSize = 10;
		TestDef->Weight = 0.25f;
		TestDef->BaseValue = 15;
	}
	return TestDef;
}

UShadowSlaveItemDefinition* UShadowSlaveItemDefinition::CreateTestQuestItemDefinition(UObject* Outer)
{
	UObject* EffectiveOuter = Outer ? Outer : GetTransientPackage();
	UShadowSlaveItemDefinition* TestDef = NewObject<UShadowSlaveItemDefinition>(EffectiveOuter, FName(TEXT("TestQuestItem")));
	if (TestDef)
	{
		TestDef->DisplayName = FText::FromString(TEXT("Generic Test Relic Key"));
		TestDef->Description = FText::FromString(TEXT("A temporary development quest item for verifying unique, non-stackable inventory slots."));
		TestDef->ItemType = EShadowSlaveItemType::Quest;
		TestDef->EquipmentSlot = EShadowSlaveEquipmentSlot::None;
		TestDef->bIsStackable = false;
		TestDef->MaxStackSize = 1;
		TestDef->Weight = 0.5f;
		TestDef->BaseValue = 0;
	}
	return TestDef;
}
