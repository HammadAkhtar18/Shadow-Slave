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

UShadowSlaveMemoryDefinition* UShadowSlaveMemoryDefinition::CreateCanonMemoryDefinition(FName CanonMemoryId, UObject* Outer)
{
	UObject* EffectiveOuter = Outer ? Outer : GetTransientPackage();
	UShadowSlaveMemoryDefinition* Def = NewObject<UShadowSlaveMemoryDefinition>(EffectiveOuter, CanonMemoryId);
	if (!Def)
	{
		return nullptr;
	}

	Def->MemoryId = CanonMemoryId;

	if (CanonMemoryId == FName(TEXT("PuppeteersShroud")))
	{
		Def->DisplayName = FText::FromString(TEXT("Puppeteer's Shroud"));
		Def->Description = FText::FromString(TEXT("An unassuming, pitch-black tunic and trousers woven from rough matte silk. Deceptively ordinary attire concealing formidable defensive durability."));
		Def->Rank = EShadowSlaveMemoryRank::Awakened;
		Def->Tier = EShadowSlaveMemoryTier::Tier1;
		Def->Category = EShadowSlaveMemoryCategory::Armor;
		Def->EquipmentSlot = EShadowSlaveEquipmentSlot::Armor;
		Def->bCanBeEquipped = true;
		Def->bRequiresExclusiveSlot = true;

		Def->Enchantments.Add(FShadowSlaveMemoryEnchantment(
			FName(TEXT("Enchantment_Featherlight")),
			FText::FromString(TEXT("Featherlight")),
			FText::FromString(TEXT("The armor possesses virtually zero physical weight, allowing completely unhindered movement and agility.")),
			0.0f
		));
		Def->Enchantments.Add(FShadowSlaveMemoryEnchantment(
			FName(TEXT("Enchantment_Undying")),
			FText::FromString(TEXT("Undying")),
			FText::FromString(TEXT("Provides formidable protection against cuts and piercing trauma, slowly mending tears and gashes over time.")),
			0.0f
		));
		Def->Enchantments.Add(FShadowSlaveMemoryEnchantment(
			FName(TEXT("Enchantment_Inconspicuous")),
			FText::FromString(TEXT("Inconspicuous")),
			FText::FromString(TEXT("Dampens the wearer's presence, making them appear completely ordinary and non-threatening.")),
			0.0f
		));
	}
	else if (CanonMemoryId == FName(TEXT("SilverBell")))
	{
		Def->DisplayName = FText::FromString(TEXT("Silver Bell"));
		Def->Description = FText::FromString(TEXT("A small, polished silver bell that fits comfortably inside a closed fist."));
		Def->Rank = EShadowSlaveMemoryRank::Awakened;
		Def->Tier = EShadowSlaveMemoryTier::Tier1;
		Def->Category = EShadowSlaveMemoryCategory::Charm;
		Def->EquipmentSlot = EShadowSlaveEquipmentSlot::Charm;
		Def->bCanBeEquipped = true;
		Def->bCanBeActivated = true;
		Def->ActivationType = EShadowSlaveMemoryActivationType::Triggered;

		Def->Enchantments.Add(FShadowSlaveMemoryEnchantment(
			FName(TEXT("Enchantment_ClearChime")),
			FText::FromString(TEXT("Clear Chime")),
			FText::FromString(TEXT("Emits an extraordinarily pure ringing tone that effortlessly pierces ambient noise, weather, and distance.")),
			0.0f
		));
		Def->Enchantments.Add(FShadowSlaveMemoryEnchantment(
			FName(TEXT("Enchantment_Attention")),
			FText::FromString(TEXT("Attention")),
			FText::FromString(TEXT("Draws the auditory curiosity and tracking focus of nearby creatures toward the sound source.")),
			0.0f
		));
	}
	else if (CanonMemoryId == FName(TEXT("MidnightShard")))
	{
		Def->DisplayName = FText::FromString(TEXT("Midnight Shard"));
		Def->Description = FText::FromString(TEXT("An elegant, slender curved sword forged from pitch-black metal that absorbs light rather than reflecting it."));
		Def->Rank = EShadowSlaveMemoryRank::Awakened;
		Def->Tier = EShadowSlaveMemoryTier::Tier2;
		Def->Category = EShadowSlaveMemoryCategory::Weapon;
		Def->EquipmentSlot = EShadowSlaveEquipmentSlot::Weapon;
		Def->bCanBeEquipped = true;
		Def->bRequiresExclusiveSlot = true;

		Def->Enchantments.Add(FShadowSlaveMemoryEnchantment(
			FName(TEXT("Enchantment_Unbroken")),
			FText::FromString(TEXT("Unbroken")),
			FText::FromString(TEXT("Endowed with extreme tensile resilience; resists shattering against heavy impacts from Awakened-rank foes.")),
			0.0f
		));
		Def->Enchantments.Add(FShadowSlaveMemoryEnchantment(
			FName(TEXT("Enchantment_WeightOfMidnight")),
			FText::FromString(TEXT("Weight of Midnight")),
			FText::FromString(TEXT("Delivers crushing kinetic momentum upon impact while feeling nimble and light in hand.")),
			0.0f
		));
		Def->Enchantments.Add(FShadowSlaveMemoryEnchantment(
			FName(TEXT("Enchantment_Hone")),
			FText::FromString(TEXT("Hone")),
			FText::FromString(TEXT("Maintains a razor edge capable of slicing through tough monster carapaces.")),
			0.0f
		));
	}
	else if (CanonMemoryId == FName(TEXT("CruelSight")))
	{
		Def->DisplayName = FText::FromString(TEXT("Cruel Sight"));
		Def->Description = FText::FromString(TEXT("A somber spear with an obsidian-like shaft and a translucent leaf blade glowing with a pale, ghostly light."));
		Def->Rank = EShadowSlaveMemoryRank::Ascended;
		Def->Tier = EShadowSlaveMemoryTier::Tier6;
		Def->Category = EShadowSlaveMemoryCategory::Weapon;
		Def->EquipmentSlot = EShadowSlaveEquipmentSlot::Weapon;
		Def->bCanBeEquipped = true;
		Def->bRequiresExclusiveSlot = true;
		Def->bCanBeActivated = true;
		Def->ActivationType = EShadowSlaveMemoryActivationType::Active;

		Def->Enchantments.Add(FShadowSlaveMemoryEnchantment(
			FName(TEXT("Enchantment_GhostBlade")),
			FText::FromString(TEXT("Ghost Blade")),
			FText::FromString(TEXT("The spearhead phases intangibly through physical armor and shields to materialize directly inside vital organs.")),
			0.0f
		));
		Def->Enchantments.Add(FShadowSlaveMemoryEnchantment(
			FName(TEXT("Enchantment_SomberGleam")),
			FText::FromString(TEXT("Somber Gleam")),
			FText::FromString(TEXT("Projects an eerie pale luminescence that pierces deep darkness and disrupts optical illusions.")),
			0.0f
		));
		Def->Enchantments.Add(FShadowSlaveMemoryEnchantment(
			FName(TEXT("Enchantment_MarrowChill")),
			FText::FromString(TEXT("Marrow Chill")),
			FText::FromString(TEXT("Inflicts numbing cold and necrotic paralysis into victim essence channels upon puncture.")),
			0.0f
		));
	}
	else if (CanonMemoryId == FName(TEXT("WeaversMask")))
	{
		Def->DisplayName = FText::FromString(TEXT("Weaver's Mask"));
		Def->Description = FText::FromString(TEXT("A smooth, faceless mask carved from abyssal black wood adorned with subtle carved web filigree. An ancient divine relic of the Daemon of Fate."));
		Def->Rank = EShadowSlaveMemoryRank::Divine;
		Def->Tier = EShadowSlaveMemoryTier::Tier7;
		Def->Category = EShadowSlaveMemoryCategory::Relic;
		Def->EquipmentSlot = EShadowSlaveEquipmentSlot::Relic;
		Def->bCanBeEquipped = true;
		Def->bRequiresExclusiveSlot = false;

		Def->Enchantments.Add(FShadowSlaveMemoryEnchantment(
			FName(TEXT("Enchantment_SimpleTrick")),
			FText::FromString(TEXT("Simple Trick")),
			FText::FromString(TEXT("Inverts the wearer's innate Flaw. For Sunny, forces him to speak only falsehoods.")),
			0.0f
		));
		Def->Enchantments.Add(FShadowSlaveMemoryEnchantment(
			FName(TEXT("Enchantment_WhereIsMyEye")),
			FText::FromString(TEXT("Where is My Eye?")),
			FText::FromString(TEXT("Grants mystical sight into the invisible strings of fate, destiny, and essence weave.")),
			0.0f
		));
		Def->Enchantments.Add(FShadowSlaveMemoryEnchantment(
			FName(TEXT("Enchantment_Unknowable")),
			FText::FromString(TEXT("Unknowable")),
			FText::FromString(TEXT("Completely conceals the wearer's identity, true name, and presence from all appraisal abilities and divination.")),
			0.0f
		));
	}

	return Def;
}
