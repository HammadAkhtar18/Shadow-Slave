// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ShadowSlaveFlawDefinition.generated.h"

/**
 * Data-driven primary data asset defining a canon or custom character Flaw.
 * Flaws are metaphysical limitations bound to a soul in the Shadow Slave universe.
 * Static definition; contains narrative identity and metadata without hardcoded execution logic.
 */
UCLASS(BlueprintType)
class SHADOWSLAVE_API UShadowSlaveFlawDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UShadowSlaveFlawDefinition();

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	/** Technical unique identifier for this Flaw */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Flaw|Identity")
	FName FlawId = NAME_None;

	/** Display name shown to players in UI/status */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Flaw|Identity")
	FText DisplayName;

	/** Descriptive/narrative text detailing the Flaw's nature */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Flaw|Identity", meta = (MultiLine = true))
	FText Description;

	/** Extensible metadata key-value pairs for future systems */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Flaw|Metadata")
	TMap<FName, FString> Metadata;

	/** Canon research provenance or verification notes */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Flaw|Metadata")
	FString CanonProvenance;
};
