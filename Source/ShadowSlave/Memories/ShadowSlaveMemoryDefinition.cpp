// Copyright Epic Games, Inc. All Rights Reserved.

#include "Memories/ShadowSlaveMemoryDefinition.h"

UShadowSlaveMemoryDefinition::UShadowSlaveMemoryDefinition()
{
	MemoryId = NAME_None;
	DisplayName = FText::FromString(TEXT("Generic Memory"));
	Description = FText::FromString(TEXT("A generic Memory prototype definition."));
	Category = EShadowSlaveMemoryCategory::Miscellaneous;
	bCanBeEquipped = false;
	EquipmentSlot = EShadowSlaveEquipmentSlot::None;
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
		TestDef->Description = FText::FromString(TEXT("A generic development memory asset for verifying lifecycle, equipment, and event operations."));
		TestDef->Category = EShadowSlaveMemoryCategory::Weapon;
		TestDef->bCanBeEquipped = true;
		TestDef->EquipmentSlot = EShadowSlaveEquipmentSlot::Weapon;
		TestDef->bCanBeActivated = true;
		TestDef->ActivationType = EShadowSlaveMemoryActivationType::Active;
		TestDef->BaseEssenceCost = 5.0f;
	}
	return TestDef;
}
