// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Nightmares/ShadowSlaveNightmareObjectiveTypes.h"
#include "ShadowSlaveNightmareObjectiveTracker.generated.h"

class UShadowSlaveNightmareScenarioDefinition;

/**
 * Authoritative runtime tracker for Nightmare scenario objectives.
 * Manages mutable objective progress, state transitions, and completion evaluation.
 *
 * NOTE ON TICK & EVENT ARCHITECTURE:
 * This tracker does NOT tick. Objective state changes and progress updates are strictly
 * event-driven and broadcast through dynamic multicast delegates.
 */
UCLASS(BlueprintType)
class SHADOWSLAVE_API UShadowSlaveNightmareObjectiveTracker : public UObject
{
	GENERATED_BODY()

public:
	UShadowSlaveNightmareObjectiveTracker();

	/** Initializes tracker from a scenario definition, populating runtime states */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Nightmare|Objectives")
	void InitializeObjectives(const UShadowSlaveNightmareScenarioDefinition* InScenarioDef);

	/** Clears and resets all tracked objective states */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Nightmare|Objectives")
	void ResetObjectives();

	/** Transitions an objective to Active state */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Nightmare|Objectives")
	bool ActivateObjective(FName ObjectiveId);

	/** Sets explicit progress on an objective, automatically completing it if TargetProgress is met */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Nightmare|Objectives")
	bool UpdateObjectiveProgress(FName ObjectiveId, float NewProgress);

	/** Accumulates progress on an objective by DeltaProgress */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Nightmare|Objectives")
	bool AddObjectiveProgress(FName ObjectiveId, float DeltaProgress);

	/** Explicitly transitions an objective to Completed state */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Nightmare|Objectives")
	bool CompleteObjective(FName ObjectiveId);

	/** Explicitly transitions an objective to Failed state */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Nightmare|Objectives")
	bool FailObjective(FName ObjectiveId);

	/* --- Queries --- */

	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Nightmare|Objectives")
	EShadowSlaveObjectiveState GetObjectiveState(FName ObjectiveId) const;

	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Nightmare|Objectives")
	bool GetObjectiveRuntimeState(FName ObjectiveId, FShadowSlaveNightmareObjectiveRuntimeState& OutState) const;

	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Nightmare|Objectives")
	TArray<FShadowSlaveNightmareObjectiveRuntimeState> GetAllObjectiveRuntimeStates() const;

	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Nightmare|Objectives")
	bool AreRequiredObjectivesComplete() const;

	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Nightmare|Objectives")
	bool HasAnyRequiredObjectiveFailed() const;

	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Nightmare|Objectives")
	bool AreAllObjectivesComplete() const;

	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Nightmare|Objectives")
	bool HasAnyObjectiveFailed() const;

	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Nightmare|Objectives")
	int32 GetCompletedRequiredObjectiveCount() const;

	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Nightmare|Objectives")
	int32 GetTotalRequiredObjectiveCount() const;

	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Nightmare|Objectives")
	const UShadowSlaveNightmareScenarioDefinition* GetBoundScenarioDefinition() const;

	/* --- Delegates --- */

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Nightmare|Objectives")
	FOnObjectiveStateChangedSignature OnObjectiveStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Nightmare|Objectives")
	FOnObjectiveProgressChangedSignature OnObjectiveProgressChanged;

protected:
	/** Internal transition helper */
	bool TransitionObjectiveState(FName ObjectiveId, EShadowSlaveObjectiveState TargetState);

	/** Authoritative map of objective ID to runtime progress state */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Nightmare|Objectives")
	TMap<FName, FShadowSlaveNightmareObjectiveRuntimeState> ObjectiveRuntimeStates;

	/** Weak reference to originating scenario definition */
	TWeakObjectPtr<const UShadowSlaveNightmareScenarioDefinition> BoundScenarioDefinition;
};
