// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Save/ShadowSlaveSaveTypes.h"
#include "Save/ShadowSlaveSaveGame.h"
#include "ShadowSlaveSaveSubsystem.generated.h"

class APawn;
class UWorld;
class UShadowSlaveAttributeComponent;
class UShadowSlaveProgressionComponent;
class UShadowSlaveAspectComponent;
class UShadowSlaveInventoryComponent;
class UShadowSlaveMemoryComponent;
class UShadowSlaveEchoComponent;
class UShadowSlaveEquipmentComponent;
class UShadowSlaveStatusEffectComponent;
class UShadowSlaveMemoryDefinition;
class UShadowSlaveEchoDefinition;
class UShadowSlaveItemDefinition;
class UShadowSlaveAspectDefinition;
class UShadowSlaveFlawDefinition;
class UShadowSlaveStatusEffectDefinition;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSaveGameCompletedSignature, bool, bSuccess, const FString&, SlotName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLoadGameCompletedSignature, bool, bSuccess, const FString&, SlotName);

/**
 * Game Instance Subsystem responsible for coordinating Save/Load operations in Shadow Slave.
 * Manages save slot persistence, deterministic serialization/deserialization,
 * definition resolution, and state restoration across runtime components and opt-in world actors.
 *
 * NOTE ON ARCHITECTURE:
 * Runtime systems remain strictly authoritative during gameplay.
 * The subsystem gathers snapshots to save, and distributes data on load.
 */
UCLASS()
class SHADOWSLAVE_API UShadowSlaveSaveSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UShadowSlaveSaveSubsystem();

	/* --- High-Level Save / Load API --- */

	/**
	 * Gathers current gameplay state, creates a save snapshot, and writes to disk.
	 * Returns true if the save succeeded.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Save")
	bool SaveGame(const FString& SlotName = TEXT("DefaultSaveSlot"), int32 UserIndex = 0);

	/**
	 * Reads save data from disk, validates schema, and restores state to runtime components.
	 * Returns true if the load and restoration succeeded.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Save")
	bool LoadGame(const FString& SlotName = TEXT("DefaultSaveSlot"), int32 UserIndex = 0);

	/** Returns whether a save file exists in the specified slot */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Save")
	bool DoesSaveExist(const FString& SlotName = TEXT("DefaultSaveSlot"), int32 UserIndex = 0) const;

	/** Deletes the save file in the specified slot */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Save")
	bool DeleteSave(const FString& SlotName = TEXT("DefaultSaveSlot"), int32 UserIndex = 0);

	/** Returns default save slot name */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Save")
	FString GetDefaultSlotName() const { return DefaultSlotName; }

	/** Returns default user index */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Save")
	int32 GetDefaultUserIndex() const { return DefaultUserIndex; }

	/* --- Snapshot Creation & Application --- */

	/** Gathers runtime state from player pawn and world into a new UShadowSlaveSaveGame object */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Save")
	UShadowSlaveSaveGame* CreateSaveSnapshot(APawn* PlayerPawn, UWorld* World);

	/**
	 * Applies a serialized save snapshot to runtime components in deterministic order:
	 * 1. Version validation
	 * 2. Player transform
	 * 3. Attributes (Health, Stamina, Essence)
	 * 4. Progression (Rank, Soul Cores)
	 * 5. Aspect (Aspect identity, abilities, Flaw)
	 * 6. Inventory (Items with preserved GUIDs and quantities)
	 * 7. Memories (Memories with preserved GUIDs, equipped state, runtime state)
	 * 8. World State (Opt-in persistent actors)
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Save")
	bool ApplySaveSnapshot(UShadowSlaveSaveGame* SaveGame, APawn* PlayerPawn, UWorld* World);

	/* --- Granular Capture & Restoration Helpers --- */

	void CapturePlayerTransform(APawn* PlayerPawn, FShadowSlavePlayerSaveTransform& OutTransform);
	void RestorePlayerTransform(APawn* PlayerPawn, const FShadowSlavePlayerSaveTransform& InTransform);

	void CaptureAttributes(UShadowSlaveAttributeComponent* AttrComp, FShadowSlaveAttributeSaveData& OutData);
	void RestoreAttributes(UShadowSlaveAttributeComponent* AttrComp, const FShadowSlaveAttributeSaveData& InData);

	void CaptureProgression(UShadowSlaveProgressionComponent* ProgComp, FShadowSlaveProgressionSaveData& OutData);
	void RestoreProgression(UShadowSlaveProgressionComponent* ProgComp, const FShadowSlaveProgressionSaveData& InData);

	void CaptureAspect(UShadowSlaveAspectComponent* AspectComp, FShadowSlaveAspectSaveData& OutData);
	void RestoreAspect(UShadowSlaveAspectComponent* AspectComp, const FShadowSlaveAspectSaveData& InData);

	void CaptureInventory(UShadowSlaveInventoryComponent* InvComp, FShadowSlaveInventorySaveData& OutData);
	void RestoreInventory(UShadowSlaveInventoryComponent* InvComp, const FShadowSlaveInventorySaveData& InData);

	void CaptureMemories(UShadowSlaveMemoryComponent* MemComp, FShadowSlaveMemoryCollectionSaveData& OutData);
	void RestoreMemories(UShadowSlaveMemoryComponent* MemComp, const FShadowSlaveMemoryCollectionSaveData& InData);

	void CaptureEchoes(UShadowSlaveEchoComponent* EchoComp, FShadowSlaveEchoCollectionSaveData& OutData);
	void RestoreEchoes(UShadowSlaveEchoComponent* EchoComp, const FShadowSlaveEchoCollectionSaveData& InData);

	void CaptureEquipment(UShadowSlaveEquipmentComponent* EquipComp, FShadowSlaveEquipmentSaveData& OutData);
	void RestoreEquipment(UShadowSlaveEquipmentComponent* EquipComp, const FShadowSlaveEquipmentSaveData& InData);

	void CaptureStatusEffects(UShadowSlaveStatusEffectComponent* EffectComp, FShadowSlaveStatusEffectCollectionSaveData& OutData);
	void RestoreStatusEffects(UShadowSlaveStatusEffectComponent* EffectComp, const FShadowSlaveStatusEffectCollectionSaveData& InData);

	void CaptureWorldState(UWorld* World, FShadowSlaveWorldSaveData& OutData);
	void RestoreWorldState(UWorld* World, const FShadowSlaveWorldSaveData& InData);

	/* --- Definition Resolution Boundary --- */

	UShadowSlaveMemoryDefinition* ResolveMemoryDefinition(FName MemoryId, const FPrimaryAssetId& PrimaryAssetId) const;
	UShadowSlaveEchoDefinition* ResolveEchoDefinition(FName EchoId, const FPrimaryAssetId& PrimaryAssetId) const;
	UShadowSlaveItemDefinition* ResolveItemDefinition(FName ItemDefinitionId, const FPrimaryAssetId& PrimaryAssetId) const;
	UShadowSlaveAspectDefinition* ResolveAspectDefinition(FName AspectId, const FPrimaryAssetId& PrimaryAssetId) const;
	UShadowSlaveFlawDefinition* ResolveFlawDefinition(FName FlawId, const FPrimaryAssetId& PrimaryAssetId) const;
	UShadowSlaveStatusEffectDefinition* ResolveStatusEffectDefinition(FName EffectId, const FPrimaryAssetId& PrimaryAssetId) const;

	/* --- Delegates --- */

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Save|Events")
	FOnSaveGameCompletedSignature OnSaveGameCompleted;

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Save|Events")
	FOnLoadGameCompletedSignature OnLoadGameCompleted;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Save|Config")
	FString DefaultSlotName = TEXT("DefaultSaveSlot");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Save|Config")
	int32 DefaultUserIndex = 0;
};
