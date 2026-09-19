// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/SoftObjectPtr.h"
#include "UObject/PrimaryAssetId.h"
#include "ShadowSlaveContentTypes.generated.h"

class UShadowSlaveContentDefinition;

/**
 * Broad generic content categories for static content definitions in the Shadow Slave prototype.
 *
 * SCOPE & ARCHITECTURE:
 * These categories are purely architectural classifications used for indexing, filtering, and
 * PrimaryAssetId type resolution. They contain ZERO canon elements (no named characters, lore, or abilities).
 */
UENUM(BlueprintType)
enum class EShadowSlaveContentType : uint8
{
	None = 0,
	Character,
	Item,
	Memory,
	Echo,
	Quest,
	Dialogue,
	Story,
	Ability,
	World,
	Custom
};

/**
 * Generic entry in the content registry mapping a stable ID to a soft reference or loaded definition.
 *
 * Allows content definitions to remain referenced as soft pointers without forcing all data assets
 * into memory upon initialization.
 */
USTRUCT(BlueprintType)
struct SHADOWSLAVE_API FShadowSlaveContentRegistryEntry
{
	GENERATED_BODY()

	/** Stable unique technical content identifier */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Content")
	FName ContentId = NAME_None;

	/** Broad generic content category */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Content")
	EShadowSlaveContentType ContentType = EShadowSlaveContentType::None;

	/** Soft object reference to the content definition data asset */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Content")
	TSoftObjectPtr<UShadowSlaveContentDefinition> SoftDefinition;

	/** Optional PrimaryAssetId for Unreal Asset Manager resolution */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Content")
	FPrimaryAssetId PrimaryAssetId;

	/** In-memory pointer to the definition (if pre-loaded or registered directly in-memory) */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "ShadowSlave|Content")
	TObjectPtr<UShadowSlaveContentDefinition> LoadedDefinition = nullptr;

	/** Checks if the entry has a valid identifier and at least one resolution target */
	bool IsValid() const
	{
		return !ContentId.IsNone() && (!SoftDefinition.IsNull() || LoadedDefinition != nullptr);
	}
};
