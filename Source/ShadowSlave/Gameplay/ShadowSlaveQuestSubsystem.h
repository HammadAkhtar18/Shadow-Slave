// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Gameplay/ShadowSlaveQuestTypes.h"
#include "Gameplay/ShadowSlaveQuestDefinition.h"
#include "ShadowSlaveQuestSubsystem.generated.h"

class UShadowSlaveStorySubsystem;

/**
 * Game Instance Subsystem acting as the authoritative runtime manager for quests and objectives.
 *
 * RESPONSIBILITIES:
 * - Authoritative owner of runtime quest states and objective progress.
 * - Registers static quest definitions without mutating data assets.
 * - Enforces deterministic lifecycle transitions and prerequisite gating.
 * - Tracks quantity-based and explicit objective progress.
 * - Dispatches decoupled event streams for UI, story, and gameplay listeners.
 * - Exports and imports passive save data with zero event emission during restoration.
 * - Operates strictly event-driven with zero Tick overhead.
 */
UCLASS()
class SHADOWSLAVE_API UShadowSlaveQuestSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UShadowSlaveQuestSubsystem();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/* --- Quest Definition Registration --- */

	/** Registers a static quest definition data asset with the subsystem */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Quest|Registration")
	bool RegisterQuestDefinition(UShadowSlaveQuestDefinition* QuestDef);

	/** Unregisters a quest definition */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Quest|Registration")
	bool UnregisterQuestDefinition(FName QuestId);

	/** Retrieves a registered quest definition by QuestId */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Quest|Registration")
	UShadowSlaveQuestDefinition* GetQuestDefinition(FName QuestId) const;

	/** Returns true if a quest definition is registered for QuestId */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Quest|Registration")
	bool HasQuestDefinition(FName QuestId) const;

	/* --- Quest State Queries --- */

	/** Queries the current lifecycle state of a quest */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Quest|State")
	EShadowSlaveQuestState GetQuestState(FName QuestId) const;

	/** Returns true if the quest is currently Active */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Quest|State")
	bool IsQuestActive(FName QuestId) const;

	/** Returns true if the quest is Completed */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Quest|State")
	bool IsQuestCompleted(FName QuestId) const;

	/** Returns true if the quest is Failed */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Quest|State")
	bool IsQuestFailed(FName QuestId) const;

	/** Returns true if the quest is Abandoned */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Quest|State")
	bool IsQuestAbandoned(FName QuestId) const;

	/** Evaluates whether all prerequisite quests are in Completed state */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Quest|State")
	bool ArePrerequisitesSatisfied(FName QuestId) const;

	/** Retrieves complete runtime state for a quest */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Quest|State")
	bool GetQuestRuntimeState(FName QuestId, FShadowSlaveQuestRuntimeState& OutState) const;

	/** Returns all currently active quest IDs */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Quest|State")
	TArray<FName> GetActiveQuestIds() const;

	/** Returns all tracked quest IDs */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Quest|State")
	TArray<FName> GetAllTrackedQuestIds() const;

	/* --- Objective State & Progress Queries --- */

	/** Queries the current operational state of a specific objective */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Quest|Objective")
	EShadowSlaveObjectiveState GetObjectiveState(FName QuestId, FName ObjectiveId) const;

	/** Queries current progress quantity of an objective */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Quest|Objective")
	int32 GetObjectiveProgress(FName QuestId, FName ObjectiveId) const;

	/** Queries target quantity required for objective completion */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Quest|Objective")
	int32 GetObjectiveRequiredQuantity(FName QuestId, FName ObjectiveId) const;

	/** Retrieves complete runtime state for a specific objective */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Quest|Objective")
	bool GetObjectiveRuntimeState(FName QuestId, FName ObjectiveId, FShadowSlaveObjectiveRuntimeState& OutState) const;

	/** Returns true if the objective is currently Active */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Quest|Objective")
	bool IsObjectiveActive(FName QuestId, FName ObjectiveId) const;

	/** Returns true if the objective is Completed */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Quest|Objective")
	bool IsObjectiveCompleted(FName QuestId, FName ObjectiveId) const;

	/* --- UI / Presentation Helper Queries --- */

	/** Returns display title for a quest */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Quest|UI")
	FText GetQuestTitle(FName QuestId) const;

	/** Returns description for a quest */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Quest|UI")
	FText GetQuestDescription(FName QuestId) const;

	/** Returns display title for an objective */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Quest|UI")
	FText GetObjectiveTitle(FName QuestId, FName ObjectiveId) const;

	/** Returns description for an objective */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Quest|UI")
	FText GetObjectiveDescription(FName QuestId, FName ObjectiveId) const;

	/* --- Quest Lifecycle Operations --- */

	/**
	 * Transitions a quest to a new lifecycle state following validation rules.
	 * Rejects invalid, unknown, or unsatisfied transitions safely.
	 * Emits events only when the state genuinely changes.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Quest|Lifecycle")
	bool SetQuestState(FName QuestId, EShadowSlaveQuestState NewState);

	/** Activates an Available or Locked quest whose prerequisites are satisfied */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Quest|Lifecycle")
	bool ActivateQuest(FName QuestId);

	/** Explicitly completes an Active quest */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Quest|Lifecycle")
	bool CompleteQuest(FName QuestId);

	/** Explicitly fails an Active quest */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Quest|Lifecycle")
	bool FailQuest(FName QuestId);

	/** Explicitly abandons an Active quest */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Quest|Lifecycle")
	bool AbandonQuest(FName QuestId);

	/**
	 * Explicit administrative reset API allowing any quest (including terminal states)
	 * to return to Locked or Available state.
	 * Rejects Unknown, Active, Completed, Failed, and Abandoned target states.
	 * If resetting to Available, prerequisites must be satisfied.
	 * Resets all child objective progress and states to Inactive.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Quest|Lifecycle")
	bool ResetQuestState(FName QuestId, EShadowSlaveQuestState ResetToState = EShadowSlaveQuestState::Locked);

	/** Resets all tracked runtime quest states to initial availability */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Quest|Lifecycle")
	void ResetAllQuests();

	/* --- Objective Progress Operations --- */

	/**
	 * Sets progress on an objective within an Active quest.
	 * Clamps progress between 0 and RequiredQuantity.
	 * Automatically transitions objective to Completed when RequiredQuantity is reached.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Quest|Progress")
	bool SetObjectiveProgress(FName QuestId, FName ObjectiveId, int32 NewProgress);

	/** Adds an amount to an objective's current progress */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Quest|Progress")
	bool AddObjectiveProgress(FName QuestId, FName ObjectiveId, int32 Amount = 1);

	/** Explicitly marks an objective as Completed and sets progress to RequiredQuantity */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Quest|Progress")
	bool CompleteObjective(FName QuestId, FName ObjectiveId);

	/** Explicitly marks an objective as Failed; fails the quest if objective is mandatory */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Quest|Progress")
	bool FailObjective(FName QuestId, FName ObjectiveId);

	/* --- Save & Persistence --- */

	/** Exports passive save data for quest and objective progression */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Quest|Save")
	FShadowSlaveQuestSaveData ExportSaveData() const;

	/** Imports passive save data, suppressing gameplay progression events */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Quest|Save")
	bool ImportSaveData(const FShadowSlaveQuestSaveData& InSaveData);

	/* --- Integration Boundaries --- */

	/** Helper to access the authoritative StorySubsystem */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Quest|Integration")
	UShadowSlaveStorySubsystem* GetStorySubsystem() const;

	/* --- Events --- */

	/** Broadcast when a quest's lifecycle state genuinely changes */
	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Quest|Events")
	FOnShadowSlaveQuestStateChangedSignature OnQuestStateChanged;

	/** Broadcast when an objective's lifecycle state genuinely changes */
	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Quest|Events")
	FOnShadowSlaveObjectiveStateChangedSignature OnObjectiveStateChanged;

	/** Broadcast when an objective's progress quantity genuinely changes */
	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Quest|Events")
	FOnShadowSlaveObjectiveProgressChangedSignature OnObjectiveProgressChanged;

	/** Broadcast when a quest is completed */
	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Quest|Events")
	FOnShadowSlaveQuestCompletedSignature OnQuestCompleted;

	/** Broadcast when a quest is failed */
	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Quest|Events")
	FOnShadowSlaveQuestFailedSignature OnQuestFailed;

protected:
	/** Static quest definitions registered by QuestId */
	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UShadowSlaveQuestDefinition>> RegisteredDefinitions;

	/** Mutable runtime quest state indexed by QuestId */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Quest")
	TMap<FName, FShadowSlaveQuestRuntimeState> QuestRuntimeStates;

	/** Internal flag indicating save data restoration is in progress */
	bool bIsRestoringState = false;

	/** Re-entrancy guard */
	bool bIsProcessingTransition = false;

	/** Validates whether a state transition is legal according to the quest state machine */
	bool CanTransitionQuest(FName QuestId, EShadowSlaveQuestState CurrentState, EShadowSlaveQuestState NewState) const;

	/** Initializes objective runtime states from definition into runtime quest state */
	void InitializeRuntimeObjectives(FShadowSlaveQuestRuntimeState& QuestRuntime, const UShadowSlaveQuestDefinition* Def);

	/** Transitions inactive objectives to Active when a quest activates */
	void ActivateObjectivesForQuest(FShadowSlaveQuestRuntimeState& QuestRuntime);

	/** Evaluates if quest auto-completion rule is met */
	void EvaluateQuestCompletion(FName QuestId);
};
