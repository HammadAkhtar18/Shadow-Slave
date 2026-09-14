// Copyright Epic Games, Inc. All Rights Reserved.

#include "Save/ShadowSlaveSaveSubsystem.h"
#include "Save/ShadowSlaveSaveGame.h"
#include "Save/ShadowSlaveSaveableInterface.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/AssetManager.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Attributes/ShadowSlaveAttributeComponent.h"
#include "Progression/ShadowSlaveProgressionComponent.h"
#include "Aspects/ShadowSlaveAspectComponent.h"
#include "Aspects/ShadowSlaveAspectDefinition.h"
#include "Aspects/ShadowSlaveFlawDefinition.h"
#include "Items/ShadowSlaveInventoryComponent.h"
#include "Items/ShadowSlaveItemDefinition.h"
#include "Memories/ShadowSlaveMemoryComponent.h"
#include "Memories/ShadowSlaveMemoryDefinition.h"
#include "ShadowSlave.h"

UShadowSlaveSaveSubsystem::UShadowSlaveSaveSubsystem()
{
	DefaultSlotName = TEXT("DefaultSaveSlot");
	DefaultUserIndex = 0;
}

bool UShadowSlaveSaveSubsystem::SaveGame(const FString& SlotName, int32 UserIndex)
{
	const FString TargetSlot = SlotName.IsEmpty() ? DefaultSlotName : SlotName;
	UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;

	APawn* PlayerPawn = nullptr;
	if (World)
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			PlayerPawn = PC->GetPawn();
		}
	}

	UShadowSlaveSaveGame* SaveObject = CreateSaveSnapshot(PlayerPawn, World);
	if (!SaveObject)
	{
		UE_LOG(LogShadowSlave, Error, TEXT("SaveGame failed: could not create SaveGame snapshot."));
		OnSaveGameCompleted.Broadcast(false, TargetSlot);
		return false;
	}

	SaveObject->SaveSlotName = TargetSlot;
	SaveObject->UserIndex = UserIndex;
	SaveObject->Timestamp = FDateTime::UtcNow();

	const bool bSuccess = UGameplayStatics::SaveGameToSlot(SaveObject, TargetSlot, UserIndex);
	if (bSuccess)
	{
		UE_LOG(LogShadowSlave, Log, TEXT("SaveGame successful for slot '%s' (v%d)."), *TargetSlot, SaveObject->SaveVersion);
	}
	else
	{
		UE_LOG(LogShadowSlave, Error, TEXT("SaveGame failed to write slot '%s' to disk."), *TargetSlot);
	}

	OnSaveGameCompleted.Broadcast(bSuccess, TargetSlot);
	return bSuccess;
}

bool UShadowSlaveSaveSubsystem::LoadGame(const FString& SlotName, int32 UserIndex)
{
	const FString TargetSlot = SlotName.IsEmpty() ? DefaultSlotName : SlotName;

	if (!DoesSaveExist(TargetSlot, UserIndex))
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("LoadGame failed: slot '%s' does not exist."), *TargetSlot);
		OnLoadGameCompleted.Broadcast(false, TargetSlot);
		return false;
	}

	USaveGame* LoadedGame = UGameplayStatics::LoadGameFromSlot(TargetSlot, UserIndex);
	UShadowSlaveSaveGame* SaveGame = Cast<UShadowSlaveSaveGame>(LoadedGame);
	if (!SaveGame)
	{
		UE_LOG(LogShadowSlave, Error, TEXT("LoadGame failed: slot '%s' could not be cast to UShadowSlaveSaveGame."), *TargetSlot);
		OnLoadGameCompleted.Broadcast(false, TargetSlot);
		return false;
	}

	if (!SaveGame->IsCompatibleVersion())
	{
		UE_LOG(LogShadowSlave, Error, TEXT("LoadGame failed: incompatible save version %d (supported range %d to %d)."),
			SaveGame->SaveVersion,
			UShadowSlaveSaveGame::MinSupportedSaveVersion,
			UShadowSlaveSaveGame::CurrentSaveVersion
		);
		OnLoadGameCompleted.Broadcast(false, TargetSlot);
		return false;
	}

	UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	APawn* PlayerPawn = nullptr;
	if (World)
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			PlayerPawn = PC->GetPawn();
		}
	}

	const bool bSuccess = ApplySaveSnapshot(SaveGame, PlayerPawn, World);
	if (bSuccess)
	{
		UE_LOG(LogShadowSlave, Log, TEXT("LoadGame successful for slot '%s'."), *TargetSlot);
	}
	else
	{
		UE_LOG(LogShadowSlave, Error, TEXT("LoadGame failed while applying snapshot for slot '%s'."), *TargetSlot);
	}

	OnLoadGameCompleted.Broadcast(bSuccess, TargetSlot);
	return bSuccess;
}

bool UShadowSlaveSaveSubsystem::DoesSaveExist(const FString& SlotName, int32 UserIndex) const
{
	const FString TargetSlot = SlotName.IsEmpty() ? DefaultSlotName : SlotName;
	return UGameplayStatics::DoesSaveGameExist(TargetSlot, UserIndex);
}

bool UShadowSlaveSaveSubsystem::DeleteSave(const FString& SlotName, int32 UserIndex)
{
	const FString TargetSlot = SlotName.IsEmpty() ? DefaultSlotName : SlotName;
	return UGameplayStatics::DeleteGameInSlot(TargetSlot, UserIndex);
}

UShadowSlaveSaveGame* UShadowSlaveSaveSubsystem::CreateSaveSnapshot(APawn* PlayerPawn, UWorld* World)
{
	UShadowSlaveSaveGame* SaveObject = Cast<UShadowSlaveSaveGame>(
		UGameplayStatics::CreateSaveGameObject(UShadowSlaveSaveGame::StaticClass())
	);

	if (!SaveObject)
	{
		return nullptr;
	}

	if (PlayerPawn)
	{
		CapturePlayerTransform(PlayerPawn, SaveObject->PlayerTransform);

		if (UShadowSlaveAttributeComponent* Attr = PlayerPawn->FindComponentByClass<UShadowSlaveAttributeComponent>())
		{
			CaptureAttributes(Attr, SaveObject->AttributeData);
		}

		if (UShadowSlaveProgressionComponent* Prog = PlayerPawn->FindComponentByClass<UShadowSlaveProgressionComponent>())
		{
			CaptureProgression(Prog, SaveObject->ProgressionData);
		}

		if (UShadowSlaveAspectComponent* Aspect = PlayerPawn->FindComponentByClass<UShadowSlaveAspectComponent>())
		{
			CaptureAspect(Aspect, SaveObject->AspectData);
		}

		if (UShadowSlaveInventoryComponent* Inv = PlayerPawn->FindComponentByClass<UShadowSlaveInventoryComponent>())
		{
			CaptureInventory(Inv, SaveObject->InventoryData);
		}

		if (UShadowSlaveMemoryComponent* Mem = PlayerPawn->FindComponentByClass<UShadowSlaveMemoryComponent>())
		{
			CaptureMemories(Mem, SaveObject->MemoryData);
		}
	}

	if (World)
	{
		CaptureWorldState(World, SaveObject->WorldData);
	}

	return SaveObject;
}

bool UShadowSlaveSaveSubsystem::ApplySaveSnapshot(UShadowSlaveSaveGame* SaveGame, APawn* PlayerPawn, UWorld* World)
{
	// 1. Validation check
	if (!SaveGame || !SaveGame->IsCompatibleVersion())
	{
		return false;
	}

	// 2. Player Transform restoration
	if (PlayerPawn && SaveGame->PlayerTransform.bIsValid)
	{
		RestorePlayerTransform(PlayerPawn, SaveGame->PlayerTransform);
	}

	// 3. Attributes restoration
	if (PlayerPawn && SaveGame->AttributeData.bIsValid)
	{
		if (UShadowSlaveAttributeComponent* Attr = PlayerPawn->FindComponentByClass<UShadowSlaveAttributeComponent>())
		{
			RestoreAttributes(Attr, SaveGame->AttributeData);
		}
	}

	// 4. Progression restoration
	if (PlayerPawn && SaveGame->ProgressionData.bIsValid)
	{
		if (UShadowSlaveProgressionComponent* Prog = PlayerPawn->FindComponentByClass<UShadowSlaveProgressionComponent>())
		{
			RestoreProgression(Prog, SaveGame->ProgressionData);
		}
	}

	// 5. Aspect restoration
	if (PlayerPawn && SaveGame->AspectData.bIsValid)
	{
		if (UShadowSlaveAspectComponent* Aspect = PlayerPawn->FindComponentByClass<UShadowSlaveAspectComponent>())
		{
			RestoreAspect(Aspect, SaveGame->AspectData);
		}
	}

	// 6. Inventory restoration
	if (PlayerPawn && SaveGame->InventoryData.bIsValid)
	{
		if (UShadowSlaveInventoryComponent* Inv = PlayerPawn->FindComponentByClass<UShadowSlaveInventoryComponent>())
		{
			RestoreInventory(Inv, SaveGame->InventoryData);
		}
	}

	// 7. Memory collection restoration
	if (PlayerPawn && SaveGame->MemoryData.bIsValid)
	{
		if (UShadowSlaveMemoryComponent* Mem = PlayerPawn->FindComponentByClass<UShadowSlaveMemoryComponent>())
		{
			RestoreMemories(Mem, SaveGame->MemoryData);
		}
	}

	// 8. World state restoration
	if (World && SaveGame->WorldData.bIsValid)
	{
		RestoreWorldState(World, SaveGame->WorldData);
	}

	return true;
}

/* --- Granular Capture & Restoration Helpers --- */

void UShadowSlaveSaveSubsystem::CapturePlayerTransform(APawn* PlayerPawn, FShadowSlavePlayerSaveTransform& OutTransform)
{
	if (!PlayerPawn)
	{
		OutTransform.bIsValid = false;
		return;
	}

	OutTransform = FShadowSlavePlayerSaveTransform(PlayerPawn->GetActorTransform());
}

void UShadowSlaveSaveSubsystem::RestorePlayerTransform(APawn* PlayerPawn, const FShadowSlavePlayerSaveTransform& InTransform)
{
	if (!PlayerPawn || !InTransform.bIsValid)
	{
		return;
	}

	PlayerPawn->SetActorTransform(InTransform.ToTransform(), false, nullptr, ETeleportType::TeleportPhysics);
}

void UShadowSlaveSaveSubsystem::CaptureAttributes(UShadowSlaveAttributeComponent* AttrComp, FShadowSlaveAttributeSaveData& OutData)
{
	if (!AttrComp)
	{
		OutData.bIsValid = false;
		return;
	}

	OutData = FShadowSlaveAttributeSaveData(
		AttrComp->GetCurrentHealth(),
		AttrComp->GetCurrentStamina(),
		AttrComp->GetCurrentEssence()
	);
}

void UShadowSlaveSaveSubsystem::RestoreAttributes(UShadowSlaveAttributeComponent* AttrComp, const FShadowSlaveAttributeSaveData& InData)
{
	if (!AttrComp || !InData.bIsValid)
	{
		return;
	}

	AttrComp->SetHealth(InData.CurrentHealth);
	AttrComp->SetStamina(InData.CurrentStamina);
	AttrComp->SetEssence(InData.CurrentEssence);
}

void UShadowSlaveSaveSubsystem::CaptureProgression(UShadowSlaveProgressionComponent* ProgComp, FShadowSlaveProgressionSaveData& OutData)
{
	if (!ProgComp)
	{
		OutData.bIsValid = false;
		return;
	}

	OutData.CharacterRank = ProgComp->GetCharacterRank();
	OutData.CurrentSoulCores = ProgComp->GetSoulCoreCount();
	OutData.MaximumSoulCores = ProgComp->GetMaxSoulCores();
	OutData.bIsValid = true;
}

void UShadowSlaveSaveSubsystem::RestoreProgression(UShadowSlaveProgressionComponent* ProgComp, const FShadowSlaveProgressionSaveData& InData)
{
	if (!ProgComp || !InData.bIsValid)
	{
		return;
	}

	ProgComp->SetCharacterRank(InData.CharacterRank);
	ProgComp->SetMaxSoulCores(InData.MaximumSoulCores);
	ProgComp->SetSoulCoreCount(InData.CurrentSoulCores);
}

void UShadowSlaveSaveSubsystem::CaptureAspect(UShadowSlaveAspectComponent* AspectComp, FShadowSlaveAspectSaveData& OutData)
{
	if (!AspectComp)
	{
		OutData.bIsValid = false;
		return;
	}

	if (UShadowSlaveAspectDefinition* AspectDef = AspectComp->GetAspectDefinition())
	{
		OutData.AspectId = AspectDef->AspectId;
		OutData.AspectAssetId = AspectDef->GetPrimaryAssetId();
	}

	if (UShadowSlaveFlawDefinition* FlawDef = AspectComp->GetFlawDefinition())
	{
		OutData.FlawId = FlawDef->FlawId;
		OutData.FlawAssetId = FlawDef->GetPrimaryAssetId();
	}

	OutData.UnlockedAbilityIds.Empty();
	for (const FShadowSlaveAspectAbilityInstance& Ability : AspectComp->GetAbilityInstances())
	{
		if (Ability.bIsUnlocked)
		{
			OutData.UnlockedAbilityIds.Add(Ability.GetAbilityId());
		}
	}

	OutData.bIsValid = true;
}

void UShadowSlaveSaveSubsystem::RestoreAspect(UShadowSlaveAspectComponent* AspectComp, const FShadowSlaveAspectSaveData& InData)
{
	if (!AspectComp || !InData.bIsValid)
	{
		return;
	}

	// Resolve Aspect definition
	UShadowSlaveAspectDefinition* ResolvedAspect = ResolveAspectDefinition(InData.AspectId, InData.AspectAssetId);
	if (ResolvedAspect)
	{
		AspectComp->SetAspectDefinition(ResolvedAspect);
	}
	else if (!InData.AspectId.IsNone())
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("RestoreAspect: could not resolve Aspect '%s'."), *InData.AspectId.ToString());
	}

	// Resolve Flaw definition
	UShadowSlaveFlawDefinition* ResolvedFlaw = ResolveFlawDefinition(InData.FlawId, InData.FlawAssetId);
	if (ResolvedFlaw)
	{
		AspectComp->SetFlawDefinition(ResolvedFlaw);
	}
	else if (!InData.FlawId.IsNone())
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("RestoreAspect: could not resolve Flaw '%s'."), *InData.FlawId.ToString());
	}

	// Unlock saved abilities
	for (const FName& AbilityId : InData.UnlockedAbilityIds)
	{
		if (!AbilityId.IsNone())
		{
			AspectComp->UnlockAbility(AbilityId);
		}
	}
}

void UShadowSlaveSaveSubsystem::CaptureInventory(UShadowSlaveInventoryComponent* InvComp, FShadowSlaveInventorySaveData& OutData)
{
	if (!InvComp)
	{
		OutData.bIsValid = false;
		return;
	}

	OutData.MaxSlots = InvComp->GetCapacity();
	OutData.Items.Empty();

	for (const FShadowSlaveItemInstance& Slot : InvComp->GetSlots())
	{
		if (Slot.IsValid())
		{
			FShadowSlaveItemSaveData ItemData;
			ItemData.InstanceId = Slot.InstanceId;
			ItemData.ItemDefinitionId = Slot.ItemDefinition ? Slot.ItemDefinition->GetFName() : NAME_None;
			ItemData.ItemPrimaryAssetId = Slot.ItemDefinition ? Slot.ItemDefinition->GetPrimaryAssetId() : FPrimaryAssetId();
			ItemData.Quantity = Slot.Quantity;
			ItemData.DynamicProperties = Slot.DynamicProperties;
			OutData.Items.Add(ItemData);
		}
	}

	OutData.bIsValid = true;
}

void UShadowSlaveSaveSubsystem::RestoreInventory(UShadowSlaveInventoryComponent* InvComp, const FShadowSlaveInventorySaveData& InData)
{
	if (!InvComp || !InData.bIsValid)
	{
		return;
	}

	TArray<FShadowSlaveItemInstance> RestoredItems;
	for (const FShadowSlaveItemSaveData& SavedItem : InData.Items)
	{
		UShadowSlaveItemDefinition* ResolvedDef = ResolveItemDefinition(SavedItem.ItemDefinitionId, SavedItem.ItemPrimaryAssetId);
		if (ResolvedDef)
		{
			FShadowSlaveItemInstance RestoredInst;
			RestoredInst.InstanceId = SavedItem.InstanceId; // Preserve original instance GUID
			RestoredInst.ItemDefinition = ResolvedDef;
			RestoredInst.Quantity = SavedItem.Quantity;
			RestoredInst.DynamicProperties = SavedItem.DynamicProperties;
			RestoredItems.Add(RestoredInst);
		}
		else
		{
			UE_LOG(LogShadowSlave, Warning, TEXT("RestoreInventory: could not resolve Item definition '%s'."), *SavedItem.ItemDefinitionId.ToString());
		}
	}

	InvComp->RestoreInventory(RestoredItems, InData.MaxSlots);
}

void UShadowSlaveSaveSubsystem::CaptureMemories(UShadowSlaveMemoryComponent* MemComp, FShadowSlaveMemoryCollectionSaveData& OutData)
{
	if (!MemComp)
	{
		OutData.bIsValid = false;
		return;
	}

	OutData.Memories.Empty();

	for (const FShadowSlaveMemoryInstance& Mem : MemComp->GetMemories())
	{
		if (Mem.IsValid())
		{
			FShadowSlaveMemorySaveData MemData;
			MemData.InstanceId = Mem.InstanceId;
			MemData.MemoryId = Mem.MemoryDefinition ? Mem.MemoryDefinition->MemoryId : NAME_None;
			MemData.MemoryPrimaryAssetId = Mem.MemoryDefinition ? Mem.MemoryDefinition->GetPrimaryAssetId() : FPrimaryAssetId();
			MemData.State = Mem.State;
			MemData.bIsEquipped = Mem.bIsEquipped;
			MemData.DynamicProperties = Mem.DynamicProperties;
			OutData.Memories.Add(MemData);
		}
	}

	OutData.bIsValid = true;
}

void UShadowSlaveSaveSubsystem::RestoreMemories(UShadowSlaveMemoryComponent* MemComp, const FShadowSlaveMemoryCollectionSaveData& InData)
{
	if (!MemComp || !InData.bIsValid)
	{
		return;
	}

	TArray<FShadowSlaveMemoryInstance> RestoredMemories;
	for (const FShadowSlaveMemorySaveData& SavedMem : InData.Memories)
	{
		UShadowSlaveMemoryDefinition* ResolvedDef = ResolveMemoryDefinition(SavedMem.MemoryId, SavedMem.MemoryPrimaryAssetId);
		if (ResolvedDef)
		{
			FShadowSlaveMemoryInstance RestoredInst;
			RestoredInst.InstanceId = SavedMem.InstanceId; // Preserve original instance GUID
			RestoredInst.MemoryDefinition = ResolvedDef;
			RestoredInst.State = SavedMem.State;
			RestoredInst.bIsEquipped = SavedMem.bIsEquipped;
			RestoredInst.DynamicProperties = SavedMem.DynamicProperties;
			RestoredMemories.Add(RestoredInst);
		}
		else
		{
			UE_LOG(LogShadowSlave, Warning, TEXT("RestoreMemories: could not resolve Memory definition '%s'."), *SavedMem.MemoryId.ToString());
		}
	}

	MemComp->RestoreMemories(RestoredMemories);
}

void UShadowSlaveSaveSubsystem::CaptureWorldState(UWorld* World, FShadowSlaveWorldSaveData& OutData)
{
	if (!World)
	{
		OutData.bIsValid = false;
		return;
	}

	OutData.PersistentActors.Empty();

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (Actor && Actor->GetClass()->ImplementsInterface(UShadowSlaveSaveableInterface::StaticClass()))
		{
			const FName PersistentId = IShadowSlaveSaveableInterface::Execute_GetPersistentSaveId(Actor);
			if (!PersistentId.IsNone())
			{
				FShadowSlaveWorldActorSaveRecord Record;
				if (IShadowSlaveSaveableInterface::Execute_CaptureSaveRecord(Actor, Record))
				{
					OutData.PersistentActors.Add(PersistentId, Record);
				}
			}
		}
	}

	OutData.bIsValid = true;
}

void UShadowSlaveSaveSubsystem::RestoreWorldState(UWorld* World, const FShadowSlaveWorldSaveData& InData)
{
	if (!World || !InData.bIsValid)
	{
		return;
	}

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (Actor && Actor->GetClass()->ImplementsInterface(UShadowSlaveSaveableInterface::StaticClass()))
		{
			const FName PersistentId = IShadowSlaveSaveableInterface::Execute_GetPersistentSaveId(Actor);
			if (!PersistentId.IsNone())
			{
				if (const FShadowSlaveWorldActorSaveRecord* FoundRecord = InData.PersistentActors.Find(PersistentId))
				{
					IShadowSlaveSaveableInterface::Execute_RestoreSaveRecord(Actor, *FoundRecord);
				}
			}
		}
	}
}

/* --- Definition Resolvers --- */

UShadowSlaveMemoryDefinition* UShadowSlaveSaveSubsystem::ResolveMemoryDefinition(FName MemoryId, const FPrimaryAssetId& PrimaryAssetId) const
{
	// 1. Attempt Asset Manager resolution if registered
	if (PrimaryAssetId.IsValid() && UAssetManager::IsInitialized())
	{
		if (UObject* AssetObj = UAssetManager::Get().GetPrimaryAssetObject(PrimaryAssetId))
		{
			if (UShadowSlaveMemoryDefinition* Def = Cast<UShadowSlaveMemoryDefinition>(AssetObj))
			{
				return Def;
			}
		}

		const FSoftObjectPath AssetPath = UAssetManager::Get().GetPrimaryAssetPath(PrimaryAssetId);
		if (AssetPath.IsValid())
		{
			if (UObject* Loaded = AssetPath.TryLoad())
			{
				if (UShadowSlaveMemoryDefinition* Def = Cast<UShadowSlaveMemoryDefinition>(Loaded))
				{
					return Def;
				}
			}
		}
	}

	// 2. Attempt canonical verified factory lookup by MemoryId
	if (!MemoryId.IsNone())
	{
		UShadowSlaveMemoryDefinition* CanonDef = UShadowSlaveMemoryDefinition::CreateCanonMemoryDefinition(MemoryId);
		if (CanonDef)
		{
			return CanonDef;
		}

		// 3. Fallback: find loaded object in memory
		if (UShadowSlaveMemoryDefinition* Found = FindObject<UShadowSlaveMemoryDefinition>(ANY_PACKAGE, *MemoryId.ToString()))
		{
			return Found;
		}
	}

	return nullptr;
}

UShadowSlaveItemDefinition* UShadowSlaveSaveSubsystem::ResolveItemDefinition(FName ItemDefinitionId, const FPrimaryAssetId& PrimaryAssetId) const
{
	// 1. Attempt Asset Manager resolution
	if (PrimaryAssetId.IsValid() && UAssetManager::IsInitialized())
	{
		if (UObject* AssetObj = UAssetManager::Get().GetPrimaryAssetObject(PrimaryAssetId))
		{
			if (UShadowSlaveItemDefinition* Def = Cast<UShadowSlaveItemDefinition>(AssetObj))
			{
				return Def;
			}
		}

		const FSoftObjectPath AssetPath = UAssetManager::Get().GetPrimaryAssetPath(PrimaryAssetId);
		if (AssetPath.IsValid())
		{
			if (UObject* Loaded = AssetPath.TryLoad())
			{
				if (UShadowSlaveItemDefinition* Def = Cast<UShadowSlaveItemDefinition>(Loaded))
				{
					return Def;
				}
			}
		}
	}

	// 2. Fallback: test item factories or loaded asset by name
	if (!ItemDefinitionId.IsNone())
	{
		if (ItemDefinitionId == FName(TEXT("TestConsumableItem")))
		{
			return UShadowSlaveItemDefinition::CreateTestConsumableDefinition();
		}
		if (ItemDefinitionId == FName(TEXT("TestQuestItem")))
		{
			return UShadowSlaveItemDefinition::CreateTestQuestItemDefinition();
		}

		if (UShadowSlaveItemDefinition* Found = FindObject<UShadowSlaveItemDefinition>(ANY_PACKAGE, *ItemDefinitionId.ToString()))
		{
			return Found;
		}
	}

	return nullptr;
}

UShadowSlaveAspectDefinition* UShadowSlaveSaveSubsystem::ResolveAspectDefinition(FName AspectId, const FPrimaryAssetId& PrimaryAssetId) const
{
	// 1. Attempt Asset Manager resolution
	if (PrimaryAssetId.IsValid() && UAssetManager::IsInitialized())
	{
		if (UObject* AssetObj = UAssetManager::Get().GetPrimaryAssetObject(PrimaryAssetId))
		{
			if (UShadowSlaveAspectDefinition* Def = Cast<UShadowSlaveAspectDefinition>(AssetObj))
			{
				return Def;
			}
		}

		const FSoftObjectPath AssetPath = UAssetManager::Get().GetPrimaryAssetPath(PrimaryAssetId);
		if (AssetPath.IsValid())
		{
			if (UObject* Loaded = AssetPath.TryLoad())
			{
				if (UShadowSlaveAspectDefinition* Def = Cast<UShadowSlaveAspectDefinition>(Loaded))
				{
					return Def;
				}
			}
		}
	}

	// 2. Fallback: find loaded object in memory
	if (!AspectId.IsNone())
	{
		if (UShadowSlaveAspectDefinition* Found = FindObject<UShadowSlaveAspectDefinition>(ANY_PACKAGE, *AspectId.ToString()))
		{
			return Found;
		}
	}

	return nullptr;
}

UShadowSlaveFlawDefinition* UShadowSlaveSaveSubsystem::ResolveFlawDefinition(FName FlawId, const FPrimaryAssetId& PrimaryAssetId) const
{
	// 1. Attempt Asset Manager resolution
	if (PrimaryAssetId.IsValid() && UAssetManager::IsInitialized())
	{
		if (UObject* AssetObj = UAssetManager::Get().GetPrimaryAssetObject(PrimaryAssetId))
		{
			if (UShadowSlaveFlawDefinition* Def = Cast<UShadowSlaveFlawDefinition>(AssetObj))
			{
				return Def;
			}
		}

		const FSoftObjectPath AssetPath = UAssetManager::Get().GetPrimaryAssetPath(PrimaryAssetId);
		if (AssetPath.IsValid())
		{
			if (UObject* Loaded = AssetPath.TryLoad())
			{
				if (UShadowSlaveFlawDefinition* Def = Cast<UShadowSlaveFlawDefinition>(Loaded))
				{
					return Def;
				}
			}
		}
	}

	// 2. Fallback: find loaded object in memory
	if (!FlawId.IsNone())
	{
		if (UShadowSlaveFlawDefinition* Found = FindObject<UShadowSlaveFlawDefinition>(ANY_PACKAGE, *FlawId.ToString()))
		{
			return Found;
		}
	}

	return nullptr;
}
