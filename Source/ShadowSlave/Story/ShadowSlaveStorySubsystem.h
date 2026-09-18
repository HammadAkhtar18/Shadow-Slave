// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Story/ShadowSlaveStoryTypes.h"
#include "Story/ShadowSlaveStoryDefinition.h"
#include "Story/ShadowSlaveStoryContentTypes.h"
#include "Story/ShadowSlaveStoryContentDefinition.h"
#include "World/ShadowSlaveWorldTypes.h"
#include "Nightmares/ShadowSlaveNightmareTypes.h"
#include "ShadowSlaveStorySubsystem.generated.h"

class UShadowSlaveNightmareScenarioDefinition;
class UShadowSlaveWorldStateComponent;
class AActor;

/**
 * Game Instance Subsystem responsible for authoritative story progression, narrative beat lifecycle,
 * and data-driven story content (chapter/arc) orchestration via the Progression Bridge.
 *
 * RESPONSIBILITIES:
 * - Registers static story definitions and story content definitions (chapters/arcs).
 * - Tracks runtime lifecycle state (Locked, Available, Active, Completed, Failed, Skipped).
 * - Enforces deterministic transition guards and prerequisite completion checks.
 * - Manages active story steps and chronological content entries.
 * - Coordinates with external domain subsystems (Quests, Dialogue, Nightmares, WorldState) via Progression Bridge.
 * - Emits progression event streams only when state or entry genuinely changes.
 * - Exports and imports passive, decoupled save data with zero event emission during restoration.
 * - Operates event-driven with zero Tick overhead.
 */
UCLASS()
class SHADOWSLAVE_API UShadowSlaveStorySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UShadowSlaveStorySubsystem();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/* --- Story Definition Registration --- */

	/** Registers a static story definition data asset with the subsystem */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Story|Registration")
	bool RegisterStoryDefinition(UShadowSlaveStoryDefinition* StoryDef);

	/** Unregisters a story definition */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Story|Registration")
	bool UnregisterStoryDefinition(FName StoryId);

	/** Retrieves a registered story definition by StoryId */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Story|Registration")
	UShadowSlaveStoryDefinition* GetStoryDefinition(FName StoryId) const;

	/** Returns true if a story definition is registered for StoryId */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Story|Registration")
	bool HasStoryDefinition(FName StoryId) const;

	/* --- Story Content Definition Registration (Step 25) --- */

	/** Registers a static story content definition (chapter/arc) with the subsystem */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Story|ContentRegistration")
	bool RegisterStoryContentDefinition(UShadowSlaveStoryContentDefinition* ContentDef);

	/** Unregisters a story content definition */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Story|ContentRegistration")
	bool UnregisterStoryContentDefinition(FName StoryContentId);

	/** Retrieves a registered story content definition by StoryContentId */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Story|ContentRegistration")
	UShadowSlaveStoryContentDefinition* GetStoryContentDefinition(FName StoryContentId) const;

	/** Returns true if a story content definition is registered for StoryContentId */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Story|ContentRegistration")
	bool HasStoryContentDefinition(FName StoryContentId) const;

	/* --- Story State Queries --- */

	/** Queries the current lifecycle state of a story */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Story|State")
	EShadowSlaveStoryState GetStoryState(FName StoryId) const;

	/** Returns true if the story is currently Active */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Story|State")
	bool IsStoryActive(FName StoryId) const;

	/** Returns true if the story is Completed */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Story|State")
	bool IsStoryCompleted(FName StoryId) const;

	/** Evaluates whether all prerequisite story units are in Completed state */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Story|State")
	bool ArePrerequisitesSatisfied(FName StoryId) const;

	/** Retrieves complete runtime state for a story */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Story|State")
	bool GetStoryRuntimeState(FName StoryId, FShadowSlaveStoryRuntimeState& OutState) const;

	/* --- Story Content State Queries (Step 25) --- */

	/** Queries the current lifecycle state of a story content chapter or arc */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Story|ContentState")
	EShadowSlaveStoryContentState GetStoryContentState(FName StoryContentId) const;

	/** Returns true if the story content is currently Active */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Story|ContentState")
	bool IsStoryContentActive(FName StoryContentId) const;

	/** Returns true if the story content is Completed */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Story|ContentState")
	bool IsStoryContentCompleted(FName StoryContentId) const;

	/** Evaluates whether all prerequisite story content units are in Completed state */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Story|ContentState")
	bool AreStoryContentPrerequisitesSatisfied(FName StoryContentId) const;

	/** Retrieves complete runtime state for a story content chapter or arc */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Story|ContentState")
	bool GetStoryContentRuntimeState(FName StoryContentId, FShadowSlaveStoryContentRuntimeState& OutState) const;

	/** Queries the current lifecycle state of an individual content entry within a story content */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Story|ContentState")
	EShadowSlaveStoryContentState GetStoryContentEntryState(FName StoryContentId, FName EntryId) const;

	/** Evaluates whether all prerequisite content entries for an entry are in Completed state */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Story|ContentState")
	bool AreStoryContentEntryPrerequisitesSatisfied(FName StoryContentId, FName EntryId) const;

	/** Retrieves the currently active content entry ID for an active story content chapter/arc */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Story|ContentState")
	FName GetCurrentActiveStoryContentEntry(FName StoryContentId) const;

	/** Returns true if all non-optional content entries within the story content are Completed */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Story|ContentState")
	bool AreAllRequiredContentEntriesCompleted(FName StoryContentId) const;

	/* --- Story State Transitions --- */

	/**
	 * Transitions a story to a new lifecycle state following transition safety rules.
	 * Rejects invalid, unknown, or unsatisfied transitions safely.
	 * Emits OnStoryStateChanged only when the state genuinely changes.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Story|Transitions")
	bool SetStoryState(FName StoryId, EShadowSlaveStoryState NewState);

	/**
	 * Explicit administrative reset API allowing any story (including terminal states)
	 * to return to Locked or Available state.
	 * Rejects Unknown, Active, Completed, Failed, and Skipped.
	 * If resetting to Available, prerequisites must be satisfied.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Story|Transitions")
	bool ResetStoryState(FName StoryId, EShadowSlaveStoryState ResetToState = EShadowSlaveStoryState::Locked);

	/** Resets all tracked runtime story states */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Story|Transitions")
	void ResetAllStoryStates();

	/* --- Story Content State Transitions (Step 25) --- */

	/** Transitions a story content chapter/arc to a new lifecycle state */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Story|ContentTransitions")
	bool SetStoryContentState(FName StoryContentId, EShadowSlaveStoryContentState NewState);

	/** Helper to transition story content to Active */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Story|ContentTransitions")
	bool ActivateStoryContent(FName StoryContentId);

	/** Helper to transition story content to Completed */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Story|ContentTransitions")
	bool CompleteStoryContent(FName StoryContentId);

	/** Helper to transition story content to Failed */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Story|ContentTransitions")
	bool FailStoryContent(FName StoryContentId);

	/** Helper to transition story content to Skipped */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Story|ContentTransitions")
	bool SkipStoryContent(FName StoryContentId);

	/** Explicit administrative reset API for story content (allows reset to Locked or Available) */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Story|ContentTransitions")
	bool ResetStoryContentState(FName StoryContentId, EShadowSlaveStoryContentState ResetToState = EShadowSlaveStoryContentState::Locked);

	/** Resets all tracked runtime story content states */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Story|ContentTransitions")
	void ResetAllStoryContentStates();

	/* --- Story Content Entry Transitions (Step 25 & 26) --- */

	/** Transitions a child content entry to a new lifecycle state */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Story|ContentTransitions")
	bool SetStoryContentEntryState(FName StoryContentId, FName EntryId, EShadowSlaveStoryContentState NewState);

	/** Activates a child content entry and establishes domain observation */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Story|ContentTransitions")
	bool ActivateStoryContentEntry(FName StoryContentId, FName EntryId);

	/** Helper to transition content entry to Completed */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Story|ContentTransitions")
	bool CompleteStoryContentEntry(FName StoryContentId, FName EntryId);

	/** Helper to transition content entry to Failed */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Story|ContentTransitions")
	bool FailStoryContentEntry(FName StoryContentId, FName EntryId);

	/** Helper to transition content entry to Skipped */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Story|ContentTransitions")
	bool SkipStoryContentEntry(FName StoryContentId, FName EntryId);

	/** Explicitly sets the currently active entry ID on an Active story content */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Story|ContentTransitions")
	bool SetCurrentActiveStoryContentEntry(FName StoryContentId, FName EntryId);

	/* --- Progression Bridge API (Step 26) --- */

	/** Initializes event bindings with external domain subsystems (Quest, Dialogue, Nightmare) */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Story|Bridge")
	void InitializeProgressionBridge();

	/** Shuts down all event bindings with external domain subsystems */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Story|Bridge")
	void ShutdownProgressionBridge();

	/** Checks if the progression bridge is currently active */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Story|Bridge")
	bool IsProgressionBridgeActive() const { return bIsBridgeActive; }

	/** Registers an actor's WorldStateComponent as an event source for WorldState story content */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Story|Bridge")
	void RegisterWorldStateSource(UShadowSlaveWorldStateComponent* WorldStateComponent);

	/** Unregisters an actor's WorldStateComponent from story content observation */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Story|Bridge")
	void UnregisterWorldStateSource(UShadowSlaveWorldStateComponent* WorldStateComponent);

	/** Direct notification API when an external domain target reaches completion */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Story|Progression")
	bool NotifyStoryContentTargetCompleted(EShadowSlaveStoryContentType ContentType, FName TargetId);

	/** Direct notification API when an external domain target fails */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Story|Progression")
	bool NotifyStoryContentTargetFailed(EShadowSlaveStoryContentType ContentType, FName TargetId);

	/** Direct notification API when a world state key changes */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Story|Progression")
	bool NotifyWorldStateChanged(FName StateKey, const FShadowSlaveWorldValue& NewValue, AActor* OwningActor = nullptr);

	/** Queries the TargetId currently being observed by an active story content */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Story|Bridge")
	FName GetActiveObservedTargetId(FName StoryContentId) const;

	/** Queries the ContentType currently being observed by an active story content */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Story|Bridge")
	EShadowSlaveStoryContentType GetActiveObservedContentType(FName StoryContentId) const;

	/** Finds the next valid entry in authored array order that can be activated */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Story|Progression")
	FName FindNextProgressionEntryId(FName StoryContentId) const;

	/** Evaluates and advances an active story content to its next authored entry or completion */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Story|Progression")
	void AdvanceStoryContentProgression(FName StoryContentId);

	/* --- Story Step Support --- */

	/**
	 * Sets the current step ID on an Active story.
	 * Requires the story definition to exist and the requested non-None StepId to exist in definition.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Story|Steps")
	bool SetCurrentStoryStep(FName StoryId, FName StepId);

	/** Retrieves the current step ID of a story */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Story|Steps")
	FName GetCurrentStoryStep(FName StoryId) const;

	/* --- Save & Persistence --- */

	/** Exports passive save data for story progression and story content */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Story|Save")
	FShadowSlaveStorySaveData ExportSaveData() const;

	/** Imports passive save data, suppressing gameplay progression events */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Story|Save")
	bool ImportSaveData(const FShadowSlaveStorySaveData& InSaveData);

	/* --- Events --- */

	/** Broadcast when a story's lifecycle state genuinely changes */
	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Story|Events")
	FOnShadowSlaveStoryStateChangedSignature OnStoryStateChanged;

	/** Broadcast when an active story's current step genuinely changes */
	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Story|Events")
	FOnShadowSlaveStoryStepChangedSignature OnStoryStepChanged;

	/** Broadcast when a story content's overall lifecycle state genuinely changes */
	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Story|Events")
	FOnShadowSlaveStoryContentStateChangedSignature OnStoryContentStateChanged;

	/** Broadcast when an individual story content entry's lifecycle state genuinely changes */
	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Story|Events")
	FOnShadowSlaveStoryContentEntryStateChangedSignature OnStoryContentEntryStateChanged;

protected:
	/* --- Progression Bridge Event Handlers (Step 26) --- */

	UFUNCTION()
	void HandleQuestCompleted(FName QuestId);

	UFUNCTION()
	void HandleQuestFailed(FName QuestId);

	UFUNCTION()
	void HandleConversationCompleted(FName DialogueId);

	UFUNCTION()
	void HandleConversationAborted(FName DialogueId, FName LastNodeId);

	UFUNCTION()
	void HandleNightmareScenarioCompleted(UShadowSlaveNightmareScenarioDefinition* ScenarioDef);

	UFUNCTION()
	void HandleNightmareScenarioFailed(UShadowSlaveNightmareScenarioDefinition* ScenarioDef, EShadowSlaveScenarioFailureReason Reason);

	UFUNCTION()
	void HandleNightmareScenarioAborted(UShadowSlaveNightmareScenarioDefinition* ScenarioDef);

	UFUNCTION()
	void HandleWorldStateChanged(FName Key, const FShadowSlaveWorldValue& NewValue, const FShadowSlaveWorldValue& OldValue, AActor* OwningActor);

	/** Internal helper to evaluate whether a world-state value change satisfies an entry's condition */
	bool EvaluateWorldStateCondition(const FShadowSlaveStoryContentEntry& EntryDef, const FShadowSlaveWorldValue& NewValue, AActor* OwningActor) const;

	/** Ensures progression bridge is bound if not already */
	void EnsureProgressionBridgeBound();

	/** Static story definitions registered by StoryId */
	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UShadowSlaveStoryDefinition>> RegisteredDefinitions;

	/** Mutable runtime story state indexed by StoryId */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Story")
	TMap<FName, FShadowSlaveStoryRuntimeState> StoryRuntimeStates;

	/** Static story content definitions registered by StoryContentId */
	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UShadowSlaveStoryContentDefinition>> RegisteredContentDefinitions;

	/** Mutable runtime story content state indexed by StoryContentId */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Story|Content")
	TMap<FName, FShadowSlaveStoryContentRuntimeState> StoryContentRuntimeStates;

	/** Registered WorldStateComponent sources for world state observation */
	TSet<TWeakObjectPtr<UShadowSlaveWorldStateComponent>> RegisteredWorldStateSources;

	/** Internal flag indicating save data restoration is in progress (suppresses gameplay events) */
	bool bIsRestoringState = false;

	/** Internal flag indicating whether the progression bridge is active */
	bool bIsBridgeActive = false;

	/** Internal re-entrancy guard for progression advancement */
	bool bIsProcessingProgression = false;

	/** Internal helper to determine if a transition from CurrentState to NewState is permitted */
	bool CanTransition(FName StoryId, EShadowSlaveStoryState CurrentState, EShadowSlaveStoryState NewState) const;

	/** Internal helper to initialize entry states from a content definition */
	void InitializeRuntimeContentEntries(FShadowSlaveStoryContentRuntimeState& RuntimeState, const UShadowSlaveStoryContentDefinition* ContentDef) const;

	/** Internal helper to determine if a transition for story content is permitted */
	bool CanTransitionStoryContent(FName StoryContentId, EShadowSlaveStoryContentState CurrentState, EShadowSlaveStoryContentState NewState) const;

	/** Internal helper to determine if a transition for a content entry is permitted */
	bool CanTransitionStoryContentEntry(FName StoryContentId, FName EntryId, EShadowSlaveStoryContentState CurrentState, EShadowSlaveStoryContentState NewState) const;
};
