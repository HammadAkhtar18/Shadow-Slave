// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Save/ShadowSlaveSaveTypes.h"
#include "Dialogue/ShadowSlaveDialogueTypes.h"
#include "Nightmares/ShadowSlaveNightmareSaveTypes.h"
#include "Story/ShadowSlaveStoryTypes.h"
#include "Gameplay/ShadowSlaveQuestTypes.h"
#include "ShadowSlaveSaveGame.generated.h"

/**
 * Main SaveGame class for Shadow Slave.
 * Encapsulates serialized data snapshots for player transform, attributes, progression,
 * Aspect, inventory, Memories, and persistent world state.
 *
 * NOTE ON RUNTIME AUTHORITY:
 * This class is strictly a passive serialized snapshot container.
 * It NEVER acts as a live runtime authority or duplicates live component logic.
 */
UCLASS()
class SHADOWSLAVE_API UShadowSlaveSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UShadowSlaveSaveGame();

	/* --- Save Metadata & Versioning --- */

	/** Schema version number for backwards compatibility and future migration support */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Save|Meta")
	int32 SaveVersion = 1;

	/** Current prototype version */
	static constexpr int32 CurrentSaveVersion = 1;

	/** Minimum supported version for backwards compatibility */
	static constexpr int32 MinSupportedSaveVersion = 1;

	/** Save slot identifier */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Save|Meta")
	FString SaveSlotName = TEXT("DefaultSaveSlot");

	/** User index */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Save|Meta")
	int32 UserIndex = 0;

	/** Real-world UTC timestamp when this save was captured */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Save|Meta")
	FDateTime Timestamp;

	/** Optional human-readable save title or note */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Save|Meta")
	FString SaveTitle;

	/* --- Serialized Snapshots --- */

	/** Player character world location, rotation, and scale */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Save|Player")
	FShadowSlavePlayerSaveTransform PlayerTransform;

	/** Serialized snapshot of player attributes (Health, Stamina, Essence) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Save|Player")
	FShadowSlaveAttributeSaveData AttributeData;

	/** Serialized snapshot of character progression and soul core state */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Save|Progression")
	FShadowSlaveProgressionSaveData ProgressionData;

	/** Serialized snapshot of Aspect identity, abilities, and Flaw */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Save|Aspect")
	FShadowSlaveAspectSaveData AspectData;

	/** Serialized snapshot of inventory items with preserved GUIDs and quantities */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Save|Inventory")
	FShadowSlaveInventorySaveData InventoryData;

	/** Serialized snapshot of owned Memories with preserved GUIDs and runtime states */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Save|Memories")
	FShadowSlaveMemoryCollectionSaveData MemoryData;

	/** Serialized snapshot of owned Echoes with preserved GUIDs and runtime states */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Save|Echoes")
	FShadowSlaveEchoCollectionSaveData EchoData;

	/** Serialized snapshot of equipment slot bindings */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Save|Equipment")
	FShadowSlaveEquipmentSaveData EquipmentData;

	/** Serialized snapshot of owned status effects with preserved GUIDs and stacks */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Save|StatusEffects")
	FShadowSlaveStatusEffectCollectionSaveData StatusEffectData;

	/** Serialized snapshot of opt-in persistent world objects */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Save|World")
	FShadowSlaveWorldSaveData WorldData;

	/** Serialized snapshot of Nightmare session and scenario state (if active) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Save|Nightmare")
	FShadowSlaveNightmareSaveData NightmareData;

	/** Serialized snapshot of story progression state */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Save|Story")
	FShadowSlaveStorySaveData StoryData;

	/** Serialized snapshot of quest and objective progression state */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Save|Quests")
	FShadowSlaveQuestSaveData QuestData;

	/** Serialized passive dialogue runtime variables; active conversation presentation is intentionally excluded. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Save|Dialogue")
	FShadowSlaveConversationSaveData ConversationData;

	/* --- Validation Helpers --- */

	/** Validates save format version and returns true if compatible */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Save|Validation")
	bool IsCompatibleVersion() const
	{
		return SaveVersion >= MinSupportedSaveVersion && SaveVersion <= CurrentSaveVersion;
	}
};
