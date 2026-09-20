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
#include "Echoes/ShadowSlaveEchoComponent.h"
#include "Echoes/ShadowSlaveEchoDefinition.h"
#include "Equipment/ShadowSlaveEquipmentComponent.h"
#include "Nightmares/ShadowSlaveNightmareSubsystem.h"
#include "Story/ShadowSlaveStorySubsystem.h"
#include "Gameplay/ShadowSlaveQuestSubsystem.h"
#include "Dialogue/ShadowSlaveConversationSubsystem.h"
#include "World/ShadowSlaveWorldStateComponent.h"
#include "StatusEffects/ShadowSlaveStatusEffectComponent.h"
#include "StatusEffects/ShadowSlaveStatusEffectDefinition.h"
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

		if (UShadowSlaveEchoComponent* Echo = PlayerPawn->FindComponentByClass<UShadowSlaveEchoComponent>())
		{
			CaptureEchoes(Echo, SaveObject->EchoData);
		}

		if (UShadowSlaveEquipmentComponent* Equip = PlayerPawn->FindComponentByClass<UShadowSlaveEquipmentComponent>())
		{
			CaptureEquipment(Equip, SaveObject->EquipmentData);
		}

		if (UShadowSlaveStatusEffectComponent* StatusComp = PlayerPawn->FindComponentByClass<UShadowSlaveStatusEffectComponent>())
		{
			CaptureStatusEffects(StatusComp, SaveObject->StatusEffectData);
		}
	}

	if (World)
	{
		CaptureWorldState(World, SaveObject->WorldData);
	}

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UShadowSlaveNightmareSubsystem* NightmareSub = GI->GetSubsystem<UShadowSlaveNightmareSubsystem>())
		{
			SaveObject->NightmareData = NightmareSub->ExportSaveData();
		}

		if (UShadowSlaveStorySubsystem* StorySub = GI->GetSubsystem<UShadowSlaveStorySubsystem>())
		{
			SaveObject->StoryData = StorySub->ExportSaveData();
		}

		if (UShadowSlaveQuestSubsystem* QuestSub = GI->GetSubsystem<UShadowSlaveQuestSubsystem>())
		{
			SaveObject->QuestData = QuestSub->ExportSaveData();
		}

		if (UShadowSlaveConversationSubsystem* ConversationSub = GI->GetSubsystem<UShadowSlaveConversationSubsystem>())
		{
			ConversationSub->CaptureConversationState(SaveObject->ConversationData);
		}
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

	// Validate the passive dialogue restore boundary before mutating any other runtime state.
	// Active conversation presentation cannot be resumed safely, and an unavailable conversation
	// authority must not be reported as a successful load.
	UShadowSlaveConversationSubsystem* ConversationSub = nullptr;
	if (SaveGame->ConversationData.bIsValid)
	{
		UGameInstance* GI = GetGameInstance();
		ConversationSub = GI ? GI->GetSubsystem<UShadowSlaveConversationSubsystem>() : nullptr;
		if (!ConversationSub || SaveGame->ConversationData.bIsActive || ConversationSub->IsConversationActive())
		{
			UE_LOG(LogShadowSlave, Warning, TEXT("ApplySaveSnapshot: Passive dialogue data cannot be restored while conversation authority is unavailable or an active conversation is present."));
			return false;
		}
	}

	// 2. Player Transform restoration
	if (PlayerPawn && SaveGame->PlayerTransform.bIsValid)
	{
		RestorePlayerTransform(PlayerPawn, SaveGame->PlayerTransform);
	}

	// 3. Progression restoration
	if (PlayerPawn && SaveGame->ProgressionData.bIsValid)
	{
		if (UShadowSlaveProgressionComponent* Prog = PlayerPawn->FindComponentByClass<UShadowSlaveProgressionComponent>())
		{
			RestoreProgression(Prog, SaveGame->ProgressionData);
		}
	}

	// 4. Aspect restoration
	if (PlayerPawn && SaveGame->AspectData.bIsValid)
	{
		if (UShadowSlaveAspectComponent* Aspect = PlayerPawn->FindComponentByClass<UShadowSlaveAspectComponent>())
		{
			RestoreAspect(Aspect, SaveGame->AspectData);
		}
	}

	// 5. Inventory restoration
	if (PlayerPawn && SaveGame->InventoryData.bIsValid)
	{
		if (UShadowSlaveInventoryComponent* Inv = PlayerPawn->FindComponentByClass<UShadowSlaveInventoryComponent>())
		{
			RestoreInventory(Inv, SaveGame->InventoryData);
		}
	}

	// 6. Memory collection restoration
	if (PlayerPawn && SaveGame->MemoryData.bIsValid)
	{
		if (UShadowSlaveMemoryComponent* Mem = PlayerPawn->FindComponentByClass<UShadowSlaveMemoryComponent>())
		{
			RestoreMemories(Mem, SaveGame->MemoryData);
		}
	}

	// 6b. Echo collection restoration
	if (PlayerPawn && SaveGame->EchoData.bIsValid)
	{
		if (UShadowSlaveEchoComponent* Echo = PlayerPawn->FindComponentByClass<UShadowSlaveEchoComponent>())
		{
			RestoreEchoes(Echo, SaveGame->EchoData);
		}
	}

	// 7. Equipment restoration (references restored inventory & memories, applies modifiers)
	if (PlayerPawn && SaveGame->EquipmentData.bIsValid)
	{
		if (UShadowSlaveEquipmentComponent* Equip = PlayerPawn->FindComponentByClass<UShadowSlaveEquipmentComponent>())
		{
			RestoreEquipment(Equip, SaveGame->EquipmentData);
		}
	}

	// 7b. Status effects restoration (persistent effects restored to character)
	if (PlayerPawn && SaveGame->StatusEffectData.bIsValid)
	{
		if (UShadowSlaveStatusEffectComponent* StatusComp = PlayerPawn->FindComponentByClass<UShadowSlaveStatusEffectComponent>())
		{
			RestoreStatusEffects(StatusComp, SaveGame->StatusEffectData);
		}
	}

	// 8. Attributes restoration (restored after equipment so max attribute modifiers are active before clamping)
	if (PlayerPawn && SaveGame->AttributeData.bIsValid)
	{
		if (UShadowSlaveAttributeComponent* Attr = PlayerPawn->FindComponentByClass<UShadowSlaveAttributeComponent>())
		{
			RestoreAttributes(Attr, SaveGame->AttributeData);
		}
	}

	// 9. World state restoration
	if (World && SaveGame->WorldData.bIsValid)
	{
		RestoreWorldState(World, SaveGame->WorldData);
	}

	// 9. Nightmare scenario restoration
	if (SaveGame->NightmareData.bIsValid)
	{
		if (UGameInstance* GI = GetGameInstance())
		{
			if (UShadowSlaveNightmareSubsystem* NightmareSub = GI->GetSubsystem<UShadowSlaveNightmareSubsystem>())
			{
				NightmareSub->ImportSaveData(SaveGame->NightmareData);
			}
		}
	}

	// 10. Story progression restoration
	if (SaveGame->StoryData.bIsValid)
	{
		if (UGameInstance* GI = GetGameInstance())
		{
			if (UShadowSlaveStorySubsystem* StorySub = GI->GetSubsystem<UShadowSlaveStorySubsystem>())
			{
				StorySub->ImportSaveData(SaveGame->StoryData);
			}
		}
	}

	// 11. Quest progression restoration
	if (SaveGame->QuestData.bIsValid)
	{
		if (UGameInstance* GI = GetGameInstance())
		{
			if (UShadowSlaveQuestSubsystem* QuestSub = GI->GetSubsystem<UShadowSlaveQuestSubsystem>())
			{
				QuestSub->ImportSaveData(SaveGame->QuestData);
			}
		}
	}

	// 12. Passive dialogue runtime variables (no dialogue node display or conversation events)
	if (ConversationSub && !ConversationSub->RestoreConversationState(SaveGame->ConversationData))
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("ApplySaveSnapshot: Failed to restore passive dialogue runtime data."));
		return false;
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
		OutData.AspectId = AspectDef->GetAspectId();
		OutData.AspectAssetId = AspectDef->GetPrimaryAssetId();
	}

	if (UShadowSlaveFlawDefinition* FlawDef = AspectComp->GetFlawDefinition())
	{
		OutData.FlawId = FlawDef->GetFlawId();
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
			MemData.MemoryId = Mem.MemoryDefinition ? Mem.MemoryDefinition->GetMemoryId() : NAME_None;
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

void UShadowSlaveSaveSubsystem::CaptureEchoes(UShadowSlaveEchoComponent* EchoComp, FShadowSlaveEchoCollectionSaveData& OutData)
{
	if (!EchoComp)
	{
		OutData.bIsValid = false;
		return;
	}

	OutData.Echoes.Empty();

	for (const FShadowSlaveEchoInstance& Echo : EchoComp->GetEchoes())
	{
		if (Echo.IsValid())
		{
			FShadowSlaveEchoSaveData EchoData;
			EchoData.InstanceId = Echo.InstanceId;
			EchoData.EchoId = Echo.EchoDefinition ? Echo.EchoDefinition->GetEchoId() : NAME_None;
			EchoData.EchoPrimaryAssetId = Echo.EchoDefinition ? Echo.EchoDefinition->GetPrimaryAssetId() : FPrimaryAssetId();

			// Persist durable state only: transient Summoned state is normalized to Dormant.
			// Destroyed is preserved as durable terminal state.
			EchoData.State = (Echo.State == EShadowSlaveEchoState::Destroyed)
				? EShadowSlaveEchoState::Destroyed
				: EShadowSlaveEchoState::Dormant;

			EchoData.DynamicProperties = Echo.DynamicProperties;
			OutData.Echoes.Add(EchoData);
		}
	}

	OutData.bIsValid = true;
}

void UShadowSlaveSaveSubsystem::RestoreEchoes(UShadowSlaveEchoComponent* EchoComp, const FShadowSlaveEchoCollectionSaveData& InData)
{
	if (!EchoComp || !InData.bIsValid)
	{
		return;
	}

	TArray<FShadowSlaveEchoInstance> RestoredEchoes;
	for (const FShadowSlaveEchoSaveData& SavedEcho : InData.Echoes)
	{
		UShadowSlaveEchoDefinition* ResolvedDef = ResolveEchoDefinition(SavedEcho.EchoId, SavedEcho.EchoPrimaryAssetId);
		if (ResolvedDef)
		{
			FShadowSlaveEchoInstance RestoredInst;
			RestoredInst.InstanceId = SavedEcho.InstanceId;
			RestoredInst.EchoDefinition = ResolvedDef;

			// Restore durable lifecycle state: Destroyed stays Destroyed, all others initialize to Dormant.
			// Transient world summon state is NEVER restored (bIsSummoned remains false).
			RestoredInst.State = (SavedEcho.State == EShadowSlaveEchoState::Destroyed)
				? EShadowSlaveEchoState::Destroyed
				: EShadowSlaveEchoState::Dormant;
			RestoredInst.bIsSummoned = false;
			RestoredInst.DynamicProperties = SavedEcho.DynamicProperties;
			RestoredEchoes.Add(RestoredInst);
		}
		else
		{
			UE_LOG(LogShadowSlave, Warning, TEXT("RestoreEchoes: could not resolve Echo definition '%s'."), *SavedEcho.EchoId.ToString());
		}
	}

	EchoComp->RestoreEchoes(RestoredEchoes);
}

void UShadowSlaveSaveSubsystem::CaptureEquipment(UShadowSlaveEquipmentComponent* EquipComp, FShadowSlaveEquipmentSaveData& OutData)
{
	if (!EquipComp)
	{
		OutData.bIsValid = false;
		return;
	}

	OutData.EquippedSlots.Empty();

	const TArray<FShadowSlaveEquippedItem> AllEquipped = EquipComp->GetAllEquippedItems();
	for (const FShadowSlaveEquippedItem& Item : AllEquipped)
	{
		if (Item.IsValid())
		{
			FShadowSlaveEquippedSlotSaveData SlotSave(Item.Slot, Item.SourceType, Item.InstanceId, Item.DefinitionId);
			OutData.EquippedSlots.Add(SlotSave);
		}
	}

	OutData.bIsValid = true;
}

void UShadowSlaveSaveSubsystem::RestoreEquipment(UShadowSlaveEquipmentComponent* EquipComp, const FShadowSlaveEquipmentSaveData& InData)
{
	if (!EquipComp || !InData.bIsValid)
	{
		return;
	}

	// Ensure any active equipment or modifiers are cleanly cleared before restoration to avoid duplicates
	EquipComp->UnequipAll();

	for (const FShadowSlaveEquippedSlotSaveData& SlotSave : InData.EquippedSlots)
	{
		if (!SlotSave.InstanceId.IsValid() || SlotSave.Slot == EShadowSlaveEquipmentSlot::None)
		{
			continue;
		}

		if (SlotSave.SourceType == EShadowSlaveEquipmentSourceType::Item)
		{
			if (UShadowSlaveInventoryComponent* InvComp = EquipComp->GetInventoryComponent())
			{
				FShadowSlaveItemInstance FoundItem;
				if (InvComp->FindItemByInstanceId(SlotSave.InstanceId, FoundItem))
				{
					EquipComp->EquipItem(SlotSave.InstanceId, SlotSave.Slot);
				}
				else
				{
					UE_LOG(LogShadowSlave, Warning, TEXT("RestoreEquipment: Saved equipped item instance '%s' not found in inventory."),
						*SlotSave.InstanceId.ToString(EGuidFormats::Short));
				}
			}
		}
		else if (SlotSave.SourceType == EShadowSlaveEquipmentSourceType::Memory)
		{
			if (UShadowSlaveMemoryComponent* MemComp = EquipComp->GetMemoryComponent())
			{
				FShadowSlaveMemoryInstance FoundMem;
				if (MemComp->FindMemory(SlotSave.InstanceId, FoundMem))
				{
					EquipComp->EquipMemory(SlotSave.InstanceId, SlotSave.Slot);
				}
				else
				{
					UE_LOG(LogShadowSlave, Warning, TEXT("RestoreEquipment: Saved equipped Memory instance '%s' not found in MemoryComponent."),
						*SlotSave.InstanceId.ToString(EGuidFormats::Short));
				}
			}
		}
	}
}

void UShadowSlaveSaveSubsystem::CaptureStatusEffects(UShadowSlaveStatusEffectComponent* EffectComp, FShadowSlaveStatusEffectCollectionSaveData& OutData)
{
	if (!EffectComp)
	{
		OutData.bIsValid = false;
		return;
	}

	OutData.Effects.Empty();

	for (const FShadowSlaveStatusEffectInstance& Effect : EffectComp->GetActiveEffects())
	{
		// Only capture effects explicitly marked as persisting across save/load
		if (!Effect.IsValid() || !Effect.EffectDefinition || !Effect.EffectDefinition->bPersistAcrossSaveLoad)
		{
			continue;
		}

		FShadowSlaveStatusEffectSaveData EffectSave;
		EffectSave.InstanceId = Effect.InstanceId;
		EffectSave.EffectId = Effect.EffectDefinition->GetEffectId();
		EffectSave.EffectPrimaryAssetId = Effect.EffectDefinition->GetPrimaryAssetId();
		EffectSave.CurrentStacks = Effect.CurrentStacks;
		EffectSave.DynamicProperties = Effect.DynamicProperties;

		// Preserve generic source attribution (SourceId, SourceName).
		// Transient actor pointer (SourceActor) is NOT serialized.
		EffectSave.SourceId = Effect.Source.SourceId;
		EffectSave.SourceName = Effect.Source.SourceName;

		// Compute remaining duration based on duration policy
		if (Effect.EffectDefinition->DurationPolicy == EStatusEffectDurationPolicy::Persistent)
		{
			// Persistent: sentinel value, infinite until explicitly removed
			EffectSave.RemainingDuration = -1.0f;
		}
		else if (Effect.EffectDefinition->DurationPolicy == EStatusEffectDurationPolicy::Timed)
		{
			// Timed: compute actual remaining seconds
			const float Remaining = EffectComp->GetRemainingDuration(Effect.InstanceId);
			if (Remaining <= 0.0f)
			{
				// Effect has expired or is about to expire; do not serialize as active
				continue;
			}
			EffectSave.RemainingDuration = Remaining;
		}
		else
		{
			// Instant effects are never in ActiveEffects; defensive skip
			continue;
		}

		OutData.Effects.Add(EffectSave);
	}

	OutData.bIsValid = true;
}

void UShadowSlaveSaveSubsystem::RestoreStatusEffects(UShadowSlaveStatusEffectComponent* EffectComp, const FShadowSlaveStatusEffectCollectionSaveData& InData)
{
	if (!EffectComp || !InData.bIsValid)
	{
		return;
	}

	TArray<FShadowSlaveStatusEffectInstance> RestoredEffects;
	for (const FShadowSlaveStatusEffectSaveData& SavedEffect : InData.Effects)
	{
		UShadowSlaveStatusEffectDefinition* ResolvedDef = ResolveStatusEffectDefinition(SavedEffect.EffectId, SavedEffect.EffectPrimaryAssetId);
		if (!ResolvedDef)
		{
			UE_LOG(LogShadowSlave, Warning, TEXT("RestoreStatusEffects: could not resolve StatusEffect definition '%s'."), *SavedEffect.EffectId.ToString());
			continue;
		}

		// For timed effects, use the saved remaining duration.
		// If saved remaining <= 0, the effect expired before or during save — do not resurrect it.
		if (ResolvedDef->DurationPolicy == EStatusEffectDurationPolicy::Timed)
		{
			if (SavedEffect.RemainingDuration <= 0.0f)
			{
				UE_LOG(LogShadowSlave, Log, TEXT("RestoreStatusEffects: Skipping expired timed effect '%s' (RemainingDuration=%.2f)."),
					*SavedEffect.EffectId.ToString(), SavedEffect.RemainingDuration);
				continue;
			}
		}

		FShadowSlaveStatusEffectInstance RestoredInst;
		RestoredInst.InstanceId = SavedEffect.InstanceId;
		RestoredInst.EffectDefinition = ResolvedDef;
		RestoredInst.CurrentStacks = FMath::Max(1, SavedEffect.CurrentStacks);
		RestoredInst.DynamicProperties = SavedEffect.DynamicProperties;

		// Restore generic source attribution; actor pointer is left null (transient, not serialized)
		RestoredInst.Source.SourceId = SavedEffect.SourceId;
		RestoredInst.Source.SourceName = SavedEffect.SourceName;
		RestoredInst.Source.SourceActor = nullptr;

		if (ResolvedDef->DurationPolicy == EStatusEffectDurationPolicy::Timed)
		{
			// Use saved remaining duration, NOT the definition's full configured duration
			RestoredInst.TotalDuration = SavedEffect.RemainingDuration;
		}

		RestoredEffects.Add(RestoredInst);
	}

	EffectComp->RestoreEffects(RestoredEffects);
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
		if (!Actor)
		{
			continue;
		}

		if (Actor->GetClass()->ImplementsInterface(UShadowSlaveSaveableInterface::StaticClass()))
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
		else if (UShadowSlaveWorldStateComponent* WorldComp = Actor->FindComponentByClass<UShadowSlaveWorldStateComponent>())
		{
			const FName PersistentId = WorldComp->GetPersistentStateId();
			if (!PersistentId.IsNone())
			{
				FShadowSlaveWorldActorSaveRecord Record;
				if (WorldComp->CaptureSaveRecord(Record))
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
		if (!Actor)
		{
			continue;
		}

		if (Actor->GetClass()->ImplementsInterface(UShadowSlaveSaveableInterface::StaticClass()))
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
		else if (UShadowSlaveWorldStateComponent* WorldComp = Actor->FindComponentByClass<UShadowSlaveWorldStateComponent>())
		{
			const FName PersistentId = WorldComp->GetPersistentStateId();
			if (!PersistentId.IsNone())
			{
				if (const FShadowSlaveWorldActorSaveRecord* FoundRecord = InData.PersistentActors.Find(PersistentId))
				{
					WorldComp->RestoreSaveRecord(*FoundRecord);
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

UShadowSlaveEchoDefinition* UShadowSlaveSaveSubsystem::ResolveEchoDefinition(FName EchoId, const FPrimaryAssetId& PrimaryAssetId) const
{
	// 1. Attempt Asset Manager resolution if registered
	if (PrimaryAssetId.IsValid() && UAssetManager::IsInitialized())
	{
		if (UObject* AssetObj = UAssetManager::Get().GetPrimaryAssetObject(PrimaryAssetId))
		{
			if (UShadowSlaveEchoDefinition* Def = Cast<UShadowSlaveEchoDefinition>(AssetObj))
			{
				return Def;
			}
		}

		const FSoftObjectPath AssetPath = UAssetManager::Get().GetPrimaryAssetPath(PrimaryAssetId);
		if (AssetPath.IsValid())
		{
			if (UObject* Loaded = AssetPath.TryLoad())
			{
				if (UShadowSlaveEchoDefinition* Def = Cast<UShadowSlaveEchoDefinition>(Loaded))
				{
					return Def;
				}
			}
		}
	}

	// 2. Fallback: find loaded object in memory
	if (!EchoId.IsNone())
	{
		if (UShadowSlaveEchoDefinition* Found = FindObject<UShadowSlaveEchoDefinition>(ANY_PACKAGE, *EchoId.ToString()))
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

UShadowSlaveStatusEffectDefinition* UShadowSlaveSaveSubsystem::ResolveStatusEffectDefinition(FName EffectId, const FPrimaryAssetId& PrimaryAssetId) const
{
	// 1. Attempt Asset Manager resolution
	if (PrimaryAssetId.IsValid() && UAssetManager::IsInitialized())
	{
		if (UObject* AssetObj = UAssetManager::Get().GetPrimaryAssetObject(PrimaryAssetId))
		{
			if (UShadowSlaveStatusEffectDefinition* Def = Cast<UShadowSlaveStatusEffectDefinition>(AssetObj))
			{
				return Def;
			}
		}

		const FSoftObjectPath AssetPath = UAssetManager::Get().GetPrimaryAssetPath(PrimaryAssetId);
		if (AssetPath.IsValid())
		{
			if (UObject* Loaded = AssetPath.TryLoad())
			{
				if (UShadowSlaveStatusEffectDefinition* Def = Cast<UShadowSlaveStatusEffectDefinition>(Loaded))
				{
					return Def;
				}
			}
		}
	}

	// 2. Fallback: find loaded object in memory
	if (!EffectId.IsNone())
	{
		if (UShadowSlaveStatusEffectDefinition* Found = FindObject<UShadowSlaveStatusEffectDefinition>(ANY_PACKAGE, *EffectId.ToString()))
		{
			return Found;
		}
	}

	return nullptr;
}
