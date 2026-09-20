// Copyright Epic Games, Inc. All Rights Reserved.

#include "Story/ShadowSlaveStorySubsystem.h"
#include "Gameplay/ShadowSlaveQuestSubsystem.h"
#include "Dialogue/ShadowSlaveConversationSubsystem.h"
#include "Nightmares/ShadowSlaveNightmareSubsystem.h"
#include "Nightmares/ShadowSlaveNightmareScenarioDefinition.h"
#include "World/ShadowSlaveWorldStateComponent.h"
#include "Characters/ShadowSlaveCharacterBase.h"
#include "Interaction/ShadowSlaveInteractableNPC.h"
#include "Interaction/ShadowSlaveInteractableActor.h"
#include "Save/ShadowSlaveSaveableInterface.h"
#include "Subsystems/SubsystemCollection.h"
#include "Engine/GameInstance.h"
#include "ShadowSlave.h"

namespace
{
	/** Helper to strictly parse an integer string without allowing partial conversions or trailing characters */
	static bool TryParseStrictInt(const FString& InStr, int32& OutValue)
	{
		const FString Trimmed = InStr.TrimStartAndEnd();
		if (Trimmed.IsEmpty())
		{
			return false;
		}

		const TCHAR* Buffer = *Trimmed;
		int32 Index = 0;
		if (Buffer[Index] == TEXT('+') || Buffer[Index] == TEXT('-'))
		{
			Index++;
		}

		// Must have at least one digit
		if (Buffer[Index] == TEXT('\0'))
		{
			return false;
		}

		while (Buffer[Index] != TEXT('\0'))
		{
			if (!FChar::IsDigit(Buffer[Index]))
			{
				return false;
			}
			Index++;
		}

		// Verify that it fits within int32 limits
		const int64 Val64 = FCString::Atoi64(Buffer);
		if (Val64 < static_cast<int64>(TNumericLimits<int32>::Lowest()) || Val64 > static_cast<int64>(TNumericLimits<int32>::Max()))
		{
			return false;
		}

		OutValue = static_cast<int32>(Val64);
		return true;
	}

	/** Helper to strictly parse a floating-point string without allowing partial conversions or trailing characters */
	static bool TryParseStrictFloat(const FString& InStr, float& OutValue)
	{
		const FString Trimmed = InStr.TrimStartAndEnd();
		if (Trimmed.IsEmpty())
		{
			return false;
		}

		const TCHAR* Buffer = *Trimmed;
		int32 Index = 0;
		if (Buffer[Index] == TEXT('+') || Buffer[Index] == TEXT('-'))
		{
			Index++;
		}

		if (Buffer[Index] == TEXT('\0'))
		{
			return false;
		}

		bool bHasDigitsBeforeDot = false;
		while (Buffer[Index] != TEXT('\0') && FChar::IsDigit(Buffer[Index]))
		{
			bHasDigitsBeforeDot = true;
			Index++;
		}

		bool bHasDot = false;
		bool bHasDigitsAfterDot = false;
		if (Buffer[Index] == TEXT('.'))
		{
			Index++;
			while (Buffer[Index] != TEXT('\0') && FChar::IsDigit(Buffer[Index]))
			{
				bHasDigitsAfterDot = true;
				Index++;
			}
		}

		// Must have at least one digit before or after the decimal point
		if (!bHasDigitsBeforeDot && !bHasDigitsAfterDot)
		{
			return false;
		}

		// Optional scientific notation exponent (e.g. 1e-4, 2.5E3)
		if (Buffer[Index] == TEXT('e') || Buffer[Index] == TEXT('E'))
		{
			Index++;
			if (Buffer[Index] == TEXT('+') || Buffer[Index] == TEXT('-'))
			{
				Index++;
			}
			bool bHasExponentDigits = false;
			while (Buffer[Index] != TEXT('\0') && FChar::IsDigit(Buffer[Index]))
			{
				bHasExponentDigits = true;
				Index++;
			}
			if (!bHasExponentDigits)
			{
				return false;
			}
		}

		// Optional float suffix 'f' or 'F'
		if (Buffer[Index] == TEXT('f') || Buffer[Index] == TEXT('F'))
		{
			Index++;
		}

		// Must have reached the end of the string
		if (Buffer[Index] != TEXT('\0'))
		{
			return false;
		}

		OutValue = FCString::Atof(Buffer);
		return true;
	}

	/** Helper to parse expected world value string according to type rules and fail closed on invalid format */
	static bool TryParseWorldValue(
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

		// Prefix check: b:, i:, f:, s:, n:
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

		// Explicit type string check
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
				return false;
			}
		}

		// Fallback to runtime type
		if (ExpectedType == EShadowSlaveWorldValueType::None)
		{
			ExpectedType = FallbackType;
		}

		switch (ExpectedType)
		{
		case EShadowSlaveWorldValueType::Bool:
		{
			const FString TrimmedBool = Payload.TrimStartAndEnd();
			if (TrimmedBool.Equals(TEXT("true"), ESearchCase::IgnoreCase) || TrimmedBool.Equals(TEXT("1")))
			{
				OutParsedValue = FShadowSlaveWorldValue::MakeBool(true);
				return true;
			}
			else if (TrimmedBool.Equals(TEXT("false"), ESearchCase::IgnoreCase) || TrimmedBool.Equals(TEXT("0")))
			{
				OutParsedValue = FShadowSlaveWorldValue::MakeBool(false);
				return true;
			}
			return false;
		}
		case EShadowSlaveWorldValueType::Int:
		{
			int32 ParsedInt = 0;
			if (!TryParseStrictInt(Payload, ParsedInt))
			{
				return false;
			}
			OutParsedValue = FShadowSlaveWorldValue::MakeInt(ParsedInt);
			return true;
		}
		case EShadowSlaveWorldValueType::Float:
		{
			float ParsedFloat = 0.0f;
			if (!TryParseStrictFloat(Payload, ParsedFloat))
			{
				return false;
			}
			OutParsedValue = FShadowSlaveWorldValue::MakeFloat(ParsedFloat);
			return true;
		}
		case EShadowSlaveWorldValueType::String:
			OutParsedValue = FShadowSlaveWorldValue::MakeString(Payload);
			return true;
		case EShadowSlaveWorldValueType::Name:
			OutParsedValue = FShadowSlaveWorldValue::MakeName(FName(*Payload));
			return true;
		default:
			return false;
		}
	}
}

UShadowSlaveStorySubsystem::UShadowSlaveStorySubsystem()
{
}

void UShadowSlaveStorySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Collection.InitializeDependency<UShadowSlaveQuestSubsystem>();
	Collection.InitializeDependency<UShadowSlaveConversationSubsystem>();
	Collection.InitializeDependency<UShadowSlaveNightmareSubsystem>();
	Super::Initialize(Collection);

	RegisteredDefinitions.Empty();
	StoryRuntimeStates.Empty();
	RegisteredContentDefinitions.Empty();
	StoryContentRuntimeStates.Empty();
	RegisteredWorldStateSources.Empty();
	bIsRestoringState = false;
	bIsBridgeActive = false;
	bIsProcessingProgression = false;

	InitializeProgressionBridge();
}

void UShadowSlaveStorySubsystem::Deinitialize()
{
	ShutdownProgressionBridge();

	RegisteredDefinitions.Empty();
	StoryRuntimeStates.Empty();
	RegisteredContentDefinitions.Empty();
	StoryContentRuntimeStates.Empty();
	RegisteredWorldStateSources.Empty();

	Super::Deinitialize();
}

/* =========================================================================
 * Progression Bridge Lifecycle & Binding (Step 26)
 * ========================================================================= */

void UShadowSlaveStorySubsystem::InitializeProgressionBridge()
{
	if (bIsBridgeActive)
	{
		return;
	}

	UGameInstance* GI = GetGameInstance();
	if (!GI)
	{
		return;
	}

	if (UShadowSlaveQuestSubsystem* QuestSub = GI->GetSubsystem<UShadowSlaveQuestSubsystem>())
	{
		QuestSub->OnQuestCompleted.AddUniqueDynamic(this, &UShadowSlaveStorySubsystem::HandleQuestCompleted);
		QuestSub->OnQuestFailed.AddUniqueDynamic(this, &UShadowSlaveStorySubsystem::HandleQuestFailed);
	}

	if (UShadowSlaveConversationSubsystem* ConvSub = GI->GetSubsystem<UShadowSlaveConversationSubsystem>())
	{
		ConvSub->OnConversationCompleted.AddUniqueDynamic(this, &UShadowSlaveStorySubsystem::HandleConversationCompleted);
		ConvSub->OnConversationAborted.AddUniqueDynamic(this, &UShadowSlaveStorySubsystem::HandleConversationAborted);
	}

	if (UShadowSlaveNightmareSubsystem* NightmareSub = GI->GetSubsystem<UShadowSlaveNightmareSubsystem>())
	{
		NightmareSub->OnScenarioCompleted.AddUniqueDynamic(this, &UShadowSlaveStorySubsystem::HandleNightmareScenarioCompleted);
		NightmareSub->OnScenarioFailed.AddUniqueDynamic(this, &UShadowSlaveStorySubsystem::HandleNightmareScenarioFailed);
		NightmareSub->OnScenarioAborted.AddUniqueDynamic(this, &UShadowSlaveStorySubsystem::HandleNightmareScenarioAborted);
	}

	bIsBridgeActive = true;
}

void UShadowSlaveStorySubsystem::ShutdownProgressionBridge()
{
	if (!bIsBridgeActive)
	{
		return;
	}

	UGameInstance* GI = GetGameInstance();
	if (GI)
	{
		if (UShadowSlaveQuestSubsystem* QuestSub = GI->GetSubsystem<UShadowSlaveQuestSubsystem>())
		{
			QuestSub->OnQuestCompleted.RemoveDynamic(this, &UShadowSlaveStorySubsystem::HandleQuestCompleted);
			QuestSub->OnQuestFailed.RemoveDynamic(this, &UShadowSlaveStorySubsystem::HandleQuestFailed);
		}

		if (UShadowSlaveConversationSubsystem* ConvSub = GI->GetSubsystem<UShadowSlaveConversationSubsystem>())
		{
			ConvSub->OnConversationCompleted.RemoveDynamic(this, &UShadowSlaveStorySubsystem::HandleConversationCompleted);
			ConvSub->OnConversationAborted.RemoveDynamic(this, &UShadowSlaveStorySubsystem::HandleConversationAborted);
		}

		if (UShadowSlaveNightmareSubsystem* NightmareSub = GI->GetSubsystem<UShadowSlaveNightmareSubsystem>())
		{
			NightmareSub->OnScenarioCompleted.RemoveDynamic(this, &UShadowSlaveStorySubsystem::HandleNightmareScenarioCompleted);
			NightmareSub->OnScenarioFailed.RemoveDynamic(this, &UShadowSlaveStorySubsystem::HandleNightmareScenarioFailed);
			NightmareSub->OnScenarioAborted.RemoveDynamic(this, &UShadowSlaveStorySubsystem::HandleNightmareScenarioAborted);
		}
	}

	for (auto It = RegisteredWorldStateSources.CreateIterator(); It; ++It)
	{
		if (UShadowSlaveWorldStateComponent* WSComp = It->Get())
		{
			WSComp->OnWorldStateChanged.RemoveDynamic(this, &UShadowSlaveStorySubsystem::HandleWorldStateChanged);
		}
	}
	RegisteredWorldStateSources.Empty();

	bIsBridgeActive = false;
}

void UShadowSlaveStorySubsystem::EnsureProgressionBridgeBound()
{
	if (!bIsBridgeActive)
	{
		InitializeProgressionBridge();
	}
}

void UShadowSlaveStorySubsystem::RegisterWorldStateSource(UShadowSlaveWorldStateComponent* WorldStateComponent)
{
	if (!WorldStateComponent)
	{
		return;
	}

	TWeakObjectPtr<UShadowSlaveWorldStateComponent> WeakComp(WorldStateComponent);
	if (!RegisteredWorldStateSources.Contains(WeakComp))
	{
		RegisteredWorldStateSources.Add(WeakComp);
		WorldStateComponent->OnWorldStateChanged.AddUniqueDynamic(this, &UShadowSlaveStorySubsystem::HandleWorldStateChanged);
	}
}

void UShadowSlaveStorySubsystem::UnregisterWorldStateSource(UShadowSlaveWorldStateComponent* WorldStateComponent)
{
	if (!WorldStateComponent)
	{
		return;
	}

	TWeakObjectPtr<UShadowSlaveWorldStateComponent> WeakComp(WorldStateComponent);
	if (RegisteredWorldStateSources.Contains(WeakComp))
	{
		WorldStateComponent->OnWorldStateChanged.RemoveDynamic(this, &UShadowSlaveStorySubsystem::HandleWorldStateChanged);
		RegisteredWorldStateSources.Remove(WeakComp);
	}
}

/* =========================================================================
 * Progression Bridge Domain Event Handlers (Step 26)
 * ========================================================================= */

void UShadowSlaveStorySubsystem::HandleQuestCompleted(FName QuestId)
{
	if (bIsRestoringState || QuestId.IsNone())
	{
		return;
	}

	NotifyStoryContentTargetCompleted(EShadowSlaveStoryContentType::Quest, QuestId);
}

void UShadowSlaveStorySubsystem::HandleQuestFailed(FName QuestId)
{
	if (bIsRestoringState || QuestId.IsNone())
	{
		return;
	}

	NotifyStoryContentTargetFailed(EShadowSlaveStoryContentType::Quest, QuestId);
}

void UShadowSlaveStorySubsystem::HandleConversationCompleted(FName DialogueId)
{
	if (bIsRestoringState || DialogueId.IsNone())
	{
		return;
	}

	NotifyStoryContentTargetCompleted(EShadowSlaveStoryContentType::Dialogue, DialogueId);
}

void UShadowSlaveStorySubsystem::HandleConversationAborted(FName DialogueId, FName LastNodeId)
{
	if (bIsRestoringState || DialogueId.IsNone())
	{
		return;
	}

	// Aborting dialogue leaves the entry active so player can re-engage
	UE_LOG(LogShadowSlave, Verbose, TEXT("UShadowSlaveStorySubsystem::HandleConversationAborted - Dialogue '%s' aborted at node '%s'. Leaving entry active."),
		*DialogueId.ToString(), *LastNodeId.ToString());
}

void UShadowSlaveStorySubsystem::HandleNightmareScenarioCompleted(UShadowSlaveNightmareScenarioDefinition* ScenarioDef)
{
	if (bIsRestoringState || !ScenarioDef || ScenarioDef->ScenarioId.IsNone())
	{
		return;
	}

	NotifyStoryContentTargetCompleted(EShadowSlaveStoryContentType::Nightmare, ScenarioDef->ScenarioId);
}

void UShadowSlaveStorySubsystem::HandleNightmareScenarioFailed(UShadowSlaveNightmareScenarioDefinition* ScenarioDef, EShadowSlaveScenarioFailureReason Reason)
{
	if (bIsRestoringState || !ScenarioDef || ScenarioDef->ScenarioId.IsNone())
	{
		return;
	}

	NotifyStoryContentTargetFailed(EShadowSlaveStoryContentType::Nightmare, ScenarioDef->ScenarioId);
}

void UShadowSlaveStorySubsystem::HandleNightmareScenarioAborted(UShadowSlaveNightmareScenarioDefinition* ScenarioDef)
{
	if (bIsRestoringState || !ScenarioDef)
	{
		return;
	}

	UE_LOG(LogShadowSlave, Verbose, TEXT("UShadowSlaveStorySubsystem::HandleNightmareScenarioAborted - Scenario '%s' aborted. Leaving entry active."),
		*ScenarioDef->ScenarioId.ToString());
}

void UShadowSlaveStorySubsystem::HandleWorldStateChanged(
	FName Key,
	const FShadowSlaveWorldValue& NewValue,
	const FShadowSlaveWorldValue& OldValue,
	AActor* OwningActor)
{
	if (bIsRestoringState || Key.IsNone())
	{
		return;
	}

	NotifyWorldStateChanged(Key, NewValue, OwningActor);
}

bool UShadowSlaveStorySubsystem::NotifyStoryContentTargetCompleted(EShadowSlaveStoryContentType ContentType, FName TargetId)
{
	if (bIsRestoringState || ContentType == EShadowSlaveStoryContentType::None || TargetId.IsNone())
	{
		return false;
	}

	bool bAnyProgressed = false;

	TArray<FName> ActiveContentIds;
	for (const auto& Pair : StoryContentRuntimeStates)
	{
		if (Pair.Value.State == EShadowSlaveStoryContentState::Active)
		{
			ActiveContentIds.Add(Pair.Key);
		}
	}

	for (const FName& StoryContentId : ActiveContentIds)
	{
		const FShadowSlaveStoryContentRuntimeState* StatePtr = StoryContentRuntimeStates.Find(StoryContentId);
		if (!StatePtr || StatePtr->State != EShadowSlaveStoryContentState::Active)
		{
			continue;
		}

		const FName ActiveEntryId = StatePtr->CurrentActiveEntryId;
		if (ActiveEntryId.IsNone())
		{
			continue;
		}

		const UShadowSlaveStoryContentDefinition* Def = GetStoryContentDefinition(StoryContentId);
		if (!Def)
		{
			continue;
		}

		const FShadowSlaveStoryContentEntry* EntryDef = Def->FindContentEntry(ActiveEntryId);
		if (!EntryDef || EntryDef->ContentType != ContentType || EntryDef->TargetId != TargetId)
		{
			continue;
		}

		if (GetStoryContentEntryState(StoryContentId, ActiveEntryId) == EShadowSlaveStoryContentState::Active)
		{
			if (CompleteStoryContentEntry(StoryContentId, ActiveEntryId))
			{
				bAnyProgressed = true;
			}
		}
	}

	return bAnyProgressed;
}

bool UShadowSlaveStorySubsystem::NotifyStoryContentTargetFailed(EShadowSlaveStoryContentType ContentType, FName TargetId)
{
	if (bIsRestoringState || ContentType == EShadowSlaveStoryContentType::None || TargetId.IsNone())
	{
		return false;
	}

	bool bAnyHandled = false;

	TArray<FName> ActiveContentIds;
	for (const auto& Pair : StoryContentRuntimeStates)
	{
		if (Pair.Value.State == EShadowSlaveStoryContentState::Active)
		{
			ActiveContentIds.Add(Pair.Key);
		}
	}

	for (const FName& StoryContentId : ActiveContentIds)
	{
		const FShadowSlaveStoryContentRuntimeState* StatePtr = StoryContentRuntimeStates.Find(StoryContentId);
		if (!StatePtr || StatePtr->State != EShadowSlaveStoryContentState::Active)
		{
			continue;
		}

		const FName ActiveEntryId = StatePtr->CurrentActiveEntryId;
		if (ActiveEntryId.IsNone())
		{
			continue;
		}

		const UShadowSlaveStoryContentDefinition* Def = GetStoryContentDefinition(StoryContentId);
		if (!Def)
		{
			continue;
		}

		const FShadowSlaveStoryContentEntry* EntryDef = Def->FindContentEntry(ActiveEntryId);
		if (!EntryDef || EntryDef->ContentType != ContentType || EntryDef->TargetId != TargetId)
		{
			continue;
		}

		if (GetStoryContentEntryState(StoryContentId, ActiveEntryId) == EShadowSlaveStoryContentState::Active)
		{
			if (FailStoryContentEntry(StoryContentId, ActiveEntryId))
			{
				bAnyHandled = true;
			}
		}
	}

	return bAnyHandled;
}

bool UShadowSlaveStorySubsystem::NotifyWorldStateChanged(FName StateKey, const FShadowSlaveWorldValue& NewValue, AActor* OwningActor)
{
	if (bIsRestoringState || StateKey.IsNone())
	{
		return false;
	}

	bool bAnyProgressed = false;

	TArray<FName> ActiveContentIds;
	for (const auto& Pair : StoryContentRuntimeStates)
	{
		if (Pair.Value.State == EShadowSlaveStoryContentState::Active)
		{
			ActiveContentIds.Add(Pair.Key);
		}
	}

	for (const FName& StoryContentId : ActiveContentIds)
	{
		const FShadowSlaveStoryContentRuntimeState* StatePtr = StoryContentRuntimeStates.Find(StoryContentId);
		if (!StatePtr || StatePtr->State != EShadowSlaveStoryContentState::Active)
		{
			continue;
		}

		const FName ActiveEntryId = StatePtr->CurrentActiveEntryId;
		if (ActiveEntryId.IsNone())
		{
			continue;
		}

		const UShadowSlaveStoryContentDefinition* Def = GetStoryContentDefinition(StoryContentId);
		if (!Def)
		{
			continue;
		}

		const FShadowSlaveStoryContentEntry* EntryDef = Def->FindContentEntry(ActiveEntryId);
		if (!EntryDef || EntryDef->ContentType != EShadowSlaveStoryContentType::WorldState || EntryDef->TargetId != StateKey)
		{
			continue;
		}

		if (GetStoryContentEntryState(StoryContentId, ActiveEntryId) != EShadowSlaveStoryContentState::Active)
		{
			continue;
		}

		if (EvaluateWorldStateCondition(*EntryDef, NewValue, OwningActor))
		{
			if (CompleteStoryContentEntry(StoryContentId, ActiveEntryId))
			{
				bAnyProgressed = true;
			}
		}
	}

	return bAnyProgressed;
}

bool UShadowSlaveStorySubsystem::EvaluateWorldStateCondition(
	const FShadowSlaveStoryContentEntry& EntryDef,
	const FShadowSlaveWorldValue& NewValue,
	AActor* OwningActor) const
{
	// 1. Optional actor validation
	if (const FString* ExpectedActorStr = EntryDef.Metadata.Find(TEXT("ActorId")))
	{
		if (!ExpectedActorStr->IsEmpty())
		{
			if (!OwningActor)
			{
				return false;
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
				return false;
			}
		}
	}

	// 2. Value comparison
	const FString* ExpectedValStr = EntryDef.Metadata.Find(TEXT("Value"));
	if (!ExpectedValStr)
	{
		ExpectedValStr = EntryDef.Metadata.Find(TEXT("ExpectedValue"));
	}

	if (ExpectedValStr)
	{
		const FString* ExplicitTypeStr = EntryDef.Metadata.Find(TEXT("ValueType"));
		if (!ExplicitTypeStr)
		{
			ExplicitTypeStr = EntryDef.Metadata.Find(TEXT("Type"));
		}

		FShadowSlaveWorldValue ExpectedVal;
		if (!TryParseWorldValue(*ExpectedValStr, ExplicitTypeStr, NewValue.ValueType, ExpectedVal))
		{
			return false;
		}

		if (NewValue.ValueType != ExpectedVal.ValueType || NewValue.ValueType == EShadowSlaveWorldValueType::None)
		{
			return false;
		}

		const FString* OpStr = EntryDef.Metadata.Find(TEXT("Op"));
		if (!OpStr)
		{
			OpStr = EntryDef.Metadata.Find(TEXT("Operator"));
		}

		if (OpStr && (*OpStr == TEXT(">=") || *OpStr == TEXT(">")))
		{
			if (NewValue.ValueType == EShadowSlaveWorldValueType::Int)
			{
				return (*OpStr == TEXT(">=")) ? (NewValue.IntValue >= ExpectedVal.IntValue) : (NewValue.IntValue > ExpectedVal.IntValue);
			}
			else if (NewValue.ValueType == EShadowSlaveWorldValueType::Float)
			{
				return (*OpStr == TEXT(">=")) ? (NewValue.FloatValue >= ExpectedVal.FloatValue) : (NewValue.FloatValue > ExpectedVal.FloatValue);
			}
			return false;
		}

		return NewValue == ExpectedVal;
	}

	// 3. Fallback: non-default / active value
	switch (NewValue.ValueType)
	{
	case EShadowSlaveWorldValueType::Bool:
		return NewValue.BoolValue;
	case EShadowSlaveWorldValueType::Int:
		return NewValue.IntValue > 0;
	case EShadowSlaveWorldValueType::Float:
		return NewValue.FloatValue > 0.0f;
	case EShadowSlaveWorldValueType::String:
		return !NewValue.StringValue.IsEmpty();
	case EShadowSlaveWorldValueType::Name:
		return !NewValue.NameValue.IsNone();
	default:
		return false;
	}
}

FName UShadowSlaveStorySubsystem::GetActiveObservedTargetId(FName StoryContentId) const
{
	if (StoryContentId.IsNone())
	{
		return NAME_None;
	}

	const FName ActiveEntryId = GetCurrentActiveStoryContentEntry(StoryContentId);
	if (ActiveEntryId.IsNone())
	{
		return NAME_None;
	}

	const UShadowSlaveStoryContentDefinition* Def = GetStoryContentDefinition(StoryContentId);
	if (!Def)
	{
		return NAME_None;
	}

	if (const FShadowSlaveStoryContentEntry* Entry = Def->FindContentEntry(ActiveEntryId))
	{
		return Entry->TargetId;
	}

	return NAME_None;
}

EShadowSlaveStoryContentType UShadowSlaveStorySubsystem::GetActiveObservedContentType(FName StoryContentId) const
{
	if (StoryContentId.IsNone())
	{
		return EShadowSlaveStoryContentType::None;
	}

	const FName ActiveEntryId = GetCurrentActiveStoryContentEntry(StoryContentId);
	if (ActiveEntryId.IsNone())
	{
		return EShadowSlaveStoryContentType::None;
	}

	const UShadowSlaveStoryContentDefinition* Def = GetStoryContentDefinition(StoryContentId);
	if (!Def)
	{
		return EShadowSlaveStoryContentType::None;
	}

	if (const FShadowSlaveStoryContentEntry* Entry = Def->FindContentEntry(ActiveEntryId))
	{
		return Entry->ContentType;
	}

	return EShadowSlaveStoryContentType::None;
}

FName UShadowSlaveStorySubsystem::FindNextProgressionEntryId(FName StoryContentId) const
{
	if (StoryContentId.IsNone())
	{
		return NAME_None;
	}

	const UShadowSlaveStoryContentDefinition* Def = GetStoryContentDefinition(StoryContentId);
	if (!Def)
	{
		return NAME_None;
	}

	for (const FShadowSlaveStoryContentEntry& Entry : Def->ContentEntries)
	{
		const EShadowSlaveStoryContentState EntryState = GetStoryContentEntryState(StoryContentId, Entry.ContentId);
		if (EntryState == EShadowSlaveStoryContentState::Completed ||
		    EntryState == EShadowSlaveStoryContentState::Failed ||
		    EntryState == EShadowSlaveStoryContentState::Skipped)
		{
			continue;
		}

		if (AreStoryContentEntryPrerequisitesSatisfied(StoryContentId, Entry.ContentId))
		{
			return Entry.ContentId;
		}
	}

	return NAME_None;
}

void UShadowSlaveStorySubsystem::AdvanceStoryContentProgression(FName StoryContentId)
{
	if (bIsRestoringState || bIsProcessingProgression || StoryContentId.IsNone())
	{
		return;
	}

	TGuardValue<bool> ProgressionGuard(bIsProcessingProgression, true);

	const UShadowSlaveStoryContentDefinition* Def = GetStoryContentDefinition(StoryContentId);
	if (!Def)
	{
		return;
	}

	FShadowSlaveStoryContentRuntimeState* RuntimeState = StoryContentRuntimeStates.Find(StoryContentId);
	if (!RuntimeState || RuntimeState->State != EShadowSlaveStoryContentState::Active)
	{
		return;
	}

	// If an entry is currently active, do not preempt it
	if (!RuntimeState->CurrentActiveEntryId.IsNone())
	{
		const EShadowSlaveStoryContentState CurrentEntryState = GetStoryContentEntryState(StoryContentId, RuntimeState->CurrentActiveEntryId);
		if (CurrentEntryState == EShadowSlaveStoryContentState::Active)
		{
			return;
		}
	}

	// Find next valid entry in authored ContentEntries array order
	const FName NextEntryId = FindNextProgressionEntryId(StoryContentId);
	if (!NextEntryId.IsNone())
	{
		ActivateStoryContentEntry(StoryContentId, NextEntryId);
		return;
	}

	// No more entries can be activated. If all required entries are completed, complete parent content!
	if (AreAllRequiredContentEntriesCompleted(StoryContentId))
	{
		CompleteStoryContent(StoryContentId);
	}
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

	const FName ContentId = ContentDef->GetStoryContentId();
	if (ContentId.IsNone())
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
				*ContentId.ToString(), *Err.ToString());
		}
		return false;
	}

	if (const TObjectPtr<UShadowSlaveStoryContentDefinition>* Existing = RegisteredContentDefinitions.Find(ContentId))
	{
		if (Existing->Get() == ContentDef)
		{
			// Idempotent re-registration of exact same object
			return true;
		}

		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveStorySubsystem::RegisterStoryContentDefinition - Conflict: StoryContentId '%s' already registered with different definition object ('%s' vs '%s')."),
			*ContentId.ToString(),
			Existing->Get() ? *Existing->Get()->GetName() : TEXT("null"),
			*ContentDef->GetName());
		return false;
	}

	RegisteredContentDefinitions.Add(ContentId, ContentDef);

	// If runtime state does not exist yet, initialize it
	if (!StoryContentRuntimeStates.Contains(ContentId))
	{
		const EShadowSlaveStoryContentState InitialState = AreStoryContentPrerequisitesSatisfied(ContentId)
			? EShadowSlaveStoryContentState::Available
			: EShadowSlaveStoryContentState::Locked;

		FShadowSlaveStoryContentRuntimeState NewEntry(ContentId, InitialState);
		InitializeRuntimeContentEntries(NewEntry, ContentDef);
		StoryContentRuntimeStates.Add(ContentId, NewEntry);
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

		const EShadowSlaveStoryContentState PrereqState = GetStoryContentEntryState(StoryContentId, PrereqEntryId);
		if (PrereqState == EShadowSlaveStoryContentState::Completed)
		{
			continue;
		}

		// Optional entries that were skipped do not permanently block downstream progression
		const FShadowSlaveStoryContentEntry* PrereqDef = Def->FindContentEntry(PrereqEntryId);
		if (PrereqDef && PrereqDef->bIsOptional && PrereqState == EShadowSlaveStoryContentState::Skipped)
		{
			continue;
		}

		return false;
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
 * Story Content Transitions (Step 25 & 26 Chapters/Arcs)
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

	// Terminal states cannot leave their state via normal transitions
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
		if (NewState == EShadowSlaveStoryContentState::Completed)
		{
			return AreAllRequiredContentEntriesCompleted(StoryContentId);
		}
		if (NewState == EShadowSlaveStoryContentState::Failed ||
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
	const bool bSuccess = SetStoryContentState(StoryContentId, EShadowSlaveStoryContentState::Active);
	if (!bSuccess)
	{
		return false;
	}

	if (!bIsRestoringState && !bIsProcessingProgression)
	{
		const FName ActiveEntry = GetCurrentActiveStoryContentEntry(StoryContentId);
		if (ActiveEntry.IsNone())
		{
			AdvanceStoryContentProgression(StoryContentId);
		}
	}

	return true;
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
 * Story Content Entry Transitions (Step 25 & 26 Chapters/Arcs)
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
	if (StoryContentId.IsNone() || EntryId.IsNone())
	{
		return false;
	}

	const UShadowSlaveStoryContentDefinition* Def = GetStoryContentDefinition(StoryContentId);
	if (!Def)
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveStorySubsystem::ActivateStoryContentEntry - Missing definition for '%s'"),
			*StoryContentId.ToString());
		return false;
	}

	const FShadowSlaveStoryContentEntry* EntryDef = Def->FindContentEntry(EntryId);
	if (!EntryDef)
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveStorySubsystem::ActivateStoryContentEntry - Entry '%s' not found in definition '%s'"),
			*EntryId.ToString(), *StoryContentId.ToString());
		return false;
	}

	// Validate prerequisites
	if (!AreStoryContentEntryPrerequisitesSatisfied(StoryContentId, EntryId))
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveStorySubsystem::ActivateStoryContentEntry - Prerequisites not satisfied for entry '%s' in '%s'"),
			*EntryId.ToString(), *StoryContentId.ToString());
		return false;
	}

	// If parent content is Available, transition it to Active first (Available -> Active)
	const EShadowSlaveStoryContentState CurrentArcState = GetStoryContentState(StoryContentId);
	if (CurrentArcState == EShadowSlaveStoryContentState::Available)
	{
		if (!SetStoryContentState(StoryContentId, EShadowSlaveStoryContentState::Active))
		{
			return false;
		}
	}
	else if (CurrentArcState != EShadowSlaveStoryContentState::Active)
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveStorySubsystem::ActivateStoryContentEntry - Cannot activate entry '%s' because parent '%s' is in state %d"),
			*EntryId.ToString(), *StoryContentId.ToString(), static_cast<uint8>(CurrentArcState));
		return false;
	}

	// Establish external domain observation if not already bound
	EnsureProgressionBridgeBound();

	// Set entry state through existing authoritative state mechanism
	return SetStoryContentEntryState(StoryContentId, EntryId, EShadowSlaveStoryContentState::Active);
}

bool UShadowSlaveStorySubsystem::CompleteStoryContentEntry(FName StoryContentId, FName EntryId)
{
	const bool bSuccess = SetStoryContentEntryState(StoryContentId, EntryId, EShadowSlaveStoryContentState::Completed);
	if (!bSuccess)
	{
		return false;
	}

	if (!bIsRestoringState && !bIsProcessingProgression)
	{
		AdvanceStoryContentProgression(StoryContentId);
	}

	return true;
}

bool UShadowSlaveStorySubsystem::FailStoryContentEntry(FName StoryContentId, FName EntryId)
{
	const UShadowSlaveStoryContentDefinition* Def = GetStoryContentDefinition(StoryContentId);
	const FShadowSlaveStoryContentEntry* EntryDef = Def ? Def->FindContentEntry(EntryId) : nullptr;
	const bool bIsOptional = EntryDef ? EntryDef->bIsOptional : false;

	const bool bSuccess = SetStoryContentEntryState(StoryContentId, EntryId, EShadowSlaveStoryContentState::Failed);
	if (!bSuccess)
	{
		return false;
	}

	if (bIsRestoringState || bIsProcessingProgression)
	{
		return true;
	}

	if (bIsOptional)
	{
		AdvanceStoryContentProgression(StoryContentId);
	}
	else
	{
		FailStoryContent(StoryContentId);
	}

	return true;
}

bool UShadowSlaveStorySubsystem::SkipStoryContentEntry(FName StoryContentId, FName EntryId)
{
	const bool bSuccess = SetStoryContentEntryState(StoryContentId, EntryId, EShadowSlaveStoryContentState::Skipped);
	if (!bSuccess)
	{
		return false;
	}

	if (!bIsRestoringState && !bIsProcessingProgression)
	{
		AdvanceStoryContentProgression(StoryContentId);
	}

	return true;
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
	// Pass 1: Resolve definitions, initialize authoritative entry structure from current definition,
	// and restore valid saved entry states while ignoring unknown states and nonexistent entries.
	struct FPendingStoryContentRestore
	{
		FName StoryContentId;
		EShadowSlaveStoryContentState DesiredState;
		FName DesiredActiveEntryId;
	};

	TArray<FPendingStoryContentRestore> PendingStoryContents;

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
		RuntimeEntry.RuntimeMetadata = SavedContentRecord.RuntimeMetadata;

		// Initialize all entries from definition first (authored entry set and ordering, all Locked)
		InitializeRuntimeContentEntries(RuntimeEntry, Def);

		// Restore saved child entries that exist in definition with valid states
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
				// Only restore structurally valid states
				if (SavedEntryState == EShadowSlaveStoryContentState::Locked ||
				    SavedEntryState == EShadowSlaveStoryContentState::Available ||
				    SavedEntryState == EShadowSlaveStoryContentState::Active ||
				    SavedEntryState == EShadowSlaveStoryContentState::Completed ||
				    SavedEntryState == EShadowSlaveStoryContentState::Failed ||
				    SavedEntryState == EShadowSlaveStoryContentState::Skipped)
				{
					EntryRuntime->State = SavedEntryState;
				}
			}
		}

		FPendingStoryContentRestore Pending;
		Pending.StoryContentId = SavedContentRecord.StoryContentId;
		Pending.DesiredState = SavedContentRecord.State;
		Pending.DesiredActiveEntryId = SavedContentRecord.CurrentActiveEntryId;
		PendingStoryContents.Add(Pending);
	}

	// Pass 2: Reconcile parent content states against restored entries and prerequisites.
	// Phase 2A: Resolve terminal states and Active-to-terminal normalizations first so subsequent prerequisite checks observe them.
	TSet<FName> FinalizedContentIds;

	for (const FPendingStoryContentRestore& Pending : PendingStoryContents)
	{
		const FName StoryContentId = Pending.StoryContentId;
		const EShadowSlaveStoryContentState DesiredState = Pending.DesiredState;
		FShadowSlaveStoryContentRuntimeState* RuntimeEntry = StoryContentRuntimeStates.Find(StoryContentId);
		const UShadowSlaveStoryContentDefinition* Def = GetStoryContentDefinition(StoryContentId);
		if (!RuntimeEntry || !Def)
		{
			continue;
		}

		// Saved terminal parents (Completed, Failed, Skipped)
		if (DesiredState == EShadowSlaveStoryContentState::Completed ||
		    DesiredState == EShadowSlaveStoryContentState::Failed ||
		    DesiredState == EShadowSlaveStoryContentState::Skipped)
		{
			RuntimeEntry->State = DesiredState;
			RuntimeEntry->CurrentActiveEntryId = NAME_None;
			// Terminal parents cannot have Active child entries
			for (auto& EntryPair : RuntimeEntry->EntryStates)
			{
				if (EntryPair.Value.State == EShadowSlaveStoryContentState::Active)
				{
					EntryPair.Value.State = EShadowSlaveStoryContentState::Locked;
				}
			}
			FinalizedContentIds.Add(StoryContentId);
		}
		else if (DesiredState == EShadowSlaveStoryContentState::Active)
		{
			// Active parent Case 1: If any required/non-optional entry is Failed, restore the parent as Failed
			bool bHasFailedRequiredEntry = false;
			for (const FShadowSlaveStoryContentEntry& EntryDef : Def->ContentEntries)
			{
				if (!EntryDef.bIsOptional)
				{
					const EShadowSlaveStoryContentState EntryState = GetStoryContentEntryState(StoryContentId, EntryDef.ContentId);
					if (EntryState == EShadowSlaveStoryContentState::Failed)
					{
						bHasFailedRequiredEntry = true;
						break;
					}
				}
			}

			if (bHasFailedRequiredEntry)
			{
				UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveStorySubsystem::ImportSaveData - Story content '%s' was saved as Active but has failed mandatory entries; normalizing to Failed."),
					*StoryContentId.ToString());
				RuntimeEntry->State = EShadowSlaveStoryContentState::Failed;
				RuntimeEntry->CurrentActiveEntryId = NAME_None;
				for (auto& EntryPair : RuntimeEntry->EntryStates)
				{
					if (EntryPair.Value.State == EShadowSlaveStoryContentState::Active)
					{
						EntryPair.Value.State = EShadowSlaveStoryContentState::Locked;
					}
				}
				FinalizedContentIds.Add(StoryContentId);
			}
			// Active parent Case 2: If all required/non-optional entries are Completed, restore the parent as Completed
			else if (AreAllRequiredContentEntriesCompleted(StoryContentId))
			{
				UE_LOG(LogShadowSlave, Log, TEXT("UShadowSlaveStorySubsystem::ImportSaveData - Story content '%s' was saved as Active with all required entries completed; normalizing to Completed."),
					*StoryContentId.ToString());
				RuntimeEntry->State = EShadowSlaveStoryContentState::Completed;
				RuntimeEntry->CurrentActiveEntryId = NAME_None;
				for (auto& EntryPair : RuntimeEntry->EntryStates)
				{
					if (EntryPair.Value.State == EShadowSlaveStoryContentState::Active)
					{
						EntryPair.Value.State = EShadowSlaveStoryContentState::Locked;
					}
				}
				FinalizedContentIds.Add(StoryContentId);
			}
		}
	}

	// Phase 2B: Resolve remaining non-terminal story contents (Active, Available, Locked) with prerequisite evaluation and active entry reconciliation
	for (const FPendingStoryContentRestore& Pending : PendingStoryContents)
	{
		const FName StoryContentId = Pending.StoryContentId;
		if (FinalizedContentIds.Contains(StoryContentId))
		{
			continue;
		}

		const EShadowSlaveStoryContentState DesiredState = Pending.DesiredState;
		FShadowSlaveStoryContentRuntimeState* RuntimeEntry = StoryContentRuntimeStates.Find(StoryContentId);
		const UShadowSlaveStoryContentDefinition* Def = GetStoryContentDefinition(StoryContentId);
		if (!RuntimeEntry || !Def)
		{
			continue;
		}

		if (DesiredState == EShadowSlaveStoryContentState::Active)
		{
			// Active parent: must satisfy parent prerequisites
			if (!AreStoryContentPrerequisitesSatisfied(StoryContentId))
			{
				UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveStorySubsystem::ImportSaveData - Story content '%s' was saved as Active but prerequisites are not satisfied; falling back to Locked."),
					*StoryContentId.ToString());
				RuntimeEntry->State = EShadowSlaveStoryContentState::Locked;
				RuntimeEntry->CurrentActiveEntryId = NAME_None;
				for (auto& EntryPair : RuntimeEntry->EntryStates)
				{
					if (EntryPair.Value.State == EShadowSlaveStoryContentState::Active ||
					    EntryPair.Value.State == EShadowSlaveStoryContentState::Available)
					{
						EntryPair.Value.State = EShadowSlaveStoryContentState::Locked;
					}
				}
				continue;
			}

			// Parent remains Active
			RuntimeEntry->State = EShadowSlaveStoryContentState::Active;

			// Reconcile single CurrentActiveEntryId deterministically
			FName SelectedActiveEntryId = NAME_None;

			// 1. Check if DesiredActiveEntryId from save is a valid candidate
			if (!Pending.DesiredActiveEntryId.IsNone() && Def->HasContentEntry(Pending.DesiredActiveEntryId))
			{
				const EShadowSlaveStoryContentState DesiredEntryState = GetStoryContentEntryState(StoryContentId, Pending.DesiredActiveEntryId);
				if (DesiredEntryState == EShadowSlaveStoryContentState::Active &&
				    AreStoryContentEntryPrerequisitesSatisfied(StoryContentId, Pending.DesiredActiveEntryId))
				{
					SelectedActiveEntryId = Pending.DesiredActiveEntryId;
				}
			}

			// 2. If not selected, check if any entry in authored order was saved as Active with prerequisites satisfied
			if (SelectedActiveEntryId.IsNone())
			{
				for (const FShadowSlaveStoryContentEntry& EntryDef : Def->ContentEntries)
				{
					const EShadowSlaveStoryContentState EntryState = GetStoryContentEntryState(StoryContentId, EntryDef.ContentId);
					if (EntryState == EShadowSlaveStoryContentState::Active &&
					    AreStoryContentEntryPrerequisitesSatisfied(StoryContentId, EntryDef.ContentId))
					{
						SelectedActiveEntryId = EntryDef.ContentId;
						break;
					}
				}
			}

			// 3. If still not selected (e.g. all saved entries were Locked or DesiredActiveEntryId was invalid),
			// deterministically select the first valid entry whose prerequisites are satisfied via FindNextProgressionEntryId
			if (SelectedActiveEntryId.IsNone())
			{
				SelectedActiveEntryId = FindNextProgressionEntryId(StoryContentId);
			}

			// Apply selected active entry and sanitize other entries
			RuntimeEntry->CurrentActiveEntryId = SelectedActiveEntryId;

			for (const FShadowSlaveStoryContentEntry& EntryDef : Def->ContentEntries)
			{
				FShadowSlaveStoryContentEntryRuntimeState* EntryRuntime = RuntimeEntry->EntryStates.Find(EntryDef.ContentId);
				if (!EntryRuntime)
				{
					continue;
				}

				if (!SelectedActiveEntryId.IsNone() && EntryDef.ContentId == SelectedActiveEntryId)
				{
					EntryRuntime->State = EShadowSlaveStoryContentState::Active;
				}
				else
				{
					// No other entry can be Active
					if (EntryRuntime->State == EShadowSlaveStoryContentState::Active)
					{
						EntryRuntime->State = AreStoryContentEntryPrerequisitesSatisfied(StoryContentId, EntryDef.ContentId)
							? EShadowSlaveStoryContentState::Available
							: EShadowSlaveStoryContentState::Locked;
					}
					else if (EntryRuntime->State == EShadowSlaveStoryContentState::Available)
					{
						if (!AreStoryContentEntryPrerequisitesSatisfied(StoryContentId, EntryDef.ContentId))
						{
							EntryRuntime->State = EShadowSlaveStoryContentState::Locked;
						}
					}
					// Terminal states (Completed, Failed, Skipped) and Locked remain preserved
				}
			}
		}
		else if (DesiredState == EShadowSlaveStoryContentState::Available)
		{
			if (AreStoryContentPrerequisitesSatisfied(StoryContentId))
			{
				RuntimeEntry->State = EShadowSlaveStoryContentState::Available;
			}
			else
			{
				UE_LOG(LogShadowSlave, Warning, TEXT("UShadowSlaveStorySubsystem::ImportSaveData - Story content '%s' was saved as Available but prerequisites are not satisfied; falling back to Locked."),
					*StoryContentId.ToString());
				RuntimeEntry->State = EShadowSlaveStoryContentState::Locked;
			}

			RuntimeEntry->CurrentActiveEntryId = NAME_None;

			for (const FShadowSlaveStoryContentEntry& EntryDef : Def->ContentEntries)
			{
				FShadowSlaveStoryContentEntryRuntimeState* EntryRuntime = RuntimeEntry->EntryStates.Find(EntryDef.ContentId);
				if (!EntryRuntime)
				{
					continue;
				}

				// Child entries cannot be Active under Available or Locked parent
				if (EntryRuntime->State == EShadowSlaveStoryContentState::Active)
				{
					EntryRuntime->State = (RuntimeEntry->State == EShadowSlaveStoryContentState::Available && AreStoryContentEntryPrerequisitesSatisfied(StoryContentId, EntryDef.ContentId))
						? EShadowSlaveStoryContentState::Available
						: EShadowSlaveStoryContentState::Locked;
				}
				else if (EntryRuntime->State == EShadowSlaveStoryContentState::Available)
				{
					if (RuntimeEntry->State != EShadowSlaveStoryContentState::Available || !AreStoryContentEntryPrerequisitesSatisfied(StoryContentId, EntryDef.ContentId))
					{
						EntryRuntime->State = EShadowSlaveStoryContentState::Locked;
					}
				}
			}
		}
		else // Locked or any other state
		{
			RuntimeEntry->State = EShadowSlaveStoryContentState::Locked;
			RuntimeEntry->CurrentActiveEntryId = NAME_None;

			for (auto& EntryPair : RuntimeEntry->EntryStates)
			{
				if (EntryPair.Value.State == EShadowSlaveStoryContentState::Active ||
				    EntryPair.Value.State == EShadowSlaveStoryContentState::Available)
				{
					EntryPair.Value.State = EShadowSlaveStoryContentState::Locked;
				}
			}
		}
	}

	EnsureProgressionBridgeBound();

	return true;
}
