// Copyright Epic Games, Inc. All Rights Reserved.

#include "Story/ShadowSlaveStorySubsystem.h"
#include "ShadowSlave.h"

UShadowSlaveStorySubsystem::UShadowSlaveStorySubsystem()
{
}

void UShadowSlaveStorySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	RegisteredDefinitions.Empty();
	StoryRuntimeStates.Empty();
	bIsRestoringState = false;
}

void UShadowSlaveStorySubsystem::Deinitialize()
{
	RegisteredDefinitions.Empty();
	StoryRuntimeStates.Empty();
	Super::Deinitialize();
}

bool UShadowSlaveStorySubsystem::RegisterStoryDefinition(UShadowSlaveStoryDefinition* StoryDef)
{
	if (!StoryDef || StoryDef->StoryId.IsNone())
	{
		return false;
	}

	RegisteredDefinitions.Add(StoryDef->StoryId, StoryDef);

	// If runtime state does not exist yet, initialize it
	if (!StoryRuntimeStates.Contains(StoryDef->StoryId))
	{
		const EShadowSlaveStoryState InitialState = ArePrerequisitesSatisfied(StoryDef->StoryId)
			? EShadowSlaveStoryState::Available
			: EShadowSlaveStoryState::Locked;

		StoryRuntimeStates.Add(StoryDef->StoryId, FShadowSlaveStoryRuntimeState(StoryDef->StoryId, InitialState));
	}

	return true;
}

bool UShadowSlaveStorySubsystem::UnregisterStoryDefinition(FName StoryId)
{
	if (StoryId.IsNone())
	{
		return false;
	}

	return RegisteredDefinitions.Remove(StoryId) > 0;
}

UShadowSlaveStoryDefinition* UShadowSlaveStorySubsystem::GetStoryDefinition(FName StoryId) const
{
	if (StoryId.IsNone())
	{
		return nullptr;
	}

	if (const TObjectPtr<UShadowSlaveStoryDefinition>* Found = RegisteredDefinitions.Find(StoryId))
	{
		return Found->Get();
	}

	return nullptr;
}

bool UShadowSlaveStorySubsystem::HasStoryDefinition(FName StoryId) const
{
	return !StoryId.IsNone() && RegisteredDefinitions.Contains(StoryId);
}

EShadowSlaveStoryState UShadowSlaveStorySubsystem::GetStoryState(FName StoryId) const
{
	if (StoryId.IsNone())
	{
		return EShadowSlaveStoryState::Unknown;
	}

	if (const FShadowSlaveStoryRuntimeState* Found = StoryRuntimeStates.Find(StoryId))
	{
		return Found->State;
	}

	return EShadowSlaveStoryState::Unknown;
}

bool UShadowSlaveStorySubsystem::IsStoryActive(FName StoryId) const
{
	return GetStoryState(StoryId) == EShadowSlaveStoryState::Active;
}

bool UShadowSlaveStorySubsystem::IsStoryCompleted(FName StoryId) const
{
	return GetStoryState(StoryId) == EShadowSlaveStoryState::Completed;
}

bool UShadowSlaveStorySubsystem::ArePrerequisitesSatisfied(FName StoryId) const
{
	if (StoryId.IsNone())
	{
		return false;
	}

	const UShadowSlaveStoryDefinition* Def = GetStoryDefinition(StoryId);
	if (!Def)
	{
		return true;
	}

	for (const FName& PrereqId : Def->PrerequisiteStoryIds)
	{
		if (PrereqId.IsNone())
		{
			continue;
		}

		if (GetStoryState(PrereqId) != EShadowSlaveStoryState::Completed)
		{
			return false;
		}
	}

	return true;
}

bool UShadowSlaveStorySubsystem::GetStoryRuntimeState(FName StoryId, FShadowSlaveStoryRuntimeState& OutState) const
{
	if (StoryId.IsNone())
	{
		OutState = FShadowSlaveStoryRuntimeState();
		return false;
	}

	if (const FShadowSlaveStoryRuntimeState* Found = StoryRuntimeStates.Find(StoryId))
	{
		OutState = *Found;
		return true;
	}

	OutState = FShadowSlaveStoryRuntimeState();
	return false;
}

bool UShadowSlaveStorySubsystem::CanTransition(FName StoryId, EShadowSlaveStoryState CurrentState, EShadowSlaveStoryState NewState) const
{
	// Unknown is never a valid target state or progression
	if (NewState == EShadowSlaveStoryState::Unknown)
	{
		return false;
	}

	// Idempotent: staying in the same state is permitted
	if (CurrentState == NewState)
	{
		return true;
	}

	// Terminal states: Completed, Failed, and Skipped cannot leave their state via normal SetStoryState
	if (CurrentState == EShadowSlaveStoryState::Completed ||
	    CurrentState == EShadowSlaveStoryState::Failed ||
	    CurrentState == EShadowSlaveStoryState::Skipped)
	{
		return false;
	}

	switch (CurrentState)
	{
	case EShadowSlaveStoryState::Unknown:
		if (NewState == EShadowSlaveStoryState::Locked)
		{
			return true;
		}
		if (NewState == EShadowSlaveStoryState::Available)
		{
			return ArePrerequisitesSatisfied(StoryId);
		}
		return false;

	case EShadowSlaveStoryState::Locked:
		if (NewState == EShadowSlaveStoryState::Available)
		{
			return ArePrerequisitesSatisfied(StoryId);
		}
		if (NewState == EShadowSlaveStoryState::Active)
		{
			return ArePrerequisitesSatisfied(StoryId);
		}
		return false;

	case EShadowSlaveStoryState::Available:
		if (NewState == EShadowSlaveStoryState::Active)
		{
			return ArePrerequisitesSatisfied(StoryId);
		}
		if (NewState == EShadowSlaveStoryState::Locked)
		{
			return true;
		}
		return false;

	case EShadowSlaveStoryState::Active:
		if (NewState == EShadowSlaveStoryState::Completed ||
		    NewState == EShadowSlaveStoryState::Failed ||
		    NewState == EShadowSlaveStoryState::Skipped)
		{
			return true;
		}
		return false;

	default:
		return false;
	}
}

bool UShadowSlaveStorySubsystem::SetStoryState(FName StoryId, EShadowSlaveStoryState NewState)
{
	if (StoryId.IsNone() || NewState == EShadowSlaveStoryState::Unknown)
	{
		return false;
	}

	FShadowSlaveStoryRuntimeState* FoundState = StoryRuntimeStates.Find(StoryId);
	if (!FoundState)
	{
		if (RegisteredDefinitions.Contains(StoryId))
		{
			FShadowSlaveStoryRuntimeState NewEntry(StoryId, EShadowSlaveStoryState::Locked);
			StoryRuntimeStates.Add(StoryId, NewEntry);
			FoundState = StoryRuntimeStates.Find(StoryId);
		}
		else
		{
			UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveStorySubsystem::SetStoryState - Untracked story ID '%s'"), *StoryId.ToString());
			return false;
		}
	}

	const EShadowSlaveStoryState OldState = FoundState->State;
	if (OldState == NewState)
	{
		// Idempotent: repeated completion or state set succeeds with zero duplicate events
		return true;
	}

	if (!CanTransition(StoryId, OldState, NewState))
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveStorySubsystem::SetStoryState - Rejected invalid transition for '%s': %d -> %d"),
			*StoryId.ToString(), static_cast<uint8>(OldState), static_cast<uint8>(NewState));
		return false;
	}

	FoundState->State = NewState;

	if (!bIsRestoringState)
	{
		OnStoryStateChanged.Broadcast(StoryId, NewState, OldState);
	}

	return true;
}

bool UShadowSlaveStorySubsystem::ResetStoryState(FName StoryId, EShadowSlaveStoryState ResetToState)
{
	if (StoryId.IsNone())
	{
		return false;
	}

	// Administrative reset API must ONLY allow Locked and Available
	if (ResetToState != EShadowSlaveStoryState::Locked && ResetToState != EShadowSlaveStoryState::Available)
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveStorySubsystem::ResetStoryState - Rejected invalid reset target state %d for '%s' (only Locked and Available are permitted)."),
			static_cast<uint8>(ResetToState), *StoryId.ToString());
		return false;
	}

	// If resetting to Available, require prerequisites to be satisfied
	if (ResetToState == EShadowSlaveStoryState::Available && !ArePrerequisitesSatisfied(StoryId))
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveStorySubsystem::ResetStoryState - Cannot reset '%s' to Available because prerequisites are not satisfied."),
			*StoryId.ToString());
		return false;
	}

	FShadowSlaveStoryRuntimeState* FoundState = StoryRuntimeStates.Find(StoryId);
	if (!FoundState)
	{
		if (RegisteredDefinitions.Contains(StoryId))
		{
			FShadowSlaveStoryRuntimeState NewEntry(StoryId, ResetToState);
			NewEntry.CurrentStepId = NAME_None;
			StoryRuntimeStates.Add(StoryId, NewEntry);

			if (!bIsRestoringState)
			{
				OnStoryStateChanged.Broadcast(StoryId, ResetToState, EShadowSlaveStoryState::Unknown);
			}
			return true;
		}
		else
		{
			return false;
		}
	}

	const EShadowSlaveStoryState OldState = FoundState->State;
	FoundState->State = ResetToState;
	FoundState->CurrentStepId = NAME_None;

	if (OldState != ResetToState && !bIsRestoringState)
	{
		OnStoryStateChanged.Broadcast(StoryId, ResetToState, OldState);
	}

	return true;
}

void UShadowSlaveStorySubsystem::ResetAllStoryStates()
{
	for (auto& Pair : StoryRuntimeStates)
	{
		const EShadowSlaveStoryState OldState = Pair.Value.State;
		const EShadowSlaveStoryState NewState = ArePrerequisitesSatisfied(Pair.Key)
			? EShadowSlaveStoryState::Available
			: EShadowSlaveStoryState::Locked;

		Pair.Value.State = NewState;
		Pair.Value.CurrentStepId = NAME_None;

		if (OldState != NewState && !bIsRestoringState)
		{
			OnStoryStateChanged.Broadcast(Pair.Key, NewState, OldState);
		}
	}
}

bool UShadowSlaveStorySubsystem::SetCurrentStoryStep(FName StoryId, FName StepId)
{
	if (StoryId.IsNone())
	{
		return false;
	}

	FShadowSlaveStoryRuntimeState* FoundState = StoryRuntimeStates.Find(StoryId);
	if (!FoundState)
	{
		return false;
	}

	if (FoundState->State != EShadowSlaveStoryState::Active)
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveStorySubsystem::SetCurrentStoryStep - Cannot set step on inactive story '%s' (State: %d)"),
			*StoryId.ToString(), static_cast<uint8>(FoundState->State));
		return false;
	}

	const FName OldStep = FoundState->CurrentStepId;
	if (OldStep == StepId)
	{
		// Idempotent: identical step, return true without broadcasting
		return true;
	}

	// Resolve definition to validate non-None StepId
	const UShadowSlaveStoryDefinition* StoryDef = GetStoryDefinition(StoryId);
	if (!StoryDef)
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveStorySubsystem::SetCurrentStoryStep - Missing story definition for '%s'"),
			*StoryId.ToString());
		return false;
	}

	if (!StepId.IsNone())
	{
		if (!StoryDef->StepIds.Contains(StepId))
		{
			UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveStorySubsystem::SetCurrentStoryStep - StepId '%s' does not exist in definition '%s'"),
				*StepId.ToString(), *StoryId.ToString());
			return false;
		}
	}

	FoundState->CurrentStepId = StepId;

	if (!bIsRestoringState)
	{
		OnStoryStepChanged.Broadcast(StoryId, StepId, OldStep);
	}

	return true;
}

FName UShadowSlaveStorySubsystem::GetCurrentStoryStep(FName StoryId) const
{
	if (StoryId.IsNone())
	{
		return NAME_None;
	}

	if (const FShadowSlaveStoryRuntimeState* Found = StoryRuntimeStates.Find(StoryId))
	{
		return Found->CurrentStepId;
	}

	return NAME_None;
}

FShadowSlaveStorySaveData UShadowSlaveStorySubsystem::ExportSaveData() const
{
	FShadowSlaveStorySaveData SaveData;
	SaveData.StorySubsystemVersion = 1;
	SaveData.bIsValid = true;

	// Sort story IDs alphabetically for deterministic export
	TArray<FName> SortedStoryIds;
	StoryRuntimeStates.GetKeys(SortedStoryIds);
	SortedStoryIds.Sort([](const FName& A, const FName& B) {
		return A.Compare(B) < 0;
	});

	for (const FName& Id : SortedStoryIds)
	{
		if (const FShadowSlaveStoryRuntimeState* State = StoryRuntimeStates.Find(Id))
		{
			SaveData.Stories.Add(FShadowSlaveStoryRecordSaveData(*State));
		}
	}

	return SaveData;
}

bool UShadowSlaveStorySubsystem::ImportSaveData(const FShadowSlaveStorySaveData& InSaveData)
{
	if (!InSaveData.bIsValid || InSaveData.StorySubsystemVersion < 1)
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveStorySubsystem::ImportSaveData - Incompatible or invalid save data"));
		return false;
	}

	// Suppress gameplay events during save restoration
	TGuardValue<bool> RestoreGuard(bIsRestoringState, true);

	for (const FShadowSlaveStoryRecordSaveData& SavedRecord : InSaveData.Stories)
	{
		if (SavedRecord.StoryId.IsNone() || SavedRecord.State == EShadowSlaveStoryState::Unknown)
		{
			continue;
		}

		FShadowSlaveStoryRuntimeState& RuntimeEntry = StoryRuntimeStates.FindOrAdd(SavedRecord.StoryId);
		RuntimeEntry.StoryId = SavedRecord.StoryId;
		RuntimeEntry.State = SavedRecord.State;
		RuntimeEntry.CurrentStepId = SavedRecord.CurrentStepId;
		RuntimeEntry.RuntimeMetadata = SavedRecord.RuntimeMetadata;
	}

	return true;
}
