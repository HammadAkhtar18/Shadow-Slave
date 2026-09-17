// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Gameplay/ShadowSlaveGameplayTypes.h"
#include "Combat/ShadowSlaveCombatTypes.h"
#include "ShadowSlaveGameplaySubsystem.generated.h"

class APawn;
class APlayerController;
class AActor;
class UShadowSlaveDialogueDefinition;
class UShadowSlaveNightmareScenarioDefinition;
class UShadowSlaveConversationSubsystem;
class UShadowSlaveNightmareSubsystem;
class UShadowSlaveStorySubsystem;
class UShadowSlaveCombatComponent;
class UShadowSlaveWorldStateComponent;
enum class EShadowSlaveScenarioFailureReason : uint8;

/**
 * Game Instance Subsystem acting as the thin orchestration layer for high-level gameplay flow.
 *
 * RESPONSIBILITIES:
 * - Coordinates transitions between high-level gameplay modes: Exploration, Dialogue, Combat, Nightmare, Transitioning, Paused.
 * - Coordinates with authoritative subsystems without duplicating their ownership:
 *   * Dialogue: coordinates with UShadowSlaveConversationSubsystem
 *   * Nightmare: coordinates with UShadowSlaveNightmareSubsystem
 *   * Story: coordinates with UShadowSlaveStorySubsystem
 *   * World State: queries UShadowSlaveWorldStateComponent
 * - Tracks weak references to active player context, interaction targets, and combat instigators.
 * - Enforces transition validation, idempotency, and re-entrancy protection.
 * - Operates strictly event-driven with zero Tick overhead.
 */
UCLASS()
class SHADOWSLAVE_API UShadowSlaveGameplaySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UShadowSlaveGameplaySubsystem();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/* --- Flow State API --- */

	/** Returns current high-level gameplay flow mode */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Gameplay|Flow")
	EShadowSlaveGameplayFlowState GetCurrentFlowState() const { return CurrentFlowState; }

	/** Returns previous gameplay flow mode */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Gameplay|Flow")
	EShadowSlaveGameplayFlowState GetPreviousFlowState() const { return PreviousFlowState; }

	/**
	 * Requests transition to a new high-level gameplay flow state.
	 * Transitions are validated, idempotent, and guarded against re-entrancy.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Gameplay|Flow")
	bool RequestFlowStateTransition(EShadowSlaveGameplayFlowState NewState);

	/** Checks if transition from CurrentState to TargetState is legally permitted */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Gameplay|Flow")
	bool CanTransitionFlowState(EShadowSlaveGameplayFlowState CurrentState, EShadowSlaveGameplayFlowState TargetState) const;

	/* --- High-Level Flow Orchestration --- */

	/** Initiates baseline world exploration flow */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Gameplay|Flow")
	bool BeginExploration();

	/**
	 * Initiates dialogue flow using the authoritative ConversationSubsystem.
	 * Automatically resolves player context if InteractorActor is omitted.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Gameplay|Flow")
	bool BeginDialogue(UShadowSlaveDialogueDefinition* DialogueDef, AActor* SpeakerActor = nullptr, AActor* InteractorActor = nullptr);

	/** Concludes active dialogue flow, optionally aborting or completing cleanly */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Gameplay|Flow")
	bool EndDialogue(bool bAbort = false);

	/** Initiates combat flow, caching an optional instigating adversary */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Gameplay|Flow")
	bool BeginCombatFlow(AActor* InstigatingEnemy = nullptr);

	/** Concludes combat flow and returns to the appropriate previous state (Exploration or Nightmare) */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Gameplay|Flow")
	bool EndCombatFlow();

	/** Initiates nightmare flow with a specified scenario identifier */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Gameplay|Flow")
	bool BeginNightmareFlow(FName ScenarioId);

	/** Initiates nightmare flow using a scenario definition asset */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Gameplay|Flow")
	bool BeginNightmareScenario(UShadowSlaveNightmareScenarioDefinition* ScenarioDef, APlayerController* InPlayerController = nullptr);

	/** Concludes nightmare flow and returns to exploration */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Gameplay|Flow")
	bool EndNightmareFlow();

	/**
	 * Dispatches a world/story transition request to external listeners.
	 * Architectural boundary only: does NOT call OpenLevel or level streaming.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Gameplay|Flow")
	bool RequestWorldStoryTransition(const FShadowSlaveGameplayTransitionRequest& Request);

	/** Enters paused gameplay flow */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Gameplay|Flow")
	bool PauseGameplayFlow();

	/** Resumes from paused gameplay flow to the previous state */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Gameplay|Flow")
	bool ResumeGameplayFlow();

	/* --- Context & Reference Management --- */

	/** Resolves and caches player character and controller from the active world */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Gameplay|Context")
	bool ResolvePlayerContext();

	/** Explicitly assigns active player context */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Gameplay|Context")
	void SetPlayerContext(APlayerController* InController, APawn* InPawn);

	/** Retrieves currently cached player pawn */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Gameplay|Context")
	APawn* GetPlayerPawn() const;

	/** Retrieves currently cached player controller */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Gameplay|Context")
	APlayerController* GetPlayerController() const;

	/** Retrieves active interaction target actor (if any) */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Gameplay|Context")
	AActor* GetInteractionTarget() const;

	/** Sets active interaction target */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Gameplay|Context")
	void SetInteractionTarget(AActor* Target);

	/** Retrieves active dialogue speaker actor (if any) */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Gameplay|Context")
	AActor* GetConversationSpeaker() const;

	/** Retrieves combat instigator actor (if any) */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Gameplay|Context")
	AActor* GetCombatInstigator() const;

	/** Retrieves active nightmare scenario ID (if in nightmare flow) */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Gameplay|Context")
	FName GetActiveNightmareScenarioId() const;

	/** Helper to find WorldStateComponent on the current player pawn */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Gameplay|WorldState")
	UShadowSlaveWorldStateComponent* GetPlayerWorldState() const;

	/** Helper to find WorldStateComponent on the current interaction target */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Gameplay|WorldState")
	UShadowSlaveWorldStateComponent* GetInteractionTargetWorldState() const;

	/** Helper to find WorldStateComponent on an arbitrary actor */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Gameplay|WorldState")
	UShadowSlaveWorldStateComponent* GetWorldStateComponentForActor(const AActor* Actor) const;

	/* --- Authoritative Subsystem Accessors --- */

	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Gameplay|Subsystems")
	UShadowSlaveConversationSubsystem* GetConversationSubsystem() const;

	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Gameplay|Subsystems")
	UShadowSlaveNightmareSubsystem* GetNightmareSubsystem() const;

	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Gameplay|Subsystems")
	UShadowSlaveStorySubsystem* GetStorySubsystem() const;

	/* --- Events --- */

	/** Broadcast when high-level gameplay flow state genuinely changes */
	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Gameplay|Events")
	FOnShadowSlaveGameplayFlowStateChangedSignature OnGameplayFlowStateChanged;

	/** Broadcast when a world/story transition is requested */
	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Gameplay|Events")
	FOnShadowSlaveGameplayTransitionRequestedSignature OnGameplayTransitionRequested;

	/** Broadcast when participating player context is updated */
	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Gameplay|Events")
	FOnShadowSlavePlayerContextUpdatedSignature OnPlayerContextUpdated;

protected:
	/** Current active gameplay flow state */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Gameplay")
	EShadowSlaveGameplayFlowState CurrentFlowState = EShadowSlaveGameplayFlowState::None;

	/** Previous gameplay flow state for clean state restoration */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Gameplay")
	EShadowSlaveGameplayFlowState PreviousFlowState = EShadowSlaveGameplayFlowState::None;

	/** Weak reference to player pawn */
	UPROPERTY(Transient)
	TWeakObjectPtr<APawn> CurrentPlayerPawn;

	/** Weak reference to player controller */
	UPROPERTY(Transient)
	TWeakObjectPtr<APlayerController> CurrentPlayerController;

	/** Weak reference to active interaction target */
	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> CurrentInteractionTarget;

	/** Weak reference to active conversation speaker */
	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> CurrentConversationSpeaker;

	/** Weak reference to active combat instigator */
	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> CurrentCombatInstigator;

	/** Active nightmare scenario ID (if in Nightmare flow) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Gameplay")
	FName ActiveNightmareScenarioId = NAME_None;

	/** Re-entrancy guard */
	bool bIsProcessingFlowTransition = false;

	/* --- Internal Event Handlers --- */

	UFUNCTION()
	void HandleConversationCompleted(FName DialogueId);

	UFUNCTION()
	void HandleConversationAborted(FName DialogueId, FName LastNodeId);

	UFUNCTION()
	void HandleNightmareScenarioStarted(UShadowSlaveNightmareScenarioDefinition* ScenarioDef);

	UFUNCTION()
	void HandleNightmareScenarioEnded(UShadowSlaveNightmareScenarioDefinition* ScenarioDef);

	UFUNCTION()
	void HandleNightmareScenarioFailed(UShadowSlaveNightmareScenarioDefinition* ScenarioDef, EShadowSlaveScenarioFailureReason Reason);

	UFUNCTION()
	void HandlePlayerCombatStateChanged(ECombatState NewState, ECombatState OldState);
};
