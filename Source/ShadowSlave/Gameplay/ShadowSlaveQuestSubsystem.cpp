// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gameplay/ShadowSlaveQuestSubsystem.h"
#include "Story/ShadowSlaveStorySubsystem.h"
#include "Subsystems/SubsystemCollection.h"
#include "Engine/GameInstance.h"
#include "ShadowSlave.h"

UShadowSlaveQuestSubsystem::UShadowSlaveQuestSubsystem()
	: bIsRestoringState(false)
	, bIsProcessingTransition(false)
{
}

void UShadowSlaveQuestSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Collection.InitializeDependency<UShadowSlaveStorySubsystem>();
	Super::Initialize(Collection);

	RegisteredDefinitions.Empty();
	QuestRuntimeStates.Empty();
	bIsRestoringState = false;
	bIsProcessingTransition = false;
}

void UShadowSlaveQuestSubsystem::Deinitialize()
{
	RegisteredDefinitions.Empty();
	QuestRuntimeStates.Empty();
	Super::Deinitialize();
}

bool UShadowSlaveQuestSubsystem::RegisterQuestDefinition(UShadowSlaveQuestDefinition* QuestDef)
{
	if (!QuestDef)
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveQuestSubsystem::RegisterQuestDefinition - QuestDef is null."));
		return false;
	}

	if (QuestDef->QuestId.IsNone())
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveQuestSubsystem::RegisterQuestDefinition - QuestDef has invalid None QuestId."));
		return false;
	}

	// Required correction 1: Validate definitions before registration
	TArray<FText> ValidationErrors;
	if (!QuestDef->ValidateDefinition(ValidationErrors))
	{
		for (const FText& Err : ValidationErrors)
		{
			UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveQuestSubsystem::RegisterQuestDefinition - Validation failure on quest '%s': %s"),
				*QuestDef->QuestId.ToString(), *Err.ToString());
		}
		return false;
	}

	// Required correction 2: Reject QuestId conflicts
	if (const TObjectPtr<UShadowSlaveQuestDefinition>* ExistingDef = RegisteredDefinitions.Find(QuestDef->QuestId))
	{
		if (ExistingDef->Get() == QuestDef)
		{
			// Idempotent re-registration of the exact same definition object
			return true;
		}

		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveQuestSubsystem::RegisterQuestDefinition - Conflict: QuestId '%s' is already registered with a different definition object ('%s' vs '%s')."),
			*QuestDef->QuestId.ToString(),
			ExistingDef->Get() ? *ExistingDef->Get()->GetName() : TEXT("null"),
			*QuestDef->GetName());
		return false;
	}

	RegisteredDefinitions.Add(QuestDef->QuestId, QuestDef);

	// If runtime state does not exist yet, initialize it
	if (!QuestRuntimeStates.Contains(QuestDef->QuestId))
	{
		const EShadowSlaveQuestState InitialState = ArePrerequisitesSatisfied(QuestDef->QuestId)
			? EShadowSlaveQuestState::Available
			: EShadowSlaveQuestState::Locked;

		FShadowSlaveQuestRuntimeState NewEntry(QuestDef->QuestId, InitialState);
		InitializeRuntimeObjectives(NewEntry, QuestDef);
		QuestRuntimeStates.Add(QuestDef->QuestId, NewEntry);
	}

	return true;
}

bool UShadowSlaveQuestSubsystem::UnregisterQuestDefinition(FName QuestId)
{
	if (QuestId.IsNone())
	{
		return false;
	}

	return RegisteredDefinitions.Remove(QuestId) > 0;
}

UShadowSlaveQuestDefinition* UShadowSlaveQuestSubsystem::GetQuestDefinition(FName QuestId) const
{
	if (QuestId.IsNone())
	{
		return nullptr;
	}

	if (const TObjectPtr<UShadowSlaveQuestDefinition>* Found = RegisteredDefinitions.Find(QuestId))
	{
		return Found->Get();
	}

	return nullptr;
}

bool UShadowSlaveQuestSubsystem::HasQuestDefinition(FName QuestId) const
{
	return !QuestId.IsNone() && RegisteredDefinitions.Contains(QuestId);
}

EShadowSlaveQuestState UShadowSlaveQuestSubsystem::GetQuestState(FName QuestId) const
{
	if (QuestId.IsNone())
	{
		return EShadowSlaveQuestState::Unknown;
	}

	if (const FShadowSlaveQuestRuntimeState* Found = QuestRuntimeStates.Find(QuestId))
	{
		return Found->State;
	}

	return EShadowSlaveQuestState::Unknown;
}

bool UShadowSlaveQuestSubsystem::IsQuestActive(FName QuestId) const
{
	return GetQuestState(QuestId) == EShadowSlaveQuestState::Active;
}

bool UShadowSlaveQuestSubsystem::IsQuestCompleted(FName QuestId) const
{
	return GetQuestState(QuestId) == EShadowSlaveQuestState::Completed;
}

bool UShadowSlaveQuestSubsystem::IsQuestFailed(FName QuestId) const
{
	return GetQuestState(QuestId) == EShadowSlaveQuestState::Failed;
}

bool UShadowSlaveQuestSubsystem::IsQuestAbandoned(FName QuestId) const
{
	return GetQuestState(QuestId) == EShadowSlaveQuestState::Abandoned;
}

bool UShadowSlaveQuestSubsystem::ArePrerequisitesSatisfied(FName QuestId) const
{
	// Required correction 3: Prerequisite evaluation must fail closed
	if (QuestId.IsNone())
	{
		return false;
	}

	const UShadowSlaveQuestDefinition* Def = GetQuestDefinition(QuestId);
	if (!Def)
	{
		// Fail closed: missing QuestDefinition cannot be satisfied
		return false;
	}

	for (const FName& PrereqId : Def->PrerequisiteQuestIds)
	{
		if (PrereqId.IsNone())
		{
			// Fail closed: invalid or empty prerequisite identifier
			return false;
		}

		// Prerequisite quest definition must be registered
		if (!HasQuestDefinition(PrereqId))
		{
			return false;
		}

		// Prerequisite quest must explicitly be in Completed state
		if (GetQuestState(PrereqId) != EShadowSlaveQuestState::Completed)
		{
			return false;
		}
	}

	return true;
}

bool UShadowSlaveQuestSubsystem::GetQuestRuntimeState(FName QuestId, FShadowSlaveQuestRuntimeState& OutState) const
{
	if (QuestId.IsNone())
	{
		OutState = FShadowSlaveQuestRuntimeState();
		return false;
	}

	if (const FShadowSlaveQuestRuntimeState* Found = QuestRuntimeStates.Find(QuestId))
	{
		OutState = *Found;
		return true;
	}

	OutState = FShadowSlaveQuestRuntimeState();
	return false;
}

TArray<FName> UShadowSlaveQuestSubsystem::GetActiveQuestIds() const
{
	TArray<FName> ActiveIds;
	for (const auto& Pair : QuestRuntimeStates)
	{
		if (Pair.Value.State == EShadowSlaveQuestState::Active)
		{
			ActiveIds.Add(Pair.Key);
		}
	}
	return ActiveIds;
}

TArray<FName> UShadowSlaveQuestSubsystem::GetAllTrackedQuestIds() const
{
	TArray<FName> AllIds;
	QuestRuntimeStates.GetKeys(AllIds);
	return AllIds;
}

EShadowSlaveObjectiveState UShadowSlaveQuestSubsystem::GetObjectiveState(FName QuestId, FName ObjectiveId) const
{
	if (QuestId.IsNone() || ObjectiveId.IsNone())
	{
		return EShadowSlaveObjectiveState::Unknown;
	}

	if (const FShadowSlaveQuestRuntimeState* FoundQuest = QuestRuntimeStates.Find(QuestId))
	{
		if (const FShadowSlaveObjectiveRuntimeState* FoundObj = FoundQuest->ObjectiveStates.Find(ObjectiveId))
		{
			return FoundObj->State;
		}
	}

	return EShadowSlaveObjectiveState::Unknown;
}

int32 UShadowSlaveQuestSubsystem::GetObjectiveProgress(FName QuestId, FName ObjectiveId) const
{
	if (QuestId.IsNone() || ObjectiveId.IsNone())
	{
		return 0;
	}

	if (const FShadowSlaveQuestRuntimeState* FoundQuest = QuestRuntimeStates.Find(QuestId))
	{
		if (const FShadowSlaveObjectiveRuntimeState* FoundObj = FoundQuest->ObjectiveStates.Find(ObjectiveId))
		{
			return FoundObj->CurrentQuantity;
		}
	}

	return 0;
}

int32 UShadowSlaveQuestSubsystem::GetObjectiveRequiredQuantity(FName QuestId, FName ObjectiveId) const
{
	if (QuestId.IsNone() || ObjectiveId.IsNone())
	{
		return 1;
	}

	if (const FShadowSlaveQuestRuntimeState* FoundQuest = QuestRuntimeStates.Find(QuestId))
	{
		if (const FShadowSlaveObjectiveRuntimeState* FoundObj = FoundQuest->ObjectiveStates.Find(ObjectiveId))
		{
			return FoundObj->RequiredQuantity;
		}
	}

	return 1;
}

bool UShadowSlaveQuestSubsystem::GetObjectiveRuntimeState(FName QuestId, FName ObjectiveId, FShadowSlaveObjectiveRuntimeState& OutState) const
{
	if (QuestId.IsNone() || ObjectiveId.IsNone())
	{
		OutState = FShadowSlaveObjectiveRuntimeState();
		return false;
	}

	if (const FShadowSlaveQuestRuntimeState* FoundQuest = QuestRuntimeStates.Find(QuestId))
	{
		if (const FShadowSlaveObjectiveRuntimeState* FoundObj = FoundQuest->ObjectiveStates.Find(ObjectiveId))
		{
			OutState = *FoundObj;
			return true;
		}
	}

	OutState = FShadowSlaveObjectiveRuntimeState();
	return false;
}

bool UShadowSlaveQuestSubsystem::IsObjectiveActive(FName QuestId, FName ObjectiveId) const
{
	return GetObjectiveState(QuestId, ObjectiveId) == EShadowSlaveObjectiveState::Active;
}

bool UShadowSlaveQuestSubsystem::IsObjectiveCompleted(FName QuestId, FName ObjectiveId) const
{
	return GetObjectiveState(QuestId, ObjectiveId) == EShadowSlaveObjectiveState::Completed;
}

FText UShadowSlaveQuestSubsystem::GetQuestTitle(FName QuestId) const
{
	if (const UShadowSlaveQuestDefinition* Def = GetQuestDefinition(QuestId))
	{
		return Def->DisplayName;
	}
	return FText::FromName(QuestId);
}

FText UShadowSlaveQuestSubsystem::GetQuestDescription(FName QuestId) const
{
	if (const UShadowSlaveQuestDefinition* Def = GetQuestDefinition(QuestId))
	{
		return Def->Description;
	}
	return FText::GetEmpty();
}

FText UShadowSlaveQuestSubsystem::GetObjectiveTitle(FName QuestId, FName ObjectiveId) const
{
	if (const UShadowSlaveQuestDefinition* Def = GetQuestDefinition(QuestId))
	{
		if (const FShadowSlaveObjectiveDefinition* ObjDef = Def->FindObjective(ObjectiveId))
		{
			return ObjDef->DisplayName;
		}
	}
	return FText::FromName(ObjectiveId);
}

FText UShadowSlaveQuestSubsystem::GetObjectiveDescription(FName QuestId, FName ObjectiveId) const
{
	if (const UShadowSlaveQuestDefinition* Def = GetQuestDefinition(QuestId))
	{
		if (const FShadowSlaveObjectiveDefinition* ObjDef = Def->FindObjective(ObjectiveId))
		{
			return ObjDef->Description;
		}
	}
	return FText::GetEmpty();
}

bool UShadowSlaveQuestSubsystem::CanTransitionQuest(FName QuestId, EShadowSlaveQuestState CurrentState, EShadowSlaveQuestState NewState) const
{
	// Required correction 4: Harden lifecycle transitions
	if (QuestId.IsNone() || NewState == EShadowSlaveQuestState::Unknown)
	{
		return false;
	}

	// Idempotent: same state transition is always permitted
	if (CurrentState == NewState)
	{
		return true;
	}

	// Any transition to Available or Active requires a valid registered definition
	if (NewState == EShadowSlaveQuestState::Available || NewState == EShadowSlaveQuestState::Active)
	{
		if (!HasQuestDefinition(QuestId))
		{
			return false;
		}
	}

	// Terminal states: Completed, Failed, and Abandoned cannot leave their state via normal SetQuestState
	if (CurrentState == EShadowSlaveQuestState::Completed ||
	    CurrentState == EShadowSlaveQuestState::Failed ||
	    CurrentState == EShadowSlaveQuestState::Abandoned)
	{
		return false;
	}

	switch (CurrentState)
	{
	case EShadowSlaveQuestState::Unknown:
		// Unknown cannot transition to arbitrary operational states
		if (NewState == EShadowSlaveQuestState::Locked)
		{
			return true;
		}
		if (NewState == EShadowSlaveQuestState::Available)
		{
			return ArePrerequisitesSatisfied(QuestId);
		}
		return false;

	case EShadowSlaveQuestState::Locked:
		if (NewState == EShadowSlaveQuestState::Available)
		{
			return ArePrerequisitesSatisfied(QuestId);
		}
		if (NewState == EShadowSlaveQuestState::Active)
		{
			return ArePrerequisitesSatisfied(QuestId);
		}
		return false;

	case EShadowSlaveQuestState::Available:
		if (NewState == EShadowSlaveQuestState::Active)
		{
			return ArePrerequisitesSatisfied(QuestId);
		}
		if (NewState == EShadowSlaveQuestState::Locked)
		{
			return true;
		}
		return false;

	case EShadowSlaveQuestState::Active:
		if (NewState == EShadowSlaveQuestState::Completed ||
		    NewState == EShadowSlaveQuestState::Failed ||
		    NewState == EShadowSlaveQuestState::Abandoned)
		{
			return true;
		}
		return false;

	default:
		return false;
	}
}

bool UShadowSlaveQuestSubsystem::SetQuestState(FName QuestId, EShadowSlaveQuestState NewState)
{
	if (QuestId.IsNone() || NewState == EShadowSlaveQuestState::Unknown)
	{
		return false;
	}

	FShadowSlaveQuestRuntimeState* FoundState = QuestRuntimeStates.Find(QuestId);
	if (!FoundState)
	{
		const UShadowSlaveQuestDefinition* Def = GetQuestDefinition(QuestId);
		if (!Def)
		{
			UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveQuestSubsystem::SetQuestState - Untracked quest ID '%s' with no registered definition."), *QuestId.ToString());
			return false;
		}

		FShadowSlaveQuestRuntimeState NewEntry(QuestId, EShadowSlaveQuestState::Locked);
		InitializeRuntimeObjectives(NewEntry, Def);
		QuestRuntimeStates.Add(QuestId, NewEntry);
		FoundState = QuestRuntimeStates.Find(QuestId);
	}

	const EShadowSlaveQuestState OldState = FoundState->State;
	if (OldState == NewState)
	{
		return true;
	}

	if (!CanTransitionQuest(QuestId, OldState, NewState))
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveQuestSubsystem::SetQuestState - Rejected invalid transition for '%s': %d -> %d"),
			*QuestId.ToString(), static_cast<uint8>(OldState), static_cast<uint8>(NewState));
		return false;
	}

	// Prevent promoting malformed runtime state to Active
	if (NewState == EShadowSlaveQuestState::Active)
	{
		const UShadowSlaveQuestDefinition* Def = GetQuestDefinition(QuestId);
		if (!Def || FoundState->ObjectiveStates.Num() == 0 || !ArePrerequisitesSatisfied(QuestId))
		{
			UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveQuestSubsystem::SetQuestState - Rejected transition to Active for '%s' due to missing definition, malformed objectives, or unsatisfied prerequisites."),
				*QuestId.ToString());
			return false;
		}
	}

	if (bIsProcessingTransition)
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveQuestSubsystem::SetQuestState - Re-entrant transition rejected for '%s'"), *QuestId.ToString());
		return false;
	}

	TGuardValue<bool> TransitionGuard(bIsProcessingTransition, true);

	FoundState->State = NewState;

	// When activating a quest, activate all inactive objectives
	if (NewState == EShadowSlaveQuestState::Active)
	{
		ActivateObjectivesForQuest(*FoundState);
	}

	if (!bIsRestoringState)
	{
		OnQuestStateChanged.Broadcast(QuestId, NewState, OldState);

		if (NewState == EShadowSlaveQuestState::Completed)
		{
			OnQuestCompleted.Broadcast(QuestId);
		}
		else if (NewState == EShadowSlaveQuestState::Failed)
		{
			OnQuestFailed.Broadcast(QuestId);
		}
	}

	return true;
}

bool UShadowSlaveQuestSubsystem::ActivateQuest(FName QuestId)
{
	return SetQuestState(QuestId, EShadowSlaveQuestState::Active);
}

bool UShadowSlaveQuestSubsystem::CompleteQuest(FName QuestId)
{
	return SetQuestState(QuestId, EShadowSlaveQuestState::Completed);
}

bool UShadowSlaveQuestSubsystem::FailQuest(FName QuestId)
{
	return SetQuestState(QuestId, EShadowSlaveQuestState::Failed);
}

bool UShadowSlaveQuestSubsystem::AbandonQuest(FName QuestId)
{
	return SetQuestState(QuestId, EShadowSlaveQuestState::Abandoned);
}

bool UShadowSlaveQuestSubsystem::ResetQuestState(FName QuestId, EShadowSlaveQuestState ResetToState)
{
	if (QuestId.IsNone())
	{
		return false;
	}

	// Administrative reset API must ONLY allow Locked and Available
	if (ResetToState != EShadowSlaveQuestState::Locked && ResetToState != EShadowSlaveQuestState::Available)
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveQuestSubsystem::ResetQuestState - Rejected invalid reset target state %d for '%s' (only Locked and Available are permitted)."),
			static_cast<uint8>(ResetToState), *QuestId.ToString());
		return false;
	}

	// If resetting to Available, require prerequisites to be satisfied
	if (ResetToState == EShadowSlaveQuestState::Available && !ArePrerequisitesSatisfied(QuestId))
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveQuestSubsystem::ResetQuestState - Cannot reset '%s' to Available because prerequisites are not satisfied."),
			*QuestId.ToString());
		return false;
	}

	FShadowSlaveQuestRuntimeState* FoundState = QuestRuntimeStates.Find(QuestId);
	if (!FoundState)
	{
		if (const UShadowSlaveQuestDefinition* Def = GetQuestDefinition(QuestId))
		{
			FShadowSlaveQuestRuntimeState NewEntry(QuestId, ResetToState);
			InitializeRuntimeObjectives(NewEntry, Def);
			QuestRuntimeStates.Add(QuestId, NewEntry);

			if (!bIsRestoringState)
			{
				OnQuestStateChanged.Broadcast(QuestId, ResetToState, EShadowSlaveQuestState::Unknown);
			}
			return true;
		}
		else
		{
			return false;
		}
	}

	const EShadowSlaveQuestState OldState = FoundState->State;
	FoundState->State = ResetToState;

	// Reset child objectives back to Inactive and 0 progress
	if (const UShadowSlaveQuestDefinition* Def = GetQuestDefinition(QuestId))
	{
		InitializeRuntimeObjectives(*FoundState, Def);
	}
	else
	{
		for (auto& Pair : FoundState->ObjectiveStates)
		{
			Pair.Value.State = EShadowSlaveObjectiveState::Inactive;
			Pair.Value.CurrentQuantity = 0;
		}
	}

	if (OldState != ResetToState && !bIsRestoringState)
	{
		OnQuestStateChanged.Broadcast(QuestId, ResetToState, OldState);
	}

	return true;
}

void UShadowSlaveQuestSubsystem::ResetAllQuests()
{
	for (auto& Pair : QuestRuntimeStates)
	{
		const EShadowSlaveQuestState OldState = Pair.Value.State;
		const EShadowSlaveQuestState NewState = ArePrerequisitesSatisfied(Pair.Key)
			? EShadowSlaveQuestState::Available
			: EShadowSlaveQuestState::Locked;

		Pair.Value.State = NewState;

		if (const UShadowSlaveQuestDefinition* Def = GetQuestDefinition(Pair.Key))
		{
			InitializeRuntimeObjectives(Pair.Value, Def);
		}
		else
		{
			for (auto& ObjPair : Pair.Value.ObjectiveStates)
			{
				ObjPair.Value.State = EShadowSlaveObjectiveState::Inactive;
				ObjPair.Value.CurrentQuantity = 0;
			}
		}

		if (OldState != NewState && !bIsRestoringState)
		{
			OnQuestStateChanged.Broadcast(Pair.Key, NewState, OldState);
		}
	}
}

bool UShadowSlaveQuestSubsystem::SetObjectiveProgress(FName QuestId, FName ObjectiveId, int32 NewProgress)
{
	// Required correction 8: Harden runtime objective progression
	if (QuestId.IsNone() || ObjectiveId.IsNone())
	{
		return false;
	}

	FShadowSlaveQuestRuntimeState* FoundQuest = QuestRuntimeStates.Find(QuestId);
	if (!FoundQuest)
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveQuestSubsystem::SetObjectiveProgress - Quest '%s' not found."), *QuestId.ToString());
		return false;
	}

	// Progress mutation is only permitted on Active quests
	if (FoundQuest->State != EShadowSlaveQuestState::Active)
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveQuestSubsystem::SetObjectiveProgress - Cannot modify objective progress on non-active quest '%s' (State: %d)."),
			*QuestId.ToString(), static_cast<uint8>(FoundQuest->State));
		return false;
	}

	FShadowSlaveObjectiveRuntimeState* FoundObj = FoundQuest->ObjectiveStates.Find(ObjectiveId);
	if (!FoundObj)
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveQuestSubsystem::SetObjectiveProgress - Objective '%s' not found on quest '%s'."),
			*ObjectiveId.ToString(), *QuestId.ToString());
		return false;
	}

	// Failed objectives reject normal progress mutation
	if (FoundObj->State == EShadowSlaveObjectiveState::Failed)
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveQuestSubsystem::SetObjectiveProgress - Cannot modify progress on failed objective '%s' in quest '%s'."),
			*ObjectiveId.ToString(), *QuestId.ToString());
		return false;
	}

	// Completed objectives: idempotent when asked to remain complete; reject reducing progress below RequiredQuantity
	if (FoundObj->State == EShadowSlaveObjectiveState::Completed)
	{
		if (NewProgress >= FoundObj->RequiredQuantity)
		{
			return true; // Idempotent: already complete
		}

		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveQuestSubsystem::SetObjectiveProgress - Rejected reducing progress on completed objective '%s' in quest '%s' (%d < %d)."),
			*ObjectiveId.ToString(), *QuestId.ToString(), NewProgress, FoundObj->RequiredQuantity);
		return false;
	}

	// Inactive objectives on an Active quest reject direct progress mutation (must be activated through normal quest activation)
	if (FoundObj->State == EShadowSlaveObjectiveState::Inactive)
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveQuestSubsystem::SetObjectiveProgress - Rejected progress mutation on inactive objective '%s' in quest '%s'."),
			*ObjectiveId.ToString(), *QuestId.ToString());
		return false;
	}

	// Recheck definition for authoritative RequiredQuantity
	if (const UShadowSlaveQuestDefinition* Def = GetQuestDefinition(QuestId))
	{
		if (const FShadowSlaveObjectiveDefinition* ObjDef = Def->FindObjective(ObjectiveId))
		{
			FoundObj->RequiredQuantity = FMath::Max(1, ObjDef->RequiredQuantity);
		}
	}

	const int32 TargetQuantity = FMath::Max(1, FoundObj->RequiredQuantity);
	const int32 ClampedProgress = FMath::Clamp(NewProgress, 0, TargetQuantity);
	const int32 OldProgress = FoundObj->CurrentQuantity;

	if (OldProgress == ClampedProgress && FoundObj->State == (ClampedProgress >= TargetQuantity ? EShadowSlaveObjectiveState::Completed : EShadowSlaveObjectiveState::Active))
	{
		return true; // Idempotent
	}

	FoundObj->CurrentQuantity = ClampedProgress;

	if (OldProgress != ClampedProgress && !bIsRestoringState)
	{
		OnObjectiveProgressChanged.Broadcast(QuestId, ObjectiveId, ClampedProgress, OldProgress);
	}

	// Check completion condition
	if (ClampedProgress >= TargetQuantity && FoundObj->State != EShadowSlaveObjectiveState::Completed)
	{
		const EShadowSlaveObjectiveState OldObjState = FoundObj->State;
		FoundObj->State = EShadowSlaveObjectiveState::Completed;

		if (!bIsRestoringState)
		{
			OnObjectiveStateChanged.Broadcast(QuestId, ObjectiveId, EShadowSlaveObjectiveState::Completed, OldObjState);
			EvaluateQuestCompletion(QuestId);
		}
	}

	return true;
}

bool UShadowSlaveQuestSubsystem::AddObjectiveProgress(FName QuestId, FName ObjectiveId, int32 Amount)
{
	if (Amount <= 0)
	{
		return false;
	}

	FShadowSlaveObjectiveRuntimeState ObjRuntime;
	if (!GetObjectiveRuntimeState(QuestId, ObjectiveId, ObjRuntime) || ObjRuntime.State != EShadowSlaveObjectiveState::Active)
	{
		return false;
	}

	return SetObjectiveProgress(QuestId, ObjectiveId, ObjRuntime.CurrentQuantity + Amount);
}

bool UShadowSlaveQuestSubsystem::CompleteObjective(FName QuestId, FName ObjectiveId)
{
	// Required correction 9: Recheck objective completion logic
	if (QuestId.IsNone() || ObjectiveId.IsNone())
	{
		return false;
	}

	FShadowSlaveQuestRuntimeState* FoundQuest = QuestRuntimeStates.Find(QuestId);
	if (!FoundQuest || FoundQuest->State != EShadowSlaveQuestState::Active)
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveQuestSubsystem::CompleteObjective - Cannot complete objective on non-active quest '%s'."),
			*QuestId.ToString());
		return false;
	}

	FShadowSlaveObjectiveRuntimeState* FoundObj = FoundQuest->ObjectiveStates.Find(ObjectiveId);
	if (!FoundObj)
	{
		return false;
	}

	// Cannot complete a failed objective
	if (FoundObj->State == EShadowSlaveObjectiveState::Failed)
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveQuestSubsystem::CompleteObjective - Cannot complete failed objective '%s' in quest '%s'."),
			*ObjectiveId.ToString(), *QuestId.ToString());
		return false;
	}

	if (FoundObj->State == EShadowSlaveObjectiveState::Completed)
	{
		return true; // Idempotent
	}

	// Recheck definition for authoritative RequiredQuantity
	if (const UShadowSlaveQuestDefinition* Def = GetQuestDefinition(QuestId))
	{
		if (const FShadowSlaveObjectiveDefinition* ObjDef = Def->FindObjective(ObjectiveId))
		{
			FoundObj->RequiredQuantity = FMath::Max(1, ObjDef->RequiredQuantity);
		}
	}

	const int32 OldProgress = FoundObj->CurrentQuantity;
	const EShadowSlaveObjectiveState OldObjState = FoundObj->State;

	FoundObj->CurrentQuantity = FoundObj->RequiredQuantity;
	FoundObj->State = EShadowSlaveObjectiveState::Completed;

	if (!bIsRestoringState)
	{
		if (OldProgress != FoundObj->CurrentQuantity)
		{
			OnObjectiveProgressChanged.Broadcast(QuestId, ObjectiveId, FoundObj->CurrentQuantity, OldProgress);
		}
		OnObjectiveStateChanged.Broadcast(QuestId, ObjectiveId, EShadowSlaveObjectiveState::Completed, OldObjState);
		EvaluateQuestCompletion(QuestId);
	}

	return true;
}

bool UShadowSlaveQuestSubsystem::FailObjective(FName QuestId, FName ObjectiveId)
{
	if (QuestId.IsNone() || ObjectiveId.IsNone())
	{
		return false;
	}

	FShadowSlaveQuestRuntimeState* FoundQuest = QuestRuntimeStates.Find(QuestId);
	if (!FoundQuest || FoundQuest->State != EShadowSlaveQuestState::Active)
	{
		return false;
	}

	FShadowSlaveObjectiveRuntimeState* FoundObj = FoundQuest->ObjectiveStates.Find(ObjectiveId);
	if (!FoundObj)
	{
		return false;
	}

	if (FoundObj->State == EShadowSlaveObjectiveState::Failed)
	{
		return true; // Idempotent
	}

	if (FoundObj->State == EShadowSlaveObjectiveState::Completed)
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveQuestSubsystem::FailObjective - Cannot fail already completed objective '%s' in quest '%s'."),
			*ObjectiveId.ToString(), *QuestId.ToString());
		return false;
	}

	const EShadowSlaveObjectiveState OldObjState = FoundObj->State;
	FoundObj->State = EShadowSlaveObjectiveState::Failed;

	if (!bIsRestoringState)
	{
		OnObjectiveStateChanged.Broadcast(QuestId, ObjectiveId, EShadowSlaveObjectiveState::Failed, OldObjState);

		// If this objective is required for quest completion, failing it fails the quest
		if (!FoundObj->bIsOptional)
		{
			FailQuest(QuestId);
		}
	}

	return true;
}

void UShadowSlaveQuestSubsystem::InitializeRuntimeObjectives(FShadowSlaveQuestRuntimeState& QuestRuntime, const UShadowSlaveQuestDefinition* Def)
{
	QuestRuntime.ObjectiveStates.Empty();

	if (!Def)
	{
		return;
	}

	for (const FShadowSlaveObjectiveDefinition& ObjDef : Def->Objectives)
	{
		if (!ObjDef.ObjectiveId.IsNone())
		{
			QuestRuntime.ObjectiveStates.Add(ObjDef.ObjectiveId, FShadowSlaveObjectiveRuntimeState(ObjDef));
		}
	}
}

void UShadowSlaveQuestSubsystem::ActivateObjectivesForQuest(FShadowSlaveQuestRuntimeState& QuestRuntime)
{
	for (auto& Pair : QuestRuntime.ObjectiveStates)
	{
		if (Pair.Value.State == EShadowSlaveObjectiveState::Inactive)
		{
			Pair.Value.State = EShadowSlaveObjectiveState::Active;
			if (!bIsRestoringState)
			{
				OnObjectiveStateChanged.Broadcast(QuestRuntime.QuestId, Pair.Key, EShadowSlaveObjectiveState::Active, EShadowSlaveObjectiveState::Inactive);
			}
		}
	}
}

void UShadowSlaveQuestSubsystem::EvaluateQuestCompletion(FName QuestId)
{
	FShadowSlaveQuestRuntimeState* FoundQuest = QuestRuntimeStates.Find(QuestId);
	if (!FoundQuest || FoundQuest->State != EShadowSlaveQuestState::Active)
	{
		return;
	}

	const UShadowSlaveQuestDefinition* Def = GetQuestDefinition(QuestId);
	if (Def && !Def->bAutoCompleteWhenObjectivesComplete)
	{
		return;
	}

	if (FoundQuest->AreAllRequiredObjectivesComplete())
	{
		CompleteQuest(QuestId);
	}
}

FShadowSlaveQuestSaveData UShadowSlaveQuestSubsystem::ExportSaveData() const
{
	FShadowSlaveQuestSaveData SaveData;
	SaveData.QuestSubsystemVersion = 1;
	SaveData.bIsValid = true;

	// Sort quest IDs alphabetically for deterministic export
	TArray<FName> SortedQuestIds;
	QuestRuntimeStates.GetKeys(SortedQuestIds);
	SortedQuestIds.Sort([](const FName& A, const FName& B) {
		return A.Compare(B) < 0;
	});

	for (const FName& Id : SortedQuestIds)
	{
		if (const FShadowSlaveQuestRuntimeState* State = QuestRuntimeStates.Find(Id))
		{
			FShadowSlaveQuestRecordSaveData Record;
			Record.QuestId = State->QuestId;
			Record.State = State->State;
			Record.RuntimeMetadata = State->RuntimeMetadata;

			if (const UShadowSlaveQuestDefinition* Def = GetQuestDefinition(Id))
			{
				Record.QuestVersion = Def->Version;
			}
			else
			{
				Record.QuestVersion = 1;
			}

			// Sort objective IDs alphabetically for deterministic export
			TArray<FName> SortedObjIds;
			State->ObjectiveStates.GetKeys(SortedObjIds);
			SortedObjIds.Sort([](const FName& A, const FName& B) {
				return A.Compare(B) < 0;
			});

			for (const FName& ObjId : SortedObjIds)
			{
				if (const FShadowSlaveObjectiveRuntimeState* ObjState = State->ObjectiveStates.Find(ObjId))
				{
					Record.Objectives.Add(FShadowSlaveObjectiveSaveData(*ObjState));
				}
			}

			SaveData.Quests.Add(Record);
		}
	}

	return SaveData;
}

bool UShadowSlaveQuestSubsystem::ImportSaveData(const FShadowSlaveQuestSaveData& InSaveData)
{
	// Required correction 5: Definition-first save restoration
	// Required correction 6: Sanitize saved quest state
	// Required correction 7: Sanitize saved objective state and progress

	// 1. Validate save container / version
	if (!InSaveData.bIsValid || InSaveData.QuestSubsystemVersion < 1)
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveQuestSubsystem::ImportSaveData - Incompatible or invalid save data."));
		return false;
	}

	// Suppress gameplay events during save restoration
	TGuardValue<bool> RestoreGuard(bIsRestoringState, true);

	// Pass 1: Resolve definitions and restore valid quest & objective structures
	TMap<FName, EShadowSlaveQuestState> DesiredQuestStates;

	for (const FShadowSlaveQuestRecordSaveData& SavedRecord : InSaveData.Quests)
	{
		// 2. Validate QuestId
		if (SavedRecord.QuestId.IsNone())
		{
			continue;
		}

		// Reject Unknown saved state
		if (SavedRecord.State == EShadowSlaveQuestState::Unknown)
		{
			UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveQuestSubsystem::ImportSaveData - Saved quest '%s' has Unknown state; skipping."),
				*SavedRecord.QuestId.ToString());
			continue;
		}

		// 3. Resolve currently registered QuestDefinition
		const UShadowSlaveQuestDefinition* Def = GetQuestDefinition(SavedRecord.QuestId);
		if (!Def)
		{
			// 4. Missing definition: skip safely and log
			UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveQuestSubsystem::ImportSaveData - Missing QuestDefinition for saved quest '%s'; skipping."),
				*SavedRecord.QuestId.ToString());
			continue;
		}

		// 5. Validate saved quest version against definition
		if (SavedRecord.QuestVersion != Def->Version)
		{
			UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveQuestSubsystem::ImportSaveData - Version mismatch for quest '%s' (Saved: %d, Def: %d)."),
				*SavedRecord.QuestId.ToString(), SavedRecord.QuestVersion, Def->Version);
		}

		// 6. Definition resolved: create / initialize runtime state
		FShadowSlaveQuestRuntimeState& RuntimeEntry = QuestRuntimeStates.FindOrAdd(SavedRecord.QuestId);
		RuntimeEntry.QuestId = SavedRecord.QuestId;
		RuntimeEntry.RuntimeMetadata = SavedRecord.RuntimeMetadata;

		// Initialize all objectives authoritatively from definition first
		InitializeRuntimeObjectives(RuntimeEntry, Def);

		// 7. Validate and restore saved objectives against resolved definition
		for (const FShadowSlaveObjectiveSaveData& ObjSave : SavedRecord.Objectives)
		{
			if (ObjSave.ObjectiveId.IsNone())
			{
				continue;
			}

			// 8 & 9. Objective must exist in definition
			const FShadowSlaveObjectiveDefinition* ObjDef = Def->FindObjective(ObjSave.ObjectiveId);
			if (!ObjDef)
			{
				UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveQuestSubsystem::ImportSaveData - Saved objective '%s' does not exist in definition '%s'; skipping."),
					*ObjSave.ObjectiveId.ToString(), *SavedRecord.QuestId.ToString());
				continue;
			}

			if (ObjSave.State == EShadowSlaveObjectiveState::Unknown)
			{
				continue;
			}

			FShadowSlaveObjectiveRuntimeState* ObjRuntime = RuntimeEntry.ObjectiveStates.Find(ObjSave.ObjectiveId);
			if (!ObjRuntime)
			{
				continue;
			}

			// Definition is authoritative for structural properties
			ObjRuntime->RequiredQuantity = FMath::Max(1, ObjDef->RequiredQuantity);
			ObjRuntime->bIsOptional = ObjDef->bIsOptional;
			ObjRuntime->RuntimeMetadata = ObjSave.RuntimeMetadata;

			// Progress and state sanitization
			if (ObjSave.State == EShadowSlaveObjectiveState::Completed)
			{
				ObjRuntime->State = EShadowSlaveObjectiveState::Completed;
				ObjRuntime->CurrentQuantity = ObjRuntime->RequiredQuantity;
			}
			else
			{
				ObjRuntime->State = ObjSave.State;
				const int32 MaxUncompletedProgress = FMath::Max(0, ObjRuntime->RequiredQuantity - 1);
				ObjRuntime->CurrentQuantity = FMath::Clamp(ObjSave.CurrentQuantity, 0, MaxUncompletedProgress);
			}
		}

		DesiredQuestStates.Add(SavedRecord.QuestId, SavedRecord.State);
	}

	// Pass 2: Sanitize quest states based on prerequisites
	for (const auto& Pair : DesiredQuestStates)
	{
		const FName QuestId = Pair.Key;
		const EShadowSlaveQuestState DesiredState = Pair.Value;
		FShadowSlaveQuestRuntimeState* RuntimeEntry = QuestRuntimeStates.Find(QuestId);
		if (!RuntimeEntry)
		{
			continue;
		}

		if (DesiredState == EShadowSlaveQuestState::Completed ||
		    DesiredState == EShadowSlaveQuestState::Failed ||
		    DesiredState == EShadowSlaveQuestState::Abandoned ||
		    DesiredState == EShadowSlaveQuestState::Locked)
		{
			RuntimeEntry->State = DesiredState;
		}
		else if (DesiredState == EShadowSlaveQuestState::Available || DesiredState == EShadowSlaveQuestState::Active)
		{
			// A quest with unsatisfied prerequisites must NOT be restored as Available or Active
			if (ArePrerequisitesSatisfied(QuestId))
			{
				RuntimeEntry->State = DesiredState;
			}
			else
			{
				UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveQuestSubsystem::ImportSaveData - Quest '%s' was saved as state %d but prerequisites are not satisfied; falling back to Locked."),
					*QuestId.ToString(), static_cast<uint8>(DesiredState));
				RuntimeEntry->State = EShadowSlaveQuestState::Locked;

				// Demote any active objectives to Inactive if quest fell back to Locked
				for (auto& ObjPair : RuntimeEntry->ObjectiveStates)
				{
					if (ObjPair.Value.State == EShadowSlaveObjectiveState::Active)
					{
						ObjPair.Value.State = EShadowSlaveObjectiveState::Inactive;
					}
				}
			}
		}
		else
		{
			// Malformed state: fall back to Locked
			RuntimeEntry->State = EShadowSlaveQuestState::Locked;
		}
	}

	return true;
}

UShadowSlaveStorySubsystem* UShadowSlaveQuestSubsystem::GetStorySubsystem() const
{
	if (UGameInstance* GI = GetGameInstance())
	{
		return GI->GetSubsystem<UShadowSlaveStorySubsystem>();
	}
	return nullptr;
}
