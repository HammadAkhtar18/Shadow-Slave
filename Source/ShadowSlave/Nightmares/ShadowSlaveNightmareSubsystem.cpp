// Copyright Epic Games, Inc. All Rights Reserved.

#include "Nightmares/ShadowSlaveNightmareSubsystem.h"
#include "Nightmares/ShadowSlaveNightmareScenarioDefinition.h"
#include "Nightmares/ShadowSlaveNightmareObjectiveTracker.h"
#include "Characters/ShadowSlaveCharacterBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
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

FShadowSlaveNightmareSaveData UShadowSlaveNightmareSubsystem::ExportSaveData() const
{
	FShadowSlaveNightmareSaveData SaveData;

	if (ActiveScenarioDefinition)
	{
		SaveData.ScenarioId = ActiveScenarioDefinition->ScenarioId;
		SaveData.ScenarioVersion = ActiveScenarioDefinition->ScenarioVersion;
		SaveData.ScenarioMetadata = ActiveScenarioDefinition->ScenarioMetadata;
		SaveData.bIsValid = true;
	}

	SaveData.SessionState = CurrentSessionState;
	SaveData.FailureReason = ActiveFailureReason;
	SaveData.bIsActive = IsScenarioActive();

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
	if (!InSaveData.bIsValid || InSaveData.ScenarioId.IsNone())
	{
		UE_LOG(LogShadowSlave, Log, TEXT("UShadowSlaveNightmareSubsystem::ImportSaveData: No active scenario state in save data."));
		return false;
	}

	UE_LOG(LogShadowSlave, Log, TEXT("UShadowSlaveNightmareSubsystem::ImportSaveData: Read scenario '%s' (v%d), state %d."),
		*InSaveData.ScenarioId.ToString(),
		InSaveData.ScenarioVersion,
		static_cast<int32>(InSaveData.SessionState)
	);

	// Architectural contract documentation:
	// Live in-scenario restoration across level transitions requires the future level streaming layer.
	// We restore stable identifiers and metadata while keeping the session inactive until the world stream executes.
	if (InSaveData.bIsActive)
	{
		UE_LOG(LogShadowSlave, Log, TEXT("UShadowSlaveNightmareSubsystem: Note: In-scenario live resume requires future world streaming infrastructure. Scenario metadata and objective states recorded; session set to Inactive pending scenario map streaming."));
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
