// Copyright Epic Games, Inc. All Rights Reserved.

#include "Nightmares/ShadowSlaveNightmareObjectiveTracker.h"
#include "Nightmares/ShadowSlaveNightmareScenarioDefinition.h"
#include "ShadowSlave.h"

UShadowSlaveNightmareObjectiveTracker::UShadowSlaveNightmareObjectiveTracker()
{
}

void UShadowSlaveNightmareObjectiveTracker::InitializeObjectives(const UShadowSlaveNightmareScenarioDefinition* InScenarioDef)
{
	ResetObjectives();

	if (!InScenarioDef)
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveNightmareObjectiveTracker::InitializeObjectives called with null scenario definition."));
		return;
	}

	BoundScenarioDefinition = InScenarioDef;

	for (const FShadowSlaveNightmareObjectiveDefinition& ObjDef : InScenarioDef->Objectives)
	{
		if (ObjDef.ObjectiveId.IsNone())
		{
			continue;
		}

		FShadowSlaveNightmareObjectiveRuntimeState RuntimeState(
			ObjDef.ObjectiveId,
			ObjDef.TargetProgress,
			ObjDef.bIsRequired
		);

		ObjectiveRuntimeStates.Add(ObjDef.ObjectiveId, RuntimeState);
	}

	UE_LOG(LogShadowSlave, Log, TEXT("UShadowSlaveNightmareObjectiveTracker: Initialized %d objectives for scenario '%s'."),
		ObjectiveRuntimeStates.Num(),
		*InScenarioDef->ScenarioId.ToString()
	);
}

void UShadowSlaveNightmareObjectiveTracker::ResetObjectives()
{
	ObjectiveRuntimeStates.Empty();
	BoundScenarioDefinition.Reset();
}

bool UShadowSlaveNightmareObjectiveTracker::ActivateObjective(FName ObjectiveId)
{
	FShadowSlaveNightmareObjectiveRuntimeState* State = ObjectiveRuntimeStates.Find(ObjectiveId);
	if (!State)
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveNightmareObjectiveTracker: Cannot activate unknown objective '%s'."), *ObjectiveId.ToString());
		return false;
	}

	if (State->State == EShadowSlaveObjectiveState::Active)
	{
		return true;
	}

	if (State->State == EShadowSlaveObjectiveState::Completed || State->State == EShadowSlaveObjectiveState::Failed)
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveNightmareObjectiveTracker: Cannot activate already finished objective '%s'."), *ObjectiveId.ToString());
		return false;
	}

	return TransitionObjectiveState(ObjectiveId, EShadowSlaveObjectiveState::Active);
}

bool UShadowSlaveNightmareObjectiveTracker::UpdateObjectiveProgress(FName ObjectiveId, float NewProgress)
{
	FShadowSlaveNightmareObjectiveRuntimeState* State = ObjectiveRuntimeStates.Find(ObjectiveId);
	if (!State)
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveNightmareObjectiveTracker: Cannot update unknown objective '%s'."), *ObjectiveId.ToString());
		return false;
	}

	if (State->State == EShadowSlaveObjectiveState::Completed || State->State == EShadowSlaveObjectiveState::Failed)
	{
		return false;
	}

	// Auto-activate objective upon receiving first progress if currently NotStarted
	if (State->State == EShadowSlaveObjectiveState::NotStarted)
	{
		TransitionObjectiveState(ObjectiveId, EShadowSlaveObjectiveState::Active);
	}

	const float ClampedProgress = FMath::Clamp(NewProgress, 0.0f, State->TargetProgress);
	State->CurrentProgress = ClampedProgress;

	OnObjectiveProgressChanged.Broadcast(ObjectiveId, State->CurrentProgress, State->TargetProgress);

	if (State->CurrentProgress >= State->TargetProgress)
	{
		return CompleteObjective(ObjectiveId);
	}

	return true;
}

bool UShadowSlaveNightmareObjectiveTracker::AddObjectiveProgress(FName ObjectiveId, float DeltaProgress)
{
	FShadowSlaveNightmareObjectiveRuntimeState* State = ObjectiveRuntimeStates.Find(ObjectiveId);
	if (!State)
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveNightmareObjectiveTracker: Cannot add progress to unknown objective '%s'."), *ObjectiveId.ToString());
		return false;
	}

	return UpdateObjectiveProgress(ObjectiveId, State->CurrentProgress + DeltaProgress);
}

bool UShadowSlaveNightmareObjectiveTracker::CompleteObjective(FName ObjectiveId)
{
	FShadowSlaveNightmareObjectiveRuntimeState* State = ObjectiveRuntimeStates.Find(ObjectiveId);
	if (!State)
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveNightmareObjectiveTracker: Cannot complete unknown objective '%s'."), *ObjectiveId.ToString());
		return false;
	}

	if (State->State == EShadowSlaveObjectiveState::Completed)
	{
		return true;
	}

	if (State->State == EShadowSlaveObjectiveState::Failed)
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveNightmareObjectiveTracker: Cannot complete already failed objective '%s'."), *ObjectiveId.ToString());
		return false;
	}

	State->CurrentProgress = State->TargetProgress;
	return TransitionObjectiveState(ObjectiveId, EShadowSlaveObjectiveState::Completed);
}

bool UShadowSlaveNightmareObjectiveTracker::FailObjective(FName ObjectiveId)
{
	FShadowSlaveNightmareObjectiveRuntimeState* State = ObjectiveRuntimeStates.Find(ObjectiveId);
	if (!State)
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveNightmareObjectiveTracker: Cannot fail unknown objective '%s'."), *ObjectiveId.ToString());
		return false;
	}

	if (State->State == EShadowSlaveObjectiveState::Failed)
	{
		return true;
	}

	if (State->State == EShadowSlaveObjectiveState::Completed)
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveNightmareObjectiveTracker: Cannot fail already completed objective '%s'."), *ObjectiveId.ToString());
		return false;
	}

	return TransitionObjectiveState(ObjectiveId, EShadowSlaveObjectiveState::Failed);
}

EShadowSlaveObjectiveState UShadowSlaveNightmareObjectiveTracker::GetObjectiveState(FName ObjectiveId) const
{
	if (const FShadowSlaveNightmareObjectiveRuntimeState* State = ObjectiveRuntimeStates.Find(ObjectiveId))
	{
		return State->State;
	}
	return EShadowSlaveObjectiveState::NotStarted;
}

bool UShadowSlaveNightmareObjectiveTracker::GetObjectiveRuntimeState(FName ObjectiveId, FShadowSlaveNightmareObjectiveRuntimeState& OutState) const
{
	if (const FShadowSlaveNightmareObjectiveRuntimeState* State = ObjectiveRuntimeStates.Find(ObjectiveId))
	{
		OutState = *State;
		return true;
	}
	return false;
}

TArray<FShadowSlaveNightmareObjectiveRuntimeState> UShadowSlaveNightmareObjectiveTracker::GetAllObjectiveRuntimeStates() const
{
	TArray<FShadowSlaveNightmareObjectiveRuntimeState> Result;
	ObjectiveRuntimeStates.GenerateValueArray(Result);
	return Result;
}

bool UShadowSlaveNightmareObjectiveTracker::AreRequiredObjectivesComplete() const
{
	int32 RequiredCount = 0;

	for (const auto& Pair : ObjectiveRuntimeStates)
	{
		const FShadowSlaveNightmareObjectiveRuntimeState& State = Pair.Value;
		if (State.bIsRequired)
		{
			++RequiredCount;
			if (State.State != EShadowSlaveObjectiveState::Completed)
			{
				return false;
			}
		}
	}

	// Returns true only if there were required objectives and all are completed
	return RequiredCount > 0;
}

bool UShadowSlaveNightmareObjectiveTracker::HasAnyRequiredObjectiveFailed() const
{
	for (const auto& Pair : ObjectiveRuntimeStates)
	{
		const FShadowSlaveNightmareObjectiveRuntimeState& State = Pair.Value;
		if (State.bIsRequired && State.State == EShadowSlaveObjectiveState::Failed)
		{
			return true;
		}
	}
	return false;
}

bool UShadowSlaveNightmareObjectiveTracker::AreAllObjectivesComplete() const
{
	if (ObjectiveRuntimeStates.Num() == 0)
	{
		return false;
	}

	for (const auto& Pair : ObjectiveRuntimeStates)
	{
		if (Pair.Value.State != EShadowSlaveObjectiveState::Completed)
		{
			return false;
		}
	}
	return true;
}

bool UShadowSlaveNightmareObjectiveTracker::HasAnyObjectiveFailed() const
{
	for (const auto& Pair : ObjectiveRuntimeStates)
	{
		if (Pair.Value.State == EShadowSlaveObjectiveState::Failed)
		{
			return true;
		}
	}
	return false;
}

int32 UShadowSlaveNightmareObjectiveTracker::GetCompletedRequiredObjectiveCount() const
{
	int32 Count = 0;
	for (const auto& Pair : ObjectiveRuntimeStates)
	{
		if (Pair.Value.bIsRequired && Pair.Value.State == EShadowSlaveObjectiveState::Completed)
		{
			++Count;
		}
	}
	return Count;
}

int32 UShadowSlaveNightmareObjectiveTracker::GetTotalRequiredObjectiveCount() const
{
	int32 Count = 0;
	for (const auto& Pair : ObjectiveRuntimeStates)
	{
		if (Pair.Value.bIsRequired)
		{
			++Count;
		}
	}
	return Count;
}

const UShadowSlaveNightmareScenarioDefinition* UShadowSlaveNightmareObjectiveTracker::GetBoundScenarioDefinition() const
{
	return BoundScenarioDefinition.Get();
}

bool UShadowSlaveNightmareObjectiveTracker::TransitionObjectiveState(FName ObjectiveId, EShadowSlaveObjectiveState TargetState)
{
	FShadowSlaveNightmareObjectiveRuntimeState* State = ObjectiveRuntimeStates.Find(ObjectiveId);
	if (!State)
	{
		return false;
	}

	if (State->State == TargetState)
	{
		return true;
	}

	const EShadowSlaveObjectiveState OldState = State->State;
	State->State = TargetState;

	UE_LOG(LogShadowSlave, Log, TEXT("UShadowSlaveNightmareObjectiveTracker: Objective '%s' state changed from %d to %d."),
		*ObjectiveId.ToString(),
		static_cast<int32>(OldState),
		static_cast<int32>(TargetState)
	);

	OnObjectiveStateChanged.Broadcast(ObjectiveId, TargetState, OldState);
	return true;
}
