// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Story/ShadowSlaveStoryTypes.h"
#include "Story/ShadowSlaveStoryDefinition.h"
#include "ShadowSlaveStorySubsystem.generated.h"

/**
 * Game Instance Subsystem responsible for authoritative story progression and narrative beat lifecycle.
 *
 * RESPONSIBILITIES:
 * - Registers static story definitions.
 * - Tracks runtime lifecycle state (Locked, Available, Active, Completed, Failed, Skipped).
 * - Enforces deterministic transition guards and prerequisite completion checks.
 * - Manages active story steps within Active stories.
 * - Emits progression event streams only when state or step genuinely changes.
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

	/* --- State Queries --- */

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

	/* --- State Transitions --- */

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

	/** Exports passive save data for story progression */
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

protected:
	/** Static story definitions registered by StoryId */
	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UShadowSlaveStoryDefinition>> RegisteredDefinitions;

	/** Mutable runtime story state indexed by StoryId */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Story")
	TMap<FName, FShadowSlaveStoryRuntimeState> StoryRuntimeStates;

	/** Internal flag indicating save data restoration is in progress (suppresses gameplay events) */
	bool bIsRestoringState = false;

	/** Internal helper to determine if a transition from CurrentState to NewState is permitted */
	bool CanTransition(FName StoryId, EShadowSlaveStoryState CurrentState, EShadowSlaveStoryState NewState) const;
};
