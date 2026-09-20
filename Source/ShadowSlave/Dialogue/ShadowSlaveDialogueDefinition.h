// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Content/ShadowSlaveContentDefinition.h"
#include "Dialogue/ShadowSlaveDialogueTypes.h"
#include "ShadowSlaveDialogueDefinition.generated.h"

/**
 * Data-driven primary data asset describing immutable dialogue graphs and conversation trees.
 * Derived from UShadowSlaveContentDefinition as part of the generic static content pipeline.
 * Cleanly separates static dialogue content from mutable runtime conversation state.
 * Contains zero hardcoded canon characters or story branches.
 *
 * ARCHITECTURAL PRINCIPLES:
 * 1. Specialized Content Definition: Inherits common metadata (ContentId, DisplayName, Description,
 *    Version, MetadataTags, ProvenanceNote) from UShadowSlaveContentDefinition.
 * 2. Single Authoritative ID: ContentId is the single authoritative stored identifier. Backwards
 *    compatibility is provided through GetDialogueId() and SetDialogueId() accessors only.
 * 3. Content Type Separation:
 *    - Generic ContentType is statically EShadowSlaveContentType::Dialogue, identifying this asset as
 *      a dialogue definition within the generic content pipeline.
 *    - Dialogue-specific taxonomy (nodes, choices, conditions, consequences) remains separate and authoritative.
 * 4. Dialogue-Specific Data: Owns starting node ID, dialogue node graph, and technical metadata.
 * 5. Runtime Separation: Cleanly separates immutable dialogue definitions from mutable runtime conversation
 *    state (managed by UShadowSlaveConversationSubsystem).
 * 6. Static Infrastructure: Participates in UShadowSlaveContentRegistrySubsystem for static definition
 *    lookup only; the registry does not manage active conversation sessions.
 */
UCLASS(BlueprintType)
class SHADOWSLAVE_API UShadowSlaveDialogueDefinition : public UShadowSlaveContentDefinition
{
	GENERATED_BODY()

public:
	UShadowSlaveDialogueDefinition();

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	/**
	 * Validates definition configuration.
	 * Combines generic content definition validation with dialogue graph structural checks.
	 */
	virtual bool IsValidDefinition(FString* OutErrorMessage = nullptr) const override;

	/* --- Identity Compatibility --- */

	/**
	 * Returns the single authoritative stable content identifier (ContentId).
	 * Provided for backwards compatibility with the existing Dialogue API.
	 */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Dialogue|Identity")
	FName GetDialogueId() const { return ContentId; }

	/**
	 * Sets the single authoritative stable content identifier (ContentId).
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Dialogue|Identity")
	void SetDialogueId(FName InDialogueId) { ContentId = InDialogueId; }

	/* --- Dialogue Flow & Content --- */

	/** Initial node to display when this conversation starts */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Dialogue|Flow")
	FName StartingNodeId = NAME_None;

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
