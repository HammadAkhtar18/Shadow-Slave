// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "AI/ShadowSlaveAITypes.h"
#include "Perception/AIPerceptionTypes.h"
#include "Combat/ShadowSlaveCombatTypes.h"
#include "ShadowSlaveAIController.generated.h"

class UAIPerceptionComponent;
class UAISenseConfig_Sight;
class AShadowSlaveEnemyCharacterBase;

/**
 * AI Controller responsible for perception sensing, decision making, state transitions,
 * target management, and movement requests for Nightmare Creature enemies.
 * Follows the state flow:
 * Idle -> Investigating -> Chasing -> Attacking -> Recovering -> Returning/Searching -> Idle
 * Overridden unconditionally by Dead.
 */
UCLASS()
class SHADOWSLAVE_API AShadowSlaveAIController : public AAIController
{
	GENERATED_BODY()

public:
	AShadowSlaveAIController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	/* --- State & Target Accessors --- */

	/** Returns current AI state */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|AI")
	EShadowSlaveAIState GetAIState() const { return CurrentAIState; }

	/** Sets new AI state and triggers enter/exit hooks */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|AI")
	void SetAIState(EShadowSlaveAIState NewState);

	/** Returns current active target actor */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|AI")
	AActor* GetCurrentTarget() const { return CurrentTarget.Get(); }

	/** Sets new target actor */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|AI")
	void SetCurrentTarget(AActor* NewTarget);

	/** Returns spawn / home location */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|AI")
	const FVector& GetHomeLocation() const { return HomeLocation; }

	/** Sets home location */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|AI")
	void SetHomeLocation(const FVector& NewHomeLocation) { HomeLocation = NewHomeLocation; }

	/** Returns active AI configuration struct */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|AI|Config")
	const FShadowSlaveAIConfig& GetAIConfig() const { return AIConfig; }

	/** Sets AI configuration struct */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|AI|Config")
	void SetAIConfig(const FShadowSlaveAIConfig& NewConfig);

	/* --- External AI Commands / Hooks --- */

	/** Handles pawn receiving damage; provides an immediate stimulus to investigate or aggro */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|AI")
	virtual void HandleDamagedBy(AActor* Attacker, const FVector& HitLocation);

	/** Notifies controller that the controlled pawn has died; immediately transitions to Dead state */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|AI")
	virtual void OnPawnDeath();

	/** Allows runtime reconfiguration of perception parameters */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|AI|Perception")
	void ConfigurePerception(float InSightRadius, float InLoseSightRadius, float InPeripheralVisionAngle);

	/** Toggles debug visualization */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|AI|Debug")
	void SetAIDebugEnabled(bool bEnabled);

	/* --- Delegates --- */

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|AI")
	FOnAIStateChangedSignature OnAIStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|AI")
	FOnAITargetChangedSignature OnAITargetChanged;

protected:
	virtual void BeginPlay() override;

	/** Navigation movement completion handler */
	virtual void OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result) override;

	/* --- State Handlers --- */

	virtual void EnterState(EShadowSlaveAIState State);
	virtual void ExitState(EShadowSlaveAIState State);

	virtual void StartIdle();
	virtual void StartInvestigating(const FVector& TargetLocation);
	virtual void StartChasing(AActor* Target);
	virtual void StartAttacking();
	virtual void StartRecovering();
	virtual void StartSearching(const FVector& Location);
	virtual void StartReturning();
	virtual void HandleDeathState();

	/* --- Perception & Event Callbacks --- */

	UFUNCTION()
	virtual void HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	UFUNCTION()
	virtual void HandleCombatStateChanged(ECombatState OldState, ECombatState NewState);

	/* --- Timers and Loop Callbacks --- */

	virtual void OnChaseUpdate();
	virtual void OnRecoveryTimerExpired();
	virtual void OnInvestigationTimerExpired();
	virtual void OnSearchTimerExpired();

	/* --- Helper Methods --- */

	/** Checks whether a candidate target is valid, alive, and can be engaged */
	virtual bool IsTargetValidAndAlive(AActor* Target) const;

	/** Orients controlled pawn toward the current target */
	void FaceTarget(AActor* Target);

	/** Clears all active state timers */
	void ClearAllStateTimers();

	/* --- Debug --- */

	void DrawAIDebug();

protected:
	/* --- Components --- */

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|AI|Perception")
	TObjectPtr<UAIPerceptionComponent> PerceptionComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|AI|Perception")
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	/* --- Configuration --- */

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ShadowSlave|AI|Config")
	FShadowSlaveAIConfig AIConfig;

	/* --- Runtime State --- */

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|AI|State")
	EShadowSlaveAIState CurrentAIState = EShadowSlaveAIState::Idle;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|AI|State")
	TWeakObjectPtr<AActor> CurrentTarget;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|AI|State")
	FVector HomeLocation = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|AI|State")
	FVector LastKnownTargetLocation = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|AI|State")
	FVector InvestigationLocation = FVector::ZeroVector;

	/* --- Cached References --- */

	TWeakObjectPtr<AShadowSlaveEnemyCharacterBase> ControlledEnemyCharacter;

	/* --- Timer Handles --- */

	FTimerHandle ChaseTimerHandle;
	FTimerHandle RecoveryTimerHandle;
	FTimerHandle InvestigationTimerHandle;
	FTimerHandle SearchTimerHandle;
	FTimerHandle DebugTimerHandle;

	/* --- Debug Options --- */

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|AI|Debug")
	bool bShowAIDebug = false;
};
