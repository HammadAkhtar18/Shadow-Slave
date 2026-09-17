// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Dialogue/ShadowSlaveDialogueTypes.h"
#include "Dialogue/ShadowSlaveDialogueDefinition.h"
#include "ShadowSlaveConversationSubsystem.generated.h"

class AActor;
class UShadowSlaveInventoryComponent;
class UShadowSlaveProgressionComponent;

/**
 * Game Instance Subsystem acting as the authoritative runtime manager for dialogue and conversations.
 *
 * RESPONSIBILITIES:
 * - Authoritative owner of active conversation state machine and current dialogue node.
 * - Manages condition evaluation against interactor gameplay components (Inventory, Progression, etc.).
 * - Executes generic dialogue consequences (flags, numerics, inventory items, technical events).
 * - Emits clean, decoupled event streams for UI/presentation consumption without constructing widgets.
 * - Guards against conversation re-entrancy, invalid choices, and recursion.
 * - Operates event-driven with zero Tick overhead.
 */
UCLASS()
class SHADOWSLAVE_API UShadowSlaveConversationSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UShadowSlaveConversationSubsystem();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/* --- Conversation Lifecycle API --- */

	/**
	 * Initiates a new conversation session using the provided dialogue definition.
	 * Fails safely if another conversation is already active, definition is null/invalid, or starting node is missing.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Dialogue|Session")
	bool StartConversation(UShadowSlaveDialogueDefinition* DialogueDef, AActor* InSpeaker, AActor* InInteractor);

	/**
	 * Selects an available dialogue choice by its 0-based index within CurrentAvailableChoices.
	 * Re-evaluates conditions immediately prior to execution; fails safely if conditions no longer pass.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Dialogue|Session")
	bool SelectChoice(int32 ChoiceIndex);

	/**
	 * Selects an available dialogue choice by its unique ChoiceId.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Dialogue|Session")
	bool SelectChoiceById(FName ChoiceId);

	/**
	 * Advances the conversation when displaying a leaf/terminal node with no response choices.
	 * Completes the conversation cleanly.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Dialogue|Session")
	bool AdvanceConversation();

	/**
	 * Aborts the active conversation immediately (e.g. interactor damaged, moved away, or cancelled).
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Dialogue|Session")
	bool AbortConversation();

	/**
	 * Completes the active conversation gracefully.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Dialogue|Session")
	void CompleteConversation();

	/* --- Runtime State Queries --- */

	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Dialogue|State")
	bool IsConversationActive() const;

	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Dialogue|State")
	EShadowSlaveConversationState GetConversationState() const { return CurrentState; }

	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Dialogue|State")
	UShadowSlaveDialogueDefinition* GetActiveDialogue() const { return ActiveDialogueDef; }

	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Dialogue|State")
	FName GetCurrentNodeId() const { return CurrentNodeId; }

	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Dialogue|State")
	const FShadowSlaveDialogueNode& GetCurrentNode() const { return CurrentNode; }

	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Dialogue|State")
	const TArray<FShadowSlaveDialogueChoice>& GetAvailableChoices() const { return CurrentAvailableChoices; }

	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Dialogue|State")
	AActor* GetCurrentSpeaker() const;

	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Dialogue|State")
	AActor* GetCurrentInteractor() const;

	/* --- Condition & Consequence API --- */

	/** Evaluates an individual dialogue condition against the current runtime context */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Dialogue|Evaluation")
	bool EvaluateCondition(const FShadowSlaveDialogueCondition& Condition) const;

	/** Evaluates an array of conditions; returns true only if all conditions pass (logical AND) */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Dialogue|Evaluation")
	bool EvaluateConditions(const TArray<FShadowSlaveDialogueCondition>& Conditions) const;

	/** Executes an individual consequence; returns true if successful */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Dialogue|Execution")
	bool ExecuteConsequence(const FShadowSlaveDialogueConsequence& Consequence);

	/** Executes an array of consequences; stops and returns false immediately if any consequence fails */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Dialogue|Execution")
	bool ExecuteConsequences(const TArray<FShadowSlaveDialogueConsequence>& Consequences);

	/* --- Runtime Variables & Flags --- */

	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Dialogue|Variables")
	bool GetRuntimeFlag(FName Key, bool DefaultValue = false) const;

	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Dialogue|Variables")
	void SetRuntimeFlag(FName Key, bool Value);

	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Dialogue|Variables")
	float GetRuntimeNumericValue(FName Key, float DefaultValue = 0.0f) const;

	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Dialogue|Variables")
	void SetRuntimeNumericValue(FName Key, float Value);

	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Dialogue|Variables")
	FString GetRuntimeMetadata(FName Key) const;

	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Dialogue|Variables")
	void SetRuntimeMetadata(FName Key, const FString& Value);

	/** Clears all runtime flags, numeric values, and metadata */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Dialogue|Variables")
	void ClearRuntimeVariables();

	/* --- Save / Load Snapshots --- */

	/** Captures serializable passive snapshot of current conversation runtime state */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Dialogue|Save")
	void CaptureConversationState(FShadowSlaveConversationSaveData& OutSaveData) const;

	/** Restores conversation runtime state from passive save data */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Dialogue|Save")
	bool RestoreConversationState(const FShadowSlaveConversationSaveData& InSaveData, UShadowSlaveDialogueDefinition* InDialogueDef = nullptr);

	/* --- Event Delegates --- */

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Dialogue|Events")
	FOnConversationStartedSignature OnConversationStarted;

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Dialogue|Events")
	FOnDialogueNodeDisplayedSignature OnDialogueNodeDisplayed;

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Dialogue|Events")
	FOnDialogueChoicesUpdatedSignature OnChoicesUpdated;

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Dialogue|Events")
	FOnDialogueChoiceSelectedSignature OnChoiceSelected;

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Dialogue|Events")
	FOnConversationCompletedSignature OnConversationCompleted;

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Dialogue|Events")
	FOnConversationAbortedSignature OnConversationAborted;

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Dialogue|Events")
	FOnDialogueEventTriggeredSignature OnDialogueEventTriggered;

protected:
	/** Transitions to and displays the designated dialogue node. Returns false if node consequences fail. */
	bool DisplayNode(const FShadowSlaveDialogueNode& Node);

	/** Resets conversation lifecycle state back to Inactive */
	void ResetState();

	/** Active static dialogue asset */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Dialogue|State")
	TObjectPtr<UShadowSlaveDialogueDefinition> ActiveDialogueDef = nullptr;

	/** Currently displayed node identifier */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Dialogue|State")
	FName CurrentNodeId = NAME_None;

	/** Current operational node payload */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Dialogue|State")
	FShadowSlaveDialogueNode CurrentNode;

	/** Current operational state of the conversation state machine */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Dialogue|State")
	EShadowSlaveConversationState CurrentState = EShadowSlaveConversationState::Inactive;

	/** Participating speaker actor (e.g. NPC) */
	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> CurrentSpeakerActor = nullptr;

	/** Participating interactor actor (e.g. Player) */
	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> CurrentInteractorActor = nullptr;

	/** Filtered choices whose conditions currently pass */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Dialogue|State")
	TArray<FShadowSlaveDialogueChoice> CurrentAvailableChoices;

	/** Runtime boolean flags for conversation session tracking */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Dialogue|State")
	TMap<FName, bool> RuntimeFlags;

	/** Runtime numeric variables for conversation session tracking */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Dialogue|State")
	TMap<FName, float> RuntimeNumericValues;

	/** Extensible session metadata */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Dialogue|State")
	TMap<FName, FString> RuntimeMetadata;

	/** Re-entrancy guard preventing recursive step executions */
	UPROPERTY(Transient)
	bool bIsProcessingStep = false;
};
