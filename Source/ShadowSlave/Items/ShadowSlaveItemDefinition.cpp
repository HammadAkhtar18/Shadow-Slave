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
	: UShadowSlaveContentDefinition()
{
	ContentId = NAME_None;
	ContentType = EShadowSlaveContentType::Item;
	DisplayName = FText::FromString(TEXT("Generic Item"));
	Description = FText::FromString(TEXT("A generic item prototype."));
	Version = 1;
	ItemType = EShadowSlaveItemType::Miscellaneous;
	EquipmentSlot = EShadowSlaveEquipmentSlot::None;
	bIsStackable = false;
	MaxStackSize = 1;
	Weight = 0.1f;
	BaseValue = 10;
}

FPrimaryAssetId UShadowSlaveItemDefinition::GetPrimaryAssetId() const
{
	return Super::GetPrimaryAssetId();
}

bool UShadowSlaveItemDefinition::IsValidDefinition(FString* OutErrorMessage) const
{
	// 1. Generic content definition validation (valid ContentId, Version >= 1)
	if (!Super::IsValidDefinition(OutErrorMessage))
	{
		return false;
	}

	// 2. Generic content validation: DisplayName must not be empty
	if (DisplayName.IsEmpty())
	{
		if (OutErrorMessage)
		{
			*OutErrorMessage = FString::Printf(TEXT("Item definition '%s' must have a non-empty DisplayName."),
				*ContentId.ToString());
		}
		return false;
	}

	// 3. Generic content type validation: must be Item
	if (ContentType != EShadowSlaveContentType::Item)
	{
		if (OutErrorMessage)
		{
			*OutErrorMessage = FString::Printf(TEXT("Item definition '%s' must have ContentType == EShadowSlaveContentType::Item."),
				*ContentId.ToString());
		}
		return false;
	}

	// 4. Item-specific validation: stack size must be at least 1 if stackable
	if (bIsStackable && MaxStackSize < 1)
	{
		if (OutErrorMessage)
		{
			*OutErrorMessage = FString::Printf(TEXT("Item definition '%s' is stackable but has MaxStackSize < 1 (%d)."),
				*ContentId.ToString(), MaxStackSize);
		}
		return false;
	}

	// 5. Item-specific validation: weight cannot be negative
	if (Weight < 0.0f)
	{
		if (OutErrorMessage)
		{
			*OutErrorMessage = FString::Printf(TEXT("Item definition '%s' has negative Weight (%.2f)."),
				*ContentId.ToString(), Weight);
		}
		return false;
	}

	// 6. Item-specific validation: base value cannot be negative
	if (BaseValue < 0)
	{
		if (OutErrorMessage)
		{
			*OutErrorMessage = FString::Printf(TEXT("Item definition '%s' has negative BaseValue (%d)."),
				*ContentId.ToString(), BaseValue);
		}
		return false;
	}

	return true;
}

UShadowSlaveItemDefinition* UShadowSlaveItemDefinition::CreateTestConsumableDefinition(UObject* Outer)
{
	UObject* EffectiveOuter = Outer ? Outer : GetTransientPackage();
	UShadowSlaveItemDefinition* TestDef = NewObject<UShadowSlaveItemDefinition>(EffectiveOuter, FName(TEXT("TestConsumableItem")));
	if (TestDef)
	{
		TestDef->ContentId = FName(TEXT("TestConsumableItem"));
		TestDef->ContentType = EShadowSlaveContentType::Item;
		TestDef->DisplayName = FText::FromString(TEXT("Generic Test Draught"));
		TestDef->Description = FText::FromString(TEXT("A temporary development consumable for verifying stackable inventory operations."));
		TestDef->Version = 1;
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
		TestDef->ContentId = FName(TEXT("TestQuestItem"));
		TestDef->ContentType = EShadowSlaveContentType::Item;
		TestDef->DisplayName = FText::FromString(TEXT("Generic Test Relic Key"));
		TestDef->Description = FText::FromString(TEXT("A temporary development quest item for verifying unique, non-stackable inventory slots."));
		TestDef->Version = 1;
		TestDef->ItemType = EShadowSlaveItemType::Quest;
		TestDef->EquipmentSlot = EShadowSlaveEquipmentSlot::None;
		TestDef->bIsStackable = false;
		TestDef->MaxStackSize = 1;
		TestDef->Weight = 0.5f;
		TestDef->BaseValue = 0;
	}
	return TestDef;
}
