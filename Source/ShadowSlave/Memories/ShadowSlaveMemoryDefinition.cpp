// Copyright Epic Games, Inc. All Rights Reserved.

#include "Memories/ShadowSlaveMemoryDefinition.h"

EShadowSlaveMemoryRank FShadowSlaveMemoryInstance::GetRank() const
{
	return MemoryDefinition ? MemoryDefinition->Rank : EShadowSlaveMemoryRank::Dormant;
}

EShadowSlaveMemoryTier FShadowSlaveMemoryInstance::GetTier() const
{
	return MemoryDefinition ? MemoryDefinition->Tier : EShadowSlaveMemoryTier::Tier1;
}

UShadowSlaveMemoryDefinition::UShadowSlaveMemoryDefinition()
{
	MemoryId = NAME_None;
	DisplayName = FText::FromString(TEXT("Generic Memory"));
	Description = FText::FromString(TEXT("A generic Memory prototype definition."));
	Rank = EShadowSlaveMemoryRank::Dormant;
	Tier = EShadowSlaveMemoryTier::Tier1;
	Category = EShadowSlaveMemoryCategory::Miscellaneous;
	EquipmentSlot = EShadowSlaveEquipmentSlot::None;
	bRequiresExclusiveSlot = false;
	bCanBeEquipped = false;
	bCanBeActivated = false;
	ActivationType = EShadowSlaveMemoryActivationType::Passive;
	BaseEssenceCost = 0.0f;
}

FPrimaryAssetId UShadowSlaveMemoryDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("Memory"), GetFName());
}

UShadowSlaveMemoryDefinition* UShadowSlaveMemoryDefinition::CreateTestMemoryDefinition(UObject* Outer)
{
	UObject* EffectiveOuter = Outer ? Outer : GetTransientPackage();
	UShadowSlaveMemoryDefinition* TestDef = NewObject<UShadowSlaveMemoryDefinition>(EffectiveOuter, FName(TEXT("TestMemoryDefinition")));
	if (TestDef)
	{
		TestDef->MemoryId = FName(TEXT("Test_GenericMemory"));
		TestDef->DisplayName = FText::FromString(TEXT("Generic Test Memory"));
		TestDef->Description = FText::FromString(TEXT("A generic development memory asset for verifying Rank, Tier, multiple enchantments, and lifecycle operations."));
		TestDef->Rank = EShadowSlaveMemoryRank::Awakened;
		TestDef->Tier = EShadowSlaveMemoryTier::Tier1;
		TestDef->Category = EShadowSlaveMemoryCategory::Weapon;
		TestDef->EquipmentSlot = EShadowSlaveEquipmentSlot::Weapon;
		TestDef->bRequiresExclusiveSlot = true;
		TestDef->bCanBeEquipped = true;
		TestDef->bCanBeActivated = true;
		TestDef->ActivationType = EShadowSlaveMemoryActivationType::Active;
		TestDef->BaseEssenceCost = 5.0f;

		// Add multiple development test enchantments (verifying 0 -> N array structure independent of Tier)
		FShadowSlaveMemoryEnchantment PassiveEnchantment(
			FName(TEXT("Enchantment_TestPassive")),
			FText::FromString(TEXT("Test Passive Enhancement")),
			FText::FromString(TEXT("Development test enchantment representing structural or passive reinforcement.")),
			0.0f
		);
		TestDef->Enchantments.Add(PassiveEnchantment);

		FShadowSlaveMemoryEnchantment ActiveEnchantment(
			FName(TEXT("Enchantment_TestActive")),
			FText::FromString(TEXT("Test Active Trigger")),
			FText::FromString(TEXT("Development test enchantment representing an activatable trigger with resource cost.")),
			5.0f
		);
		TestDef->Enchantments.Add(ActiveEnchantment);

		// Consumption effect explicitly non-consumable for baseline test
		TestDef->ConsumptionEffect.bCanBeConsumed = false;
	}
	return TestDef;
}
