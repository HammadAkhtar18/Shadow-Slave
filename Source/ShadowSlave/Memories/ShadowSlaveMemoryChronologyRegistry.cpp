// Copyright Epic Games, Inc. All Rights Reserved.

#include "Memories/ShadowSlaveMemoryChronologyRegistry.h"
#include "Memories/ShadowSlaveMemoryDefinition.h"
#include "ShadowSlave.h"

UShadowSlaveMemoryChronologyRegistry::UShadowSlaveMemoryChronologyRegistry()
{
	RegistryId = FName(TEXT("Registry_Default"));
	RegistryName = FText::FromString(TEXT("Default Memory Chronology Registry"));
	Description = FText::FromString(TEXT("Chronological timeline and verified canon metadata for Memories."));
}

FPrimaryAssetId UShadowSlaveMemoryChronologyRegistry::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("MemoryChronologyRegistry"), GetFName());
}

TArray<FShadowSlaveMemoryChronologyEntry> UShadowSlaveMemoryChronologyRegistry::GetEntriesByArc(EShadowSlaveStoryArc Arc) const
{
	TArray<FShadowSlaveMemoryChronologyEntry> Filtered;
	for (const FShadowSlaveMemoryChronologyEntry& Entry : ChronologyEntries)
	{
		if (Entry.StoryArc == Arc)
		{
			Filtered.Add(Entry);
		}
	}
	return Filtered;
}

TArray<FShadowSlaveMemoryChronologyEntry> UShadowSlaveMemoryChronologyRegistry::GetEntriesForCharacter(FName CharacterId) const
{
	TArray<FShadowSlaveMemoryChronologyEntry> Filtered;
	for (const FShadowSlaveMemoryChronologyEntry& Entry : ChronologyEntries)
	{
		if (Entry.TargetCharacterId == CharacterId)
		{
			Filtered.Add(Entry);
		}
	}
	return Filtered;
}

bool UShadowSlaveMemoryChronologyRegistry::FindEntryById(FName EntryId, FShadowSlaveMemoryChronologyEntry& OutEntry) const
{
	if (EntryId.IsNone())
	{
		return false;
	}

	for (const FShadowSlaveMemoryChronologyEntry& Entry : ChronologyEntries)
	{
		if (Entry.EntryId == EntryId)
		{
			OutEntry = Entry;
			return true;
		}
	}

	return false;
}

bool UShadowSlaveMemoryChronologyRegistry::FindEntryByMemoryDefinition(const UShadowSlaveMemoryDefinition* MemoryDef, FShadowSlaveMemoryChronologyEntry& OutEntry) const
{
	if (!MemoryDef)
	{
		return false;
	}

	for (const FShadowSlaveMemoryChronologyEntry& Entry : ChronologyEntries)
	{
		if (Entry.MemoryDefinition.Get() == MemoryDef || Entry.MemoryDefinition.ToSoftObjectPath() == MemoryDef)
		{
			OutEntry = Entry;
			return true;
		}
	}

	return false;
}

TArray<FShadowSlaveMemoryChronologyEntry> UShadowSlaveMemoryChronologyRegistry::GetVerifiedEntries() const
{
	TArray<FShadowSlaveMemoryChronologyEntry> VerifiedEntries;
	for (const FShadowSlaveMemoryChronologyEntry& Entry : ChronologyEntries)
	{
		if (Entry.CanonConfidence == EShadowSlaveCanonConfidence::Verified)
		{
			VerifiedEntries.Add(Entry);
		}
	}
	return VerifiedEntries;
}

void UShadowSlaveMemoryChronologyRegistry::LogChronology() const
{
	UE_LOG(LogShadowSlave, Log, TEXT("[Chronology Registry: %s] %d total entries:"), *RegistryName.ToString(), ChronologyEntries.Num());

	for (int32 i = 0; i < ChronologyEntries.Num(); ++i)
	{
		const FShadowSlaveMemoryChronologyEntry& Entry = ChronologyEntries[i];
		const FString ArcStr = UEnum::GetValueAsString(Entry.StoryArc);
		const FString ConfStr = UEnum::GetValueAsString(Entry.CanonConfidence);
		const FString RetainedStr = Entry.bIsRetainedAtArcEnd ? TEXT("Retained") : TEXT("Relinquished/Broken");

		UE_LOG(LogShadowSlave, Log, TEXT("  [%d] %s (Ch. %d, Arc: %s) | Source: %s | Status: %s | Confidence: %s"),
			i,
			*Entry.EntryId.ToString(),
			Entry.ApproximateChapter,
			*ArcStr,
			*Entry.SourceDetails.ToString(),
			*RetainedStr,
			*ConfStr
		);
	}
}

UShadowSlaveMemoryChronologyRegistry* UShadowSlaveMemoryChronologyRegistry::CreateSunnyBaselineChronologyRegistry(UObject* Outer)
{
	UObject* EffectiveOuter = Outer ? Outer : GetTransientPackage();
	UShadowSlaveMemoryChronologyRegistry* Registry = NewObject<UShadowSlaveMemoryChronologyRegistry>(
		EffectiveOuter,
		FName(TEXT("Sunny_Baseline_Memory_Chronology_Registry"))
	);

	if (!Registry)
	{
		return nullptr;
	}

	Registry->RegistryId = FName(TEXT("Registry_Sunny_Baseline"));
	Registry->RegistryName = FText::FromString(TEXT("Sunny Baseline Memory Chronology (First Nightmare -> Antarctica)"));
	Registry->Description = FText::FromString(TEXT("Chronological canon registry of Sunny's verified Memories across the First Nightmare, Forgotten Shore, Chained Isles, and Antarctica."));

	// 1. Silver Bell (First Nightmare, Ch. 8)
	{
		FShadowSlaveMemoryChronologyEntry Entry;
		Entry.EntryId = FName(TEXT("Entry_SilverBell"));
		Entry.MemoryDefinition = UShadowSlaveMemoryDefinition::CreateCanonMemoryDefinition(FName(TEXT("SilverBell")), Registry);
		Entry.TargetCharacterId = FName(TEXT("Sunless"));
		Entry.StoryArc = EShadowSlaveStoryArc::FirstNightmare;
		Entry.ApproximateChapter = 8;
		Entry.AcquisitionSource = EShadowSlaveMemoryAcquisitionSource::Other;
		Entry.SourceDetails = FText::FromString(TEXT("Slain awakened veteran soldier / slave thrall on the mountain path (First Nightmare)"));
		Entry.AcquisitionEvent = FText::FromString(TEXT("Acquired after slaying the old veteran soldier in the mountain pass during the First Nightmare (Ch. 8; runes inspected Ch. 83)"));
		Entry.OwnershipStartEvent = FText::FromString(TEXT("Obtained from the fallen veteran in the mountain pass (Ch. 8)"));
		Entry.OwnershipEndEvent = FText::FromString(TEXT("Retained across all arcs; upgraded by Sunny in Ch. 695 to Tier II with added enchantment [Sonorous]; lost upon becoming Fateless (Ch. 1758)"));
		Entry.bIsRetainedAtArcEnd = true;
		Entry.CanonConfidence = EShadowSlaveCanonConfidence::Verified;
		Entry.VerificationNotes = FText::FromString(TEXT("Chapters 8, 83, 695, 1758. Original state: Dormant Tier I charm with enchantment [Silver Song]. Upgraded to Tier II with [Sonorous] via weaving in Ch. 695."));
		Registry->ChronologyEntries.Add(Entry);
	}

	// 2. Puppeteer's Shroud (First Nightmare, Ch. 15)
	{
		FShadowSlaveMemoryChronologyEntry Entry;
		Entry.EntryId = FName(TEXT("Entry_PuppeteersShroud"));
		Entry.MemoryDefinition = UShadowSlaveMemoryDefinition::CreateCanonMemoryDefinition(FName(TEXT("PuppeteersShroud")), Registry);
		Entry.TargetCharacterId = FName(TEXT("Sunless"));
		Entry.StoryArc = EShadowSlaveStoryArc::FirstNightmare;
		Entry.ApproximateChapter = 15;
		Entry.AcquisitionSource = EShadowSlaveMemoryAcquisitionSource::Spell;
		Entry.SourceDetails = FText::FromString(TEXT("Nightmare Spell Glorious Evaluation Reward"));
		Entry.AcquisitionEvent = FText::FromString(TEXT("Waking in the NQSC outskirts after orchestrating the Mountain King's death as a slave (Ch. 15; runes inspected Ch. 83)"));
		Entry.OwnershipStartEvent = FText::FromString(TEXT("Awarded by the Spell upon First Nightmare evaluation (Ch. 15)"));
		Entry.OwnershipEndEvent = FText::FromString(TEXT("Retained through Forgotten Shore, Chained Isles, and Antarctica; modified via weaving in Ch. 1473 ([Blessing of Spirit] added); lost upon becoming Fateless (Ch. 1758)"));
		Entry.bIsRetainedAtArcEnd = true;
		Entry.CanonConfidence = EShadowSlaveCanonConfidence::Verified;
		Entry.VerificationNotes = FText::FromString(TEXT("Chapters 15, 83, 1473, 1758. Awakened Tier V armor with original enchantments [Enhanced Durability] and [Doubtless]. [Blessing of Spirit] added much later via weaving in Ch. 1473."));
		Registry->ChronologyEntries.Add(Entry);
	}

	// 3. Endless Spring (Forgotten Shore, Ch. 36)
	{
		FShadowSlaveMemoryChronologyEntry Entry;
		Entry.EntryId = FName(TEXT("Entry_EndlessSpring"));
		Entry.MemoryDefinition = UShadowSlaveMemoryDefinition::CreateCanonMemoryDefinition(FName(TEXT("EndlessSpring")), Registry);
		Entry.TargetCharacterId = FName(TEXT("Sunless"));
		Entry.StoryArc = EShadowSlaveStoryArc::ForgottenShore;
		Entry.ApproximateChapter = 36;
		Entry.AcquisitionSource = EShadowSlaveMemoryAcquisitionSource::Character;
		Entry.SourceDetails = FText::FromString(TEXT("Gift from Cassie in the coastal grove (Ch. 36)"));
		Entry.AcquisitionEvent = FText::FromString(TEXT("Given to Sunny by Cassie in the coastal grove (Ch. 36; runes inspected Ch. 104)"));
		Entry.OwnershipStartEvent = FText::FromString(TEXT("Received from Cassie (Ch. 36)"));
		Entry.OwnershipEndEvent = FText::FromString(TEXT("Retained through Antarctica; upgraded via weaving in Ch. 1473 ([Blessing of Flesh] added); lost upon becoming Fateless (Ch. 1758)"));
		Entry.bIsRetainedAtArcEnd = true;
		Entry.CanonConfidence = EShadowSlaveCanonConfidence::Verified;
		Entry.VerificationNotes = FText::FromString(TEXT("Chapters 36, 104, 1473, 1758. Original state: Dormant Tier IV tool with single enchantment [Gift of Water]. [Blessing of Flesh] added much later via weaving in Ch. 1473."));
		Registry->ChronologyEntries.Add(Entry);
	}

	// 4. Midnight Shard (Forgotten Shore, Ch. 74)
	{
		FShadowSlaveMemoryChronologyEntry Entry;
		Entry.EntryId = FName(TEXT("Entry_MidnightShard"));
		Entry.MemoryDefinition = UShadowSlaveMemoryDefinition::CreateCanonMemoryDefinition(FName(TEXT("MidnightShard")), Registry);
		Entry.TargetCharacterId = FName(TEXT("Sunless"));
		Entry.StoryArc = EShadowSlaveStoryArc::ForgottenShore;
		Entry.ApproximateChapter = 74;
		Entry.AcquisitionSource = EShadowSlaveMemoryAcquisitionSource::NightmareCreature;
		Entry.SourceDetails = FText::FromString(TEXT("Awakened Demon: Centurion of the Carapace (yielded by Nephis in Ch. 74)"));
		Entry.AcquisitionEvent = FText::FromString(TEXT("Slaying the Carapace Centurion in the labyrinth with Nephis; yielded to Sunny in Ch. 74, runes inspected in Ch. 83"));
		Entry.OwnershipStartEvent = FText::FromString(TEXT("Received following the Carapace Centurion battle (Ch. 74)"));
		Entry.OwnershipEndEvent = FText::FromString(TEXT("Retained through Chained Isles and Antarctica (inspected in Ch. 1030 to study its weave for Siege Souvenir; verified in inventory Ch. 1344); lost upon becoming Fateless (Ch. 1758)"));
		Entry.bIsRetainedAtArcEnd = true;
		Entry.CanonConfidence = EShadowSlaveCanonConfidence::Verified;
		Entry.VerificationNotes = FText::FromString(TEXT("Chapters 74, 83, 1030, 1344, 1758. Awakened Tier III tachi sword with single enchantment [Unbroken]. Not destroyed in Ch. 336; retained and later studied in Ch. 1030."));
		Registry->ChronologyEntries.Add(Entry);
	}

	// 5. Ordinary Rock (Forgotten Shore, Ch. 96)
	{
		FShadowSlaveMemoryChronologyEntry Entry;
		Entry.EntryId = FName(TEXT("Entry_OrdinaryRock"));
		Entry.MemoryDefinition = UShadowSlaveMemoryDefinition::CreateCanonMemoryDefinition(FName(TEXT("OrdinaryRock")), Registry);
		Entry.TargetCharacterId = FName(TEXT("Sunless"));
		Entry.StoryArc = EShadowSlaveStoryArc::ForgottenShore;
		Entry.ApproximateChapter = 96;
		Entry.AcquisitionSource = EShadowSlaveMemoryAcquisitionSource::NightmareCreature;
		Entry.SourceDetails = FText::FromString(TEXT("Awakened Monster: Subterranean Rock-Eater (Ch. 96)"));
		Entry.AcquisitionEvent = FText::FromString(TEXT("Slaying the subterranean rock-eating monster in the labyrinth underground (Ch. 96; runes inspected Ch. 104)"));
		Entry.OwnershipStartEvent = FText::FromString(TEXT("Acquired from subterranean rock-eater drop (Ch. 96)"));
		Entry.OwnershipEndEvent = FText::FromString(TEXT("Retained; modified and upgraded in Ch. 696 to Tier II with thought-to-sound and [Sonorous], becoming Extraordinary Rock; lost upon becoming Fateless (Ch. 1758)"));
		Entry.bIsRetainedAtArcEnd = true;
		Entry.CanonConfidence = EShadowSlaveCanonConfidence::Verified;
		Entry.VerificationNotes = FText::FromString(TEXT("Chapters 96, 104, 696, 1758. Original state: Ordinary Rock, Awakened Tier I tool with [Not Really]. Upgraded to Tier II with thought-to-sound functionality and [Sonorous] in Ch. 696, becoming Extraordinary Rock."));
		Registry->ChronologyEntries.Add(Entry);
	}

	// 6. Prowling Thorn (Forgotten Shore, Ch. 96)
	{
		FShadowSlaveMemoryChronologyEntry Entry;
		Entry.EntryId = FName(TEXT("Entry_ProwlingThorn"));
		Entry.MemoryDefinition = UShadowSlaveMemoryDefinition::CreateCanonMemoryDefinition(FName(TEXT("ProwlingThorn")), Registry);
		Entry.TargetCharacterId = FName(TEXT("Sunless"));
		Entry.StoryArc = EShadowSlaveStoryArc::ForgottenShore;
		Entry.ApproximateChapter = 96;
		Entry.AcquisitionSource = EShadowSlaveMemoryAcquisitionSource::NightmareCreature;
		Entry.SourceDetails = FText::FromString(TEXT("Awakened Monster: Armored Porcupine (Ch. 96)"));
		Entry.AcquisitionEvent = FText::FromString(TEXT("Dropped after slaying the armored porcupine beast in the labyrinth (Ch. 96; runes inspected Ch. 104)"));
		Entry.OwnershipStartEvent = FText::FromString(TEXT("Obtained from porcupine monster drop (Ch. 96)"));
		Entry.OwnershipEndEvent = FText::FromString(TEXT("Fed to and consumed by Onyx Saint in Antarctica (Ch. 1026)"));
		Entry.bIsRetainedAtArcEnd = true;
		Entry.CanonConfidence = EShadowSlaveCanonConfidence::Verified;
		Entry.VerificationNotes = FText::FromString(TEXT("Chapters 96, 104, 1026. Awakened Tier II weapon with [Rose of Betrayal] (invisible tether). Consumed by Onyx Saint in Ch. 1026."));
		Registry->ChronologyEntries.Add(Entry);
	}

	// 7. Mantle of the Underworld (Forgotten Shore, Ch. 174)
	{
		FShadowSlaveMemoryChronologyEntry Entry;
		Entry.EntryId = FName(TEXT("Entry_MantleOfTheUnderworld"));
		Entry.MemoryDefinition = UShadowSlaveMemoryDefinition::CreateCanonMemoryDefinition(FName(TEXT("MantleOfTheUnderworld")), Registry);
		Entry.TargetCharacterId = FName(TEXT("Sunless"));
		Entry.StoryArc = EShadowSlaveStoryArc::ForgottenShore;
		Entry.ApproximateChapter = 174;
		Entry.AcquisitionSource = EShadowSlaveMemoryAcquisitionSource::Other;
		Entry.SourceDetails = FText::FromString(TEXT("Purchased broken in Dark City market from scavenger merchant (Ch. 174); forged by Prince Nether, Daemon of Choice"));
		Entry.AcquisitionEvent = FText::FromString(TEXT("Purchased as broken fragments ('M... Un...old') for 7 soul shards in Dark City market (Ch. 174; first seen Ch. 173)"));
		Entry.OwnershipStartEvent = FText::FromString(TEXT("Bought broken fragments in Dark City (Ch. 174); fully repaired as Ascended Tier VI armor in Ch. 377-378"));
		Entry.OwnershipEndEvent = FText::FromString(TEXT("Bound to Sunny's soul via [Soulbound Relic] in Ch. 974 after reaching Tier VII (Ch. 972), permanently transforming into the [Marble Shell] Attribute"));
		Entry.bIsRetainedAtArcEnd = true;
		Entry.CanonConfidence = EShadowSlaveCanonConfidence::Verified;
		Entry.VerificationNotes = FText::FromString(TEXT("Chapters 173-174 (bought broken), 377-378 (repaired to Ascended Tier VI), 972 (upgraded to Tier VII), 974 (bound into Marble Shell via [Soulbound Relic]). Six verified enchantments: [Living Stone], [Feather of Truth], [Stalwart], [Underworld Armament], [Prince of the Underworld], [Soulbound Relic]."));
		Registry->ChronologyEntries.Add(Entry);
	}

	// 8. Dark Wing (Forgotten Shore, Ch. 242)
	{
		FShadowSlaveMemoryChronologyEntry Entry;
		Entry.EntryId = FName(TEXT("Entry_DarkWing"));
		Entry.MemoryDefinition = UShadowSlaveMemoryDefinition::CreateCanonMemoryDefinition(FName(TEXT("DarkWing")), Registry);
		Entry.TargetCharacterId = FName(TEXT("Sunless"));
		Entry.StoryArc = EShadowSlaveStoryArc::ForgottenShore;
		Entry.ApproximateChapter = 242;
		Entry.AcquisitionSource = EShadowSlaveMemoryAcquisitionSource::NightmareCreature;
		Entry.SourceDetails = FText::FromString(TEXT("Awakened Monster: Flesh Reaver (Ch. 242)"));
		Entry.AcquisitionEvent = FText::FromString(TEXT("Dropped after slaying a Flesh Reaver monster on the Dark City perimeter (Ch. 242)"));
		Entry.OwnershipStartEvent = FText::FromString(TEXT("Acquired from Flesh Reaver drop (Ch. 242)"));
		Entry.OwnershipEndEvent = FText::FromString(TEXT("Retained through Antarctica until loss of Spell Memories (Ch. 1758)"));
		Entry.bIsRetainedAtArcEnd = true;
		Entry.CanonConfidence = EShadowSlaveCanonConfidence::Verified;
		Entry.VerificationNotes = FText::FromString(TEXT("Chapters 242, 1758. Awakened Tier I garment/charm with [Glide]. Maintained across arcs until becoming Fateless."));
		Registry->ChronologyEntries.Add(Entry);
	}

	// 9. Blood Blossom (Forgotten Shore, Ch. 242)
	{
		FShadowSlaveMemoryChronologyEntry Entry;
		Entry.EntryId = FName(TEXT("Entry_BloodBlossom"));
		Entry.MemoryDefinition = UShadowSlaveMemoryDefinition::CreateCanonMemoryDefinition(FName(TEXT("BloodBlossom")), Registry);
		Entry.TargetCharacterId = FName(TEXT("Sunless"));
		Entry.StoryArc = EShadowSlaveStoryArc::ForgottenShore;
		Entry.ApproximateChapter = 242;
		Entry.AcquisitionSource = EShadowSlaveMemoryAcquisitionSource::NightmareCreature;
		Entry.SourceDetails = FText::FromString(TEXT("Awakened Monster: Blood Flower (Ch. 242)"));
		Entry.AcquisitionEvent = FText::FromString(TEXT("Dropped after destroying a Blood Flower monster (Ch. 242)"));
		Entry.OwnershipStartEvent = FText::FromString(TEXT("Acquired from Blood Flower drop (Ch. 242)"));
		Entry.OwnershipEndEvent = FText::FromString(TEXT("Transferred and given to Belle during the Antarctica campaign (Ch. 844)"));
		Entry.bIsRetainedAtArcEnd = true;
		Entry.CanonConfidence = EShadowSlaveCanonConfidence::Verified;
		Entry.VerificationNotes = FText::FromString(TEXT("Chapters 242, 844. Awakened Tier II charm with [Flower of Evil]. Transferred to Belle in Ch. 844."));
		Registry->ChronologyEntries.Add(Entry);
	}

	// 10. Moonlight Shard (Forgotten Shore, Ch. 261)
	{
		FShadowSlaveMemoryChronologyEntry Entry;
		Entry.EntryId = FName(TEXT("Entry_MoonlightShard"));
		Entry.MemoryDefinition = UShadowSlaveMemoryDefinition::CreateCanonMemoryDefinition(FName(TEXT("MoonlightShard")), Registry);
		Entry.TargetCharacterId = FName(TEXT("Sunless"));
		Entry.StoryArc = EShadowSlaveStoryArc::ForgottenShore;
		Entry.ApproximateChapter = 261;
		Entry.AcquisitionSource = EShadowSlaveMemoryAcquisitionSource::Character;
		Entry.SourceDetails = FText::FromString(TEXT("Gift from Nephis in the Bright Castle (Ch. 261)"));
		Entry.AcquisitionEvent = FText::FromString(TEXT("Given by Nephis to Sunny in the Bright Castle before hunting expeditions (Ch. 261; runes inspected Ch. 262)"));
		Entry.OwnershipStartEvent = FText::FromString(TEXT("Gifted by Nephis (Ch. 261)"));
		Entry.OwnershipEndEvent = FText::FromString(TEXT("Retained through Forgotten Shore and later arcs until becoming Fateless (Ch. 1758)"));
		Entry.bIsRetainedAtArcEnd = true;
		Entry.CanonConfidence = EShadowSlaveCanonConfidence::Verified;
		Entry.VerificationNotes = FText::FromString(TEXT("Chapters 261-262, 336-337, 1758. Ascended Tier I glass stiletto dagger ([Unseen]). Gifted by Nephis in Ch. 261 (not Cassie, not Ch. 324)."));
		Registry->ChronologyEntries.Add(Entry);
	}

	// 11. Weaver's Mask (Forgotten Shore, Ch. 276)
	{
		FShadowSlaveMemoryChronologyEntry Entry;
		Entry.EntryId = FName(TEXT("Entry_WeaversMask"));
		Entry.MemoryDefinition = UShadowSlaveMemoryDefinition::CreateCanonMemoryDefinition(FName(TEXT("WeaversMask")), Registry);
		Entry.TargetCharacterId = FName(TEXT("Sunless"));
		Entry.StoryArc = EShadowSlaveStoryArc::ForgottenShore;
		Entry.ApproximateChapter = 276;
		Entry.AcquisitionSource = EShadowSlaveMemoryAcquisitionSource::StoryEvent;
		Entry.SourceDetails = FText::FromString(TEXT("Daemon of Fate (Weaver) Subterranean Cathedral Sanctuary"));
		Entry.AcquisitionEvent = FText::FromString(TEXT("Taken from the headless statue of Weaver in the hidden underground sanctum beneath the Dark City Cathedral (Ch. 276-278)"));
		Entry.OwnershipStartEvent = FText::FromString(TEXT("Claimed from the headless statue of Weaver (Ch. 278)"));
		Entry.OwnershipEndEvent = FText::FromString(TEXT("Retained through all arcs until becoming Fateless (Ch. 1758)"));
		Entry.bIsRetainedAtArcEnd = true;
		Entry.CanonConfidence = EShadowSlaveCanonConfidence::Verified;
		Entry.VerificationNotes = FText::FromString(TEXT("Chapters 276-278, 300+, 1758. Divine Tier VII tool ([Simple Trick], [Where is my eye?], [Mantle of Lies]). Inverts Flaw, reveals destiny/essence strings, conceals identity and true name."));
		Registry->ChronologyEntries.Add(Entry);
	}

	// 12. Autumn Leaf (Chained Isles, Ch. 388)
	{
		FShadowSlaveMemoryChronologyEntry Entry;
		Entry.EntryId = FName(TEXT("Entry_AutumnLeaf"));
		Entry.MemoryDefinition = UShadowSlaveMemoryDefinition::CreateCanonMemoryDefinition(FName(TEXT("AutumnLeaf")), Registry);
		Entry.TargetCharacterId = FName(TEXT("Sunless"));
		Entry.StoryArc = EShadowSlaveStoryArc::ChainedIsles;
		Entry.ApproximateChapter = 388;
		Entry.AcquisitionSource = EShadowSlaveMemoryAcquisitionSource::Other;
		Entry.SourceDetails = FText::FromString(TEXT("Purchased in Sanctuary market (Ch. 388/392)"));
		Entry.AcquisitionEvent = FText::FromString(TEXT("Purchased by Sunny in the Sanctuary market to alter hair appearance (Ch. 388/392)"));
		Entry.OwnershipStartEvent = FText::FromString(TEXT("Purchased in market (Ch. 388)"));
		Entry.OwnershipEndEvent = FText::FromString(TEXT("Weave extracted and incorporated into the Lord of Shadows mask (Ch. 1685)"));
		Entry.bIsRetainedAtArcEnd = true;
		Entry.CanonConfidence = EShadowSlaveCanonConfidence::Unknown;
		Entry.VerificationNotes = FText::FromString(TEXT("Chapters 388, 392, 1685. Cosmetic charm used to change hair color. Rank, Tier, and specific Enchantment name are canonically UNKNOWN. Weave extracted in Ch. 1685."));
		Registry->ChronologyEntries.Add(Entry);
	}

	// 13. Cruel Sight (Chained Isles, Ch. 413)
	{
		FShadowSlaveMemoryChronologyEntry Entry;
		Entry.EntryId = FName(TEXT("Entry_CruelSight"));
		Entry.MemoryDefinition = UShadowSlaveMemoryDefinition::CreateCanonMemoryDefinition(FName(TEXT("CruelSight")), Registry);
		Entry.TargetCharacterId = FName(TEXT("Sunless"));
		Entry.StoryArc = EShadowSlaveStoryArc::ChainedIsles;
		Entry.ApproximateChapter = 413;
		Entry.AcquisitionSource = EShadowSlaveMemoryAcquisitionSource::NightmareCreature;
		Entry.SourceDetails = FText::FromString(TEXT("Ascended Demon: Mirror Beast (Reckoning Island, Chained Isles)"));
		Entry.AcquisitionEvent = FText::FromString(TEXT("Dropped after slaying the Mirror Beast on Reckoning Island (Ch. 413; runes inspected Ch. 415)"));
		Entry.OwnershipStartEvent = FText::FromString(TEXT("Acquired from Mirror Beast drop (Ch. 413)"));
		Entry.OwnershipEndEvent = FText::FromString(TEXT("Retained through Antarctica until loss of Spell Memories (Ch. 1758)"));
		Entry.bIsRetainedAtArcEnd = true;
		Entry.CanonConfidence = EShadowSlaveCanonConfidence::Verified;
		Entry.VerificationNotes = FText::FromString(TEXT("Chapters 413, 415, 1758. Ascended Tier IV spear dropped by Mirror Beast in the Chained Isles. Four original enchantments: [Shapeshifter], [Light Eater], [Ghost Blade], [Dark Mirror]."));
		Registry->ChronologyEntries.Add(Entry);
	}

	// 14. Covetous Coffer (Chained Isles, Ch. 427)
	{
		FShadowSlaveMemoryChronologyEntry Entry;
		Entry.EntryId = FName(TEXT("Entry_CovetousCoffer"));
		Entry.MemoryDefinition = UShadowSlaveMemoryDefinition::CreateCanonMemoryDefinition(FName(TEXT("CovetousCoffer")), Registry);
		Entry.TargetCharacterId = FName(TEXT("Sunless"));
		Entry.StoryArc = EShadowSlaveStoryArc::ChainedIsles;
		Entry.ApproximateChapter = 427;
		Entry.AcquisitionSource = EShadowSlaveMemoryAcquisitionSource::NightmareCreature;
		Entry.SourceDetails = FText::FromString(TEXT("Ascended Monster / Demon: Mordant Mimic (Shipwreck Island, Chained Isles)"));
		Entry.AcquisitionEvent = FText::FromString(TEXT("Slaying the Mordant Mimic on Shipwreck Island (Ch. 427; runes inspected Ch. 428)"));
		Entry.OwnershipStartEvent = FText::FromString(TEXT("Acquired from Mordant Mimic drop (Ch. 427)"));
		Entry.OwnershipEndEvent = FText::FromString(TEXT("Retained through Antarctica; later transformed/re-woven into Marvelous Mimic Echo/Memory"));
		Entry.bIsRetainedAtArcEnd = true;
		Entry.CanonConfidence = EShadowSlaveCanonConfidence::Verified;
		Entry.VerificationNotes = FText::FromString(TEXT("Chapters 427-428. Ascended Tier IV tool. Four enchantments: [Mendacious Coffer], [Capacious Chest], [Locomotive Chiffonier], [Lurid Trunk]."));
		Registry->ChronologyEntries.Add(Entry);
	}

	// 15. Sin of Solace (Antarctica, Ch. 867)
	{
		FShadowSlaveMemoryChronologyEntry Entry;
		Entry.EntryId = FName(TEXT("Entry_SinOfSolace"));
		Entry.MemoryDefinition = UShadowSlaveMemoryDefinition::CreateCanonMemoryDefinition(FName(TEXT("SinOfSolace")), Registry);
		Entry.TargetCharacterId = FName(TEXT("Sunless"));
		Entry.StoryArc = EShadowSlaveStoryArc::Antarctica;
		Entry.ApproximateChapter = 867;
		Entry.AcquisitionSource = EShadowSlaveMemoryAcquisitionSource::NightmareCreature;
		Entry.SourceDetails = FText::FromString(TEXT("Corrupted Tyrant: Remnant of the Jade Queen (Antarctica)"));
		Entry.AcquisitionEvent = FText::FromString(TEXT("Slaying the Corrupted Tyrant Remnant of the Jade Queen in Antarctica (Ch. 867; runes inspected Ch. 869)"));
		Entry.OwnershipStartEvent = FText::FromString(TEXT("Dropped upon slaying the Remnant of the Jade Queen (Ch. 867)"));
		Entry.OwnershipEndEvent = FText::FromString(TEXT("Fed to and consumed by Onyx Saint in Ch. 1576 prior to the Estuary"));
		Entry.bIsRetainedAtArcEnd = true;
		Entry.CanonConfidence = EShadowSlaveCanonConfidence::Verified;
		Entry.VerificationNotes = FText::FromString(TEXT("Chapters 867, 869, 1576. Transcendent Tier V jade straight sword (shuangshou jian). Acquired in Antarctica from Remnant of the Jade Queen. Five enchantments: [Sinister Whisper], [Perfect Jade], [Omen of Dread], [Hideous Truth], [Cursed]. Consumed by Onyx Saint in Ch. 1576."));
		Registry->ChronologyEntries.Add(Entry);
	}

	// 16. Siege Souvenir (Antarctica, Ch. 1030)
	{
		FShadowSlaveMemoryChronologyEntry Entry;
		Entry.EntryId = FName(TEXT("Entry_SiegeSouvenir"));
		Entry.MemoryDefinition = UShadowSlaveMemoryDefinition::CreateCanonMemoryDefinition(FName(TEXT("SiegeSouvenir")), Registry);
		Entry.TargetCharacterId = FName(TEXT("Sunless"));
		Entry.StoryArc = EShadowSlaveStoryArc::Antarctica;
		Entry.ApproximateChapter = 1030;
		Entry.AcquisitionSource = EShadowSlaveMemoryAcquisitionSource::SpellsmithMemoryCreation;
		Entry.SourceDetails = FText::FromString(TEXT("Hand-crafted by Sunny via Weaving from a Corrupted Devil quill (Ch. 1030)"));
		Entry.AcquisitionEvent = FText::FromString(TEXT("Woven by Sunny using a quill from a Corrupted Devil and the weave structure of Midnight Shard (Ch. 1030)"));
		Entry.OwnershipStartEvent = FText::FromString(TEXT("Created via Weaving (Ch. 1030)"));
		Entry.OwnershipEndEvent = FText::FromString(TEXT("Destroyed / expended during the battle against the Corrupted Titan Goliath at Falcon Scott (Ch. 1031)"));
		Entry.bIsRetainedAtArcEnd = false;
		Entry.CanonConfidence = EShadowSlaveCanonConfidence::Verified;
		Entry.VerificationNotes = FText::FromString(TEXT("Chapters 1030-1031. Transcendent weapon (3-meter white javelin). Rank is explicitly verified Transcendent in Ch. 1030; Tier is canonically UNKNOWN. Two enchantments: [Death Dealer] and an unnamed custom essence reservoir based on [Unbroken]. Single-use weapon destroyed killing Goliath in Ch. 1031."));
		Registry->ChronologyEntries.Add(Entry);
	}

	return Registry;
}
