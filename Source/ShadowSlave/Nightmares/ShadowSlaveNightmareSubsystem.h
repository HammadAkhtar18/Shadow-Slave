// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Nightmares/ShadowSlaveNightmareTypes.h"
#include "Nightmares/ShadowSlaveNightmareObjectiveTypes.h"
#include "Nightmares/ShadowSlaveNightmareSaveTypes.h"
#include "Combat/ShadowSlaveCombatTypes.h"
#include "ShadowSlaveNightmareSubsystem.generated.h"

class UShadowSlaveNightmareScenarioDefinition;
class UShadowSlaveNightmareObjectiveTracker;
class APlayerController;
class APawn;
class AShadowSlaveCharacterBase;

/**
 * Game Instance Subsystem coordinating Nightmare gameplay sessions and scenarios.
 *
 * RESPONSIBILITIES:
 * - Coordinates lifecycle: Inactive -> Preparing -> Active -> Completed/Failed/Aborted -> Exiting -> Inactive.
 * - Owns authoritative runtime UShadowSlaveNightmareObjectiveTracker.
 * - Tracks participating player context and evaluates scenario completion/failure rules.
 * - Exposes extensible world transition and save/load boundaries.
 *
 * NOTE ON AUTHORITY & TICK:
 * - Subsystem does NOT tick. All state transitions and objective updates are event-driven.
 * - Subsystem is the sole authority for Nightmare session and scenario state.
 * - ZERO hardcoded canon First Nightmare content.
 */
UCLASS()
class SHADOWSLAVE_API UShadowSlaveNightmareSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UShadowSlaveNightmareSubsystem();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/* --- Scenario Lifecycle Operations --- */

	/** Checks if a scenario can be started */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Nightmare|Session")
	bool CanStartScenario(const UShadowSlaveNightmareScenarioDefinition* ScenarioDef, FText& OutFailureReason) const;

	/** Starts a new Nightmare scenario session */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Nightmare|Session")
	bool StartScenario(UShadowSlaveNightmareScenarioDefinition* ScenarioDef, APlayerController* InParticipatingController = nullptr);

	/** Pauses an active scenario */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Nightmare|Session")
	bool PauseScenario();

	/** Resumes a paused scenario */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Nightmare|Session")
	bool ResumeScenario();

	/** Explicitly completes the active scenario */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Nightmare|Session")
	bool CompleteScenario();

	/** Explicitly fails the active scenario with a specific failure reason */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Nightmare|Session")
	bool FailScenario(EShadowSlaveScenarioFailureReason Reason = EShadowSlaveScenarioFailureReason::ScenarioRule);

	/** Aborts the active scenario externally */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Nightmare|Session")
	bool AbortScenario();

	/** Checks if current session state allows beginning the exit sequence */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Nightmare|Session")
	bool CanExitScenario() const;

	/** Initiates exit sequence from completed, failed, or aborted state */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Nightmare|Session")
	bool BeginExitScenario();

	/** Finalizes scenario exit, resets session, and restores Inactive state */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Nightmare|Session")
	bool FinishExitScenario();

	/* --- State Queries --- */

	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Nightmare|Session")
	EShadowSlaveNightmareSessionState GetSessionState() const { return CurrentSessionState; }

	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Nightmare|Session")
	bool IsScenarioActive() const { return CurrentSessionState == EShadowSlaveNightmareSessionState::Active; }

	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Nightmare|Session")
	bool IsScenarioRunning() const { return CurrentSessionState == EShadowSlaveNightmareSessionState::Active || CurrentSessionState == EShadowSlaveNightmareSessionState::Paused; }

	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Nightmare|Session")
	UShadowSlaveNightmareScenarioDefinition* GetCurrentScenario() const { return ActiveScenarioDefinition; }

	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Nightmare|Session")
	FName GetCurrentScenarioId() const;

	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Nightmare|Session")
	int32 GetCurrentScenarioVersion() const;

	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Nightmare|Session")
	UShadowSlaveNightmareObjectiveTracker* GetObjectiveTracker() const { return ObjectiveTracker; }

	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Nightmare|Session")
	EShadowSlaveScenarioFailureReason GetActiveFailureReason() const { return ActiveFailureReason; }

	/* --- Player Context --- */

	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Nightmare|Player")
	void SetParticipatingPlayer(APlayerController* InPC, APawn* InPawn = nullptr);

	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Nightmare|Player")
	APlayerController* GetParticipatingPlayerController() const { return ParticipatingController.Get(); }

	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Nightmare|Player")
	APawn* GetParticipatingPawn() const { return ParticipatingPawn.Get(); }

	/* --- World & Level Transition Boundary --- */

	/** Returns soft reference to scenario map asset (if configured in scenario definition) */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Nightmare|World")
	TSoftObjectPtr<UWorld> GetCurrentScenarioMapAsset() const;

	/**
	 * Extensible boundary hook for requesting scenario world travel/streaming.
	 * Decoupled: does not force level travel in headless/test environments.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Nightmare|World")
	virtual bool RequestScenarioWorldTransition();

	/* --- Persistence Boundary --- */

	/** Exports serializable snapshot of active session and objective progress */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Nightmare|Save")
	FShadowSlaveNightmareSaveData ExportSaveData() const;

	/** Restores session and objective progress from a saved snapshot */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Nightmare|Save")
	bool ImportSaveData(const FShadowSlaveNightmareSaveData& InSaveData);

	/* --- Lifecycle Delegates --- */

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Nightmare|Events")
	FOnNightmareStateChangedSignature OnNightmareStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Nightmare|Events")
	FOnScenarioStartedSignature OnScenarioStarted;

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Nightmare|Events")
	FOnScenarioCompletedSignature OnScenarioCompleted;

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Nightmare|Events")
	FOnScenarioFailedSignature OnScenarioFailed;

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Nightmare|Events")
	FOnScenarioAbortedSignature OnScenarioAborted;

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Nightmare|Events")
	FOnScenarioExitingSignature OnScenarioExiting;

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Nightmare|Events")
	FOnScenarioExitedSignature OnScenarioExited;

	/** Forwarded objective state delegate for convenient UI/external system binding */
	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Nightmare|Events")
	FOnObjectiveStateChangedSignature OnObjectiveStateChanged;

protected:
	/** Validates whether a state transition is legal according to the session state machine */
	bool CanTransitionToState(EShadowSlaveNightmareSessionState TargetState) const;

	/** Transitions session to a new state, updating internal state and broadcasting delegates */
	bool SetSessionState(EShadowSlaveNightmareSessionState NewState);

	/** Evaluates current objective progress against the scenario completion rule */
	void EvaluateScenarioCompletion();

	/** Custom completion rule hook for extensible subclasses or scripting */
	virtual bool EvaluateCustomCompletionRule() const;

	/* --- Component/Player Event Handlers --- */

	UFUNCTION()
	void HandleObjectiveStateChanged(FName ObjectiveId, EShadowSlaveObjectiveState NewState, EShadowSlaveObjectiveState OldState);

	UFUNCTION()
	void HandleCharacterDamaged(const FShadowSlaveDamageInfo& DamageInfo);

	/** Binds to participating player character delegates */
	void BindPlayerDelegates();

	/** Unbinds participating player character delegates */
	void UnbindPlayerDelegates();

	/** Cleans up active scenario state */
	void CleanupSession();

protected:
	/** Current session lifecycle state */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Nightmare|Session")
	EShadowSlaveNightmareSessionState CurrentSessionState = EShadowSlaveNightmareSessionState::Inactive;

	/** Active failure reason if session failed */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Nightmare|Session")
	EShadowSlaveScenarioFailureReason ActiveFailureReason = EShadowSlaveScenarioFailureReason::None;

	/** Currently loaded scenario definition */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Nightmare|Session")
	TObjectPtr<UShadowSlaveNightmareScenarioDefinition> ActiveScenarioDefinition;

	/** Authoritative runtime objective tracker */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Nightmare|Session")
	TObjectPtr<UShadowSlaveNightmareObjectiveTracker> ObjectiveTracker;

	/** Participating player controller */
	TWeakObjectPtr<APlayerController> ParticipatingController;

	/** Participating player pawn */
	TWeakObjectPtr<APawn> ParticipatingPawn;

	/** Participating player character (for damage/death listening) */
	TWeakObjectPtr<AShadowSlaveCharacterBase> ParticipatingCharacter;
};
