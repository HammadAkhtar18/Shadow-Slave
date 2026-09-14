// Copyright Epic Games, Inc. All Rights Reserved.

#include "Nightmares/ShadowSlaveNightmareSubsystem.h"
#include "Nightmares/ShadowSlaveNightmareScenarioDefinition.h"
#include "Nightmares/ShadowSlaveNightmareObjectiveTracker.h"
#include "Characters/ShadowSlaveCharacterBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "Engine/AssetManager.h"
#include "ShadowSlave.h"

UShadowSlaveNightmareSubsystem::UShadowSlaveNightmareSubsystem()
{
	CurrentSessionState = EShadowSlaveNightmareSessionState::Inactive;
	ActiveFailureReason = EShadowSlaveScenarioFailureReason::None;
	ActiveScenarioDefinition = nullptr;
	ObjectiveTracker = nullptr;
}

void UShadowSlaveNightmareSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	ObjectiveTracker = NewObject<UShadowSlaveNightmareObjectiveTracker>(this);
	ObjectiveTracker->OnObjectiveStateChanged.AddDynamic(this, &UShadowSlaveNightmareSubsystem::HandleObjectiveStateChanged);

	UE_LOG(LogShadowSlave, Log, TEXT("UShadowSlaveNightmareSubsystem initialized."));
}

void UShadowSlaveNightmareSubsystem::Deinitialize()
{
	CleanupSession();

	if (ObjectiveTracker)
	{
		ObjectiveTracker->OnObjectiveStateChanged.RemoveDynamic(this, &UShadowSlaveNightmareSubsystem::HandleObjectiveStateChanged);
		ObjectiveTracker = nullptr;
	}

	Super::Deinitialize();
}

bool UShadowSlaveNightmareSubsystem::CanStartScenario(const UShadowSlaveNightmareScenarioDefinition* ScenarioDef, FText& OutFailureReason) const
{
	if (CurrentSessionState != EShadowSlaveNightmareSessionState::Inactive)
	{
		OutFailureReason = FText::FromString(TEXT("Cannot start scenario: A Nightmare session is already running, preparing, or exiting."));
		return false;
	}

	if (!ScenarioDef)
	{
		OutFailureReason = FText::FromString(TEXT("Cannot start scenario: Scenario definition is null."));
		return false;
	}

	if (!ScenarioDef->ValidateScenario(OutFailureReason))
	{
		return false;
	}

	OutFailureReason = FText::GetEmpty();
	return true;
}

bool UShadowSlaveNightmareSubsystem::StartScenario(UShadowSlaveNightmareScenarioDefinition* ScenarioDef, APlayerController* InParticipatingController)
{
	FText ValidationFailure;
	if (!CanStartScenario(ScenarioDef, ValidationFailure))
	{
		UE_LOG(LogShadowSlave, Error, TEXT("UShadowSlaveNightmareSubsystem::StartScenario rejected: %s"), *ValidationFailure.ToString());
		return false;
	}

	if (!SetSessionState(EShadowSlaveNightmareSessionState::Preparing))
	{
		return false;
	}

	ActiveScenarioDefinition = ScenarioDef;
	ActiveFailureReason = EShadowSlaveScenarioFailureReason::None;

	if (ObjectiveTracker)
	{
		ObjectiveTracker->InitializeObjectives(ScenarioDef);
	}

	// Resolve participating player context
	if (InParticipatingController)
	{
		SetParticipatingPlayer(InParticipatingController);
	}
	else if (UWorld* World = GetWorld())
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			SetParticipatingPlayer(PC);
		}
	}

	if (!SetSessionState(EShadowSlaveNightmareSessionState::Active))
	{
		return false;
	}

	UE_LOG(LogShadowSlave, Log, TEXT("UShadowSlaveNightmareSubsystem: Scenario '%s' (v%d) is now ACTIVE."),
		*ScenarioDef->ScenarioId.ToString(),
		ScenarioDef->ScenarioVersion
	);

	OnScenarioStarted.Broadcast(ActiveScenarioDefinition);
	return true;
}

bool UShadowSlaveNightmareSubsystem::PauseScenario()
{
	if (CurrentSessionState != EShadowSlaveNightmareSessionState::Active)
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveNightmareSubsystem: Cannot pause scenario when not in Active state (Current: %d)."),
			static_cast<int32>(CurrentSessionState)
		);
		return false;
	}

	return SetSessionState(EShadowSlaveNightmareSessionState::Paused);
}

bool UShadowSlaveNightmareSubsystem::ResumeScenario()
{
	if (CurrentSessionState != EShadowSlaveNightmareSessionState::Paused)
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveNightmareSubsystem: Cannot resume scenario when not in Paused state (Current: %d)."),
			static_cast<int32>(CurrentSessionState)
		);
		return false;
	}

	return SetSessionState(EShadowSlaveNightmareSessionState::Active);
}

bool UShadowSlaveNightmareSubsystem::CompleteScenario()
{
	if (CurrentSessionState != EShadowSlaveNightmareSessionState::Active &&
		CurrentSessionState != EShadowSlaveNightmareSessionState::Paused)
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveNightmareSubsystem: Cannot complete scenario from state %d."),
			static_cast<int32>(CurrentSessionState)
		);
		return false;
	}

	if (!SetSessionState(EShadowSlaveNightmareSessionState::Completed))
	{
		return false;
	}

	UE_LOG(LogShadowSlave, Log, TEXT("UShadowSlaveNightmareSubsystem: Scenario '%s' COMPLETED."),
		ActiveScenarioDefinition ? *ActiveScenarioDefinition->ScenarioId.ToString() : TEXT("Unknown")
	);

	OnScenarioCompleted.Broadcast(ActiveScenarioDefinition);
	return true;
}

bool UShadowSlaveNightmareSubsystem::FailScenario(EShadowSlaveScenarioFailureReason Reason)
{
	if (CurrentSessionState != EShadowSlaveNightmareSessionState::Active &&
		CurrentSessionState != EShadowSlaveNightmareSessionState::Preparing &&
		CurrentSessionState != EShadowSlaveNightmareSessionState::Paused)
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveNightmareSubsystem: Cannot fail scenario from state %d."),
			static_cast<int32>(CurrentSessionState)
		);
		return false;
	}

	ActiveFailureReason = Reason;

	if (!SetSessionState(EShadowSlaveNightmareSessionState::Failed))
	{
		return false;
	}

	UE_LOG(LogShadowSlave, Log, TEXT("UShadowSlaveNightmareSubsystem: Scenario '%s' FAILED (Reason: %d)."),
		ActiveScenarioDefinition ? *ActiveScenarioDefinition->ScenarioId.ToString() : TEXT("Unknown"),
		static_cast<int32>(Reason)
	);

	OnScenarioFailed.Broadcast(ActiveScenarioDefinition, Reason);
	return true;
}

bool UShadowSlaveNightmareSubsystem::AbortScenario()
{
	if (CurrentSessionState == EShadowSlaveNightmareSessionState::Inactive ||
		CurrentSessionState == EShadowSlaveNightmareSessionState::Completed ||
		CurrentSessionState == EShadowSlaveNightmareSessionState::Failed ||
		CurrentSessionState == EShadowSlaveNightmareSessionState::Exiting)
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveNightmareSubsystem: Cannot abort scenario from state %d."),
			static_cast<int32>(CurrentSessionState)
		);
		return false;
	}

	ActiveFailureReason = EShadowSlaveScenarioFailureReason::Aborted;

	if (!SetSessionState(EShadowSlaveNightmareSessionState::Aborted))
	{
		return false;
	}

	UE_LOG(LogShadowSlave, Log, TEXT("UShadowSlaveNightmareSubsystem: Scenario '%s' ABORTED."),
		ActiveScenarioDefinition ? *ActiveScenarioDefinition->ScenarioId.ToString() : TEXT("Unknown")
	);

	OnScenarioAborted.Broadcast(ActiveScenarioDefinition);
	return true;
}

bool UShadowSlaveNightmareSubsystem::CanExitScenario() const
{
	return CurrentSessionState == EShadowSlaveNightmareSessionState::Completed ||
		   CurrentSessionState == EShadowSlaveNightmareSessionState::Failed ||
		   CurrentSessionState == EShadowSlaveNightmareSessionState::Aborted;
}

bool UShadowSlaveNightmareSubsystem::BeginExitScenario()
{
	if (!CanExitScenario())
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveNightmareSubsystem: Cannot begin exit from state %d (must be Completed, Failed, or Aborted)."),
			static_cast<int32>(CurrentSessionState)
		);
		return false;
	}

	if (!SetSessionState(EShadowSlaveNightmareSessionState::Exiting))
	{
		return false;
	}

	UE_LOG(LogShadowSlave, Log, TEXT("UShadowSlaveNightmareSubsystem: Scenario '%s' beginning exit sequence."),
		ActiveScenarioDefinition ? *ActiveScenarioDefinition->ScenarioId.ToString() : TEXT("Unknown")
	);

	OnScenarioExiting.Broadcast(ActiveScenarioDefinition);
	return true;
}

bool UShadowSlaveNightmareSubsystem::FinishExitScenario()
{
	if (CurrentSessionState != EShadowSlaveNightmareSessionState::Exiting)
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveNightmareSubsystem: Cannot finish exit from state %d (must be in Exiting state)."),
			static_cast<int32>(CurrentSessionState)
		);
		return false;
	}

	UShadowSlaveNightmareScenarioDefinition* FinishedDef = ActiveScenarioDefinition;

	CleanupSession();
	SetSessionState(EShadowSlaveNightmareSessionState::Inactive);

	UE_LOG(LogShadowSlave, Log, TEXT("UShadowSlaveNightmareSubsystem: Scenario exit finalized; session restored to Inactive."));

	OnScenarioExited.Broadcast(FinishedDef);
	return true;
}

FName UShadowSlaveNightmareSubsystem::GetCurrentScenarioId() const
{
	return ActiveScenarioDefinition ? ActiveScenarioDefinition->ScenarioId : NAME_None;
}

int32 UShadowSlaveNightmareSubsystem::GetCurrentScenarioVersion() const
{
	return ActiveScenarioDefinition ? ActiveScenarioDefinition->ScenarioVersion : 0;
}

void UShadowSlaveNightmareSubsystem::SetParticipatingPlayer(APlayerController* InPC, APawn* InPawn)
{
	UnbindPlayerDelegates();

	ParticipatingController = InPC;
	ParticipatingPawn = InPawn ? InPawn : (InPC ? InPC->GetPawn() : nullptr);
	ParticipatingCharacter = Cast<AShadowSlaveCharacterBase>(ParticipatingPawn.Get());

	BindPlayerDelegates();
}

TSoftObjectPtr<UWorld> UShadowSlaveNightmareSubsystem::GetCurrentScenarioMapAsset() const
{
	return ActiveScenarioDefinition ? ActiveScenarioDefinition->ScenarioMap : nullptr;
}

bool UShadowSlaveNightmareSubsystem::RequestScenarioWorldTransition()
{
	if (!ActiveScenarioDefinition)
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveNightmareSubsystem: Cannot request world transition: No active scenario definition."));
		return false;
	}

	if (ActiveScenarioDefinition->ScenarioMap.IsNull())
	{
		UE_LOG(LogShadowSlave, Log, TEXT("UShadowSlaveNightmareSubsystem: Scenario '%s' defines no world map; executing within current world context."),
			*ActiveScenarioDefinition->ScenarioId.ToString()
		);
		return false;
	}

	UE_LOG(LogShadowSlave, Log, TEXT("UShadowSlaveNightmareSubsystem: World transition requested for scenario map '%s'. Boundary ready for future level streaming integration."),
		*ActiveScenarioDefinition->ScenarioMap.ToString()
	);

	return true;
}

void UShadowSlaveNightmareSubsystem::ClearPendingScenarioRestore()
{
	PendingRestoreState.Clear();
}

bool UShadowSlaveNightmareSubsystem::ResumePendingScenario(APlayerController* InParticipatingController)
{
	if (!PendingRestoreState.bHasPendingRestore)
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveNightmareSubsystem::ResumePendingScenario called, but no scenario restoration is pending."));
		return false;
	}

	if (!ActiveScenarioDefinition)
	{
		UE_LOG(LogShadowSlave, Error, TEXT("UShadowSlaveNightmareSubsystem::ResumePendingScenario: Pending scenario has no active definition."));
		return false;
	}

	// Resolve participating player context
	if (InParticipatingController)
	{
		SetParticipatingPlayer(InParticipatingController);
	}
	else if (UWorld* World = GetWorld())
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			SetParticipatingPlayer(PC);
		}
	}

	// Determine resume target state
	EShadowSlaveNightmareSessionState TargetState = PendingRestoreState.SavedSessionState;
	if (TargetState != EShadowSlaveNightmareSessionState::Active &&
		TargetState != EShadowSlaveNightmareSessionState::Paused &&
		TargetState != EShadowSlaveNightmareSessionState::Preparing)
	{
		TargetState = EShadowSlaveNightmareSessionState::Active;
	}

	const EShadowSlaveNightmareSessionState OldState = CurrentSessionState;
	CurrentSessionState = TargetState;
	ActiveFailureReason = PendingRestoreState.SavedFailureReason;

	const FName ResumedScenarioId = PendingRestoreState.ScenarioId;
	PendingRestoreState.Clear();

	UE_LOG(LogShadowSlave, Log, TEXT("UShadowSlaveNightmareSubsystem: Resumed pending scenario '%s' into state %d."),
		*ResumedScenarioId.ToString(),
		static_cast<int32>(CurrentSessionState)
	);

	OnNightmareStateChanged.Broadcast(CurrentSessionState, OldState);
	OnScenarioStarted.Broadcast(ActiveScenarioDefinition);
	return true;
}

UShadowSlaveNightmareScenarioDefinition* UShadowSlaveNightmareSubsystem::ResolveScenarioDefinition(FName ScenarioId) const
{
	if (ScenarioId.IsNone())
	{
		return nullptr;
	}

	const FPrimaryAssetId PrimaryAssetId(TEXT("NightmareScenario"), ScenarioId);

	// 1. Attempt Asset Manager resolution if initialized
	if (UAssetManager::IsInitialized())
	{
		if (UObject* AssetObj = UAssetManager::Get().GetPrimaryAssetObject(PrimaryAssetId))
		{
			if (UShadowSlaveNightmareScenarioDefinition* Def = Cast<UShadowSlaveNightmareScenarioDefinition>(AssetObj))
			{
				return Def;
			}
		}

		const FSoftObjectPath AssetPath = UAssetManager::Get().GetPrimaryAssetPath(PrimaryAssetId);
		if (AssetPath.IsValid())
		{
			if (UObject* Loaded = AssetPath.TryLoad())
			{
				if (UShadowSlaveNightmareScenarioDefinition* Def = Cast<UShadowSlaveNightmareScenarioDefinition>(Loaded))
				{
					return Def;
				}
			}
		}
	}

	// 2. Find loaded object in memory
	if (UShadowSlaveNightmareScenarioDefinition* Found = FindObject<UShadowSlaveNightmareScenarioDefinition>(ANY_PACKAGE, *ScenarioId.ToString()))
	{
		return Found;
	}

	// 3. Fallback for development test scenario
	if (ScenarioId == FName(TEXT("Scenario_Dev_Test")))
	{
		return UShadowSlaveNightmareScenarioDefinition::CreateTestScenarioDefinition(const_cast<UShadowSlaveNightmareSubsystem*>(this));
	}

	return nullptr;
}

FShadowSlaveNightmareSaveData UShadowSlaveNightmareSubsystem::ExportSaveData() const
{
	FShadowSlaveNightmareSaveData SaveData;

	if (PendingRestoreState.bHasPendingRestore)
	{
		SaveData.ScenarioId = PendingRestoreState.ScenarioId;
		SaveData.ScenarioVersion = PendingRestoreState.ScenarioVersion;
		SaveData.SessionState = PendingRestoreState.SavedSessionState;
		SaveData.FailureReason = PendingRestoreState.SavedFailureReason;
		SaveData.ScenarioMetadata = PendingRestoreState.SavedScenarioMetadata;
		SaveData.bIsActive = (PendingRestoreState.SavedSessionState == EShadowSlaveNightmareSessionState::Active ||
							  PendingRestoreState.SavedSessionState == EShadowSlaveNightmareSessionState::Paused);
		SaveData.bIsValid = true;
	}
	else if (ActiveScenarioDefinition)
	{
		SaveData.ScenarioId = ActiveScenarioDefinition->ScenarioId;
		SaveData.ScenarioVersion = ActiveScenarioDefinition->ScenarioVersion;
		SaveData.ScenarioMetadata = ActiveScenarioDefinition->ScenarioMetadata;
		SaveData.SessionState = CurrentSessionState;
		SaveData.FailureReason = ActiveFailureReason;
		SaveData.bIsActive = IsScenarioActive();
		SaveData.bIsValid = true;
	}
	else if (CurrentSessionState != EShadowSlaveNightmareSessionState::Inactive)
	{
		SaveData.SessionState = CurrentSessionState;
		SaveData.FailureReason = ActiveFailureReason;
		SaveData.bIsActive = IsScenarioActive();
		SaveData.bIsValid = true;
	}

	if (ObjectiveTracker)
	{
		for (const FShadowSlaveNightmareObjectiveRuntimeState& RuntimeState : ObjectiveTracker->GetAllObjectiveRuntimeStates())
		{
			SaveData.Objectives.Add(FShadowSlaveNightmareObjectiveSaveData(RuntimeState));
		}
	}

	return SaveData;
}

bool UShadowSlaveNightmareSubsystem::ImportSaveData(const FShadowSlaveNightmareSaveData& InSaveData)
{
	// 1. Reset/replace previous Nightmare runtime state safely before importing (Repeated Load Safety)
	CleanupSession();
	PendingRestoreState.Clear();

	if (!InSaveData.bIsValid)
	{
		UE_LOG(LogShadowSlave, Log, TEXT("UShadowSlaveNightmareSubsystem::ImportSaveData: Save data has bIsValid=false. Subsystem remains Inactive."));
		return false;
	}

	if (InSaveData.ScenarioId.IsNone())
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveNightmareSubsystem::ImportSaveData: Missing ScenarioId in save data. Rejecting corrupted scenario save."));
		return false;
	}

	// 2. Resolve Scenario Definition
	UShadowSlaveNightmareScenarioDefinition* ResolvedDef = ResolveScenarioDefinition(InSaveData.ScenarioId);
	if (!ResolvedDef)
	{
		UE_LOG(LogShadowSlave, Error, TEXT("UShadowSlaveNightmareSubsystem::ImportSaveData: Failed to resolve scenario definition for ScenarioId '%s'. Rejecting save."),
			*InSaveData.ScenarioId.ToString()
		);
		return false;
	}

	// 3. Validate Scenario Version
	if (InSaveData.ScenarioVersion != ResolvedDef->ScenarioVersion)
	{
		UE_LOG(LogShadowSlave, Error, TEXT("UShadowSlaveNightmareSubsystem::ImportSaveData: Incompatible scenario version %d for '%s' (definition version is %d). Rejecting save."),
			InSaveData.ScenarioVersion,
			*InSaveData.ScenarioId.ToString(),
			ResolvedDef->ScenarioVersion
		);
		return false;
	}

	// 4. Validate resolved Scenario Definition integrity
	FText DefValidationError;
	if (!ResolvedDef->ValidateScenario(DefValidationError))
	{
		UE_LOG(LogShadowSlave, Error, TEXT("UShadowSlaveNightmareSubsystem::ImportSaveData: Resolved scenario definition '%s' failed validation: %s"),
			*InSaveData.ScenarioId.ToString(),
			*DefValidationError.ToString()
		);
		return false;
	}

	// 5. Restore scenario identity
	ActiveScenarioDefinition = ResolvedDef;

	// 6. Initialize Objective Tracker from definition
	if (!ObjectiveTracker)
	{
		ObjectiveTracker = NewObject<UShadowSlaveNightmareObjectiveTracker>(this);
		ObjectiveTracker->OnObjectiveStateChanged.AddDynamic(this, &UShadowSlaveNightmareSubsystem::HandleObjectiveStateChanged);
	}
	ObjectiveTracker->InitializeObjectives(ResolvedDef);

	// 7. Restore Objective Runtime States
	const bool bObjectivesRestored = ObjectiveTracker->RestoreObjectiveRuntimeStates(InSaveData.Objectives);
	if (!bObjectivesRestored)
	{
		UE_LOG(LogShadowSlave, Error, TEXT("UShadowSlaveNightmareSubsystem::ImportSaveData: Failed to restore objective states for scenario '%s'."),
			*InSaveData.ScenarioId.ToString()
		);
		CleanupSession();
		return false;
	}

	// 8. Determine safe restoration state based on saved session lifecycle state
	ActiveFailureReason = InSaveData.FailureReason;

	if (InSaveData.SessionState == EShadowSlaveNightmareSessionState::Completed ||
		InSaveData.SessionState == EShadowSlaveNightmareSessionState::Failed ||
		InSaveData.SessionState == EShadowSlaveNightmareSessionState::Aborted)
	{
		// Historical state restoration: restore state directly without re-triggering completion/failure events
		CurrentSessionState = InSaveData.SessionState;
		PendingRestoreState.Clear();

		UE_LOG(LogShadowSlave, Log, TEXT("UShadowSlaveNightmareSubsystem::ImportSaveData: Restored historical scenario state %d for '%s' (FailureReason: %d). No duplicate events fired."),
			static_cast<int32>(CurrentSessionState),
			*InSaveData.ScenarioId.ToString(),
			static_cast<int32>(ActiveFailureReason)
		);
	}
	else if (InSaveData.SessionState == EShadowSlaveNightmareSessionState::Active ||
			 InSaveData.SessionState == EShadowSlaveNightmareSessionState::Preparing ||
			 InSaveData.SessionState == EShadowSlaveNightmareSessionState::Paused)
	{
		// Active-scenario restoration boundary:
		// Do NOT set subsystem to Active before world streaming occurs.
		// Maintain a safe pending-restoration representation; subsystem remains Inactive.
		CurrentSessionState = EShadowSlaveNightmareSessionState::Inactive;

		PendingRestoreState.ScenarioId = InSaveData.ScenarioId;
		PendingRestoreState.ScenarioVersion = InSaveData.ScenarioVersion;
		PendingRestoreState.SavedSessionState = InSaveData.SessionState;
		PendingRestoreState.SavedFailureReason = InSaveData.FailureReason;
		PendingRestoreState.SavedScenarioMetadata = InSaveData.ScenarioMetadata;
		PendingRestoreState.bHasPendingRestore = true;

		UE_LOG(LogShadowSlave, Log, TEXT("UShadowSlaveNightmareSubsystem::ImportSaveData: Restored scenario definition and objective states for '%s'. Subsystem remains Inactive with PendingScenarioRestore armed awaiting world streaming."),
			*InSaveData.ScenarioId.ToString()
		);
	}
	else
	{
		// Inactive or Unknown
		CurrentSessionState = EShadowSlaveNightmareSessionState::Inactive;
		PendingRestoreState.Clear();
	}

	return true;
}

bool UShadowSlaveNightmareSubsystem::CanTransitionToState(EShadowSlaveNightmareSessionState TargetState) const
{
	if (CurrentSessionState == TargetState)
	{
		return true;
	}

	switch (CurrentSessionState)
	{
	case EShadowSlaveNightmareSessionState::Inactive:
		return TargetState == EShadowSlaveNightmareSessionState::Preparing;

	case EShadowSlaveNightmareSessionState::Preparing:
		return TargetState == EShadowSlaveNightmareSessionState::Active ||
			   TargetState == EShadowSlaveNightmareSessionState::Failed ||
			   TargetState == EShadowSlaveNightmareSessionState::Aborted;

	case EShadowSlaveNightmareSessionState::Active:
		return TargetState == EShadowSlaveNightmareSessionState::Paused ||
			   TargetState == EShadowSlaveNightmareSessionState::Completed ||
			   TargetState == EShadowSlaveNightmareSessionState::Failed ||
			   TargetState == EShadowSlaveNightmareSessionState::Aborted;

	case EShadowSlaveNightmareSessionState::Paused:
		return TargetState == EShadowSlaveNightmareSessionState::Active ||
			   TargetState == EShadowSlaveNightmareSessionState::Aborted;

	case EShadowSlaveNightmareSessionState::Completed:
	case EShadowSlaveNightmareSessionState::Failed:
	case EShadowSlaveNightmareSessionState::Aborted:
		return TargetState == EShadowSlaveNightmareSessionState::Exiting;

	case EShadowSlaveNightmareSessionState::Exiting:
		return TargetState == EShadowSlaveNightmareSessionState::Inactive;

	default:
		return false;
	}
}

bool UShadowSlaveNightmareSubsystem::SetSessionState(EShadowSlaveNightmareSessionState NewState)
{
	if (!CanTransitionToState(NewState))
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveNightmareSubsystem: Rejected illegal state transition from %d to %d."),
			static_cast<int32>(CurrentSessionState),
			static_cast<int32>(NewState)
		);
		return false;
	}

	const EShadowSlaveNightmareSessionState OldState = CurrentSessionState;
	CurrentSessionState = NewState;

	UE_LOG(LogShadowSlave, Log, TEXT("UShadowSlaveNightmareSubsystem: State transitioned from %d to %d."),
		static_cast<int32>(OldState),
		static_cast<int32>(CurrentSessionState)
	);

	OnNightmareStateChanged.Broadcast(CurrentSessionState, OldState);
	return true;
}

void UShadowSlaveNightmareSubsystem::EvaluateScenarioCompletion()
{
	if (CurrentSessionState != EShadowSlaveNightmareSessionState::Active)
	{
		return;
	}

	if (!ActiveScenarioDefinition || !ObjectiveTracker)
	{
		return;
	}

	// 1. Check for immediate failure: Has any required objective failed?
	if (ObjectiveTracker->HasAnyRequiredObjectiveFailed())
	{
		FailScenario(EShadowSlaveScenarioFailureReason::ObjectiveFailure);
		return;
	}

	// 2. Check for completion rule satisfaction
	bool bCompleted = false;

	switch (ActiveScenarioDefinition->CompletionRule)
	{
	case EShadowSlaveScenarioCompletionRule::AllRequiredObjectives:
		bCompleted = ObjectiveTracker->AreRequiredObjectivesComplete();
		break;

	case EShadowSlaveScenarioCompletionRule::AnyRequiredObjective:
		bCompleted = (ObjectiveTracker->GetCompletedRequiredObjectiveCount() > 0);
		break;

	case EShadowSlaveScenarioCompletionRule::Custom:
		bCompleted = EvaluateCustomCompletionRule();
		break;
	}

	if (bCompleted)
	{
		CompleteScenario();
	}
}

bool UShadowSlaveNightmareSubsystem::EvaluateCustomCompletionRule() const
{
	// Default base implementation for custom rule; can be overridden in C++ subclasses
	return ObjectiveTracker ? ObjectiveTracker->AreAllObjectivesComplete() : false;
}

void UShadowSlaveNightmareSubsystem::HandleObjectiveStateChanged(FName ObjectiveId, EShadowSlaveObjectiveState NewState, EShadowSlaveObjectiveState OldState)
{
	OnObjectiveStateChanged.Broadcast(ObjectiveId, NewState, OldState);
	EvaluateScenarioCompletion();
}

void UShadowSlaveNightmareSubsystem::HandleCharacterDamaged(const FShadowSlaveDamageInfo& DamageInfo)
{
	if (IsScenarioActive() && ParticipatingCharacter.IsValid())
	{
		if (!ParticipatingCharacter->IsAlive())
		{
			UE_LOG(LogShadowSlave, Log, TEXT("UShadowSlaveNightmareSubsystem: Participating player character died during active scenario. Failing scenario."));
			FailScenario(EShadowSlaveScenarioFailureReason::PlayerDeath);
		}
	}
}

void UShadowSlaveNightmareSubsystem::BindPlayerDelegates()
{
	if (ParticipatingCharacter.IsValid())
	{
		ParticipatingCharacter->OnCharacterDamaged.AddDynamic(this, &UShadowSlaveNightmareSubsystem::HandleCharacterDamaged);
	}
}

void UShadowSlaveNightmareSubsystem::UnbindPlayerDelegates()
{
	if (ParticipatingCharacter.IsValid())
	{
		ParticipatingCharacter->OnCharacterDamaged.RemoveDynamic(this, &UShadowSlaveNightmareSubsystem::HandleCharacterDamaged);
	}

	ParticipatingCharacter.Reset();
	ParticipatingPawn.Reset();
	ParticipatingController.Reset();
}

void UShadowSlaveNightmareSubsystem::CleanupSession()
{
	UnbindPlayerDelegates();

	if (ObjectiveTracker)
	{
		ObjectiveTracker->ResetObjectives();
	}

	ActiveScenarioDefinition = nullptr;
	ActiveFailureReason = EShadowSlaveScenarioFailureReason::None;
}
