// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Progression/ShadowSlaveProgressionTypes.h"
#include "ShadowSlaveDialogueTypes.generated.h"

class UShadowSlaveDialogueDefinition;
class UShadowSlaveItemDefinition;
class UTexture2D;
class AActor;

/**
 * Operational runtime state for the conversation state machine.
 */
UENUM(BlueprintType)
enum class EShadowSlaveConversationState : uint8
{
	Inactive         UMETA(DisplayName = "Inactive"),
	Starting         UMETA(DisplayName = "Starting"),
	Displaying       UMETA(DisplayName = "Displaying"),
	WaitingForChoice UMETA(DisplayName = "Waiting For Choice"),
	Advancing        UMETA(DisplayName = "Advancing"),
	Completed        UMETA(DisplayName = "Completed"),
	Aborted          UMETA(DisplayName = "Aborted")
};

/**
 * Generic comparison operator for dialogue conditions.
 */
UENUM(BlueprintType)
enum class EShadowSlaveComparisonOperator : uint8
{
	Equal              UMETA(DisplayName = "Equal (==)"),
	NotEqual           UMETA(DisplayName = "Not Equal (!=)"),
	GreaterThan        UMETA(DisplayName = "Greater Than (>)"),
	GreaterThanOrEqual UMETA(DisplayName = "Greater Than Or Equal (>=)"),
	LessThan           UMETA(DisplayName = "Less Than (<)"),
	LessThanOrEqual    UMETA(DisplayName = "Less Than Or Equal (<=)")
};

/**
 * Generic condition classification for dialogue gating.
 * Kept completely data-driven without hardcoding story branches.
 */
UENUM(BlueprintType)
enum class EShadowSlaveDialogueConditionType : uint8
{
	AlwaysTrue        UMETA(DisplayName = "Always True"),
	Flag              UMETA(DisplayName = "Boolean Flag"),
	NumericComparison UMETA(DisplayName = "Numeric Comparison"),
	HasItem           UMETA(DisplayName = "Has Inventory Item"),
	CharacterRank     UMETA(DisplayName = "Minimum Character Rank"),
	SoulCores         UMETA(DisplayName = "Soul Core Count")
};

/**
 * Generic consequence classification executed upon dialogue transitions.
 * Kept completely data-driven without hardcoding story events.
 */
UENUM(BlueprintType)
enum class EShadowSlaveDialogueConsequenceType : uint8
{
	None          UMETA(DisplayName = "None"),
	SetFlag       UMETA(DisplayName = "Set Boolean Flag"),
	ModifyNumeric UMETA(DisplayName = "Modify Numeric Value"),
	GiveItem      UMETA(DisplayName = "Give Inventory Item"),
	RemoveItem    UMETA(DisplayName = "Remove Inventory Item"),
	TriggerEvent  UMETA(DisplayName = "Trigger Gameplay Event")
};

/**
 * Data-driven condition required for a choice to be available or executable.
 */
USTRUCT(BlueprintType)
struct SHADOWSLAVE_API FShadowSlaveDialogueCondition
{
	GENERATED_BODY()

	/** Type of condition evaluation to perform */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Dialogue|Condition")
	EShadowSlaveDialogueConditionType ConditionType = EShadowSlaveDialogueConditionType::AlwaysTrue;

	/** Identifier for target flag, numeric variable, or item identifier */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Dialogue|Condition")
	FName ParamKey = NAME_None;

	/** Comparison operator for numeric and core evaluations */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Dialogue|Condition")
	EShadowSlaveComparisonOperator ComparisonOperator = EShadowSlaveComparisonOperator::Equal;

	/** Target numeric threshold for numeric evaluations */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Dialogue|Condition")
	float ComparisonValue = 0.0f;

	/** Expected boolean state for flag conditions */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Dialogue|Condition")
	bool bExpectedFlagValue = true;

	/** Optional soft reference to item definition required in interactor inventory */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Dialogue|Condition")
	TSoftObjectPtr<UShadowSlaveItemDefinition> RequiredItemDef;

	/** Required item count in interactor inventory */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Dialogue|Condition", meta = (ClampMin = "1"))
	int32 RequiredItemQuantity = 1;

	/** Minimum Nightmare Spell character rank required */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Dialogue|Condition")
	EShadowSlaveCharacterRank RequiredRank = EShadowSlaveCharacterRank::Unknown;

	/** Inverts the evaluation result when true */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Dialogue|Condition")
	bool bInvertCondition = false;
};

/**
 * Data-driven consequence executed upon choice selection or node entry.
 */
USTRUCT(BlueprintType)
struct SHADOWSLAVE_API FShadowSlaveDialogueConsequence
{
	GENERATED_BODY()

	/** Action to perform */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Dialogue|Consequence")
	EShadowSlaveDialogueConsequenceType ConsequenceType = EShadowSlaveDialogueConsequenceType::None;

	/** Identifier for target flag, numeric variable, or event hook */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Dialogue|Consequence")
	FName TargetKey = NAME_None;

	/** Value to assign for boolean flags */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Dialogue|Consequence")
	bool BoolValue = true;

	/** Value delta or assignment for numeric variables */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Dialogue|Consequence")
	float NumericValue = 0.0f;

	/** Soft reference to item definition for give/remove actions */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Dialogue|Consequence")
	TSoftObjectPtr<UShadowSlaveItemDefinition> ItemDef;

	/** Stack quantity for give/remove actions */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Dialogue|Consequence", meta = (ClampMin = "1"))
	int32 ItemQuantity = 1;

	/** Technical event identifier broadcast to game systems */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Dialogue|Consequence")
	FName EventId = NAME_None;

	/** Optional extensible metadata payload */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Dialogue|Consequence")
	TMap<FName, FString> Metadata;
};

/**
 * A selectable response option presented to the player during a dialogue node.
 */
USTRUCT(BlueprintType)
struct SHADOWSLAVE_API FShadowSlaveDialogueChoice
{
	GENERATED_BODY()

	/** Unique identifier distinguishing this choice within its parent node */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Dialogue|Choice")
	FName ChoiceId = NAME_None;

	/** Player-facing prompt text for this choice */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Dialogue|Choice")
	FText ChoiceText;

	/** Target node to transition to. If NAME_None, choice cleanly completes the conversation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Dialogue|Choice")
	FName TargetNodeId = NAME_None;

	/** Conditions that must all pass for this choice to be presented and selectable */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Dialogue|Choice")
	TArray<FShadowSlaveDialogueCondition> Conditions;

	/** Consequences applied immediately upon selecting this choice */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Dialogue|Choice")
	TArray<FShadowSlaveDialogueConsequence> Consequences;

	/** Extensible metadata for styling, tone, or quest tagging */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Dialogue|Choice")
	TMap<FName, FString> Metadata;

	bool IsValid() const
	{
		return !ChoiceId.IsNone() && !ChoiceText.IsEmpty();
	}
};

/**
 * An individual dialogue utterance or beat within a conversation tree.
 */
USTRUCT(BlueprintType)
struct SHADOWSLAVE_API FShadowSlaveDialogueNode
{
	GENERATED_BODY()

	/** Stable unique identifier for this node within its parent dialogue asset */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Dialogue|Node")
	FName NodeId = NAME_None;

	/** Identifier of the speaker (e.g. NPC ID, "Player", "Narrator") */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Dialogue|Node")
	FName SpeakerId = NAME_None;

	/** Dialogue text spoken or presented */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Dialogue|Node", meta = (MultiLine = true))
	FText DialogueText;

	/** Optional custom display name override for the speaker */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Dialogue|Node")
	FText SpeakerDisplayName;

	/** Optional portrait or icon for speaker presentation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Dialogue|Node")
	TSoftObjectPtr<UTexture2D> SpeakerPortrait;

	/** Ordered response choices available from this node */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Dialogue|Node")
	TArray<FShadowSlaveDialogueChoice> Choices;

	/** Optional consequences applied immediately when this node is displayed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Dialogue|Node")
	TArray<FShadowSlaveDialogueConsequence> NodeConsequences;

	/** Extensible metadata for audio cues, camera tags, or presentation modes */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Dialogue|Node")
	TMap<FName, FString> Metadata;

	bool IsValid() const
	{
		return !NodeId.IsNone();
	}

	bool IsTerminal() const
	{
		return Choices.Num() == 0;
	}
};

/**
 * Lightweight serializable snapshot for conversation state persistence.
 * Completely decoupled from static DataAsset definitions and contains zero raw UObject pointers.
 */
USTRUCT(BlueprintType)
struct SHADOWSLAVE_API FShadowSlaveConversationSaveData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Dialogue|Save")
	FName DialogueId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Dialogue|Save")
	int32 DialogueVersion = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Dialogue|Save")
	FName CurrentNodeId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Dialogue|Save")
	TMap<FName, bool> RuntimeFlags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Dialogue|Save")
	TMap<FName, float> RuntimeNumericValues;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Dialogue|Save")
	TMap<FName, FString> RuntimeMetadata;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Dialogue|Save")
	bool bIsActive = false;
};

/* --- Dialogue Event Delegates --- */

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnConversationStartedSignature, UShadowSlaveDialogueDefinition*, DialogueDef, AActor*, Speaker, AActor*, Interactor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDialogueNodeDisplayedSignature, const FShadowSlaveDialogueNode&, Node, const TArray<FShadowSlaveDialogueChoice>&, AvailableChoices);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDialogueChoicesUpdatedSignature, const TArray<FShadowSlaveDialogueChoice>&, AvailableChoices);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDialogueChoiceSelectedSignature, const FShadowSlaveDialogueChoice&, SelectedChoice, AActor*, Interactor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnConversationCompletedSignature, FName, DialogueId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnConversationAbortedSignature, FName, DialogueId, FName, LastNodeId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnDialogueEventTriggeredSignature, FName, EventId, AActor*, Interactor, AActor*, Speaker);
