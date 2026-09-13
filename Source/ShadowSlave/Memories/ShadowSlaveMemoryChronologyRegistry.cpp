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

	// 1. Puppeteer's Shroud (First Nightmare)
	{
		FShadowSlaveMemoryChronologyEntry Entry;
		Entry.EntryId = FName(TEXT("Entry_PuppeteersShroud"));
		Entry.MemoryDefinition = UShadowSlaveMemoryDefinition::CreateCanonMemoryDefinition(FName(TEXT("PuppeteersShroud")), Registry);
		Entry.TargetCharacterId = FName(TEXT("Sunless"));
		Entry.StoryArc = EShadowSlaveStoryArc::FirstNightmare;
		Entry.ApproximateChapter = 20;
		Entry.AcquisitionSource = EShadowSlaveMemoryAcquisitionSource::Spell;
		Entry.SourceDetails = FText::FromString(TEXT("Nightmare Spell Glorious Evaluation Reward"));
		Entry.AcquisitionEvent = FText::FromString(TEXT("Waking in the NQSC outskirts after orchestrating the Mountain King's death as a slave"));
		Entry.OwnershipStartEvent = FText::FromString(TEXT("Awarded by the Spell upon First Nightmare evaluation"));
		Entry.OwnershipEndEvent = FText::FromString(TEXT("Retired to Soul Sea reserve upon acquiring Mantle of the Underworld (Ch. 455)"));
		Entry.bIsRetainedAtArcEnd = true;
		Entry.CanonConfidence = EShadowSlaveCanonConfidence::Verified;
		Entry.VerificationNotes = FText::FromString(TEXT("Chapters 20-22, 100+. Awakened Tier I armor ([Featherlight], [Undying], [Inconspicuous]). Primary armor through Forgotten Shore."));
		Registry->ChronologyEntries.Add(Entry);
	}

	// 2. Silver Bell (First Nightmare)
	{
		FShadowSlaveMemoryChronologyEntry Entry;
		Entry.EntryId = FName(TEXT("Entry_SilverBell"));
		Entry.MemoryDefinition = UShadowSlaveMemoryDefinition::CreateCanonMemoryDefinition(FName(TEXT("SilverBell")), Registry);
		Entry.TargetCharacterId = FName(TEXT("Sunless"));
		Entry.StoryArc = EShadowSlaveStoryArc::FirstNightmare;
		Entry.ApproximateChapter = 20;
		Entry.AcquisitionSource = EShadowSlaveMemoryAcquisitionSource::NightmareCreature;
		Entry.SourceDetails = FText::FromString(TEXT("Awakened Tyrant: Mountain King (Larva)"));
		Entry.AcquisitionEvent = FText::FromString(TEXT("Demise of the Mountain King in the mountain pass"));
		Entry.OwnershipStartEvent = FText::FromString(TEXT("Acquired directly from the Mountain King kill"));
		Entry.OwnershipEndEvent = FText::GetEmpty();
		Entry.bIsRetainedAtArcEnd = true;
		Entry.CanonConfidence = EShadowSlaveCanonConfidence::Verified;
		Entry.VerificationNotes = FText::FromString(TEXT("Chapters 19-23, 60+. Awakened Tier I charm ([Clear Chime], [Attention/Lure]). Vital acoustic decoy used across all volumes."));
		Registry->ChronologyEntries.Add(Entry);
	}

	// 3. Midnight Shard (Forgotten Shore)
	{
		FShadowSlaveMemoryChronologyEntry Entry;
		Entry.EntryId = FName(TEXT("Entry_MidnightShard"));
		Entry.MemoryDefinition = UShadowSlaveMemoryDefinition::CreateCanonMemoryDefinition(FName(TEXT("MidnightShard")), Registry);
		Entry.TargetCharacterId = FName(TEXT("Sunless"));
		Entry.StoryArc = EShadowSlaveStoryArc::ForgottenShore;
		Entry.ApproximateChapter = 41;
		Entry.AcquisitionSource = EShadowSlaveMemoryAcquisitionSource::NightmareCreature;
		Entry.SourceDetails = FText::FromString(TEXT("Awakened Demon: Centurion of the Carapace"));
		Entry.AcquisitionEvent = FText::FromString(TEXT("Slaying the Carapace Centurion in the labyrinth"));
		Entry.OwnershipStartEvent = FText::FromString(TEXT("Acquired from Centurion drop"));
		Entry.OwnershipEndEvent = FText::FromString(TEXT("Shattered against Caster's Broken Oath in the duel (Ch. 336); reforged into Moonlight Shard (Ch. 435)"));
		Entry.bIsRetainedAtArcEnd = false;
		Entry.CanonConfidence = EShadowSlaveCanonConfidence::Verified;
		Entry.VerificationNotes = FText::FromString(TEXT("Chapters 41, 335-338. Awakened Tier II tachi ([Unbroken], [Weight of Midnight], [Hone]). Primary Forgotten Shore melee weapon."));
		Registry->ChronologyEntries.Add(Entry);
	}

	// 4. Extraordinary Rock (Forgotten Shore)
	{
		FShadowSlaveMemoryChronologyEntry Entry;
		Entry.EntryId = FName(TEXT("Entry_ExtraordinaryRock"));
		Entry.TargetCharacterId = FName(TEXT("Sunless"));
		Entry.StoryArc = EShadowSlaveStoryArc::ForgottenShore;
		Entry.ApproximateChapter = 95;
		Entry.AcquisitionSource = EShadowSlaveMemoryAcquisitionSource::NightmareCreature;
		Entry.SourceDetails = FText::FromString(TEXT("Awakened Monster (Subterranean rock-eater)"));
		Entry.AcquisitionEvent = FText::FromString(TEXT("Slaying the rock-eating monster in the labyrinth underground"));
		Entry.OwnershipStartEvent = FText::FromString(TEXT("Acquired from beast drop"));
		Entry.OwnershipEndEvent = FText::GetEmpty();
		Entry.bIsRetainedAtArcEnd = true;
		Entry.CanonConfidence = EShadowSlaveCanonConfidence::Verified;
		Entry.VerificationNotes = FText::FromString(TEXT("Chapters 93-95, 160+. Awakened Tier I tool ([Extraordinary Durability], [Echoing Voice]). Retained permanently."));
		Registry->ChronologyEntries.Add(Entry);
	}

	// 5. Dark Wing (Forgotten Shore)
	{
		FShadowSlaveMemoryChronologyEntry Entry;
		Entry.EntryId = FName(TEXT("Entry_DarkWing"));
		Entry.TargetCharacterId = FName(TEXT("Sunless"));
		Entry.StoryArc = EShadowSlaveStoryArc::ForgottenShore;
		Entry.ApproximateChapter = 125;
		Entry.AcquisitionSource = EShadowSlaveMemoryAcquisitionSource::NightmareCreature;
		Entry.SourceDetails = FText::FromString(TEXT("Awakened Monster: Spire Messenger"));
		Entry.AcquisitionEvent = FText::FromString(TEXT("Ambushing and killing a Spire Messenger on the Dark City walls"));
		Entry.OwnershipStartEvent = FText::FromString(TEXT("Acquired from Spire Messenger drop"));
		Entry.OwnershipEndEvent = FText::GetEmpty();
		Entry.bIsRetainedAtArcEnd = true;
		Entry.CanonConfidence = EShadowSlaveCanonConfidence::Verified;
		Entry.VerificationNotes = FText::FromString(TEXT("Chapters 122-125. Awakened Tier II cloak ([Glide], [Featherfall], [Updraft]). Essential vertical traversal tool."));
		Registry->ChronologyEntries.Add(Entry);
	}

	// 6. Weaver's Mask (Forgotten Shore)
	{
		FShadowSlaveMemoryChronologyEntry Entry;
		Entry.EntryId = FName(TEXT("Entry_WeaversMask"));
		Entry.MemoryDefinition = UShadowSlaveMemoryDefinition::CreateCanonMemoryDefinition(FName(TEXT("WeaversMask")), Registry);
		Entry.TargetCharacterId = FName(TEXT("Sunless"));
		Entry.StoryArc = EShadowSlaveStoryArc::ForgottenShore;
		Entry.ApproximateChapter = 170;
		Entry.AcquisitionSource = EShadowSlaveMemoryAcquisitionSource::StoryEvent;
		Entry.SourceDetails = FText::FromString(TEXT("Daemon of Fate (Weaver) Sunken Temple Relic"));
		Entry.AcquisitionEvent = FText::FromString(TEXT("Recovered from the pedestal before the headless statue of Weaver"));
		Entry.OwnershipStartEvent = FText::FromString(TEXT("Claimed from the altar of Weaver"));
		Entry.OwnershipEndEvent = FText::GetEmpty();
		Entry.bIsRetainedAtArcEnd = true;
		Entry.CanonConfidence = EShadowSlaveCanonConfidence::Verified;
		Entry.VerificationNotes = FText::FromString(TEXT("Chapters 168-170+. Divine Tier VII relic ([Simple Trick], [Where is My Eye?], [Unknowable]). Foundations of Mongrel and Weaving."));
		Registry->ChronologyEntries.Add(Entry);
	}

	// 7. Cruel Sight (Forgotten Shore)
	{
		FShadowSlaveMemoryChronologyEntry Entry;
		Entry.EntryId = FName(TEXT("Entry_CruelSight"));
		Entry.MemoryDefinition = UShadowSlaveMemoryDefinition::CreateCanonMemoryDefinition(FName(TEXT("CruelSight")), Registry);
		Entry.TargetCharacterId = FName(TEXT("Sunless"));
		Entry.StoryArc = EShadowSlaveStoryArc::ForgottenShore;
		Entry.ApproximateChapter = 258;
		Entry.AcquisitionSource = EShadowSlaveMemoryAcquisitionSource::StoryEvent;
		Entry.SourceDetails = FText::FromString(TEXT("Cathedral Subterranean Altar / Fallen Devil"));
		Entry.AcquisitionEvent = FText::FromString(TEXT("Retrieved from the underground cathedral altar vault after defeating the Dark Stalker"));
		Entry.OwnershipStartEvent = FText::FromString(TEXT("Claimed from the cathedral altar"));
		Entry.OwnershipEndEvent = FText::GetEmpty();
		Entry.bIsRetainedAtArcEnd = true;
		Entry.CanonConfidence = EShadowSlaveCanonConfidence::Verified;
		Entry.VerificationNotes = FText::FromString(TEXT("Chapters 255-260, 450+, 860+. Ascended Tier VI spear ([Ghost Blade], [Somber Gleam], [Marrow Chill]). Primary anti-titan weapon; upgraded via Weaving."));
		Registry->ChronologyEntries.Add(Entry);
	}

	// 8. Covetous Coffer (Chained Isles)
	{
		FShadowSlaveMemoryChronologyEntry Entry;
		Entry.EntryId = FName(TEXT("Entry_CovetousCoffer"));
		Entry.TargetCharacterId = FName(TEXT("Sunless"));
		Entry.StoryArc = EShadowSlaveStoryArc::ChainedIsles;
		Entry.ApproximateChapter = 400;
		Entry.AcquisitionSource = EShadowSlaveMemoryAcquisitionSource::NightmareCreature;
		Entry.SourceDetails = FText::FromString(TEXT("Awakened Mimic (Demon)"));
		Entry.AcquisitionEvent = FText::FromString(TEXT("Slaying an Awakened Mimic disguised as a treasure chest in the Chained Isles"));
		Entry.OwnershipStartEvent = FText::FromString(TEXT("Acquired from Mimic drop"));
		Entry.OwnershipEndEvent = FText::GetEmpty();
		Entry.bIsRetainedAtArcEnd = true;
		Entry.CanonConfidence = EShadowSlaveCanonConfidence::Verified;
		Entry.VerificationNotes = FText::FromString(TEXT("Chapters 398-402, 600+. Awakened Tier II utility ([Voracious Appetite], [Preservation], [Living Mimic]). Semi-sentient mobile chest."));
		Registry->ChronologyEntries.Add(Entry);
	}

	// 9. Mantle of the Underworld (Chained Isles)
	{
		FShadowSlaveMemoryChronologyEntry Entry;
		Entry.EntryId = FName(TEXT("Entry_MantleOfTheUnderworld"));
		Entry.TargetCharacterId = FName(TEXT("Sunless"));
		Entry.StoryArc = EShadowSlaveStoryArc::ChainedIsles;
		Entry.ApproximateChapter = 455;
		Entry.AcquisitionSource = EShadowSlaveMemoryAcquisitionSource::StoryEvent;
		Entry.SourceDetails = FText::FromString(TEXT("Nether (Daemon of Choice, Prince of the Underworld) Forge"));
		Entry.AcquisitionEvent = FText::FromString(TEXT("Recovered from the crypt and forge of Prince Nether in the Chained Isles"));
		Entry.OwnershipStartEvent = FText::FromString(TEXT("Claimed from Nether's forge"));
		Entry.OwnershipEndEvent = FText::GetEmpty();
		Entry.bIsRetainedAtArcEnd = true;
		Entry.CanonConfidence = EShadowSlaveCanonConfidence::Verified;
		Entry.VerificationNotes = FText::FromString(TEXT("Chapters 452-458, 800+. Ascended Tier IV evolving stone plate ([Prince of the Underworld], [Feather of Stone], [Living Stone])."));
		Registry->ChronologyEntries.Add(Entry);
	}

	// 10. Siege Souvenir (Antarctica Campaign)
	{
		FShadowSlaveMemoryChronologyEntry Entry;
		Entry.EntryId = FName(TEXT("Entry_SiegeSouvenir"));
		Entry.TargetCharacterId = FName(TEXT("Sunless"));
		Entry.StoryArc = EShadowSlaveStoryArc::Antarctica;
		Entry.ApproximateChapter = 865;
		Entry.AcquisitionSource = EShadowSlaveMemoryAcquisitionSource::NightmareCreature;
		Entry.SourceDetails = FText::FromString(TEXT("Fallen / Corrupted Siege Demon"));
		Entry.AcquisitionEvent = FText::FromString(TEXT("Slain during the mass bombardment at the Falcon Scott perimeter"));
		Entry.OwnershipStartEvent = FText::FromString(TEXT("Acquired from Fallen Siege Demon drop"));
		Entry.OwnershipEndEvent = FText::FromString(TEXT("Weave extracted and dismantled to augment artillery and weapon strike ballistics"));
		Entry.bIsRetainedAtArcEnd = false;
		Entry.CanonConfidence = EShadowSlaveCanonConfidence::Verified;
		Entry.VerificationNotes = FText::FromString(TEXT("Chapters 860-880. Ascended Tier III/IV catalyst ([Siege Detonation], [Thermal Rupture])."));
		Registry->ChronologyEntries.Add(Entry);
	}

	return Registry;
}
