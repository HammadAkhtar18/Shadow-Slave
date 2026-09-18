// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gameplay/ShadowSlaveQuestSubsystem.h"
#include "Story/ShadowSlaveStorySubsystem.h"
#include "Dialogue/ShadowSlaveConversationSubsystem.h"
#include "Nightmares/ShadowSlaveNightmareSubsystem.h"
#include "Nightmares/ShadowSlaveNightmareScenarioDefinition.h"
#include "Items/ShadowSlaveInventoryComponent.h"
#include "Items/ShadowSlaveItemDefinition.h"
#include "Interaction/ShadowSlaveInteractionComponent.h"
#include "Interaction/ShadowSlaveInteractableActor.h"
#include "Interaction/ShadowSlaveInteractableNPC.h"
#include "World/ShadowSlaveWorldStateComponent.h"
#include "Characters/ShadowSlaveCharacterBase.h"
#include "Save/ShadowSlaveSaveableInterface.h"
#include "Subsystems/SubsystemCollection.h"
#include "Engine/GameInstance.h"
#include "GameFramework/Pawn.h"
#include "ShadowSlave.h"

namespace
{
	/** Helper to parse expected world value string according to type rules and fail closed on invalid format */
	static bool TryParseExpectedWorldValue(
		const FString& InValueStr,
		const FString* InExplicitTypeStr,
		EShadowSlaveWorldValueType FallbackType,
		FShadowSlaveWorldValue& OutParsedValue)
	{
		if (InValueStr.IsEmpty() || InValueStr.Equals(TEXT("none"), ESearchCase::IgnoreCase))
		{
			return false;
		}

		EShadowSlaveWorldValueType ExpectedType = EShadowSlaveWorldValueType::None;
		FString Payload = InValueStr;

		// 1. Check for standard serialization prefix: "b:", "i:", "f:", "s:", "n:"
		if (InValueStr.Len() >= 2 && InValueStr[1] == TEXT(':'))
		{
			const TCHAR Prefix = InValueStr[0];
			switch (Prefix)
			{
			case TEXT('b'): ExpectedType = EShadowSlaveWorldValueType::Bool; break;
			case TEXT('i'): ExpectedType = EShadowSlaveWorldValueType::Int; break;
			case TEXT('f'): ExpectedType = EShadowSlaveWorldValueType::Float; break;
			case TEXT('s'): ExpectedType = EShadowSlaveWorldValueType::String; break;
			case TEXT('n'): ExpectedType = EShadowSlaveWorldValueType::Name; break;
			default: break;
			}

			if (ExpectedType != EShadowSlaveWorldValueType::None)
			{
				Payload = InValueStr.RightChop(2);
			}
		}

		// 2. If no prefix, check explicit type string from metadata
		if (ExpectedType == EShadowSlaveWorldValueType::None && InExplicitTypeStr && !InExplicitTypeStr->IsEmpty())
		{
			if (InExplicitTypeStr->Equals(TEXT("Bool"), ESearchCase::IgnoreCase) || InExplicitTypeStr->Equals(TEXT("Boolean"), ESearchCase::IgnoreCase))
			{
				ExpectedType = EShadowSlaveWorldValueType::Bool;
			}
			else if (InExplicitTypeStr->Equals(TEXT("Int"), ESearchCase::IgnoreCase) || InExplicitTypeStr->Equals(TEXT("Integer"), ESearchCase::IgnoreCase))
			{
				ExpectedType = EShadowSlaveWorldValueType::Int;
			}
			else if (InExplicitTypeStr->Equals(TEXT("Float"), ESearchCase::IgnoreCase))
			{
				ExpectedType = EShadowSlaveWorldValueType::Float;
			}
			else if (InExplicitTypeStr->Equals(TEXT("String"), ESearchCase::IgnoreCase))
			{
				ExpectedType = EShadowSlaveWorldValueType::String;
			}
			else if (InExplicitTypeStr->Equals(TEXT("Name"), ESearchCase::IgnoreCase))
			{
				ExpectedType = EShadowSlaveWorldValueType::Name;
			}
			else
			{
				return false; // Unknown explicit type -> unparseable
			}
		}

		// 3. If still undetermined, infer type from literal format or fallback to matching runtime type
		if (ExpectedType == EShadowSlaveWorldValueType::None)
		{
			if (InValueStr.Equals(TEXT("true"), ESearchCase::IgnoreCase) || InValueStr.Equals(TEXT("false"), ESearchCase::IgnoreCase))
			{
				ExpectedType = EShadowSlaveWorldValueType::Bool;
			}
			else if (InValueStr.IsNumeric())
			{
				ExpectedType = InValueStr.Contains(TEXT(".")) ? EShadowSlaveWorldValueType::Float : EShadowSlaveWorldValueType::Int;
			}
			else if (FallbackType == EShadowSlaveWorldValueType::Name)
			{
				ExpectedType = EShadowSlaveWorldValueType::Name;
			}
			else
			{
				ExpectedType = EShadowSlaveWorldValueType::String;
			}
		}

		// 4. Parse payload into specified type with strict validation
		switch (ExpectedType)
		{
		case EShadowSlaveWorldValueType::Bool:
		{
			if (Payload.Equals(TEXT("true"), ESearchCase::IgnoreCase) || Payload == TEXT("1"))
			{
				OutParsedValue = FShadowSlaveWorldValue::MakeBool(true);
				return true;
			}
			if (Payload.Equals(TEXT("false"), ESearchCase::IgnoreCase) || Payload == TEXT("0"))
			{
				OutParsedValue = FShadowSlaveWorldValue::MakeBool(false);
				return true;
			}
			return false; // Unparseable bool
		}
		case EShadowSlaveWorldValueType::Int:
		{
			const FString Trimmed = Payload.TrimStartAndEnd();
			if (Trimmed.IsEmpty())
			{
				return false;
			}
			const int32 StartIdx = (Trimmed[0] == TEXT('-') || Trimmed[0] == TEXT('+')) ? 1 : 0;
			if (StartIdx >= Trimmed.Len())
			{
				return false;
			}
			for (int32 i = StartIdx; i < Trimmed.Len(); ++i)
			{
				if (!FChar::IsDigit(Trimmed[i]))
				{
					return false;
				}
			}
			OutParsedValue = FShadowSlaveWorldValue::MakeInt(FCString::Atoi(*Trimmed));
			return true;
		}
		case EShadowSlaveWorldValueType::Float:
		{
			const FString Trimmed = Payload.TrimStartAndEnd();
			if (Trimmed.IsEmpty() || !Trimmed.IsNumeric())
			{
				return false;
			}
			OutParsedValue = FShadowSlaveWorldValue::MakeFloat(FCString::Atof(*Trimmed));
			return true;
		}
		case EShadowSlaveWorldValueType::String:
		{
			OutParsedValue = FShadowSlaveWorldValue::MakeString(Payload);
			return true;
		}
		case EShadowSlaveWorldValueType::Name:
		{
			if (Payload.IsEmpty() || Payload.Equals(TEXT("none"), ESearchCase::IgnoreCase))
			{
				return false;
			}
			OutParsedValue = FShadowSlaveWorldValue::MakeName(FName(*Payload));
			return true;
		}
		default:
			return false;
		}
	}
}

UShadowSlaveQuestSubsystem::UShadowSlaveQuestSubsystem()
	: bIsRestoringState(false)
	, bIsProcessingTransition(false)
{
}

void UShadowSlaveQuestSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Collection.InitializeDependency<UShadowSlaveStorySubsystem>();
	Collection.InitializeDependency<UShadowSlaveConversationSubsystem>();
	Collection.InitializeDependency<UShadowSlaveNightmareSubsystem>();
	Super::Initialize(Collection);

	RegisteredDefinitions.Empty();
	QuestRuntimeStates.Empty();
	bIsRestoringState = false;
	bIsProcessingTransition = false;

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UShadowSlaveConversationSubsystem* ConvSub = GI->GetSubsystem<UShadowSlaveConversationSubsystem>())
		{
			ConvSub->OnConversationCompleted.AddUniqueDynamic(this, &UShadowSlaveQuestSubsystem::HandleConversationCompleted);
		}

		if (UShadowSlaveNightmareSubsystem* NightmareSub = GI->GetSubsystem<UShadowSlaveNightmareSubsystem>())
		{
			NightmareSub->OnScenarioCompleted.AddUniqueDynamic(this, &UShadowSlaveQuestSubsystem::HandleNightmareScenarioCompleted);
		}
	}

	OnQuestCompleted.AddUniqueDynamic(this, &UShadowSlaveQuestSubsystem::HandleSelfQuestCompleted);
}

void UShadowSlaveQuestSubsystem::Deinitialize()
{
	UnregisterPlayerContext();

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UShadowSlaveConversationSubsystem* ConvSub = GI->GetSubsystem<UShadowSlaveConversationSubsystem>())
		{
			ConvSub->OnConversationCompleted.RemoveDynamic(this, &UShadowSlaveQuestSubsystem::HandleConversationCompleted);
		}

		if (UShadowSlaveNightmareSubsystem* NightmareSub = GI->GetSubsystem<UShadowSlaveNightmareSubsystem>())
		{
			NightmareSub->OnScenarioCompleted.RemoveDynamic(this, &UShadowSlaveQuestSubsystem::HandleNightmareScenarioCompleted);
		}
	}

	OnQuestCompleted.RemoveDynamic(this, &UShadowSlaveQuestSubsystem::HandleSelfQuestCompleted);

	for (auto It = RegisteredInventorySources.CreateIterator(); It; ++It)
	{
		if (UShadowSlaveInventoryComponent* InvComp = It->Get())
		{
			InvComp->OnItemAdded.RemoveDynamic(this, &UShadowSlaveQuestSubsystem::HandleInventoryItemAdded);
		}
	}
	RegisteredInventorySources.Empty();

	for (auto It = RegisteredInteractionSources.CreateIterator(); It; ++It)
	{
		if (UShadowSlaveInteractionComponent* InterComp = It->Get())
		{
			InterComp->OnInteracted.RemoveDynamic(this, &UShadowSlaveQuestSubsystem::HandleInteractionExecuted);
		}
	}
	RegisteredInteractionSources.Empty();

	for (auto It = RegisteredWorldStateSources.CreateIterator(); It; ++It)
	{
		if (UShadowSlaveWorldStateComponent* WSComp = It->Get())
		{
			WSComp->OnWorldStateChanged.RemoveDynamic(this, &UShadowSlaveQuestSubsystem::HandleWorldStateChanged);
		}
	}
	RegisteredWorldStateSources.Empty();

	for (auto It = RegisteredCharacterSources.CreateIterator(); It; ++It)
	{
		if (AShadowSlaveCharacterBase* Char = It->Get())
		{
			Char->OnCharacterDied.RemoveDynamic(this, &UShadowSlaveQuestSubsystem::HandleCharacterDied);
		}
	}
	RegisteredCharacterSources.Empty();

	ProcessedDefeatedActors.Empty();
	LastInteractionFrames.Empty();
	LastConversationFrames.Empty();

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

	ProcessedDefeatedActors.Empty();
	LastInteractionFrames.Empty();
	LastConversationFrames.Empty();
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

	// Pass 2: Reconcile and sanitize quest states against prerequisites and restored objectives
	// Phase 2A: Resolve terminal states and Active-to-terminal normalizations first so prerequisite checks can observe them
	TMap<FName, EShadowSlaveQuestState> FinalQuestStates;

	for (const auto& Pair : DesiredQuestStates)
	{
		const FName QuestId = Pair.Key;
		const EShadowSlaveQuestState DesiredState = Pair.Value;
		FShadowSlaveQuestRuntimeState* RuntimeEntry = QuestRuntimeStates.Find(QuestId);
		if (!RuntimeEntry)
		{
			continue;
		}

		const UShadowSlaveQuestDefinition* Def = GetQuestDefinition(QuestId);
		if (!Def)
		{
			continue;
		}

		if (DesiredState == EShadowSlaveQuestState::Completed ||
		    DesiredState == EShadowSlaveQuestState::Failed ||
		    DesiredState == EShadowSlaveQuestState::Abandoned)
		{
			FinalQuestStates.Add(QuestId, DesiredState);
			RuntimeEntry->State = DesiredState;
		}
		else if (DesiredState == EShadowSlaveQuestState::Active)
		{
			// Case 1 — Mandatory objective failed:
			// If any non-optional objective is Failed, restore the quest as Failed
			if (RuntimeEntry->HasAnyRequiredObjectiveFailed())
			{
				UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveQuestSubsystem::ImportSaveData - Quest '%s' was saved as Active but has failed mandatory objectives; normalizing to Failed."),
					*QuestId.ToString());
				FinalQuestStates.Add(QuestId, EShadowSlaveQuestState::Failed);
				RuntimeEntry->State = EShadowSlaveQuestState::Failed;
			}
			// Case 2 — All required objectives completed:
			// If all non-optional objectives are Completed and auto-complete is enabled, restore as Completed
			else if (RuntimeEntry->AreAllRequiredObjectivesComplete() && Def->bAutoCompleteWhenObjectivesComplete)
			{
				UE_LOG(LogShadowSlave, Log, TEXT("UShadowSlaveQuestSubsystem::ImportSaveData - Quest '%s' was saved as Active with all required objectives completed; normalizing to Completed."),
					*QuestId.ToString());
				FinalQuestStates.Add(QuestId, EShadowSlaveQuestState::Completed);
				RuntimeEntry->State = EShadowSlaveQuestState::Completed;
			}
		}
	}

	// Phase 2B: Resolve remaining non-terminal quests with prerequisite evaluation and objective normalization
	for (const auto& Pair : DesiredQuestStates)
	{
		const FName QuestId = Pair.Key;
		const EShadowSlaveQuestState DesiredState = Pair.Value;
		FShadowSlaveQuestRuntimeState* RuntimeEntry = QuestRuntimeStates.Find(QuestId);
		if (!RuntimeEntry)
		{
			continue;
		}

		// If already finalized in Phase 2A (terminal), skip
		if (FinalQuestStates.Contains(QuestId))
		{
			continue;
		}

		const UShadowSlaveQuestDefinition* Def = GetQuestDefinition(QuestId);
		if (!Def)
		{
			continue;
		}

		if (DesiredState == EShadowSlaveQuestState::Active)
		{
			// Case 3 — Quest remains Active
			// Prerequisites must be satisfied to restore as Active
			if (ArePrerequisitesSatisfied(QuestId))
			{
				RuntimeEntry->State = EShadowSlaveQuestState::Active;

				// Under an Active quest: every objective that is neither Completed nor Failed must be Active.
				// No objective may remain Inactive under an Active quest.
				for (auto& ObjPair : RuntimeEntry->ObjectiveStates)
				{
					if (ObjPair.Value.State != EShadowSlaveObjectiveState::Completed &&
					    ObjPair.Value.State != EShadowSlaveObjectiveState::Failed)
					{
						ObjPair.Value.State = EShadowSlaveObjectiveState::Active;
					}
				}
			}
			else
			{
				UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveQuestSubsystem::ImportSaveData - Quest '%s' was saved as Active but prerequisites are not satisfied; falling back to Locked."),
					*QuestId.ToString());
				RuntimeEntry->State = EShadowSlaveQuestState::Locked;

				// Case 4 — Quest is Locked: objectives that are not terminal should be Inactive
				for (auto& ObjPair : RuntimeEntry->ObjectiveStates)
				{
					if (ObjPair.Value.State != EShadowSlaveObjectiveState::Completed &&
					    ObjPair.Value.State != EShadowSlaveObjectiveState::Failed)
					{
						ObjPair.Value.State = EShadowSlaveObjectiveState::Inactive;
					}
				}
			}
		}
		else if (DesiredState == EShadowSlaveQuestState::Available)
		{
			if (ArePrerequisitesSatisfied(QuestId))
			{
				RuntimeEntry->State = EShadowSlaveQuestState::Available;
			}
			else
			{
				UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveQuestSubsystem::ImportSaveData - Quest '%s' was saved as Available but prerequisites are not satisfied; falling back to Locked."),
					*QuestId.ToString());
				RuntimeEntry->State = EShadowSlaveQuestState::Locked;
			}

			// Case 4 — Quest is Available or Locked: objectives that are not terminal should be Inactive
			for (auto& ObjPair : RuntimeEntry->ObjectiveStates)
			{
				if (ObjPair.Value.State != EShadowSlaveObjectiveState::Completed &&
				    ObjPair.Value.State != EShadowSlaveObjectiveState::Failed)
				{
					ObjPair.Value.State = EShadowSlaveObjectiveState::Inactive;
				}
			}
		}
		else if (DesiredState == EShadowSlaveQuestState::Locked)
		{
			RuntimeEntry->State = EShadowSlaveQuestState::Locked;

			// Case 4 — Quest is Locked: objectives that are not terminal should be Inactive
			for (auto& ObjPair : RuntimeEntry->ObjectiveStates)
			{
				if (ObjPair.Value.State != EShadowSlaveObjectiveState::Completed &&
				    ObjPair.Value.State != EShadowSlaveObjectiveState::Failed)
				{
					ObjPair.Value.State = EShadowSlaveObjectiveState::Inactive;
				}
			}
		}
		else
		{
			// Malformed state: fall back to Locked with Inactive objectives
			RuntimeEntry->State = EShadowSlaveQuestState::Locked;
			for (auto& ObjPair : RuntimeEntry->ObjectiveStates)
			{
				if (ObjPair.Value.State != EShadowSlaveObjectiveState::Completed &&
				    ObjPair.Value.State != EShadowSlaveObjectiveState::Failed)
				{
					ObjPair.Value.State = EShadowSlaveObjectiveState::Inactive;
				}
			}
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

bool UShadowSlaveQuestSubsystem::RegisterPlayerContext(APawn* PlayerPawn)
{
	if (!PlayerPawn)
	{
		return false;
	}

	UnregisterPlayerContext();

	CurrentPlayerPawn = PlayerPawn;

	if (AShadowSlaveCharacterBase* Char = Cast<AShadowSlaveCharacterBase>(PlayerPawn))
	{
		RegisterCharacterSource(Char);
	}

	if (UShadowSlaveInventoryComponent* InvComp = PlayerPawn->FindComponentByClass<UShadowSlaveInventoryComponent>())
	{
		RegisterInventorySource(InvComp);
	}

	if (UShadowSlaveInteractionComponent* InterComp = PlayerPawn->FindComponentByClass<UShadowSlaveInteractionComponent>())
	{
		RegisterInteractionSource(InterComp);
	}

	if (UShadowSlaveWorldStateComponent* WSComp = PlayerPawn->FindComponentByClass<UShadowSlaveWorldStateComponent>())
	{
		RegisterWorldStateSource(WSComp);
	}

	return true;
}

void UShadowSlaveQuestSubsystem::UnregisterPlayerContext()
{
	if (CurrentPlayerPawn.IsValid())
	{
		if (APawn* Pawn = CurrentPlayerPawn.Get())
		{
			if (AShadowSlaveCharacterBase* Char = Cast<AShadowSlaveCharacterBase>(Pawn))
			{
				UnregisterCharacterSource(Char);
			}

			if (UShadowSlaveInventoryComponent* InvComp = Pawn->FindComponentByClass<UShadowSlaveInventoryComponent>())
			{
				UnregisterInventorySource(InvComp);
			}

			if (UShadowSlaveInteractionComponent* InterComp = Pawn->FindComponentByClass<UShadowSlaveInteractionComponent>())
			{
				UnregisterInteractionSource(InterComp);
			}

			if (UShadowSlaveWorldStateComponent* WSComp = Pawn->FindComponentByClass<UShadowSlaveWorldStateComponent>())
			{
				UnregisterWorldStateSource(WSComp);
			}
		}
		CurrentPlayerPawn.Reset();
	}
}

void UShadowSlaveQuestSubsystem::RegisterInventorySource(UShadowSlaveInventoryComponent* InventoryComponent)
{
	if (!InventoryComponent)
	{
		return;
	}

	TWeakObjectPtr<UShadowSlaveInventoryComponent> WeakComp(InventoryComponent);
	if (!RegisteredInventorySources.Contains(WeakComp))
	{
		RegisteredInventorySources.Add(WeakComp);
		InventoryComponent->OnItemAdded.AddUniqueDynamic(this, &UShadowSlaveQuestSubsystem::HandleInventoryItemAdded);
	}
}

void UShadowSlaveQuestSubsystem::UnregisterInventorySource(UShadowSlaveInventoryComponent* InventoryComponent)
{
	if (!InventoryComponent)
	{
		return;
	}

	TWeakObjectPtr<UShadowSlaveInventoryComponent> WeakComp(InventoryComponent);
	if (RegisteredInventorySources.Contains(WeakComp))
	{
		InventoryComponent->OnItemAdded.RemoveDynamic(this, &UShadowSlaveQuestSubsystem::HandleInventoryItemAdded);
		RegisteredInventorySources.Remove(WeakComp);
	}
}

void UShadowSlaveQuestSubsystem::RegisterInteractionSource(UShadowSlaveInteractionComponent* InteractionComponent)
{
	if (!InteractionComponent)
	{
		return;
	}

	TWeakObjectPtr<UShadowSlaveInteractionComponent> WeakComp(InteractionComponent);
	if (!RegisteredInteractionSources.Contains(WeakComp))
	{
		RegisteredInteractionSources.Add(WeakComp);
		InteractionComponent->OnInteracted.AddUniqueDynamic(this, &UShadowSlaveQuestSubsystem::HandleInteractionExecuted);
	}
}

void UShadowSlaveQuestSubsystem::UnregisterInteractionSource(UShadowSlaveInteractionComponent* InteractionComponent)
{
	if (!InteractionComponent)
	{
		return;
	}

	TWeakObjectPtr<UShadowSlaveInteractionComponent> WeakComp(InteractionComponent);
	if (RegisteredInteractionSources.Contains(WeakComp))
	{
		InteractionComponent->OnInteracted.RemoveDynamic(this, &UShadowSlaveQuestSubsystem::HandleInteractionExecuted);
		RegisteredInteractionSources.Remove(WeakComp);
	}
}

void UShadowSlaveQuestSubsystem::RegisterWorldStateSource(UShadowSlaveWorldStateComponent* WorldStateComponent)
{
	if (!WorldStateComponent)
	{
		return;
	}

	TWeakObjectPtr<UShadowSlaveWorldStateComponent> WeakComp(WorldStateComponent);
	if (!RegisteredWorldStateSources.Contains(WeakComp))
	{
		RegisteredWorldStateSources.Add(WeakComp);
		WorldStateComponent->OnWorldStateChanged.AddUniqueDynamic(this, &UShadowSlaveQuestSubsystem::HandleWorldStateChanged);
	}
}

void UShadowSlaveQuestSubsystem::UnregisterWorldStateSource(UShadowSlaveWorldStateComponent* WorldStateComponent)
{
	if (!WorldStateComponent)
	{
		return;
	}

	TWeakObjectPtr<UShadowSlaveWorldStateComponent> WeakComp(WorldStateComponent);
	if (RegisteredWorldStateSources.Contains(WeakComp))
	{
		WorldStateComponent->OnWorldStateChanged.RemoveDynamic(this, &UShadowSlaveQuestSubsystem::HandleWorldStateChanged);
		RegisteredWorldStateSources.Remove(WeakComp);
	}
}

void UShadowSlaveQuestSubsystem::RegisterCharacterSource(AShadowSlaveCharacterBase* Character)
{
	if (!Character)
	{
		return;
	}

	TWeakObjectPtr<AShadowSlaveCharacterBase> WeakChar(Character);
	if (!RegisteredCharacterSources.Contains(WeakChar))
	{
		RegisteredCharacterSources.Add(WeakChar);
		Character->OnCharacterDied.AddUniqueDynamic(this, &UShadowSlaveQuestSubsystem::HandleCharacterDied);
	}
}

void UShadowSlaveQuestSubsystem::UnregisterCharacterSource(AShadowSlaveCharacterBase* Character)
{
	if (!Character)
	{
		return;
	}

	TWeakObjectPtr<AShadowSlaveCharacterBase> WeakChar(Character);
	if (RegisteredCharacterSources.Contains(WeakChar))
	{
		Character->OnCharacterDied.RemoveDynamic(this, &UShadowSlaveQuestSubsystem::HandleCharacterDied);
		RegisteredCharacterSources.Remove(WeakChar);
	}
}

void UShadowSlaveQuestSubsystem::HandleCharacterDied(AShadowSlaveCharacterBase* DeadCharacter, AActor* KillerActor)
{
	if (!DeadCharacter)
	{
		return;
	}

	const FName CharId = DeadCharacter->GetCharacterId();
	NotifyTargetDefeated(CharId, DeadCharacter, KillerActor);
	UnregisterCharacterSource(DeadCharacter);
}

bool UShadowSlaveQuestSubsystem::NotifyInteraction(AActor* Interactor, AActor* InteractableObject, FName InteractionId)
{
	if (!InteractableObject)
	{
		return false;
	}

	// Frame-level anti-duplication for the same interactable object
	const uint64 CurrentFrame = GFrameCounter;
	if (uint64* LastFrame = LastInteractionFrames.Find(InteractableObject))
	{
		if (*LastFrame == CurrentFrame)
		{
			return false;
		}
		*LastFrame = CurrentFrame;
	}
	else
	{
		LastInteractionFrames.Add(InteractableObject, CurrentFrame);
	}

	TArray<FName> CandidateIds;
	if (!InteractionId.IsNone())
	{
		CandidateIds.Add(InteractionId);
	}

	if (const AShadowSlaveInteractableActor* InteractableActor = Cast<AShadowSlaveInteractableActor>(InteractableObject))
	{
		const FName ActorInterId = InteractableActor->GetInteractionId();
		if (!ActorInterId.IsNone())
		{
			CandidateIds.AddUnique(ActorInterId);
		}

		const FName SaveId = InteractableActor->GetPersistentSaveId();
		if (!SaveId.IsNone())
		{
			CandidateIds.AddUnique(SaveId);
		}
	}

	if (const AShadowSlaveInteractableNPC* NPC = Cast<AShadowSlaveInteractableNPC>(InteractableObject))
	{
		const FName NPCId = NPC->GetNPCId();
		if (!NPCId.IsNone())
		{
			CandidateIds.AddUnique(NPCId);
		}
	}

	if (InteractableObject->GetClass()->ImplementsInterface(UShadowSlaveSaveableInterface::StaticClass()))
	{
		const FName SaveId = IShadowSlaveSaveableInterface::Execute_GetPersistentSaveId(InteractableObject);
		if (!SaveId.IsNone())
		{
			CandidateIds.AddUnique(SaveId);
		}
	}

	TArray<TPair<FName, FName>> ObjectivesToAdvance;

	for (const auto& QuestPair : QuestRuntimeStates)
	{
		if (QuestPair.Value.State != EShadowSlaveQuestState::Active)
		{
			continue;
		}

		const UShadowSlaveQuestDefinition* Def = GetQuestDefinition(QuestPair.Key);
		if (!Def)
		{
			continue;
		}

		for (const auto& ObjPair : QuestPair.Value.ObjectiveStates)
		{
			if (ObjPair.Value.State != EShadowSlaveObjectiveState::Active)
			{
				continue;
			}

			const FShadowSlaveObjectiveDefinition* ObjDef = Def->FindObjective(ObjPair.Key);
			if (!ObjDef || ObjDef->ObjectiveType != EShadowSlaveObjectiveType::Interact || ObjDef->TargetId.IsNone())
			{
				continue;
			}

			bool bMatches = false;
			for (const FName& CandId : CandidateIds)
			{
				if (ObjDef->TargetId == CandId)
				{
					bMatches = true;
					break;
				}
			}

			if (!bMatches && InteractableObject->ActorHasTag(ObjDef->TargetId))
			{
				bMatches = true;
			}

			if (bMatches)
			{
				ObjectivesToAdvance.Add(TPair<FName, FName>(QuestPair.Key, ObjPair.Key));
			}
		}
	}

	bool bAnyAdvanced = false;
	for (const auto& Target : ObjectivesToAdvance)
	{
		if (AddObjectiveProgress(Target.Key, Target.Value, 1))
		{
			bAnyAdvanced = true;
		}
	}

	return bAnyAdvanced;
}

bool UShadowSlaveQuestSubsystem::NotifyConversationCompleted(FName DialogueId, AActor* SpeakerActor)
{
	if (DialogueId.IsNone() && !SpeakerActor)
	{
		return false;
	}

	const uint64 CurrentFrame = GFrameCounter;
	if (!DialogueId.IsNone())
	{
		if (uint64* LastFrame = LastConversationFrames.Find(DialogueId))
		{
			if (*LastFrame == CurrentFrame)
			{
				return false;
			}
			*LastFrame = CurrentFrame;
		}
		else
		{
			LastConversationFrames.Add(DialogueId, CurrentFrame);
		}
	}

	TArray<FName> CandidateIds;
	if (!DialogueId.IsNone())
	{
		CandidateIds.Add(DialogueId);
	}

	if (SpeakerActor)
	{
		if (const AShadowSlaveInteractableNPC* NPC = Cast<AShadowSlaveInteractableNPC>(SpeakerActor))
		{
			const FName NPCId = NPC->GetNPCId();
			if (!NPCId.IsNone())
			{
				CandidateIds.AddUnique(NPCId);
			}
		}

		if (const AShadowSlaveCharacterBase* Char = Cast<AShadowSlaveCharacterBase>(SpeakerActor))
		{
			const FName CharId = Char->GetCharacterId();
			if (!CharId.IsNone())
			{
				CandidateIds.AddUnique(CharId);
			}
		}

		if (const AShadowSlaveInteractableActor* InteractableActor = Cast<AShadowSlaveInteractableActor>(SpeakerActor))
		{
			const FName ActorInterId = InteractableActor->GetInteractionId();
			if (!ActorInterId.IsNone())
			{
				CandidateIds.AddUnique(ActorInterId);
			}

			const FName SaveId = InteractableActor->GetPersistentSaveId();
			if (!SaveId.IsNone())
			{
				CandidateIds.AddUnique(SaveId);
			}
		}

		if (SpeakerActor->GetClass()->ImplementsInterface(UShadowSlaveSaveableInterface::StaticClass()))
		{
			const FName SaveId = IShadowSlaveSaveableInterface::Execute_GetPersistentSaveId(SpeakerActor);
			if (!SaveId.IsNone())
			{
				CandidateIds.AddUnique(SaveId);
			}
		}
	}

	TArray<TPair<FName, FName>> ObjectivesToAdvance;

	for (const auto& QuestPair : QuestRuntimeStates)
	{
		if (QuestPair.Value.State != EShadowSlaveQuestState::Active)
		{
			continue;
		}

		const UShadowSlaveQuestDefinition* Def = GetQuestDefinition(QuestPair.Key);
		if (!Def)
		{
			continue;
		}

		for (const auto& ObjPair : QuestPair.Value.ObjectiveStates)
		{
			if (ObjPair.Value.State != EShadowSlaveObjectiveState::Active)
			{
				continue;
			}

			const FShadowSlaveObjectiveDefinition* ObjDef = Def->FindObjective(ObjPair.Key);
			if (!ObjDef || ObjDef->ObjectiveType != EShadowSlaveObjectiveType::TalkToCharacter || ObjDef->TargetId.IsNone())
			{
				continue;
			}

			bool bMatches = false;
			for (const FName& CandId : CandidateIds)
			{
				if (ObjDef->TargetId == CandId)
				{
					bMatches = true;
					break;
				}
			}

			if (!bMatches && SpeakerActor && SpeakerActor->ActorHasTag(ObjDef->TargetId))
			{
				bMatches = true;
			}

			if (bMatches)
			{
				ObjectivesToAdvance.Add(TPair<FName, FName>(QuestPair.Key, ObjPair.Key));
			}
		}
	}

	bool bAnyAdvanced = false;
	for (const auto& Target : ObjectivesToAdvance)
	{
		if (AddObjectiveProgress(Target.Key, Target.Value, 1))
		{
			bAnyAdvanced = true;
		}
	}

	return bAnyAdvanced;
}

bool UShadowSlaveQuestSubsystem::NotifyTargetDefeated(FName TargetId, AActor* DefeatedActor, AActor* KillerActor)
{
	if (TargetId.IsNone() && !DefeatedActor)
	{
		return false;
	}

	if (DefeatedActor)
	{
		if (ProcessedDefeatedActors.Contains(DefeatedActor))
		{
			return false;
		}
		ProcessedDefeatedActors.Add(DefeatedActor);
	}

	TArray<FName> CandidateIds;
	if (!TargetId.IsNone())
	{
		CandidateIds.Add(TargetId);
	}

	if (DefeatedActor)
	{
		if (const AShadowSlaveCharacterBase* Char = Cast<AShadowSlaveCharacterBase>(DefeatedActor))
		{
			const FName CharId = Char->GetCharacterId();
			if (!CharId.IsNone())
			{
				CandidateIds.AddUnique(CharId);
			}
		}

		if (const AShadowSlaveInteractableNPC* NPC = Cast<AShadowSlaveInteractableNPC>(DefeatedActor))
		{
			const FName NPCId = NPC->GetNPCId();
			if (!NPCId.IsNone())
			{
				CandidateIds.AddUnique(NPCId);
			}
		}

		if (const AShadowSlaveInteractableActor* InteractableActor = Cast<AShadowSlaveInteractableActor>(DefeatedActor))
		{
			const FName ActorInterId = InteractableActor->GetInteractionId();
			if (!ActorInterId.IsNone())
			{
				CandidateIds.AddUnique(ActorInterId);
			}

			const FName SaveId = InteractableActor->GetPersistentSaveId();
			if (!SaveId.IsNone())
			{
				CandidateIds.AddUnique(SaveId);
			}
		}

		if (DefeatedActor->GetClass()->ImplementsInterface(UShadowSlaveSaveableInterface::StaticClass()))
		{
			const FName SaveId = IShadowSlaveSaveableInterface::Execute_GetPersistentSaveId(DefeatedActor);
			if (!SaveId.IsNone())
			{
				CandidateIds.AddUnique(SaveId);
			}
		}
	}

	TArray<TPair<FName, FName>> ObjectivesToAdvance;

	for (const auto& QuestPair : QuestRuntimeStates)
	{
		if (QuestPair.Value.State != EShadowSlaveQuestState::Active)
		{
			continue;
		}

		const UShadowSlaveQuestDefinition* Def = GetQuestDefinition(QuestPair.Key);
		if (!Def)
		{
			continue;
		}

		for (const auto& ObjPair : QuestPair.Value.ObjectiveStates)
		{
			if (ObjPair.Value.State != EShadowSlaveObjectiveState::Active)
			{
				continue;
			}

			const FShadowSlaveObjectiveDefinition* ObjDef = Def->FindObjective(ObjPair.Key);
			if (!ObjDef || ObjDef->ObjectiveType != EShadowSlaveObjectiveType::DefeatTarget || ObjDef->TargetId.IsNone())
			{
				continue;
			}

			bool bMatches = false;
			for (const FName& CandId : CandidateIds)
			{
				if (ObjDef->TargetId == CandId)
				{
					bMatches = true;
					break;
				}
			}

			if (!bMatches && DefeatedActor && DefeatedActor->ActorHasTag(ObjDef->TargetId))
			{
				bMatches = true;
			}

			if (bMatches)
			{
				ObjectivesToAdvance.Add(TPair<FName, FName>(QuestPair.Key, ObjPair.Key));
			}
		}
	}

	bool bAnyAdvanced = false;
	for (const auto& Target : ObjectivesToAdvance)
	{
		if (AddObjectiveProgress(Target.Key, Target.Value, 1))
		{
			bAnyAdvanced = true;
		}
	}

	return bAnyAdvanced;
}

bool UShadowSlaveQuestSubsystem::NotifyItemCollected(FName ItemId, int32 Quantity, UShadowSlaveItemDefinition* ItemDef)
{
	if (Quantity <= 0 || (ItemId.IsNone() && !ItemDef))
	{
		return false;
	}

	TArray<FName> CandidateIds;
	if (!ItemId.IsNone())
	{
		CandidateIds.Add(ItemId);
	}

	if (ItemDef)
	{
		const FName AssetName = ItemDef->GetPrimaryAssetId().PrimaryAssetName;
		if (!AssetName.IsNone())
		{
			CandidateIds.AddUnique(AssetName);
		}
	}

	TArray<TPair<FName, FName>> ObjectivesToAdvance;

	for (const auto& QuestPair : QuestRuntimeStates)
	{
		if (QuestPair.Value.State != EShadowSlaveQuestState::Active)
		{
			continue;
		}

		const UShadowSlaveQuestDefinition* Def = GetQuestDefinition(QuestPair.Key);
		if (!Def)
		{
			continue;
		}

		for (const auto& ObjPair : QuestPair.Value.ObjectiveStates)
		{
			if (ObjPair.Value.State != EShadowSlaveObjectiveState::Active)
			{
				continue;
			}

			const FShadowSlaveObjectiveDefinition* ObjDef = Def->FindObjective(ObjPair.Key);
			if (!ObjDef || ObjDef->ObjectiveType != EShadowSlaveObjectiveType::CollectItem || ObjDef->TargetId.IsNone())
			{
				continue;
			}

			bool bMatches = false;
			for (const FName& CandId : CandidateIds)
			{
				if (ObjDef->TargetId == CandId)
				{
					bMatches = true;
					break;
				}
			}

			if (bMatches)
			{
				ObjectivesToAdvance.Add(TPair<FName, FName>(QuestPair.Key, ObjPair.Key));
			}
		}
	}

	bool bAnyAdvanced = false;
	for (const auto& Target : ObjectivesToAdvance)
	{
		if (AddObjectiveProgress(Target.Key, Target.Value, Quantity))
		{
			bAnyAdvanced = true;
		}
	}

	return bAnyAdvanced;
}

bool UShadowSlaveQuestSubsystem::NotifyWorldStateChanged(FName StateKey, const FShadowSlaveWorldValue& NewValue, AActor* OwningActor)
{
	if (StateKey.IsNone())
	{
		return false;
	}

	TArray<TPair<FName, FName>> ObjectivesToComplete;

	for (const auto& QuestPair : QuestRuntimeStates)
	{
		if (QuestPair.Value.State != EShadowSlaveQuestState::Active)
		{
			continue;
		}

		const UShadowSlaveQuestDefinition* Def = GetQuestDefinition(QuestPair.Key);
		if (!Def)
		{
			continue;
		}

		for (const auto& ObjPair : QuestPair.Value.ObjectiveStates)
		{
			if (ObjPair.Value.State != EShadowSlaveObjectiveState::Active)
			{
				continue;
			}

			const FShadowSlaveObjectiveDefinition* ObjDef = Def->FindObjective(ObjPair.Key);
			if (!ObjDef || ObjDef->ObjectiveType != EShadowSlaveObjectiveType::WorldState || ObjDef->TargetId != StateKey)
			{
				continue;
			}

			if (const FString* ExpectedActorStr = ObjDef->Metadata.Find(TEXT("ActorId")))
			{
				if (!ExpectedActorStr->IsEmpty())
				{
					if (!OwningActor)
					{
						continue;
					}

					const FName ExpectedActorName(*(*ExpectedActorStr));
					bool bActorMatches = false;

					if (const AShadowSlaveCharacterBase* Char = Cast<AShadowSlaveCharacterBase>(OwningActor))
					{
						if (Char->GetCharacterId() == ExpectedActorName)
						{
							bActorMatches = true;
						}
					}

					if (!bActorMatches)
					{
						if (const AShadowSlaveInteractableNPC* NPC = Cast<AShadowSlaveInteractableNPC>(OwningActor))
						{
							if (NPC->GetNPCId() == ExpectedActorName)
							{
								bActorMatches = true;
							}
						}
					}

					if (!bActorMatches)
					{
						if (const AShadowSlaveInteractableActor* InteractableActor = Cast<AShadowSlaveInteractableActor>(OwningActor))
						{
							if (InteractableActor->GetInteractionId() == ExpectedActorName ||
								InteractableActor->GetPersistentSaveId() == ExpectedActorName)
							{
								bActorMatches = true;
							}
						}
					}

					if (!bActorMatches && OwningActor->GetClass()->ImplementsInterface(UShadowSlaveSaveableInterface::StaticClass()))
					{
						bActorMatches = (IShadowSlaveSaveableInterface::Execute_GetPersistentSaveId(OwningActor) == ExpectedActorName);
					}

					if (!bActorMatches && OwningActor->ActorHasTag(ExpectedActorName))
					{
						bActorMatches = true;
					}

					if (!bActorMatches)
					{
						continue;
					}
				}
			}

			bool bConditionSatisfied = false;
			const FString* ExpectedValStr = ObjDef->Metadata.Find(TEXT("Value"));
			if (!ExpectedValStr)
			{
				ExpectedValStr = ObjDef->Metadata.Find(TEXT("ExpectedValue"));
			}

			if (ExpectedValStr)
			{
				const FString* ExplicitTypeStr = ObjDef->Metadata.Find(TEXT("ValueType"));
				if (!ExplicitTypeStr)
				{
					ExplicitTypeStr = ObjDef->Metadata.Find(TEXT("Type"));
				}

				FShadowSlaveWorldValue ExpectedVal;
				if (!TryParseExpectedWorldValue(*ExpectedValStr, ExplicitTypeStr, NewValue.ValueType, ExpectedVal))
				{
					// Invalid/unparseable expected value -> fail closed
					continue;
				}

				// Strict type compatibility: any mismatch fails closed
				if (NewValue.ValueType != ExpectedVal.ValueType || NewValue.ValueType == EShadowSlaveWorldValueType::None)
				{
					continue;
				}

				const FString* OpStr = ObjDef->Metadata.Find(TEXT("Op"));
				if (!OpStr)
				{
					OpStr = ObjDef->Metadata.Find(TEXT("Operator"));
				}

				if (OpStr && (*OpStr == TEXT(">=") || *OpStr == TEXT(">")))
				{
					// >= and > are only valid for Int with Int OR Float with Float
					if (NewValue.ValueType == EShadowSlaveWorldValueType::Int)
					{
						bConditionSatisfied = (*OpStr == TEXT(">=")) ? (NewValue.IntValue >= ExpectedVal.IntValue) : (NewValue.IntValue > ExpectedVal.IntValue);
					}
					else if (NewValue.ValueType == EShadowSlaveWorldValueType::Float)
					{
						bConditionSatisfied = (*OpStr == TEXT(">=")) ? (NewValue.FloatValue >= ExpectedVal.FloatValue) : (NewValue.FloatValue > ExpectedVal.FloatValue);
					}
					else
					{
						// Incompatible operator for non-numeric type -> fail closed
						continue;
					}
				}
				else if (OpStr && (*OpStr == TEXT("<=") || *OpStr == TEXT("<")))
				{
					// <= and < are only valid for Int with Int OR Float with Float
					if (NewValue.ValueType == EShadowSlaveWorldValueType::Int)
					{
						bConditionSatisfied = (*OpStr == TEXT("<=")) ? (NewValue.IntValue <= ExpectedVal.IntValue) : (NewValue.IntValue < ExpectedVal.IntValue);
					}
					else if (NewValue.ValueType == EShadowSlaveWorldValueType::Float)
					{
						bConditionSatisfied = (*OpStr == TEXT("<=")) ? (NewValue.FloatValue <= ExpectedVal.FloatValue) : (NewValue.FloatValue < ExpectedVal.FloatValue);
					}
					else
					{
						// Incompatible operator for non-numeric type -> fail closed
						continue;
					}
				}
				else if (OpStr && *OpStr == TEXT("!="))
				{
					// != only between strictly compatible types
					switch (NewValue.ValueType)
					{
					case EShadowSlaveWorldValueType::Bool:
						bConditionSatisfied = (NewValue.BoolValue != ExpectedVal.BoolValue);
						break;
					case EShadowSlaveWorldValueType::Int:
						bConditionSatisfied = (NewValue.IntValue != ExpectedVal.IntValue);
						break;
					case EShadowSlaveWorldValueType::Float:
						bConditionSatisfied = !FMath::IsNearlyEqual(NewValue.FloatValue, ExpectedVal.FloatValue);
						break;
					case EShadowSlaveWorldValueType::String:
						bConditionSatisfied = !NewValue.StringValue.Equals(ExpectedVal.StringValue, ESearchCase::CaseSensitive);
						break;
					case EShadowSlaveWorldValueType::Name:
						bConditionSatisfied = (NewValue.NameValue != ExpectedVal.NameValue);
						break;
					default:
						bConditionSatisfied = false;
						break;
					}
				}
				else if (!OpStr || OpStr->IsEmpty() || *OpStr == TEXT("=="))
				{
					// == only between strictly compatible types
					switch (NewValue.ValueType)
					{
					case EShadowSlaveWorldValueType::Bool:
						bConditionSatisfied = (NewValue.BoolValue == ExpectedVal.BoolValue);
						break;
					case EShadowSlaveWorldValueType::Int:
						bConditionSatisfied = (NewValue.IntValue == ExpectedVal.IntValue);
						break;
					case EShadowSlaveWorldValueType::Float:
						bConditionSatisfied = FMath::IsNearlyEqual(NewValue.FloatValue, ExpectedVal.FloatValue);
						break;
					case EShadowSlaveWorldValueType::String:
						bConditionSatisfied = NewValue.StringValue.Equals(ExpectedVal.StringValue, ESearchCase::CaseSensitive);
						break;
					case EShadowSlaveWorldValueType::Name:
						bConditionSatisfied = (NewValue.NameValue == ExpectedVal.NameValue);
						break;
					default:
						bConditionSatisfied = false;
						break;
					}
				}
				else
				{
					// Unknown operator -> fail closed
					continue;
				}
			}
			else
			{
				// No expected value supplied: preserve intentional default semantics without reinterpreting unrelated payload fields
				switch (NewValue.ValueType)
				{
				case EShadowSlaveWorldValueType::Bool:
					bConditionSatisfied = NewValue.BoolValue;
					break;
				case EShadowSlaveWorldValueType::Int:
					bConditionSatisfied = (NewValue.IntValue != 0);
					break;
				case EShadowSlaveWorldValueType::Float:
					bConditionSatisfied = !FMath::IsNearlyZero(NewValue.FloatValue);
					break;
				case EShadowSlaveWorldValueType::String:
					bConditionSatisfied = !NewValue.StringValue.IsEmpty();
					break;
				case EShadowSlaveWorldValueType::Name:
					bConditionSatisfied = (!NewValue.NameValue.IsNone() && NewValue.NameValue != NAME_None);
					break;
				default:
					bConditionSatisfied = false;
					break;
				}
			}

			if (bConditionSatisfied)
			{
				ObjectivesToComplete.Add(TPair<FName, FName>(QuestPair.Key, ObjPair.Key));
			}
		}
	}

	bool bAnyCompleted = false;
	for (const auto& Target : ObjectivesToComplete)
	{
		if (CompleteObjective(Target.Key, Target.Value))
		{
			bAnyCompleted = true;
		}
	}

	return bAnyCompleted;
}

bool UShadowSlaveQuestSubsystem::NotifyLocationReached(FName LocationId, AActor* TriggerActor)
{
	if (LocationId.IsNone() && !TriggerActor)
	{
		return false;
	}

	TArray<FName> CandidateIds;
	if (!LocationId.IsNone())
	{
		CandidateIds.Add(LocationId);
	}

	if (TriggerActor)
	{
		if (const AShadowSlaveInteractableActor* InteractableActor = Cast<AShadowSlaveInteractableActor>(TriggerActor))
		{
			const FName SaveId = InteractableActor->GetPersistentSaveId();
			if (!SaveId.IsNone())
			{
				CandidateIds.AddUnique(SaveId);
			}
		}
		else if (TriggerActor->GetClass()->ImplementsInterface(UShadowSlaveSaveableInterface::StaticClass()))
		{
			const FName SaveId = IShadowSlaveSaveableInterface::Execute_GetPersistentSaveId(TriggerActor);
			if (!SaveId.IsNone())
			{
				CandidateIds.AddUnique(SaveId);
			}
		}
	}

	TArray<TPair<FName, FName>> ObjectivesToComplete;

	for (const auto& QuestPair : QuestRuntimeStates)
	{
		if (QuestPair.Value.State != EShadowSlaveQuestState::Active)
		{
			continue;
		}

		const UShadowSlaveQuestDefinition* Def = GetQuestDefinition(QuestPair.Key);
		if (!Def)
		{
			continue;
		}

		for (const auto& ObjPair : QuestPair.Value.ObjectiveStates)
		{
			if (ObjPair.Value.State != EShadowSlaveObjectiveState::Active)
			{
				continue;
			}

			const FShadowSlaveObjectiveDefinition* ObjDef = Def->FindObjective(ObjPair.Key);
			if (!ObjDef || ObjDef->ObjectiveType != EShadowSlaveObjectiveType::ReachLocation || ObjDef->TargetId.IsNone())
			{
				continue;
			}

			bool bMatches = false;
			for (const FName& CandId : CandidateIds)
			{
				if (ObjDef->TargetId == CandId)
				{
					bMatches = true;
					break;
				}
			}

			if (!bMatches && TriggerActor && TriggerActor->ActorHasTag(ObjDef->TargetId))
			{
				bMatches = true;
			}

			if (bMatches)
			{
				ObjectivesToComplete.Add(TPair<FName, FName>(QuestPair.Key, ObjPair.Key));
			}
		}
	}

	bool bAnyCompleted = false;
	for (const auto& Target : ObjectivesToComplete)
	{
		if (CompleteObjective(Target.Key, Target.Value))
		{
			bAnyCompleted = true;
		}
	}

	return bAnyCompleted;
}

bool UShadowSlaveQuestSubsystem::NotifySurvivalCompleted(FName SurvivalId, AActor* Actor)
{
	if (SurvivalId.IsNone())
	{
		return false;
	}

	TArray<TPair<FName, FName>> ObjectivesToComplete;

	for (const auto& QuestPair : QuestRuntimeStates)
	{
		if (QuestPair.Value.State != EShadowSlaveQuestState::Active)
		{
			continue;
		}

		const UShadowSlaveQuestDefinition* Def = GetQuestDefinition(QuestPair.Key);
		if (!Def)
		{
			continue;
		}

		for (const auto& ObjPair : QuestPair.Value.ObjectiveStates)
		{
			if (ObjPair.Value.State != EShadowSlaveObjectiveState::Active)
			{
				continue;
			}

			const FShadowSlaveObjectiveDefinition* ObjDef = Def->FindObjective(ObjPair.Key);
			if (!ObjDef || ObjDef->ObjectiveType != EShadowSlaveObjectiveType::Survive || ObjDef->TargetId.IsNone())
			{
				continue;
			}

			if (ObjDef->TargetId == SurvivalId || (Actor && Actor->ActorHasTag(ObjDef->TargetId)))
			{
				ObjectivesToComplete.Add(TPair<FName, FName>(QuestPair.Key, ObjPair.Key));
			}
		}
	}

	bool bAnyCompleted = false;
	for (const auto& Target : ObjectivesToComplete)
	{
		if (CompleteObjective(Target.Key, Target.Value))
		{
			bAnyCompleted = true;
		}
	}

	return bAnyCompleted;
}

void UShadowSlaveQuestSubsystem::HandleInventoryItemAdded(const FShadowSlaveItemInstance& ItemInstance, int32 QuantityAdded)
{
	if (QuantityAdded <= 0 || !ItemInstance.IsValid() || !ItemInstance.ItemDefinition)
	{
		return;
	}

	const FName ItemId = ItemInstance.ItemDefinition->GetPrimaryAssetId().PrimaryAssetName;
	NotifyItemCollected(ItemId, QuantityAdded, ItemInstance.ItemDefinition);
}

void UShadowSlaveQuestSubsystem::HandleInteractionExecuted(AActor* Interactor, AActor* InteractableObject, const FShadowSlaveInteractionResult& Result)
{
	if (Result.bSuccess)
	{
		NotifyInteraction(Interactor, InteractableObject, Result.InteractionId);
	}
}

void UShadowSlaveQuestSubsystem::HandleConversationCompleted(FName DialogueId)
{
	AActor* Speaker = nullptr;
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UShadowSlaveConversationSubsystem* ConvSub = GI->GetSubsystem<UShadowSlaveConversationSubsystem>())
		{
			Speaker = ConvSub->GetCurrentSpeaker();
		}
	}
	NotifyConversationCompleted(DialogueId, Speaker);
}

void UShadowSlaveQuestSubsystem::HandleWorldStateChanged(FName Key, const FShadowSlaveWorldValue& NewValue, const FShadowSlaveWorldValue& OldValue, AActor* OwningActor)
{
	NotifyWorldStateChanged(Key, NewValue, OwningActor);
}

void UShadowSlaveQuestSubsystem::HandleSelfQuestCompleted(FName CompletedQuestId)
{
	if (CompletedQuestId.IsNone() || bIsRestoringState)
	{
		return;
	}

	TArray<TPair<FName, FName>> ObjectivesToComplete;

	for (const auto& QuestPair : QuestRuntimeStates)
	{
		if (QuestPair.Value.State != EShadowSlaveQuestState::Active || QuestPair.Key == CompletedQuestId)
		{
			continue;
		}

		const UShadowSlaveQuestDefinition* Def = GetQuestDefinition(QuestPair.Key);
		if (!Def)
		{
			continue;
		}

		for (const auto& ObjPair : QuestPair.Value.ObjectiveStates)
		{
			if (ObjPair.Value.State != EShadowSlaveObjectiveState::Active)
			{
				continue;
			}

			const FShadowSlaveObjectiveDefinition* ObjDef = Def->FindObjective(ObjPair.Key);
			if (ObjDef && ObjDef->ObjectiveType == EShadowSlaveObjectiveType::CompleteQuest && ObjDef->TargetId == CompletedQuestId)
			{
				ObjectivesToComplete.Add(TPair<FName, FName>(QuestPair.Key, ObjPair.Key));
			}
		}
	}

	for (const auto& Target : ObjectivesToComplete)
	{
		CompleteObjective(Target.Key, Target.Value);
	}
}

void UShadowSlaveQuestSubsystem::HandleNightmareScenarioCompleted(UShadowSlaveNightmareScenarioDefinition* ScenarioDef)
{
	if (ScenarioDef && !ScenarioDef->ScenarioId.IsNone())
	{
		NotifySurvivalCompleted(ScenarioDef->ScenarioId);
	}
}
