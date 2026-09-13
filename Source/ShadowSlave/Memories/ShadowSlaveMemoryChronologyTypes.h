// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ShadowSlaveMemoryChronologyTypes.generated.h"

class UShadowSlaveMemoryDefinition;

/**
 * Canon confidence grading for Shadow Slave narrative and lore data.
 * CRITICAL RULE: Only 'Verified' entries are permitted to drive canon-critical
 * story progression and gameplay rules by default.
 */
UENUM(BlueprintType)
enum class EShadowSlaveCanonConfidence : uint8
{
	/** Directly stated, named, and demonstrated in the novel */
	Verified        UMETA(DisplayName = "Verified Canon"),

	/** Highly supported by multiple canon observations but not explicitly stated */
	StrongInference UMETA(DisplayName = "Strong Inference"),

	/** Speculative or cannot be established reliably from canon; must never be hard-coded into gameplay */
	Unknown         UMETA(DisplayName = "Unknown / Unsafe")
};

/**
 * Primary narrative story arcs of the Shadow Slave novel.
 * Used for chronological tracking and ownership window gating.
 */
UENUM(BlueprintType)
enum class EShadowSlaveStoryArc : uint8
{
	None           UMETA(DisplayName = "None / Unassigned"),
	FirstNightmare UMETA(DisplayName = "First Nightmare"),
	ForgottenShore UMETA(DisplayName = "Forgotten Shore"),
	ChainedIsles   UMETA(DisplayName = "Chained Isles / Academy"),
	Antarctica     UMETA(DisplayName = "Antarctica Campaign"),
	LaterArcs      UMETA(DisplayName = "Later Arcs")
};

/**
 * Extensible categorization of how a Memory was acquired in canon.
 * Reflects that Memories come from diverse canon sources, not exclusively monster drops.
 */
UENUM(BlueprintType)
enum class EShadowSlaveMemoryAcquisitionSource : uint8
{
	Unknown                  UMETA(DisplayName = "Unknown"),
	NightmareCreature        UMETA(DisplayName = "Nightmare Creature Drop"),
	Character                UMETA(DisplayName = "Character / Gift / Trade"),
	Spell                    UMETA(DisplayName = "Nightmare Spell Reward"),
	SpellsmithMemoryCreation UMETA(DisplayName = "Spellsmith / Memory Creation"),
	StoryEvent               UMETA(DisplayName = "Story Event / Relic Discovery"),
	Other                    UMETA(DisplayName = "Other")
};

/**
 * Chronological ownership and lore metadata for a canonical Memory.
 * Decoupled from runtime instances and intrinsic definitions to prevent
 * polluting runtime structures with story timeline data.
 */
USTRUCT(BlueprintType)
struct SHADOWSLAVE_API FShadowSlaveMemoryChronologyEntry
{
	GENERATED_BODY()

	/** Unique identifier for this chronology entry */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Chronology")
	FName EntryId = NAME_None;

	/** Reference to the intrinsic static Memory definition asset */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Chronology")
	TSoftObjectPtr<UShadowSlaveMemoryDefinition> MemoryDefinition;

	/** Character who owns/acquired the Memory in this entry (e.g. 'Sunless') */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Chronology")
	FName TargetCharacterId = FName(TEXT("Sunless"));

	/** Story arc during which the Memory is acquired */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Chronology")
	EShadowSlaveStoryArc StoryArc = EShadowSlaveStoryArc::None;

	/** Approximate novel chapter number where the Memory is acquired (0 if unverified) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Chronology", meta = (ClampMin = "0"))
	int32 ApproximateChapter = 0;

	/** Narrative event description of acquisition */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Chronology")
	FText AcquisitionEvent;

	/** Broad classification of acquisition origin */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Chronology")
	EShadowSlaveMemoryAcquisitionSource AcquisitionSource = EShadowSlaveMemoryAcquisitionSource::Unknown;

	/** Specific creature, NPC, or lore origin details */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Chronology")
	FText SourceDetails;

	/** Narrative milestone where character begins possessing this Memory */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Chronology")
	FText OwnershipStartEvent;

	/** Narrative milestone where possession ends (empty if retained indefinitely) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Chronology")
	FText OwnershipEndEvent;

	/** Whether the character still possesses this Memory at the end of the specified StoryArc */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Chronology")
	bool bIsRetainedAtArcEnd = true;

	/** Verified canon confidence grading for this entry */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Chronology")
	EShadowSlaveCanonConfidence CanonConfidence = EShadowSlaveCanonConfidence::Verified;

	/** Chapter citations, novel text references, and verification rationale */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Chronology", meta = (MultiLine = true))
	FText VerificationNotes;

	FShadowSlaveMemoryChronologyEntry() = default;
};
