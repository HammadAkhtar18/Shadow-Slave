// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Content/ShadowSlaveContentDefinition.h"
#include "ShadowSlaveFlawDefinition.generated.h"

/**
 * Data-driven content definition defining a canon or custom character Flaw.
 * Specialization of UShadowSlaveContentDefinition for the generic content pipeline.
 *
 * ARCHITECTURAL PRINCIPLES:
 * 1. SPECIALIZED CONTENT DEFINITION: FlawDefinition is a specialized static content definition
 *    deriving from UShadowSlaveContentDefinition.
 * 2. GENERIC CONTENT CLASSIFICATION: Generic ContentType is EShadowSlaveContentType::Custom because
 *    the generic content pipeline taxonomy currently does not define a dedicated Flaw member.
 * 3. SPECIALIZED PRIMARY ASSET TYPE: Utilizes the GetCustomPrimaryAssetType() hook to produce
 *    deterministic PrimaryAssetId with PrimaryAssetType "Flaw": FPrimaryAssetId(TEXT("Flaw"), ContentId).
 * 4. SINGLE AUTHORITATIVE ID: ContentId is the sole stored stable identifier. GetFlawId() and SetFlawId()
 *    provide backward-compatible accessors without duplicate storage or reference-member aliases.
 * 5. RUNTIME STATE SEPARATION: Flaw runtime binding and ownership remain authoritative with
 *    UShadowSlaveAspectComponent; static definition data remains immutable.
 * 6. CONTENT REGISTRY ROLE: UShadowSlaveContentRegistrySubsystem provides static definition lookup and
 *    discovery only; it does NOT track runtime aspect, character, or flaw binding state.
 */
UCLASS(BlueprintType)
class SHADOWSLAVE_API UShadowSlaveFlawDefinition : public UShadowSlaveContentDefinition
{
	GENERATED_BODY()

public:
	UShadowSlaveFlawDefinition();

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	/**
	 * Validates definition configuration.
	 * Combines base generic content validation (valid ContentId, valid DisplayName, Version >= 1, ContentType == Custom).
	 */
	virtual bool IsValidDefinition(FString* OutErrorMessage = nullptr) const override;

	/* --- Identification Compatibility Accessors --- */

	/** Compatibility accessor returning authoritative ContentId as FlawId */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Flaw|Identity")
	FName GetFlawId() const { return ContentId; }

	/** Compatibility accessor setting authoritative ContentId as FlawId */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Flaw|Identity")
	void SetFlawId(FName InFlawId) { ContentId = InFlawId; }

	/* --- Metadata & Provenance --- */

	/** Extensible metadata key-value pairs for future systems */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Flaw|Metadata")
	TMap<FName, FString> Metadata;

	/** Canon research provenance or verification notes */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Flaw|Metadata")
	FString CanonProvenance;

protected:
	/** Hook returning specialized PrimaryAssetType "Flaw" for ContentType == Custom */
	virtual FName GetCustomPrimaryAssetType() const override { return TEXT("Flaw"); }
};
