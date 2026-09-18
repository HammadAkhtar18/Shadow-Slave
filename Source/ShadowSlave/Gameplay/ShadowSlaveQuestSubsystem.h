// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Gameplay/ShadowSlaveQuestTypes.h"
#include "Gameplay/ShadowSlaveQuestDefinition.h"
#include "World/ShadowSlaveWorldTypes.h"
#include "Items/ShadowSlaveItemTypes.h"
#include "Interaction/ShadowSlaveInteractionTypes.h"
#include "ShadowSlaveQuestSubsystem.generated.h"

class UShadowSlaveStorySubsystem;
class APawn;
class AActor;
class UShadowSlaveInventoryComponent;
class UShadowSlaveInteractionComponent;
class UShadowSlaveWorldStateComponent;
class UShadowSlaveItemDefinition;
class UShadowSlaveNightmareScenarioDefinition;
class AShadowSlaveCharacterBase;

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

	/* --- Context & Source Registration --- */

	/** Registers participating player pawn and binds to its gameplay components (inventory, interaction, world state) */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Quest|Context")
	bool RegisterPlayerContext(APawn* PlayerPawn);

	/** Unregisters player pawn and unbinds from its gameplay components */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Quest|Context")
	void UnregisterPlayerContext();

	/** Registers an inventory component source to observe item acquisition */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Quest|Integration")
	void RegisterInventorySource(UShadowSlaveInventoryComponent* InventoryComponent);

	/** Unregisters an observed inventory component source */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Quest|Integration")
	void UnregisterInventorySource(UShadowSlaveInventoryComponent* InventoryComponent);

	/** Registers an interaction component source to observe player interactions */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Quest|Integration")
	void RegisterInteractionSource(UShadowSlaveInteractionComponent* InteractionComponent);

	/** Unregisters an observed interaction component source */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Quest|Integration")
	void UnregisterInteractionSource(UShadowSlaveInteractionComponent* InteractionComponent);

	/** Registers a world state component source to observe state changes */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Quest|Integration")
	void RegisterWorldStateSource(UShadowSlaveWorldStateComponent* WorldStateComponent);

	/** Unregisters an observed world state component source */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Quest|Integration")
	void UnregisterWorldStateSource(UShadowSlaveWorldStateComponent* WorldStateComponent);

	/** Registers a participating character to observe death events */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Quest|Integration")
	void RegisterCharacterSource(AShadowSlaveCharacterBase* Character);

	/** Unregisters an observed participating character */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Quest|Integration")
	void UnregisterCharacterSource(AShadowSlaveCharacterBase* Character);

	/* --- Gameplay Event Notification API --- */

	/**
	 * Notifies the quest subsystem of a successful interaction with a world object.
	 * Advances matching active Interact objectives.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Quest|Events")
	bool NotifyInteraction(AActor* Interactor, AActor* InteractableObject, FName InteractionId = NAME_None);

	/**
	 * Notifies the quest subsystem that a conversation completed cleanly.
	 * Advances matching active TalkToCharacter objectives.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Quest|Events")
	bool NotifyConversationCompleted(FName DialogueId, AActor* SpeakerActor = nullptr);

	/**
	 * Notifies the quest subsystem that a target actor was defeated.
	 * Advances matching active DefeatTarget objectives. Idempotent against duplicate death reports.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Quest|Events")
	bool NotifyTargetDefeated(FName TargetId, AActor* DefeatedActor = nullptr, AActor* KillerActor = nullptr);

	/**
	 * Notifies the quest subsystem that items were accepted into inventory.
	 * Advances matching active CollectItem objectives using the confirmed quantity.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Quest|Events")
	bool NotifyItemCollected(FName ItemId, int32 Quantity = 1, UShadowSlaveItemDefinition* ItemDef = nullptr);

	/**
	 * Notifies the quest subsystem that an authoritative world state variable changed.
	 * Evaluates and completes matching active WorldState objectives.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Quest|Events")
	bool NotifyWorldStateChanged(FName StateKey, const FShadowSlaveWorldValue& NewValue, AActor* OwningActor = nullptr);

	/**
	 * Notifies the quest subsystem that a location boundary or trigger was reached.
	 * Completes matching active ReachLocation objectives.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Quest|Events")
	bool NotifyLocationReached(FName LocationId, AActor* TriggerActor = nullptr);

	/**
	 * Notifies the quest subsystem that a survival condition or encounter was completed.
	 * Completes matching active Survive objectives.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Quest|Events")
	bool NotifySurvivalCompleted(FName SurvivalId, AActor* Actor = nullptr);

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

	/* --- Internal Event Listeners --- */

	UFUNCTION()
	void HandleInventoryItemAdded(const FShadowSlaveItemInstance& ItemInstance, int32 QuantityAdded);

	UFUNCTION()
	void HandleInteractionExecuted(AActor* Interactor, AActor* InteractableObject, const FShadowSlaveInteractionResult& Result);

	UFUNCTION()
	void HandleConversationCompleted(FName DialogueId);

	UFUNCTION()
	void HandleWorldStateChanged(FName Key, const FShadowSlaveWorldValue& NewValue, const FShadowSlaveWorldValue& OldValue, AActor* OwningActor);

	UFUNCTION()
	void HandleSelfQuestCompleted(FName CompletedQuestId);

	UFUNCTION()
	void HandleNightmareScenarioCompleted(UShadowSlaveNightmareScenarioDefinition* ScenarioDef);

	UFUNCTION()
	void HandleCharacterDied(AShadowSlaveCharacterBase* DeadCharacter, AActor* KillerActor);

	/** Weak reference to currently registered player pawn */
	UPROPERTY(Transient)
	TWeakObjectPtr<APawn> CurrentPlayerPawn = nullptr;

	/** Registered inventory components observed for item collection events */
	TSet<TWeakObjectPtr<UShadowSlaveInventoryComponent>> RegisteredInventorySources;

	/** Registered interaction components observed for player interaction events */
	TSet<TWeakObjectPtr<UShadowSlaveInteractionComponent>> RegisteredInteractionSources;

	/** Registered world state components observed for state changes */
	TSet<TWeakObjectPtr<UShadowSlaveWorldStateComponent>> RegisteredWorldStateSources;

	/** Registered character sources observed for death events */
	TSet<TWeakObjectPtr<AShadowSlaveCharacterBase>> RegisteredCharacterSources;

	/** Actors already processed for death to guarantee idempotency */
	UPROPERTY(Transient)
	TSet<TWeakObjectPtr<AActor>> ProcessedDefeatedActors;

	/** Frame counters for interaction anti-duplication */
	TMap<TWeakObjectPtr<AActor>, uint64> LastInteractionFrames;

	/** Frame counters for conversation anti-duplication */
	TMap<FName, uint64> LastConversationFrames;
};
