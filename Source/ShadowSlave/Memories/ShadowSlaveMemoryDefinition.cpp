// Copyright Epic Games, Inc. All Rights Reserved.

#include "Memories/ShadowSlaveMemoryDefinition.h"

EShadowSlaveMemoryRank FShadowSlaveMemoryInstance::GetRank() const
{
	return MemoryDefinition ? MemoryDefinition->Rank : EShadowSlaveMemoryRank::Unknown;
}

EShadowSlaveMemoryTier FShadowSlaveMemoryInstance::GetTier() const
{
	return MemoryDefinition ? MemoryDefinition->Tier : EShadowSlaveMemoryTier::Unknown;
}

bool FShadowSlaveMemoryInstance::HasKnownRank() const
{
	return GetRank() != EShadowSlaveMemoryRank::Unknown;
}

bool FShadowSlaveMemoryInstance::HasKnownTier() const
{
	return GetTier() != EShadowSlaveMemoryTier::Unknown;
}

int32 FShadowSlaveMemoryInstance::GetTotalEnchantmentCount() const
{
	const int32 BaseCount = MemoryDefinition ? MemoryDefinition->GetEnchantmentCount() : 0;
	return BaseCount + DynamicEnchantments.Num();
}

TArray<FShadowSlaveMemoryEnchantment> FShadowSlaveMemoryInstance::GetAllEnchantments() const
{
	TArray<FShadowSlaveMemoryEnchantment> All;
	if (MemoryDefinition)
	{
		All.Append(MemoryDefinition->Enchantments);
	}
	All.Append(DynamicEnchantments);
	return All;
}

void FShadowSlaveMemoryInstance::SetDynamicProperty(FName Key, const FString& Value)
{
	DynamicProperties.Add(Key, Value);
}

bool FShadowSlaveMemoryInstance::GetDynamicProperty(FName Key, FString& OutValue) const
{
	if (const FString* Found = DynamicProperties.Find(Key))
	{
		OutValue = *Found;
		return true;
	}
	return false;
}

bool FShadowSlaveMemoryInstance::RemoveDynamicProperty(FName Key)
{
	return DynamicProperties.Remove(Key) > 0;
}

UShadowSlaveMemoryDefinition::UShadowSlaveMemoryDefinition()
	: UShadowSlaveContentDefinition()
{
	ContentId = NAME_None;
	ContentType = EShadowSlaveContentType::Memory;
	DisplayName = FText::FromString(TEXT("Generic Memory"));
	Description = FText::FromString(TEXT("A generic Memory prototype definition."));
	Version = 1;
	Rank = EShadowSlaveMemoryRank::Unknown;
	Tier = EShadowSlaveMemoryTier::Unknown;
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
	return Super::GetPrimaryAssetId();
}

bool UShadowSlaveMemoryDefinition::IsValidDefinition(FString* OutErrorMessage) const
{
	// 1. Generic content definition validation (valid ContentId, Version >= 1)
	if (!Super::IsValidDefinition(OutErrorMessage))
	{
		return false;
	}

	// 2. Generic content type validation: must be Memory
	if (ContentType != EShadowSlaveContentType::Memory)
	{
		if (OutErrorMessage)
		{
			*OutErrorMessage = FString::Printf(TEXT("Memory definition '%s' must have ContentType == EShadowSlaveContentType::Memory."),
				*ContentId.ToString());
		}
		return false;
	}

	// 3. Memory-specific validation: essence costs must not be negative
	if (BaseEssenceCost < 0.0f)
	{
		if (OutErrorMessage)
		{
			*OutErrorMessage = FString::Printf(TEXT("Memory definition '%s' has negative BaseEssenceCost (%.2f)."),
				*ContentId.ToString(), BaseEssenceCost);
		}
		return false;
	}

	for (const FShadowSlaveMemoryEnchantment& Enchantment : Enchantments)
	{
		if (Enchantment.EssenceCost < 0.0f)
		{
			if (OutErrorMessage)
			{
				*OutErrorMessage = FString::Printf(TEXT("Memory definition '%s' enchantment '%s' has negative EssenceCost (%.2f)."),
					*ContentId.ToString(), *Enchantment.EnchantmentId.ToString(), Enchantment.EssenceCost);
			}
			return false;
		}
	}

	return true;
}

UShadowSlaveMemoryDefinition* UShadowSlaveMemoryDefinition::CreateTestMemoryDefinition(UObject* Outer)
{
	UObject* EffectiveOuter = Outer ? Outer : GetTransientPackage();
	UShadowSlaveMemoryDefinition* TestDef = NewObject<UShadowSlaveMemoryDefinition>(EffectiveOuter, FName(TEXT("TestMemoryDefinition")));
	if (TestDef)
	{
		TestDef->ContentId = FName(TEXT("Test_GenericMemory"));
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

	Def->ContentId = CanonMemoryId;

	if (CanonMemoryId == FName(TEXT("PuppeteersShroud")))
	{
		Def->DisplayName = FText::FromString(TEXT("Puppeteer's Shroud"));
		Def->Description = FText::FromString(TEXT("An unassuming, pitch-black tunic and trousers woven from rough matte silk. Deceptively ordinary attire concealing formidable defensive durability. Awarded upon First Nightmare evaluation (Ch. 15); runes inspected Ch. 83."));
		Def->Rank = EShadowSlaveMemoryRank::Awakened;
		Def->Tier = EShadowSlaveMemoryTier::Tier5;
		Def->Category = EShadowSlaveMemoryCategory::Armor;
		Def->EquipmentSlot = EShadowSlaveEquipmentSlot::Armor;
		Def->bCanBeEquipped = true;
		Def->bRequiresExclusiveSlot = true;

		// Original canonical enchantments (inspected Ch. 83)
		Def->Enchantments.Add(FShadowSlaveMemoryEnchantment(
			FName(TEXT("Enchantment_EnhancedDurability")),
			FText::FromString(TEXT("Enhanced Durability")),
			FText::FromString(TEXT("Provides formidable structural durability and resilience against physical cuts, piercing, and kinetic trauma.")),
			0.0f
		));
		Def->Enchantments.Add(FShadowSlaveMemoryEnchantment(
			FName(TEXT("Enchantment_Doubtless")),
			FText::FromString(TEXT("Doubtless")),
			FText::FromString(TEXT("Powerful mental ward protecting the wielder's mind against psychological manipulation, fear, and psychic intrusion.")),
			0.0f
		));
		// NOTE: [Blessing of Spirit] was transplanted later via weaving in Ch. 1473 from Shroud of Graceless Dusk. Not an original enchantment.
	}
	else if (CanonMemoryId == FName(TEXT("SilverBell")))
	{
		Def->DisplayName = FText::FromString(TEXT("Silver Bell"));
		Def->Description = FText::FromString(TEXT("A small, polished silver bell that fits comfortably inside a closed fist. Acquired in the First Nightmare (Ch. 8) from a slain veteran; runes inspected Ch. 83."));
		Def->Rank = EShadowSlaveMemoryRank::Dormant;
		Def->Tier = EShadowSlaveMemoryTier::Tier1;
		Def->Category = EShadowSlaveMemoryCategory::Charm;
		Def->EquipmentSlot = EShadowSlaveEquipmentSlot::Charm;
		Def->bCanBeEquipped = true;
		Def->bCanBeActivated = true;
		Def->ActivationType = EShadowSlaveMemoryActivationType::Triggered;

		// Original canonical enchantment (inspected Ch. 83)
		Def->Enchantments.Add(FShadowSlaveMemoryEnchantment(
			FName(TEXT("Enchantment_SilverSong")),
			FText::FromString(TEXT("Silver Song")),
			FText::FromString(TEXT("Emits an extraordinarily pure, ringing chime that effortlessly travels vast distances and pierces ambient noise, attracting nearby creatures.")),
			0.0f
		));
		// NOTE: Upgraded to Tier II with added enchantment [Sonorous] via weaving in Ch. 695.
	}
	else if (CanonMemoryId == FName(TEXT("MidnightShard")))
	{
		Def->DisplayName = FText::FromString(TEXT("Midnight Shard"));
		Def->Description = FText::FromString(TEXT("An elegant, slender curved tachi sword forged from pitch-black metal that absorbs light. Dropped by the Carapace Centurion and yielded to Sunny by Nephis in Ch. 74; runes inspected Ch. 83."));
		Def->Rank = EShadowSlaveMemoryRank::Awakened;
		Def->Tier = EShadowSlaveMemoryTier::Tier3;
		Def->Category = EShadowSlaveMemoryCategory::Weapon;
		Def->EquipmentSlot = EShadowSlaveEquipmentSlot::Weapon;
		Def->bCanBeEquipped = true;
		Def->bRequiresExclusiveSlot = true;

		// Sole canonical enchantment
		Def->Enchantments.Add(FShadowSlaveMemoryEnchantment(
			FName(TEXT("Enchantment_Unbroken")),
			FText::FromString(TEXT("Unbroken")),
			FText::FromString(TEXT("Endowed with extreme tensile resilience; resists shattering against heavy impacts from Awakened-rank foes. Possesses no inherent magical sharpness.")),
			0.0f
		));
		// NOTE: Retained in inventory until loss of Spell Memories in Ch. 1758; weave studied in Ch. 1030 for Siege Souvenir. Not destroyed in Ch. 336; not reforged.
	}
	else if (CanonMemoryId == FName(TEXT("MoonlightShard")))
	{
		Def->DisplayName = FText::FromString(TEXT("Moonlight Shard"));
		Def->Description = FText::FromString(TEXT("A slender stiletto-like dagger sculpted from translucent, luminous crystalline glass. Gifted to Sunny by Nephis in the Bright Castle (Ch. 261); runes inspected Ch. 262."));
		Def->Rank = EShadowSlaveMemoryRank::Ascended;
		Def->Tier = EShadowSlaveMemoryTier::Tier1;
		Def->Category = EShadowSlaveMemoryCategory::Weapon;
		Def->EquipmentSlot = EShadowSlaveEquipmentSlot::Weapon;
		Def->bCanBeEquipped = true;
		Def->bRequiresExclusiveSlot = false;

		Def->Enchantments.Add(FShadowSlaveMemoryEnchantment(
			FName(TEXT("Enchantment_Unseen")),
			FText::FromString(TEXT("Unseen")),
			FText::FromString(TEXT("Enables instantaneous manifestation without delay or summoning flare directly into the wielder's hand or strike trajectory.")),
			0.0f
		));
		// NOTE: Gifted by Nephis in Ch. 261 (not Cassie, not Ch. 324). Retained until loss of Spell Memories in Ch. 1758.
	}
	else if (CanonMemoryId == FName(TEXT("OrdinaryRock")))
	{
		Def->DisplayName = FText::FromString(TEXT("Ordinary Rock"));
		Def->Description = FText::FromString(TEXT("An ordinary-looking grey pebble dropped by a subterranean rock-eater in Ch. 96; runes inspected Ch. 104. Remarkable only for its absolute ordinariness."));
		Def->Rank = EShadowSlaveMemoryRank::Awakened;
		Def->Tier = EShadowSlaveMemoryTier::Tier1;
		Def->Category = EShadowSlaveMemoryCategory::Tool;
		Def->EquipmentSlot = EShadowSlaveEquipmentSlot::None;
		Def->bCanBeEquipped = false;

		Def->Enchantments.Add(FShadowSlaveMemoryEnchantment(
			FName(TEXT("Enchantment_NotReally")),
			FText::FromString(TEXT("Not Really")),
			FText::FromString(TEXT("The rock is completely and thoroughly ordinary. Possesses no supernatural aura, edge, or overt power.")),
			0.0f
		));
		// NOTE: Upgraded in Ch. 696 to Tier II with thought-to-sound functionality and [Sonorous], renamed Extraordinary Rock.
	}
	else if (CanonMemoryId == FName(TEXT("ExtraordinaryRock")))
	{
		Def->DisplayName = FText::FromString(TEXT("Extraordinary Rock"));
		Def->Description = FText::FromString(TEXT("The upgraded form of Ordinary Rock, modified by Sunny via weaving in the Chained Isles (Ch. 696) with emerald amulet thought-to-sound weave."));
		Def->Rank = EShadowSlaveMemoryRank::Awakened;
		Def->Tier = EShadowSlaveMemoryTier::Tier2;
		Def->Category = EShadowSlaveMemoryCategory::Tool;
		Def->EquipmentSlot = EShadowSlaveEquipmentSlot::None;
		Def->bCanBeEquipped = false;

		Def->Enchantments.Add(FShadowSlaveMemoryEnchantment(
			FName(TEXT("Enchantment_NotReally")),
			FText::FromString(TEXT("Not Really")),
			FText::FromString(TEXT("The rock is completely and thoroughly ordinary.")),
			0.0f
		));
		Def->Enchantments.Add(FShadowSlaveMemoryEnchantment(
			FName(TEXT("Enchantment_Sonorous")),
			FText::FromString(TEXT("Sonorous")),
			FText::FromString(TEXT("Emits clear acoustic vibrations based on thought-to-sound transmission.")),
			0.0f
		));
	}
	else if (CanonMemoryId == FName(TEXT("ProwlingThorn")))
	{
		Def->DisplayName = FText::FromString(TEXT("Prowling Thorn"));
		Def->Description = FText::FromString(TEXT("A curved barbed thorn blade dropped by an armored porcupine beast in the labyrinth (Ch. 96); runes inspected Ch. 104."));
		Def->Rank = EShadowSlaveMemoryRank::Awakened;
		Def->Tier = EShadowSlaveMemoryTier::Tier2;
		Def->Category = EShadowSlaveMemoryCategory::Weapon;
		Def->EquipmentSlot = EShadowSlaveEquipmentSlot::Weapon;
		Def->bCanBeEquipped = true;
		Def->bRequiresExclusiveSlot = false;

		Def->Enchantments.Add(FShadowSlaveMemoryEnchantment(
			FName(TEXT("Enchantment_RoseOfBetrayal")),
			FText::FromString(TEXT("Rose of Betrayal")),
			FText::FromString(TEXT("Launches a barbed thorn attached to an invisible essence tether, allowing trajectory control, silent snaring, and instant recall.")),
			0.0f
		));
		// NOTE: Consumed by Onyx Saint in Antarctica (Ch. 1026).
	}
	else if (CanonMemoryId == FName(TEXT("DarkWing")))
	{
		Def->DisplayName = FText::FromString(TEXT("Dark Wing"));
		Def->Description = FText::FromString(TEXT("A translucent, membranous black cape dropped by a Flesh Reaver on the Dark City perimeter (Ch. 242)."));
		Def->Rank = EShadowSlaveMemoryRank::Awakened;
		Def->Tier = EShadowSlaveMemoryTier::Tier1;
		Def->Category = EShadowSlaveMemoryCategory::Charm;
		Def->EquipmentSlot = EShadowSlaveEquipmentSlot::Charm;
		Def->bCanBeEquipped = true;

		Def->Enchantments.Add(FShadowSlaveMemoryEnchantment(
			FName(TEXT("Enchantment_Glide")),
			FText::FromString(TEXT("Glide")),
			FText::FromString(TEXT("Grants the wearer the ability to catch aerial currents and glide horizontally through the air with controlled slow descent.")),
			0.0f
		));
		// NOTE: Retained across arcs until loss of Spell Memories in Ch. 1758.
	}
	else if (CanonMemoryId == FName(TEXT("BloodBlossom")))
	{
		Def->DisplayName = FText::FromString(TEXT("Blood Blossom"));
		Def->Description = FText::FromString(TEXT("A crimson blossom charm dropped by a Blood Flower in the Dark City (Ch. 242)."));
		Def->Rank = EShadowSlaveMemoryRank::Awakened;
		Def->Tier = EShadowSlaveMemoryTier::Tier2;
		Def->Category = EShadowSlaveMemoryCategory::Charm;
		Def->EquipmentSlot = EShadowSlaveEquipmentSlot::Charm;
		Def->bCanBeEquipped = true;

		Def->Enchantments.Add(FShadowSlaveMemoryEnchantment(
			FName(TEXT("Enchantment_FlowerOfEvil")),
			FText::FromString(TEXT("Flower of Evil")),
			FText::FromString(TEXT("Absorbs spilled blood from enemies and environment to nourish the wearer's vitality and stamina.")),
			0.0f
		));
		// NOTE: Given to Belle in Antarctica in Ch. 844. Not permanently retained.
	}
	else if (CanonMemoryId == FName(TEXT("EndlessSpring")))
	{
		Def->DisplayName = FText::FromString(TEXT("Endless Spring"));
		Def->Description = FText::FromString(TEXT("A smooth, deep blue ceramic bottle gifted to Sunny by Cassie in Ch. 36; runes inspected Ch. 104."));
		Def->Rank = EShadowSlaveMemoryRank::Dormant;
		Def->Tier = EShadowSlaveMemoryTier::Tier4;
		Def->Category = EShadowSlaveMemoryCategory::Tool;
		Def->EquipmentSlot = EShadowSlaveEquipmentSlot::None;
		Def->bCanBeEquipped = false;

		// Original canonical enchantment
		Def->Enchantments.Add(FShadowSlaveMemoryEnchantment(
			FName(TEXT("Enchantment_GiftOfWater")),
			FText::FromString(TEXT("Gift of Water")),
			FText::FromString(TEXT("Produces an endless reservoir of pure, refreshing drinking water.")),
			0.0f
		));
		// NOTE: [Blessing of Flesh] added much later via weaving in Ch. 1473.
	}
	else if (CanonMemoryId == FName(TEXT("CruelSight")))
	{
		Def->DisplayName = FText::FromString(TEXT("Cruel Sight"));
		Def->Description = FText::FromString(TEXT("A somber spear with an obsidian-like shaft and a translucent leaf blade glowing with pale ghostly light. Dropped by the Mirror Beast on Reckoning Island (Ch. 413); runes inspected Ch. 415."));
		Def->Rank = EShadowSlaveMemoryRank::Ascended;
		Def->Tier = EShadowSlaveMemoryTier::Tier4;
		Def->Category = EShadowSlaveMemoryCategory::Weapon;
		Def->EquipmentSlot = EShadowSlaveEquipmentSlot::Weapon;
		Def->bCanBeEquipped = true;
		Def->bRequiresExclusiveSlot = true;
		Def->bCanBeActivated = true;
		Def->ActivationType = EShadowSlaveMemoryActivationType::Active;

		// Four verified original enchantments
		Def->Enchantments.Add(FShadowSlaveMemoryEnchantment(
			FName(TEXT("Enchantment_Shapeshifter")),
			FText::FromString(TEXT("Shapeshifter")),
			FText::FromString(TEXT("Allows the spear to freely shift its physical form between a spear, a dagger, and other bladed weapon configurations.")),
			0.0f
		));
		Def->Enchantments.Add(FShadowSlaveMemoryEnchantment(
			FName(TEXT("Enchantment_LightEater")),
			FText::FromString(TEXT("Light Eater")),
			FText::FromString(TEXT("Devours ambient illumination and light sources, shrouding the weapon in gloom and dimming surroundings.")),
			0.0f
		));
		Def->Enchantments.Add(FShadowSlaveMemoryEnchantment(
			FName(TEXT("Enchantment_GhostBlade")),
			FText::FromString(TEXT("Ghost Blade")),
			FText::FromString(TEXT("Phases intangibly through physical armor and shields to materialize directly inside vital organs and soul structures.")),
			0.0f
		));
		Def->Enchantments.Add(FShadowSlaveMemoryEnchantment(
			FName(TEXT("Enchantment_DarkMirror")),
			FText::FromString(TEXT("Dark Mirror")),
			FText::FromString(TEXT("Captures and reflects incoming light and spiritual energy through its polished somber blade.")),
			0.0f
		));
	}
	else if (CanonMemoryId == FName(TEXT("CovetousCoffer")))
	{
		Def->DisplayName = FText::FromString(TEXT("Covetous Coffer"));
		Def->Description = FText::FromString(TEXT("A heavy wooden chest adorned with brass fittings and sharp, carnivorous wooden teeth. Dropped by the Mordant Mimic on Shipwreck Island in the Chained Isles (Ch. 427); runes inspected Ch. 428."));
		Def->Rank = EShadowSlaveMemoryRank::Ascended;
		Def->Tier = EShadowSlaveMemoryTier::Tier4;
		Def->Category = EShadowSlaveMemoryCategory::Tool;
		Def->EquipmentSlot = EShadowSlaveEquipmentSlot::None;
		Def->bCanBeEquipped = false;

		Def->Enchantments.Add(FShadowSlaveMemoryEnchantment(
			FName(TEXT("Enchantment_MendaciousCoffer")),
			FText::FromString(TEXT("Mendacious Coffer")),
			FText::FromString(TEXT("Disguises the chest as an ordinary, inanimate piece of furniture to avoid suspicion.")),
			0.0f
		));
		Def->Enchantments.Add(FShadowSlaveMemoryEnchantment(
			FName(TEXT("Enchantment_CapaciousChest")),
			FText::FromString(TEXT("Capacious Chest")),
			FText::FromString(TEXT("Bends internal space to provide expansive extradimensional storage capacity.")),
			0.0f
		));
		Def->Enchantments.Add(FShadowSlaveMemoryEnchantment(
			FName(TEXT("Enchantment_LocomotiveChiffonier")),
			FText::FromString(TEXT("Locomotive Chiffonier")),
			FText::FromString(TEXT("Sprouts sturdy wooden legs allowing the coffer to crawl, walk, and follow its master autonomously.")),
			0.0f
		));
		Def->Enchantments.Add(FShadowSlaveMemoryEnchantment(
			FName(TEXT("Enchantment_LuridTrunk")),
			FText::FromString(TEXT("Lurid Trunk")),
			FText::FromString(TEXT("Snaps its lid shut with terrifying bite force, crushing and devouring unwanted matter placed within.")),
			0.0f
		));
		// NOTE: Later transformed and re-woven into the Marvelous Mimic.
	}
	else if (CanonMemoryId == FName(TEXT("WeaversMask")))
	{
		Def->DisplayName = FText::FromString(TEXT("Weaver's Mask"));
		Def->Description = FText::FromString(TEXT("A smooth, faceless mask carved from abyssal black wood adorned with subtle carved web filigree. An ancient divine tool of Weaver, Daemon of Fate. Claimed from the headless statue of Weaver (Ch. 276-278)."));
		Def->Rank = EShadowSlaveMemoryRank::Divine;
		Def->Tier = EShadowSlaveMemoryTier::Tier7;
		Def->Category = EShadowSlaveMemoryCategory::Tool;
		Def->EquipmentSlot = EShadowSlaveEquipmentSlot::None;
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
			FName(TEXT("Enchantment_MantleOfLies")),
			FText::FromString(TEXT("Mantle of Lies")),
			FText::FromString(TEXT("Completely conceals the wearer's identity, true name, runes, and presence from all appraisal abilities and divination.")),
			0.0f
		));
	}
	else if (CanonMemoryId == FName(TEXT("MantleOfTheUnderworld")))
	{
		Def->DisplayName = FText::FromString(TEXT("Mantle of the Underworld"));
		Def->Description = FText::FromString(TEXT("A magnificent suit of full plate armor carved from living black stone paired with a mantle resembling petrified liquid shadow. Forged by Prince Nether, Daemon of Choice."));
		Def->Rank = EShadowSlaveMemoryRank::Ascended;
		Def->Tier = EShadowSlaveMemoryTier::Tier6; // Tier VI upon repair (Ch. 377-378); upgraded to Tier VII in Ch. 972
		Def->Category = EShadowSlaveMemoryCategory::Armor;
		Def->EquipmentSlot = EShadowSlaveEquipmentSlot::Armor;
		Def->bCanBeEquipped = true;
		Def->bRequiresExclusiveSlot = true;

		// Six canonical enchantments
		Def->Enchantments.Add(FShadowSlaveMemoryEnchantment(
			FName(TEXT("Enchantment_LivingStone")),
			FText::FromString(TEXT("Living Stone")),
			FText::FromString(TEXT("The stone armor adjusts smoothly to the wearer's anatomy, causes zero physical encumbrance, and slowly mends fractures from essence.")),
			0.0f
		));
		Def->Enchantments.Add(FShadowSlaveMemoryEnchantment(
			FName(TEXT("Enchantment_FeatherOfTruth")),
			FText::FromString(TEXT("Feather of Truth")),
			FText::FromString(TEXT("Enables dynamic mass and weight manipulation: weightless as a feather for extreme speed, or heavy as a mountain to resist knockback.")),
			0.0f
		));
		Def->Enchantments.Add(FShadowSlaveMemoryEnchantment(
			FName(TEXT("Enchantment_Stalwart")),
			FText::FromString(TEXT("Stalwart")),
			FText::FromString(TEXT("Endows the armor with supreme defensive resilience against physical, piercing, and elemental trauma.")),
			0.0f
		));
		Def->Enchantments.Add(FShadowSlaveMemoryEnchantment(
			FName(TEXT("Enchantment_UnderworldArmament")),
			FText::FromString(TEXT("Underworld Armament")),
			FText::FromString(TEXT("Daemon-forged evolving kill counter. Slaying enemies reinforces the armor's matrix and unlocks higher tiers.")),
			0.0f
		));
		Def->Enchantments.Add(FShadowSlaveMemoryEnchantment(
			FName(TEXT("Enchantment_PrinceOfTheUnderworld")),
			FText::FromString(TEXT("Prince of the Underworld")),
			FText::FromString(TEXT("Radiates an aura of demonic majesty, instilling dread in enemies and granting significant resistance to mind attacks.")),
			0.0f
		));
		Def->Enchantments.Add(FShadowSlaveMemoryEnchantment(
			FName(TEXT("Enchantment_SoulboundRelic")),
			FText::FromString(TEXT("Soulbound Relic")),
			FText::FromString(TEXT("Upon maximum progression or critical binding, the living stone matrix dissolves into the soul and flesh of its master, manifesting as a permanent attribute.")),
			0.0f
		));
		// NOTE: First seen Ch. 173; purchased broken Ch. 174 (for 7 soul shards as 'M... Un...old'); repaired Ch. 377-378 (Ascended Tier VI); upgraded Ch. 972 (Tier VII); bound into [Marble Shell] Attribute in Ch. 974.
	}
	else if (CanonMemoryId == FName(TEXT("SinOfSolace")))
	{
		Def->DisplayName = FText::FromString(TEXT("Sin of Solace"));
		Def->Description = FText::FromString(TEXT("An elegant double-edged straight sword (shuangshou jian) carved from polished, translucent white-green jade. Dropped by the Remnant of the Jade Queen in Antarctica (Ch. 867); runes inspected Ch. 869."));
		Def->Rank = EShadowSlaveMemoryRank::Transcendent;
		Def->Tier = EShadowSlaveMemoryTier::Tier5;
		Def->Category = EShadowSlaveMemoryCategory::Weapon;
		Def->EquipmentSlot = EShadowSlaveEquipmentSlot::Weapon;
		Def->bCanBeEquipped = true;
		Def->bRequiresExclusiveSlot = true;

		// Five verified canonical enchantments
		Def->Enchantments.Add(FShadowSlaveMemoryEnchantment(
			FName(TEXT("Enchantment_SinisterWhisper")),
			FText::FromString(TEXT("Sinister Whisper")),
			FText::FromString(TEXT("Wounds inflicted by the jade blade transmit maddening, disorienting psychic whispers into the victim's mind.")),
			0.0f
		));
		Def->Enchantments.Add(FShadowSlaveMemoryEnchantment(
			FName(TEXT("Enchantment_PerfectJade")),
			FText::FromString(TEXT("Perfect Jade")),
			FText::FromString(TEXT("Bestows transcendent sharpness and supreme durability that never dulls or chips against physical armor.")),
			0.0f
		));
		Def->Enchantments.Add(FShadowSlaveMemoryEnchantment(
			FName(TEXT("Enchantment_OmenOfDread")),
			FText::FromString(TEXT("Omen of Dread")),
			FText::FromString(TEXT("Exudes a suffocating aura of malevolent dread that erodes enemy courage and incites panic.")),
			0.0f
		));
		Def->Enchantments.Add(FShadowSlaveMemoryEnchantment(
			FName(TEXT("Enchantment_HideousTruth")),
			FText::FromString(TEXT("Hideous Truth")),
			FText::FromString(TEXT("Strikes tear through illusory protections and spiritual wards to expose and sever hidden vulnerabilities.")),
			0.0f
		));
		Def->Enchantments.Add(FShadowSlaveMemoryEnchantment(
			FName(TEXT("Enchantment_Cursed")),
			FText::FromString(TEXT("Cursed")),
			FText::FromString(TEXT("Afflicts the wielder with an insidious psychic phantom clone that incessantly mocks, derides, and undermines their sanity.")),
			0.0f
		));
		// NOTE: Acquired in Antarctica (Ch. 867-869) from Remnant of the Jade Queen (not Ch. 755 Dread Pagoda). Consumed by Onyx Saint in Ch. 1576 prior to the Estuary.
	}
	else if (CanonMemoryId == FName(TEXT("AutumnLeaf")))
	{
		Def->DisplayName = FText::FromString(TEXT("Autumn Leaf"));
		Def->Description = FText::FromString(TEXT("A small leaf-shaped charm purchased in the Sanctuary market to alter hair color (Ch. 388/392). Its rank, tier, and specific enchantment name are unrecorded in canon."));
		Def->Rank = EShadowSlaveMemoryRank::Unknown;
		Def->Tier = EShadowSlaveMemoryTier::Unknown;
		Def->Category = EShadowSlaveMemoryCategory::Charm;
		Def->EquipmentSlot = EShadowSlaveEquipmentSlot::Charm;
		Def->bCanBeEquipped = true;

		Def->Enchantments.Add(FShadowSlaveMemoryEnchantment(
			FName(TEXT("Enchantment_AutumnLeafHairAlteration")),
			FText::FromString(TEXT("Hair Alteration")),
			FText::FromString(TEXT("Cosmetic charm effect that alters the wielder's hair color. (Canonical enchantment name is unrecorded).")),
			0.0f
		));
		// NOTE: Rank, Tier, and enchantment name are canonically UNKNOWN. Weave incorporated into Lord of Shadows mask in Ch. 1685.
	}
	else if (CanonMemoryId == FName(TEXT("SiegeSouvenir")))
	{
		Def->DisplayName = FText::FromString(TEXT("Siege Souvenir"));
		Def->Description = FText::FromString(TEXT("A three-meter-long white javelin hand-crafted by Sunny via Weaving in Antarctica (Ch. 1030) from the quill of a Corrupted Devil."));
		Def->Rank = EShadowSlaveMemoryRank::Transcendent; // Explicitly verified Transcendent in Ch. 1030 text
		Def->Tier = EShadowSlaveMemoryTier::Unknown;       // Tier is canonically UNKNOWN (no explicit novel runes recorded)
		Def->Category = EShadowSlaveMemoryCategory::Weapon;
		Def->EquipmentSlot = EShadowSlaveEquipmentSlot::Weapon;
		Def->bCanBeEquipped = true;
		Def->bRequiresExclusiveSlot = false;

		// Two verified enchantments
		Def->Enchantments.Add(FShadowSlaveMemoryEnchantment(
			FName(TEXT("Enchantment_DeathDealer")),
			FText::FromString(TEXT("Death Dealer")),
			FText::FromString(TEXT("Empowers the javelin with cataclysmic armor-piercing kinetic impact upon release.")),
			0.0f
		));
		Def->Enchantments.Add(FShadowSlaveMemoryEnchantment(
			FName(TEXT("Enchantment_UnnamedEssenceReservoir")),
			FText::FromString(TEXT("[Unnamed Essence Reservoir]")),
			FText::FromString(TEXT("Custom essence reservoir woven by Sunny using the conceptual weave structure of [Unbroken] from Midnight Shard. Explicitly unnamed in canon.")),
			0.0f
		));
		// NOTE: Designed as an expendable one-shot anti-titan weapon; destroyed killing the Corrupted Titan Goliath in Ch. 1031.
	}

	return Def;
}
