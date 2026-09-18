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
	RegisteredContentDefinitions.Empty();
	StoryContentRuntimeStates.Empty();
	bIsRestoringState = false;
}

void UShadowSlaveStorySubsystem::Deinitialize()
{
	RegisteredDefinitions.Empty();
	StoryRuntimeStates.Empty();
	RegisteredContentDefinitions.Empty();
	StoryContentRuntimeStates.Empty();
	Super::Deinitialize();
}

/* =========================================================================
 * Story Definition Registration & Lifecycle (Existing Story Beats)
 * ========================================================================= */

bool UShadowSlaveStorySubsystem::RegisterStoryDefinition(UShadowSlaveStoryDefinition* StoryDef)
{
	if (!StoryDef)
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveStorySubsystem::RegisterStoryDefinition - StoryDef is null."));
		return false;
	}

	if (StoryDef->StoryId.IsNone())
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveStorySubsystem::RegisterStoryDefinition - StoryDef has invalid None StoryId."));
		return false;
	}

	TArray<FText> ValidationErrors;
	if (!StoryDef->ValidateDefinition(ValidationErrors))
	{
		for (const FText& Err : ValidationErrors)
		{
			UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveStorySubsystem::RegisterStoryDefinition - Validation failure on '%s': %s"),
				*StoryDef->StoryId.ToString(), *Err.ToString());
		}
		return false;
	}

	if (const TObjectPtr<UShadowSlaveStoryDefinition>* Existing = RegisteredDefinitions.Find(StoryDef->StoryId))
	{
		if (Existing->Get() == StoryDef)
		{
			// Idempotent re-registration of the exact same definition object
			return true;
		}

		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveStorySubsystem::RegisterStoryDefinition - Conflict: StoryId '%s' already registered with different definition object ('%s' vs '%s')."),
			*StoryDef->StoryId.ToString(),
			Existing->Get() ? *Existing->Get()->GetName() : TEXT("null"),
			*StoryDef->GetName());
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

/* =========================================================================
 * Story Content Definition Registration (Step 25 Chapters/Arcs)
 * ========================================================================= */

bool UShadowSlaveStorySubsystem::RegisterStoryContentDefinition(UShadowSlaveStoryContentDefinition* ContentDef)
{
	if (!ContentDef)
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveStorySubsystem::RegisterStoryContentDefinition - ContentDef is null."));
		return false;
	}

	if (ContentDef->StoryContentId.IsNone())
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveStorySubsystem::RegisterStoryContentDefinition - ContentDef has invalid None StoryContentId."));
		return false;
	}

	TArray<FText> ValidationErrors;
	if (!ContentDef->ValidateDefinition(ValidationErrors))
	{
		for (const FText& Err : ValidationErrors)
		{
			UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveStorySubsystem::RegisterStoryContentDefinition - Validation failure on '%s': %s"),
				*ContentDef->StoryContentId.ToString(), *Err.ToString());
		}
		return false;
	}

	if (const TObjectPtr<UShadowSlaveStoryContentDefinition>* Existing = RegisteredContentDefinitions.Find(ContentDef->StoryContentId))
	{
		if (Existing->Get() == ContentDef)
		{
			// Idempotent re-registration of exact same object
			return true;
		}

		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveStorySubsystem::RegisterStoryContentDefinition - Conflict: StoryContentId '%s' already registered with different definition object ('%s' vs '%s')."),
			*ContentDef->StoryContentId.ToString(),
			Existing->Get() ? *Existing->Get()->GetName() : TEXT("null"),
			*ContentDef->GetName());
		return false;
	}

	RegisteredContentDefinitions.Add(ContentDef->StoryContentId, ContentDef);

	// If runtime state does not exist yet, initialize it
	if (!StoryContentRuntimeStates.Contains(ContentDef->StoryContentId))
	{
		const EShadowSlaveStoryContentState InitialState = AreStoryContentPrerequisitesSatisfied(ContentDef->StoryContentId)
			? EShadowSlaveStoryContentState::Available
			: EShadowSlaveStoryContentState::Locked;

		FShadowSlaveStoryContentRuntimeState NewEntry(ContentDef->StoryContentId, InitialState);
		InitializeRuntimeContentEntries(NewEntry, ContentDef);
		StoryContentRuntimeStates.Add(ContentDef->StoryContentId, NewEntry);
	}

	return true;
}

bool UShadowSlaveStorySubsystem::UnregisterStoryContentDefinition(FName StoryContentId)
{
	if (StoryContentId.IsNone())
	{
		return false;
	}

	return RegisteredContentDefinitions.Remove(StoryContentId) > 0;
}

UShadowSlaveStoryContentDefinition* UShadowSlaveStorySubsystem::GetStoryContentDefinition(FName StoryContentId) const
{
	if (StoryContentId.IsNone())
	{
		return nullptr;
	}

	if (const TObjectPtr<UShadowSlaveStoryContentDefinition>* Found = RegisteredContentDefinitions.Find(StoryContentId))
	{
		return Found->Get();
	}

	return nullptr;
}

bool UShadowSlaveStorySubsystem::HasStoryContentDefinition(FName StoryContentId) const
{
	return !StoryContentId.IsNone() && RegisteredContentDefinitions.Contains(StoryContentId);
}

/* =========================================================================
 * Story State Queries (Existing Story Beats)
 * ========================================================================= */

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

/* =========================================================================
 * Story Content State Queries (Step 25 Chapters/Arcs)
 * ========================================================================= */

EShadowSlaveStoryContentState UShadowSlaveStorySubsystem::GetStoryContentState(FName StoryContentId) const
{
	if (StoryContentId.IsNone())
	{
		return EShadowSlaveStoryContentState::Unknown;
	}

	if (const FShadowSlaveStoryContentRuntimeState* Found = StoryContentRuntimeStates.Find(StoryContentId))
	{
		return Found->State;
	}

	return EShadowSlaveStoryContentState::Unknown;
}

bool UShadowSlaveStorySubsystem::IsStoryContentActive(FName StoryContentId) const
{
	return GetStoryContentState(StoryContentId) == EShadowSlaveStoryContentState::Active;
}

bool UShadowSlaveStorySubsystem::IsStoryContentCompleted(FName StoryContentId) const
{
	return GetStoryContentState(StoryContentId) == EShadowSlaveStoryContentState::Completed;
}

bool UShadowSlaveStorySubsystem::AreStoryContentPrerequisitesSatisfied(FName StoryContentId) const
{
	if (StoryContentId.IsNone())
	{
		return false;
	}

	const UShadowSlaveStoryContentDefinition* Def = GetStoryContentDefinition(StoryContentId);
	if (!Def)
	{
		return true;
	}

	for (const FName& PrereqId : Def->PrerequisiteStoryContentIds)
	{
		if (PrereqId.IsNone())
		{
			continue;
		}

		if (GetStoryContentState(PrereqId) != EShadowSlaveStoryContentState::Completed)
		{
			return false;
		}
	}

	return true;
}

bool UShadowSlaveStorySubsystem::GetStoryContentRuntimeState(FName StoryContentId, FShadowSlaveStoryContentRuntimeState& OutState) const
{
	if (StoryContentId.IsNone())
	{
		OutState = FShadowSlaveStoryContentRuntimeState();
		return false;
	}

	if (const FShadowSlaveStoryContentRuntimeState* Found = StoryContentRuntimeStates.Find(StoryContentId))
	{
		OutState = *Found;
		return true;
	}

	OutState = FShadowSlaveStoryContentRuntimeState();
	return false;
}

EShadowSlaveStoryContentState UShadowSlaveStorySubsystem::GetStoryContentEntryState(FName StoryContentId, FName EntryId) const
{
	if (StoryContentId.IsNone() || EntryId.IsNone())
	{
		return EShadowSlaveStoryContentState::Unknown;
	}

	if (const FShadowSlaveStoryContentRuntimeState* Found = StoryContentRuntimeStates.Find(StoryContentId))
	{
		if (const FShadowSlaveStoryContentEntryRuntimeState* EntryFound = Found->EntryStates.Find(EntryId))
		{
			return EntryFound->State;
		}
	}

	return EShadowSlaveStoryContentState::Unknown;
}

bool UShadowSlaveStorySubsystem::AreStoryContentEntryPrerequisitesSatisfied(FName StoryContentId, FName EntryId) const
{
	if (StoryContentId.IsNone() || EntryId.IsNone())
	{
		return false;
	}

	const UShadowSlaveStoryContentDefinition* Def = GetStoryContentDefinition(StoryContentId);
	if (!Def)
	{
		return false;
	}

	const FShadowSlaveStoryContentEntry* EntryDef = Def->FindContentEntry(EntryId);
	if (!EntryDef)
	{
		return false;
	}

	for (const FName& PrereqEntryId : EntryDef->PrerequisiteContentIds)
	{
		if (PrereqEntryId.IsNone())
		{
			continue;
		}

		if (GetStoryContentEntryState(StoryContentId, PrereqEntryId) != EShadowSlaveStoryContentState::Completed)
		{
			return false;
		}
	}

	return true;
}

FName UShadowSlaveStorySubsystem::GetCurrentActiveStoryContentEntry(FName StoryContentId) const
{
	if (StoryContentId.IsNone())
	{
		return NAME_None;
	}

	if (const FShadowSlaveStoryContentRuntimeState* Found = StoryContentRuntimeStates.Find(StoryContentId))
	{
		return Found->CurrentActiveEntryId;
	}

	return NAME_None;
}

bool UShadowSlaveStorySubsystem::AreAllRequiredContentEntriesCompleted(FName StoryContentId) const
{
	if (StoryContentId.IsNone())
	{
		return false;
	}

	const UShadowSlaveStoryContentDefinition* Def = GetStoryContentDefinition(StoryContentId);
	if (!Def)
	{
		return false;
	}

	for (const FShadowSlaveStoryContentEntry& Entry : Def->ContentEntries)
	{
		if (!Entry.bIsOptional)
		{
			if (GetStoryContentEntryState(StoryContentId, Entry.ContentId) != EShadowSlaveStoryContentState::Completed)
			{
				return false;
			}
		}
	}

	return true;
}

/* =========================================================================
 * Story State Transitions (Existing Story Beats)
 * ========================================================================= */

bool UShadowSlaveStorySubsystem::CanTransition(FName StoryId, EShadowSlaveStoryState CurrentState, EShadowSlaveStoryState NewState) const
{
	if (NewState == EShadowSlaveStoryState::Unknown)
	{
		return false;
	}

	if (CurrentState == NewState)
	{
		return true;
	}

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

	if (ResetToState != EShadowSlaveStoryState::Locked && ResetToState != EShadowSlaveStoryState::Available)
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveStorySubsystem::ResetStoryState - Rejected invalid reset target state %d for '%s' (only Locked and Available are permitted)."),
			static_cast<uint8>(ResetToState), *StoryId.ToString());
		return false;
	}

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

/* =========================================================================
 * Story Content Transitions (Step 25 Chapters/Arcs)
 * ========================================================================= */

bool UShadowSlaveStorySubsystem::CanTransitionStoryContent(FName StoryContentId, EShadowSlaveStoryContentState CurrentState, EShadowSlaveStoryContentState NewState) const
{
	if (NewState == EShadowSlaveStoryContentState::Unknown)
	{
		return false;
	}

	if (CurrentState == NewState)
	{
		return true;
	}

	if (CurrentState == EShadowSlaveStoryContentState::Completed ||
	    CurrentState == EShadowSlaveStoryContentState::Failed ||
	    CurrentState == EShadowSlaveStoryContentState::Skipped)
	{
		return false;
	}

	switch (CurrentState)
	{
	case EShadowSlaveStoryContentState::Unknown:
		if (NewState == EShadowSlaveStoryContentState::Locked)
		{
			return true;
		}
		if (NewState == EShadowSlaveStoryContentState::Available)
		{
			return AreStoryContentPrerequisitesSatisfied(StoryContentId);
		}
		return false;

	case EShadowSlaveStoryContentState::Locked:
		if (NewState == EShadowSlaveStoryContentState::Available)
		{
			return AreStoryContentPrerequisitesSatisfied(StoryContentId);
		}
		if (NewState == EShadowSlaveStoryContentState::Active)
		{
			return AreStoryContentPrerequisitesSatisfied(StoryContentId);
		}
		return false;

	case EShadowSlaveStoryContentState::Available:
		if (NewState == EShadowSlaveStoryContentState::Active)
		{
			return AreStoryContentPrerequisitesSatisfied(StoryContentId);
		}
		if (NewState == EShadowSlaveStoryContentState::Locked)
		{
			return true;
		}
		return false;

	case EShadowSlaveStoryContentState::Active:
		if (NewState == EShadowSlaveStoryContentState::Completed ||
		    NewState == EShadowSlaveStoryContentState::Failed ||
		    NewState == EShadowSlaveStoryContentState::Skipped)
		{
			return true;
		}
		return false;

	default:
		return false;
	}
}

bool UShadowSlaveStorySubsystem::SetStoryContentState(FName StoryContentId, EShadowSlaveStoryContentState NewState)
{
	if (StoryContentId.IsNone() || NewState == EShadowSlaveStoryContentState::Unknown)
	{
		return false;
	}

	FShadowSlaveStoryContentRuntimeState* FoundState = StoryContentRuntimeStates.Find(StoryContentId);
	if (!FoundState)
	{
		const UShadowSlaveStoryContentDefinition* Def = GetStoryContentDefinition(StoryContentId);
		if (Def)
		{
			FShadowSlaveStoryContentRuntimeState NewEntry(StoryContentId, EShadowSlaveStoryContentState::Locked);
			InitializeRuntimeContentEntries(NewEntry, Def);
			StoryContentRuntimeStates.Add(StoryContentId, NewEntry);
			FoundState = StoryContentRuntimeStates.Find(StoryContentId);
		}
		else
		{
			UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveStorySubsystem::SetStoryContentState - Untracked story content ID '%s'"), *StoryContentId.ToString());
			return false;
		}
	}

	const EShadowSlaveStoryContentState OldState = FoundState->State;
	if (OldState == NewState)
	{
		return true;
	}

	if (!CanTransitionStoryContent(StoryContentId, OldState, NewState))
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveStorySubsystem::SetStoryContentState - Rejected invalid transition for '%s': %d -> %d"),
			*StoryContentId.ToString(), static_cast<uint8>(OldState), static_cast<uint8>(NewState));
		return false;
	}

	FoundState->State = NewState;

	if (!bIsRestoringState)
	{
		OnStoryContentStateChanged.Broadcast(StoryContentId, NewState, OldState);
	}

	return true;
}

bool UShadowSlaveStorySubsystem::ActivateStoryContent(FName StoryContentId)
{
	return SetStoryContentState(StoryContentId, EShadowSlaveStoryContentState::Active);
}

bool UShadowSlaveStorySubsystem::CompleteStoryContent(FName StoryContentId)
{
	return SetStoryContentState(StoryContentId, EShadowSlaveStoryContentState::Completed);
}

bool UShadowSlaveStorySubsystem::FailStoryContent(FName StoryContentId)
{
	return SetStoryContentState(StoryContentId, EShadowSlaveStoryContentState::Failed);
}

bool UShadowSlaveStorySubsystem::SkipStoryContent(FName StoryContentId)
{
	return SetStoryContentState(StoryContentId, EShadowSlaveStoryContentState::Skipped);
}

bool UShadowSlaveStorySubsystem::ResetStoryContentState(FName StoryContentId, EShadowSlaveStoryContentState ResetToState)
{
	if (StoryContentId.IsNone())
	{
		return false;
	}

	if (ResetToState != EShadowSlaveStoryContentState::Locked && ResetToState != EShadowSlaveStoryContentState::Available)
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveStorySubsystem::ResetStoryContentState - Rejected invalid reset target state %d for '%s' (only Locked and Available are permitted)."),
			static_cast<uint8>(ResetToState), *StoryContentId.ToString());
		return false;
	}

	if (ResetToState == EShadowSlaveStoryContentState::Available && !AreStoryContentPrerequisitesSatisfied(StoryContentId))
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveStorySubsystem::ResetStoryContentState - Cannot reset '%s' to Available because prerequisites are not satisfied."),
			*StoryContentId.ToString());
		return false;
	}

	const UShadowSlaveStoryContentDefinition* Def = GetStoryContentDefinition(StoryContentId);

	FShadowSlaveStoryContentRuntimeState* FoundState = StoryContentRuntimeStates.Find(StoryContentId);
	if (!FoundState)
	{
		if (Def)
		{
			FShadowSlaveStoryContentRuntimeState NewEntry(StoryContentId, ResetToState);
			InitializeRuntimeContentEntries(NewEntry, Def);
			StoryContentRuntimeStates.Add(StoryContentId, NewEntry);

			if (!bIsRestoringState)
			{
				OnStoryContentStateChanged.Broadcast(StoryContentId, ResetToState, EShadowSlaveStoryContentState::Unknown);
			}
			return true;
		}
		else
		{
			return false;
		}
	}

	const EShadowSlaveStoryContentState OldState = FoundState->State;
	FoundState->State = ResetToState;
	FoundState->CurrentActiveEntryId = NAME_None;
	if (Def)
	{
		InitializeRuntimeContentEntries(*FoundState, Def);
	}
	else
	{
		for (auto& EntryPair : FoundState->EntryStates)
		{
			EntryPair.Value.State = EShadowSlaveStoryContentState::Locked;
		}
	}

	if (OldState != ResetToState && !bIsRestoringState)
	{
		OnStoryContentStateChanged.Broadcast(StoryContentId, ResetToState, OldState);
	}

	return true;
}

void UShadowSlaveStorySubsystem::ResetAllStoryContentStates()
{
	for (auto& Pair : StoryContentRuntimeStates)
	{
		const EShadowSlaveStoryContentState OldState = Pair.Value.State;
		const EShadowSlaveStoryContentState NewState = AreStoryContentPrerequisitesSatisfied(Pair.Key)
			? EShadowSlaveStoryContentState::Available
			: EShadowSlaveStoryContentState::Locked;

		Pair.Value.State = NewState;
		Pair.Value.CurrentActiveEntryId = NAME_None;

		const UShadowSlaveStoryContentDefinition* Def = GetStoryContentDefinition(Pair.Key);
		if (Def)
		{
			InitializeRuntimeContentEntries(Pair.Value, Def);
		}
		else
		{
			for (auto& EntryPair : Pair.Value.EntryStates)
			{
				EntryPair.Value.State = EShadowSlaveStoryContentState::Locked;
			}
		}

		if (OldState != NewState && !bIsRestoringState)
		{
			OnStoryContentStateChanged.Broadcast(Pair.Key, NewState, OldState);
		}
	}
}

/* =========================================================================
 * Story Content Entry Transitions (Step 25 Chapters/Arcs)
 * ========================================================================= */

bool UShadowSlaveStorySubsystem::CanTransitionStoryContentEntry(
	FName StoryContentId,
	FName EntryId,
	EShadowSlaveStoryContentState CurrentState,
	EShadowSlaveStoryContentState NewState) const
{
	if (NewState == EShadowSlaveStoryContentState::Unknown)
	{
		return false;
	}

	if (CurrentState == NewState)
	{
		return true;
	}

	// Owning story content must be Active
	if (GetStoryContentState(StoryContentId) != EShadowSlaveStoryContentState::Active)
	{
		return false;
	}

	// Terminal states cannot transition away through normal transitions
	if (CurrentState == EShadowSlaveStoryContentState::Completed ||
	    CurrentState == EShadowSlaveStoryContentState::Failed ||
	    CurrentState == EShadowSlaveStoryContentState::Skipped)
	{
		return false;
	}

	switch (CurrentState)
	{
	case EShadowSlaveStoryContentState::Unknown:
		if (NewState == EShadowSlaveStoryContentState::Locked)
		{
			return true;
		}
		if (NewState == EShadowSlaveStoryContentState::Available)
		{
			return AreStoryContentEntryPrerequisitesSatisfied(StoryContentId, EntryId);
		}
		return false;

	case EShadowSlaveStoryContentState::Locked:
		if (NewState == EShadowSlaveStoryContentState::Available)
		{
			return AreStoryContentEntryPrerequisitesSatisfied(StoryContentId, EntryId);
		}
		if (NewState == EShadowSlaveStoryContentState::Active)
		{
			return AreStoryContentEntryPrerequisitesSatisfied(StoryContentId, EntryId);
		}
		return false;

	case EShadowSlaveStoryContentState::Available:
		if (NewState == EShadowSlaveStoryContentState::Active)
		{
			return AreStoryContentEntryPrerequisitesSatisfied(StoryContentId, EntryId);
		}
		if (NewState == EShadowSlaveStoryContentState::Locked)
		{
			return true;
		}
		return false;

	case EShadowSlaveStoryContentState::Active:
		if (NewState == EShadowSlaveStoryContentState::Completed ||
		    NewState == EShadowSlaveStoryContentState::Failed ||
		    NewState == EShadowSlaveStoryContentState::Skipped)
		{
			return true;
		}
		return false;

	default:
		return false;
	}
}

bool UShadowSlaveStorySubsystem::SetStoryContentEntryState(FName StoryContentId, FName EntryId, EShadowSlaveStoryContentState NewState)
{
	if (StoryContentId.IsNone() || EntryId.IsNone() || NewState == EShadowSlaveStoryContentState::Unknown)
	{
		return false;
	}

	const UShadowSlaveStoryContentDefinition* Def = GetStoryContentDefinition(StoryContentId);
	if (!Def)
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveStorySubsystem::SetStoryContentEntryState - Missing story content definition for '%s'"),
			*StoryContentId.ToString());
		return false;
	}

	if (!Def->HasContentEntry(EntryId))
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveStorySubsystem::SetStoryContentEntryState - Entry '%s' not found in definition '%s'"),
			*EntryId.ToString(), *StoryContentId.ToString());
		return false;
	}

	FShadowSlaveStoryContentRuntimeState* FoundArcState = StoryContentRuntimeStates.Find(StoryContentId);
	if (!FoundArcState)
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveStorySubsystem::SetStoryContentEntryState - Untracked story content runtime state for '%s'"),
			*StoryContentId.ToString());
		return false;
	}

	FShadowSlaveStoryContentEntryRuntimeState* EntryState = FoundArcState->EntryStates.Find(EntryId);
	if (!EntryState)
	{
		FShadowSlaveStoryContentEntryRuntimeState NewEntry(EntryId, EShadowSlaveStoryContentState::Locked);
		FoundArcState->EntryStates.Add(EntryId, NewEntry);
		EntryState = FoundArcState->EntryStates.Find(EntryId);
	}

	const EShadowSlaveStoryContentState OldState = EntryState->State;
	if (OldState == NewState)
	{
		return true;
	}

	if (!CanTransitionStoryContentEntry(StoryContentId, EntryId, OldState, NewState))
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveStorySubsystem::SetStoryContentEntryState - Rejected invalid entry transition for '%s'::'%s': %d -> %d"),
			*StoryContentId.ToString(), *EntryId.ToString(), static_cast<uint8>(OldState), static_cast<uint8>(NewState));
		return false;
	}

	EntryState->State = NewState;

	// Update active entry reference deterministically
	if (NewState == EShadowSlaveStoryContentState::Active)
	{
		FoundArcState->CurrentActiveEntryId = EntryId;
	}
	else if (FoundArcState->CurrentActiveEntryId == EntryId &&
	         (NewState == EShadowSlaveStoryContentState::Completed ||
	          NewState == EShadowSlaveStoryContentState::Failed ||
	          NewState == EShadowSlaveStoryContentState::Skipped))
	{
		FoundArcState->CurrentActiveEntryId = NAME_None;
	}

	if (!bIsRestoringState)
	{
		OnStoryContentEntryStateChanged.Broadcast(StoryContentId, EntryId, NewState, OldState);
	}

	return true;
}

bool UShadowSlaveStorySubsystem::ActivateStoryContentEntry(FName StoryContentId, FName EntryId)
{
	return SetStoryContentEntryState(StoryContentId, EntryId, EShadowSlaveStoryContentState::Active);
}

bool UShadowSlaveStorySubsystem::CompleteStoryContentEntry(FName StoryContentId, FName EntryId)
{
	return SetStoryContentEntryState(StoryContentId, EntryId, EShadowSlaveStoryContentState::Completed);
}

bool UShadowSlaveStorySubsystem::FailStoryContentEntry(FName StoryContentId, FName EntryId)
{
	return SetStoryContentEntryState(StoryContentId, EntryId, EShadowSlaveStoryContentState::Failed);
}

bool UShadowSlaveStorySubsystem::SkipStoryContentEntry(FName StoryContentId, FName EntryId)
{
	return SetStoryContentEntryState(StoryContentId, EntryId, EShadowSlaveStoryContentState::Skipped);
}

bool UShadowSlaveStorySubsystem::SetCurrentActiveStoryContentEntry(FName StoryContentId, FName EntryId)
{
	if (StoryContentId.IsNone())
	{
		return false;
	}

	FShadowSlaveStoryContentRuntimeState* FoundArcState = StoryContentRuntimeStates.Find(StoryContentId);
	if (!FoundArcState)
	{
		return false;
	}

	if (FoundArcState->State != EShadowSlaveStoryContentState::Active)
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveStorySubsystem::SetCurrentActiveStoryContentEntry - Cannot set active entry on inactive story content '%s'"),
			*StoryContentId.ToString());
		return false;
	}

	if (EntryId.IsNone())
	{
		FoundArcState->CurrentActiveEntryId = NAME_None;
		return true;
	}

	const UShadowSlaveStoryContentDefinition* Def = GetStoryContentDefinition(StoryContentId);
	if (!Def || !Def->HasContentEntry(EntryId))
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveStorySubsystem::SetCurrentActiveStoryContentEntry - Entry '%s' not in definition '%s'"),
			*EntryId.ToString(), *StoryContentId.ToString());
		return false;
	}

	return ActivateStoryContentEntry(StoryContentId, EntryId);
}

void UShadowSlaveStorySubsystem::InitializeRuntimeContentEntries(
	FShadowSlaveStoryContentRuntimeState& RuntimeState,
	const UShadowSlaveStoryContentDefinition* ContentDef) const
{
	RuntimeState.EntryStates.Empty();
	RuntimeState.CurrentActiveEntryId = NAME_None;

	if (!ContentDef)
	{
		return;
	}

	for (const FShadowSlaveStoryContentEntry& Entry : ContentDef->ContentEntries)
	{
		if (Entry.ContentId.IsNone())
		{
			continue;
		}

		FShadowSlaveStoryContentEntryRuntimeState EntryState(Entry.ContentId, EShadowSlaveStoryContentState::Locked);
		RuntimeState.EntryStates.Add(Entry.ContentId, EntryState);
	}
}

/* =========================================================================
 * Story Step Support (Existing Story Beats)
 * ========================================================================= */

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

/* =========================================================================
 * Save & Persistence
 * ========================================================================= */

FShadowSlaveStorySaveData UShadowSlaveStorySubsystem::ExportSaveData() const
{
	FShadowSlaveStorySaveData SaveData;
	SaveData.StorySubsystemVersion = 1;
	SaveData.bIsValid = true;

	// 1. Stories: sort story IDs alphabetically for deterministic export
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

	// 2. StoryContents: sort content IDs alphabetically for deterministic export
	TArray<FName> SortedContentIds;
	StoryContentRuntimeStates.GetKeys(SortedContentIds);
	SortedContentIds.Sort([](const FName& A, const FName& B) {
		return A.Compare(B) < 0;
	});

	for (const FName& ContentId : SortedContentIds)
	{
		if (const FShadowSlaveStoryContentRuntimeState* State = StoryContentRuntimeStates.Find(ContentId))
		{
			const UShadowSlaveStoryContentDefinition* Def = GetStoryContentDefinition(ContentId);
			const int32 DefVersion = Def ? Def->Version : 1;
			SaveData.StoryContents.Add(FShadowSlaveStoryContentRecordSaveData(*State, DefVersion));
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

	// 1. Restore story beats
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

	// 2. Restore story content (chapters/arcs)
	for (const FShadowSlaveStoryContentRecordSaveData& SavedContentRecord : InSaveData.StoryContents)
	{
		if (SavedContentRecord.StoryContentId.IsNone() || SavedContentRecord.State == EShadowSlaveStoryContentState::Unknown)
		{
			continue;
		}

		// Resolve against current registered definition
		const UShadowSlaveStoryContentDefinition* Def = GetStoryContentDefinition(SavedContentRecord.StoryContentId);
		if (!Def)
		{
			UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveStorySubsystem::ImportSaveData - Missing StoryContentDefinition for saved content '%s'; skipping."),
				*SavedContentRecord.StoryContentId.ToString());
			continue;
		}

		// Version verification
		if (SavedContentRecord.ContentVersion != Def->Version)
		{
			UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveStorySubsystem::ImportSaveData - Version mismatch for story content '%s' (Saved: %d, Def: %d)."),
				*SavedContentRecord.StoryContentId.ToString(), SavedContentRecord.ContentVersion, Def->Version);
		}

		// Create or find runtime entry
		FShadowSlaveStoryContentRuntimeState& RuntimeEntry = StoryContentRuntimeStates.FindOrAdd(SavedContentRecord.StoryContentId);
		RuntimeEntry.StoryContentId = SavedContentRecord.StoryContentId;
		RuntimeEntry.State = SavedContentRecord.State;
		RuntimeEntry.RuntimeMetadata = SavedContentRecord.RuntimeMetadata;

		// Initialize all entries from definition first
		InitializeRuntimeContentEntries(RuntimeEntry, Def);

		// Restore saved child entries that exist in definition
		for (const auto& EntryPair : SavedContentRecord.EntryStates)
		{
			const FName EntryId = EntryPair.Key;
			const EShadowSlaveStoryContentState SavedEntryState = EntryPair.Value;

			if (EntryId.IsNone() || SavedEntryState == EShadowSlaveStoryContentState::Unknown)
			{
				continue;
			}

			if (!Def->HasContentEntry(EntryId))
			{
				UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveStorySubsystem::ImportSaveData - Saved content entry '%s' does not exist in definition '%s'; skipping."),
					*EntryId.ToString(), *SavedContentRecord.StoryContentId.ToString());
				continue;
			}

			if (FShadowSlaveStoryContentEntryRuntimeState* EntryRuntime = RuntimeEntry.EntryStates.Find(EntryId))
			{
				EntryRuntime->State = SavedEntryState;
			}
		}

		// Restore CurrentActiveEntryId if valid in definition
		if (!SavedContentRecord.CurrentActiveEntryId.IsNone())
		{
			if (Def->HasContentEntry(SavedContentRecord.CurrentActiveEntryId))
			{
				RuntimeEntry.CurrentActiveEntryId = SavedContentRecord.CurrentActiveEntryId;
			}
			else
			{
				RuntimeEntry.CurrentActiveEntryId = NAME_None;
			}
		}
		else
		{
			RuntimeEntry.CurrentActiveEntryId = NAME_None;
		}
	}

	return true;
}
