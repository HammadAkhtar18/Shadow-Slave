// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ShadowSlaveNightmareTypes.generated.h"

class UShadowSlaveNightmareScenarioDefinition;

/**
 * Technical runtime state for a Nightmare session.
 * NOTE: These are software implementation states and do NOT represent canon novel terminology.
 */
UENUM(BlueprintType)
enum class EShadowSlaveNightmareSessionState : uint8
{
	Unknown   UMETA(DisplayName = "Unknown"),
	Inactive  UMETA(DisplayName = "Inactive"),
	Preparing UMETA(DisplayName = "Preparing"),
	Active    UMETA(DisplayName = "Active"),
	Paused    UMETA(DisplayName = "Paused"),
	Completed UMETA(DisplayName = "Completed"),
	Failed    UMETA(DisplayName = "Failed"),
	Aborted   UMETA(DisplayName = "Aborted"),
	Exiting   UMETA(DisplayName = "Exiting")
};

/**
 * Generic evaluation rule for determining scenario completion.
 */
UENUM(BlueprintType)
enum class EShadowSlaveScenarioCompletionRule : uint8
{
	AllRequiredObjectives UMETA(DisplayName = "All Required Objectives"),
	AnyRequiredObjective  UMETA(DisplayName = "Any Required Objective"),
	Custom                UMETA(DisplayName = "Custom")
};

/**
 * Technical classification of scenario failure reasons.
 */
UENUM(BlueprintType)
enum class EShadowSlaveScenarioFailureReason : uint8
{
	None             UMETA(DisplayName = "None"),
	Unknown          UMETA(DisplayName = "Unknown"),
	PlayerDeath      UMETA(DisplayName = "Player Death"),
	ObjectiveFailure UMETA(DisplayName = "Objective Failure"),
	Timeout          UMETA(DisplayName = "Timeout"),
	ScenarioRule     UMETA(DisplayName = "Scenario Rule"),
	External         UMETA(DisplayName = "External"),
	Aborted          UMETA(DisplayName = "Aborted")
};

/**
 * Holds deferred restoration state for a saved Nightmare scenario that was Active/Paused/Preparing,
 * awaiting future level streaming / world transition infrastructure before becoming live.
 */
USTRUCT(BlueprintType)
struct SHADOWSLAVE_API FShadowSlavePendingNightmareRestore
{
	GENERATED_BODY()

	/** Programmatic identifier of the pending scenario */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Nightmare|Restore")
	FName ScenarioId = NAME_None;

	/** Schema version of the scenario at save time */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Nightmare|Restore")
	int32 ScenarioVersion = 1;

	/** Session state at save time (Active, Paused, Preparing) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Nightmare|Restore")
	EShadowSlaveNightmareSessionState SavedSessionState = EShadowSlaveNightmareSessionState::Inactive;

	/** Failure reason at save time */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Nightmare|Restore")
	EShadowSlaveScenarioFailureReason SavedFailureReason = EShadowSlaveScenarioFailureReason::None;

	/** Metadata saved with the scenario */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Nightmare|Restore")
	TMap<FName, FString> SavedScenarioMetadata;

	/** True if a scenario restoration is deferred awaiting world transition */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Nightmare|Restore")
	bool bHasPendingRestore = false;

	void Clear()
	{
		ScenarioId = NAME_None;
		ScenarioVersion = 1;
		SavedSessionState = EShadowSlaveNightmareSessionState::Inactive;
		SavedFailureReason = EShadowSlaveScenarioFailureReason::None;
		SavedScenarioMetadata.Empty();
		bHasPendingRestore = false;
	}
};

/* --- Nightmare Lifecycle Delegates --- */

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnNightmareStateChangedSignature, EShadowSlaveNightmareSessionState, NewState, EShadowSlaveNightmareSessionState, OldState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnScenarioStartedSignature, UShadowSlaveNightmareScenarioDefinition*, ScenarioDef);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnScenarioCompletedSignature, UShadowSlaveNightmareScenarioDefinition*, ScenarioDef);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnScenarioFailedSignature, UShadowSlaveNightmareScenarioDefinition*, ScenarioDef, EShadowSlaveScenarioFailureReason, Reason);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnScenarioAbortedSignature, UShadowSlaveNightmareScenarioDefinition*, ScenarioDef);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnScenarioExitingSignature, UShadowSlaveNightmareScenarioDefinition*, ScenarioDef);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnScenarioExitedSignature, UShadowSlaveNightmareScenarioDefinition*, ScenarioDef);
