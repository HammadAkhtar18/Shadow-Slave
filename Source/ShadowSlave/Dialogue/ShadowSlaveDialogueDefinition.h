// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Dialogue/ShadowSlaveDialogueTypes.h"
#include "ShadowSlaveDialogueDefinition.generated.h"

/**
 * Data-driven primary data asset describing immutable dialogue graphs and conversation trees.
 * Cleanly separates static dialogue content from mutable runtime conversation state.
 * Contains zero hardcoded canon characters or story branches.
 */
UCLASS(BlueprintType)
class SHADOWSLAVE_API UShadowSlaveDialogueDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UShadowSlaveDialogueDefinition();

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	/* --- Identification & Meta --- */

	/** Stable unique technical identifier for this dialogue asset */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Dialogue|Identity")
	FName DialogueId = NAME_None;

	/** Human-readable title of this dialogue */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Dialogue|Identity")
	FText DisplayName;

	/** Internal summary or designer notes */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Dialogue|Identity", meta = (MultiLine = true))
	FText Description;

	/** Initial node to display when this conversation starts */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Dialogue|Flow")
	FName StartingNodeId = NAME_None;

	/** Content version number for migration and compatibility tracking */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Dialogue|Flow", meta = (ClampMin = "1"))
	int32 Version = 1;

	/** Complete collection of dialogue nodes constituting this conversation */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Dialogue|Content")
	TArray<FShadowSlaveDialogueNode> Nodes;

	/** Extensible metadata for quest, journal, or analytics tagging */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Dialogue|Metadata")
	TMap<FName, FString> Metadata;

	/* --- Node Queries --- */

	/** Locates a dialogue node by its unique NodeId (returns nullptr if not found) */
	const FShadowSlaveDialogueNode* FindNode(FName InNodeId) const;

	/** Blueprint wrapper to find a node by NodeId */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Dialogue", meta = (DisplayName = "Find Dialogue Node"))
	bool GetNodeById(FName InNodeId, FShadowSlaveDialogueNode& OutNode) const;

	/* --- Validation --- */

	/**
	 * Performs structural integrity validation on the dialogue graph.
	 * Checks unique node/choice IDs, starting node presence, and target node validity.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Dialogue|Validation")
	bool ValidateDefinition(TArray<FText>& OutErrors) const;

	/* --- Development Test Factory --- */

	/** Creates a generic development test dialogue definition with branching choices (zero canon content) */
	static UShadowSlaveDialogueDefinition* CreateTestDialogueDefinition(UObject* Outer = nullptr);
};
