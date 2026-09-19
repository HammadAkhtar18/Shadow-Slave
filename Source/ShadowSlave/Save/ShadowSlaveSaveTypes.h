// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Progression/ShadowSlaveProgressionTypes.h"
#include "Aspects/ShadowSlaveAspectTypes.h"
#include "Memories/ShadowSlaveMemoryTypes.h"
#include "Echoes/ShadowSlaveEchoTypes.h"
#include "Items/ShadowSlaveItemTypes.h"
#include "Equipment/ShadowSlaveEquipmentTypes.h"
#include "ShadowSlaveSaveTypes.generated.h"

/**
 * Serializable snapshot of the player character's world transform.
 */
USTRUCT(BlueprintType)
struct SHADOWSLAVE_API FShadowSlavePlayerSaveTransform
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Save")
	FVector Location = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Save")
	FRotator Rotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Save")
	FVector Scale = FVector::OneVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Save")
	bool bIsValid = false;

	FShadowSlavePlayerSaveTransform() = default;

	FShadowSlavePlayerSaveTransform(const FTransform& InTransform)
		: Location(InTransform.GetLocation())
		, Rotation(InTransform.GetRotation().Rotator())
		, Scale(InTransform.GetScale3D())
		, bIsValid(true)
	{
	}

	FTransform ToTransform() const
	{
		return FTransform(Rotation, Location, Scale);
	}
};

/**
 * Serializable snapshot of character attributes (Health, Stamina, Essence).
 * Pure snapshot data; does NOT duplicate or act as runtime attribute authority.
 */
USTRUCT(BlueprintType)
struct SHADOWSLAVE_API FShadowSlaveAttributeSaveData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Save")
	float CurrentHealth = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Save")
	float CurrentStamina = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Save")
	float CurrentEssence = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Save")
	bool bIsValid = false;

	FShadowSlaveAttributeSaveData() = default;

	FShadowSlaveAttributeSaveData(float InHealth, float InStamina, float InEssence)
		: CurrentHealth(InHealth), CurrentStamina(InStamina), CurrentEssence(InEssence), bIsValid(true)
	{
	}
};

/**
 * Serializable snapshot of character progression and soul core state.
 */
USTRUCT(BlueprintType)
struct SHADOWSLAVE_API FShadowSlaveProgressionSaveData
{
	GENERATED_BODY()

	/** Nightmare Spell character rank */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Save")
	EShadowSlaveCharacterRank CharacterRank = EShadowSlaveCharacterRank::Unknown;

	/** Number of active soul cores */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Save")
	int32 CurrentSoulCores = 1;

	/** Maximum soul core capacity */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Save")
	int32 MaximumSoulCores = 1;

	/** Extensible metadata key-value storage for advancement tracking */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Save")
	TMap<FName, FString> ProgressionMetadata;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Save")
	bool bIsValid = false;

	FShadowSlaveProgressionSaveData() = default;
};

/**
 * Serializable snapshot of Aspect identity, unlocked abilities, and bound Flaw.
 * References assets via stable identifiers rather than raw memory pointers.
 */
USTRUCT(BlueprintType)
struct SHADOWSLAVE_API FShadowSlaveAspectSaveData
{
	GENERATED_BODY()

	/** Stable identifier for the Aspect definition */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Save")
	FName AspectId = NAME_None;

	/** Primary Asset ID of the Aspect definition if registered */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Save")
	FPrimaryAssetId AspectAssetId;

	/** Stable identifiers of all unlocked Aspect abilities */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Save")
	TArray<FName> UnlockedAbilityIds;

	/** Stable identifier for the active Flaw definition */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Save")
	FName FlawId = NAME_None;

	/** Primary Asset ID of the Flaw definition if registered */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Save")
	FPrimaryAssetId FlawAssetId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Save")
	bool bIsValid = false;

	FShadowSlaveAspectSaveData() = default;
};

/**
 * Serializable snapshot of an individual inventory item instance.
 */
USTRUCT(BlueprintType)
struct SHADOWSLAVE_API FShadowSlaveItemSaveData
{
	GENERATED_BODY()

	/** Unique instance GUID preserved across save/load */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Save")
	FGuid InstanceId;

	/** Stable identifier or asset name of the Item Definition */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Save")
	FName ItemDefinitionId = NAME_None;

	/** Primary Asset ID of the Item Definition */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Save")
	FPrimaryAssetId ItemPrimaryAssetId;

	/** Item stack count */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Save")
	int32 Quantity = 1;

	/** Instance-specific dynamic properties */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Save")
	TMap<FName, FString> DynamicProperties;

	FShadowSlaveItemSaveData() = default;
};

/**
 * Serializable snapshot of an inventory component.
 */
USTRUCT(BlueprintType)
struct SHADOWSLAVE_API FShadowSlaveInventorySaveData
{
	GENERATED_BODY()

	/** Inventory slot capacity */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Save")
	int32 MaxSlots = 20;

	/** Serialized item slots */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Save")
	TArray<FShadowSlaveItemSaveData> Items;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Save")
	bool bIsValid = false;

	FShadowSlaveInventorySaveData() = default;
};

/**
 * Serializable snapshot of an individual Memory instance.
 * Preserves GUID, equipped state, runtime state, and dynamic properties.
 * Does NOT duplicate static canon classification (Rank, Tier, static enchantments).
 */
USTRUCT(BlueprintType)
struct SHADOWSLAVE_API FShadowSlaveMemorySaveData
{
	GENERATED_BODY()

	/** Unique instance GUID preserved across save/load */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Save")
	FGuid InstanceId;

	/** Stable identifier or asset name of the Memory Definition */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Save")
	FName MemoryId = NAME_None;

	/** Primary Asset ID of the Memory Definition */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Save")
	FPrimaryAssetId MemoryPrimaryAssetId;

	/** Operational runtime state */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Save")
	EShadowSlaveMemoryState State = EShadowSlaveMemoryState::Dormant;

	/** Whether this Memory is actively manifested / equipped */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Save")
	bool bIsEquipped = false;

	/** Instance-specific dynamic properties */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Save")
	TMap<FName, FString> DynamicProperties;

	FShadowSlaveMemorySaveData() = default;
};

/**
 * Serializable snapshot of a Memory component collection.
 */
USTRUCT(BlueprintType)
struct SHADOWSLAVE_API FShadowSlaveMemoryCollectionSaveData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Save")
	TArray<FShadowSlaveMemorySaveData> Memories;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Save")
	bool bIsValid = false;

	FShadowSlaveMemoryCollectionSaveData() = default;
};

/**
 * Serializable snapshot of an individual Echo instance.
 * Preserves GUID, summoned state, runtime state, and dynamic properties.
 * Does NOT duplicate static classification (Rank, Class, costs).
 */
USTRUCT(BlueprintType)
struct SHADOWSLAVE_API FShadowSlaveEchoSaveData
{
	GENERATED_BODY()

	/** Unique instance GUID preserved across save/load */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Save")
	FGuid InstanceId;

	/** Stable identifier or asset name of the Echo Definition */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Save")
	FName EchoId = NAME_None;

	/** Primary Asset ID of the Echo Definition */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Save")
	FPrimaryAssetId EchoPrimaryAssetId;

	/** Operational runtime state */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Save")
	EShadowSlaveEchoState State = EShadowSlaveEchoState::Dormant;

	/** Whether this Echo is actively summoned in the world */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Save")
	bool bIsSummoned = false;

	/** Instance-specific dynamic properties */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Save")
	TMap<FName, FString> DynamicProperties;

	FShadowSlaveEchoSaveData() = default;
};

/**
 * Serializable snapshot of an Echo component collection.
 */
USTRUCT(BlueprintType)
struct SHADOWSLAVE_API FShadowSlaveEchoCollectionSaveData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Save")
	TArray<FShadowSlaveEchoSaveData> Echoes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Save")
	bool bIsValid = false;

	FShadowSlaveEchoCollectionSaveData() = default;
};

/**
 * Serializable snapshot of an individual equipped slot.
 * Preserves slot, source type (Item vs Memory), instance GUID, and definition ID.
 * Does not duplicate the underlying item or Memory instance payload.
 */
USTRUCT(BlueprintType)
struct SHADOWSLAVE_API FShadowSlaveEquippedSlotSaveData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Save")
	EShadowSlaveEquipmentSlot Slot = EShadowSlaveEquipmentSlot::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Save")
	EShadowSlaveEquipmentSourceType SourceType = EShadowSlaveEquipmentSourceType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Save")
	FGuid InstanceId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Save")
	FName DefinitionId = NAME_None;

	FShadowSlaveEquippedSlotSaveData() = default;

	FShadowSlaveEquippedSlotSaveData(EShadowSlaveEquipmentSlot InSlot, EShadowSlaveEquipmentSourceType InSourceType, const FGuid& InInstanceId, FName InDefId = NAME_None)
		: Slot(InSlot), SourceType(InSourceType), InstanceId(InInstanceId), DefinitionId(InDefId)
	{
	}
};

/**
 * Serializable snapshot of the equipment component.
 */
USTRUCT(BlueprintType)
struct SHADOWSLAVE_API FShadowSlaveEquipmentSaveData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Save")
	TArray<FShadowSlaveEquippedSlotSaveData> EquippedSlots;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Save")
	bool bIsValid = false;

	FShadowSlaveEquipmentSaveData() = default;
};

/**
 * Serializable record representing the persistent state of a world actor.
 */
USTRUCT(BlueprintType)
struct SHADOWSLAVE_API FShadowSlaveWorldActorSaveRecord
{
	GENERATED_BODY()

	/** Stable persistence identifier for this actor */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Save")
	FName PersistentId = NAME_None;

	/** Actor class for validation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Save")
	TSubclassOf<AActor> ActorClass = nullptr;

	/** World transform of the actor */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Save")
	FTransform ActorTransform;

	/** Arbitrary string key-value pairs representing custom state (door open, switch active, etc.) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Save")
	TMap<FName, FString> CustomStateData;

	/** Whether the actor was active / present in the world */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Save")
	bool bIsActive = true;

	FShadowSlaveWorldActorSaveRecord() = default;
};

/**
 * Serializable snapshot of persistent world actors.
 */
USTRUCT(BlueprintType)
struct SHADOWSLAVE_API FShadowSlaveWorldSaveData
{
	GENERATED_BODY()

	/** Map of persistent actor ID to actor state record */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Save")
	TMap<FName, FShadowSlaveWorldActorSaveRecord> PersistentActors;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Save")
	bool bIsValid = false;

	FShadowSlaveWorldSaveData() = default;
};
