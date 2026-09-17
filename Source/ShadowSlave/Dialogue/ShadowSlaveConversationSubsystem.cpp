// Copyright Epic Games, Inc. All Rights Reserved.

#include "Dialogue/ShadowSlaveConversationSubsystem.h"
#include "Dialogue/ShadowSlaveDialogueDefinition.h"
#include "Items/ShadowSlaveInventoryComponent.h"
#include "Items/ShadowSlaveItemDefinition.h"
#include "Progression/ShadowSlaveProgressionComponent.h"
#include "Core/ShadowSlaveLogChannels.h"
#include "GameFramework/Actor.h"

namespace
{
	static bool CompareValues(float A, float B, EShadowSlaveComparisonOperator Op)
	{
		switch (Op)
		{
		case EShadowSlaveComparisonOperator::Equal:
			return FMath::IsNearlyEqual(A, B);
		case EShadowSlaveComparisonOperator::NotEqual:
			return !FMath::IsNearlyEqual(A, B);
		case EShadowSlaveComparisonOperator::GreaterThan:
			return A > B;
		case EShadowSlaveComparisonOperator::GreaterThanOrEqual:
			return A >= B;
		case EShadowSlaveComparisonOperator::LessThan:
			return A < B;
		case EShadowSlaveComparisonOperator::LessThanOrEqual:
			return A <= B;
		default:
			return false;
		}
	}
}

UShadowSlaveConversationSubsystem::UShadowSlaveConversationSubsystem()
{
	CurrentState = EShadowSlaveConversationState::Inactive;
	CurrentNodeId = NAME_None;
	ActiveDialogueDef = nullptr;
	bIsProcessingStep = false;
}

void UShadowSlaveConversationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	ResetState();
	ClearRuntimeVariables();
	UE_LOG(LogShadowSlave, Log, TEXT("UShadowSlaveConversationSubsystem initialized."));
}

void UShadowSlaveConversationSubsystem::Deinitialize()
{
	if (IsConversationActive())
	{
		AbortConversation();
	}
	ClearRuntimeVariables();
	Super::Deinitialize();
}

bool UShadowSlaveConversationSubsystem::IsConversationActive() const
{
	return CurrentState != EShadowSlaveConversationState::Inactive &&
	       CurrentState != EShadowSlaveConversationState::Completed &&
	       CurrentState != EShadowSlaveConversationState::Aborted;
}

AActor* UShadowSlaveConversationSubsystem::GetCurrentSpeaker() const
{
	return CurrentSpeakerActor.IsValid() ? CurrentSpeakerActor.Get() : nullptr;
}

AActor* UShadowSlaveConversationSubsystem::GetCurrentInteractor() const
{
	return CurrentInteractorActor.IsValid() ? CurrentInteractorActor.Get() : nullptr;
}

bool UShadowSlaveConversationSubsystem::StartConversation(UShadowSlaveDialogueDefinition* DialogueDef, AActor* InSpeaker, AActor* InInteractor)
{
	if (bIsProcessingStep)
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("StartConversation: Cannot start while a conversation step is being processed."));
		return false;
	}

	if (IsConversationActive())
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("StartConversation: Another conversation is already active. Current Dialogue: '%s'."),
			ActiveDialogueDef ? *ActiveDialogueDef->DialogueId.ToString() : TEXT("None"));
		return false;
	}

	if (!DialogueDef)
	{
		UE_LOG(LogShadowSlave, Error, TEXT("StartConversation: DialogueDef is null."));
		return false;
	}

	TArray<FText> ValidationErrors;
	if (!DialogueDef->ValidateDefinition(ValidationErrors))
	{
		UE_LOG(LogShadowSlave, Error, TEXT("StartConversation: Dialogue '%s' failed validation with %d error(s)."),
			*DialogueDef->DialogueId.ToString(), ValidationErrors.Num());
		for (const FText& Err : ValidationErrors)
		{
			UE_LOG(LogShadowSlave, Error, TEXT("  - %s"), *Err.ToString());
		}
		return false;
	}

	const FShadowSlaveDialogueNode* StartNode = DialogueDef->FindNode(DialogueDef->StartingNodeId);
	if (!StartNode)
	{
		UE_LOG(LogShadowSlave, Error, TEXT("StartConversation: Starting node '%s' could not be resolved."),
			*DialogueDef->StartingNodeId.ToString());
		return false;
	}

	bIsProcessingStep = true;

	ActiveDialogueDef = DialogueDef;
	CurrentSpeakerActor = InSpeaker;
	CurrentInteractorActor = InInteractor;
	CurrentState = EShadowSlaveConversationState::Starting;

	OnConversationStarted.Broadcast(ActiveDialogueDef, InSpeaker, InInteractor);

	DisplayNode(*StartNode);

	bIsProcessingStep = false;
	return true;
}

void UShadowSlaveConversationSubsystem::DisplayNode(const FShadowSlaveDialogueNode& Node)
{
	CurrentNodeId = Node.NodeId;
	CurrentNode = Node;
	CurrentState = EShadowSlaveConversationState::Displaying;

	// Execute any node-level entry consequences
	ExecuteConsequences(Node.NodeConsequences);

	// Evaluate conditions for each choice to assemble available choices
	CurrentAvailableChoices.Empty();
	for (const FShadowSlaveDialogueChoice& Choice : Node.Choices)
	{
		if (EvaluateConditions(Choice.Conditions))
		{
			CurrentAvailableChoices.Add(Choice);
		}
	}

	OnDialogueNodeDisplayed.Broadcast(CurrentNode, CurrentAvailableChoices);

	if (CurrentAvailableChoices.Num() > 0)
	{
		CurrentState = EShadowSlaveConversationState::WaitingForChoice;
		OnChoicesUpdated.Broadcast(CurrentAvailableChoices);
	}
	else
	{
		// Leaf node without choices: remains Displaying until AdvanceConversation() or AbortConversation()
		CurrentState = EShadowSlaveConversationState::Displaying;
	}
}

bool UShadowSlaveConversationSubsystem::SelectChoice(int32 ChoiceIndex)
{
	if (bIsProcessingStep || CurrentState != EShadowSlaveConversationState::WaitingForChoice)
	{
		return false;
	}

	if (!CurrentAvailableChoices.IsValidIndex(ChoiceIndex))
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("SelectChoice: Invalid choice index %d (Available count: %d)."),
			ChoiceIndex, CurrentAvailableChoices.Num());
		return false;
	}

	const FShadowSlaveDialogueChoice SelectedChoice = CurrentAvailableChoices[ChoiceIndex];

	// Strict requirement: Re-evaluate conditions immediately before execution
	if (!EvaluateConditions(SelectedChoice.Conditions))
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("SelectChoice: Choice '%s' conditions failed immediately prior to execution."),
			*SelectedChoice.ChoiceId.ToString());

		// Remove the newly invalid choice from available options and refresh UI
		CurrentAvailableChoices.RemoveAt(ChoiceIndex);
		OnChoicesUpdated.Broadcast(CurrentAvailableChoices);
		return false;
	}

	bIsProcessingStep = true;
	CurrentState = EShadowSlaveConversationState::Advancing;

	OnChoiceSelected.Broadcast(SelectedChoice, CurrentInteractorActor.Get());

	// Execute choice consequences
	ExecuteConsequences(SelectedChoice.Consequences);

	// Transition to target node or complete
	if (SelectedChoice.TargetNodeId.IsNone())
	{
		CompleteConversation();
		bIsProcessingStep = false;
		return true;
	}

	if (!ActiveDialogueDef)
	{
		AbortConversation();
		bIsProcessingStep = false;
		return false;
	}

	const FShadowSlaveDialogueNode* TargetNode = ActiveDialogueDef->FindNode(SelectedChoice.TargetNodeId);
	if (!TargetNode)
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("SelectChoice: TargetNodeId '%s' could not be found; completing conversation."),
			*SelectedChoice.TargetNodeId.ToString());
		CompleteConversation();
		bIsProcessingStep = false;
		return true;
	}

	DisplayNode(*TargetNode);
	bIsProcessingStep = false;
	return true;
}

bool UShadowSlaveConversationSubsystem::SelectChoiceById(FName ChoiceId)
{
	if (ChoiceId.IsNone())
	{
		return false;
	}

	for (int32 Index = 0; Index < CurrentAvailableChoices.Num(); ++Index)
	{
		if (CurrentAvailableChoices[Index].ChoiceId == ChoiceId)
		{
			return SelectChoice(Index);
		}
	}

	return false;
}

bool UShadowSlaveConversationSubsystem::AdvanceConversation()
{
	if (bIsProcessingStep || CurrentState != EShadowSlaveConversationState::Displaying)
	{
		return false;
	}

	// If choices are available, player must explicitly select a choice
	if (CurrentAvailableChoices.Num() > 0)
	{
		return false;
	}

	// Leaf / terminal dialogue beat completed cleanly
	CompleteConversation();
	return true;
}

bool UShadowSlaveConversationSubsystem::AbortConversation()
{
	if (CurrentState == EShadowSlaveConversationState::Inactive)
	{
		return false;
	}

	const FName LastNodeId = CurrentNodeId;
	const FName DialogId = ActiveDialogueDef ? ActiveDialogueDef->DialogueId : NAME_None;

	CurrentState = EShadowSlaveConversationState::Aborted;
	OnConversationAborted.Broadcast(DialogId, LastNodeId);

	ResetState();
	return true;
}

void UShadowSlaveConversationSubsystem::CompleteConversation()
{
	const FName DialogId = ActiveDialogueDef ? ActiveDialogueDef->DialogueId : NAME_None;

	CurrentState = EShadowSlaveConversationState::Completed;
	OnConversationCompleted.Broadcast(DialogId);

	ResetState();
}

void UShadowSlaveConversationSubsystem::ResetState()
{
	ActiveDialogueDef = nullptr;
	CurrentNodeId = NAME_None;
	CurrentNode = FShadowSlaveDialogueNode();
	CurrentAvailableChoices.Empty();
	CurrentSpeakerActor = nullptr;
	CurrentInteractorActor = nullptr;
	bIsProcessingStep = false;
	CurrentState = EShadowSlaveConversationState::Inactive;
}

bool UShadowSlaveConversationSubsystem::EvaluateCondition(const FShadowSlaveDialogueCondition& Condition) const
{
	bool bResult = true;

	switch (Condition.ConditionType)
	{
	case EShadowSlaveDialogueConditionType::AlwaysTrue:
		bResult = true;
		break;

	case EShadowSlaveDialogueConditionType::Flag:
		if (!Condition.ParamKey.IsNone())
		{
			const bool ActualVal = GetRuntimeFlag(Condition.ParamKey, false);
			bResult = (ActualVal == Condition.bExpectedFlagValue);
		}
		break;

	case EShadowSlaveDialogueConditionType::NumericComparison:
		if (!Condition.ParamKey.IsNone())
		{
			const float ActualVal = GetRuntimeNumericValue(Condition.ParamKey, 0.0f);
			bResult = CompareValues(ActualVal, Condition.ComparisonValue, Condition.ComparisonOperator);
		}
		break;

	case EShadowSlaveDialogueConditionType::HasItem:
		if (AActor* Interactor = CurrentInteractorActor.Get())
		{
			if (UShadowSlaveInventoryComponent* InvComp = Interactor->FindComponentByClass<UShadowSlaveInventoryComponent>())
			{
				UShadowSlaveItemDefinition* ItemDef = Condition.RequiredItemDef.Get();
				if (!ItemDef && !Condition.RequiredItemDef.IsNull())
				{
					ItemDef = Condition.RequiredItemDef.LoadSynchronous();
				}

				if (ItemDef)
				{
					bResult = InvComp->HasItem(ItemDef, FMath::Max(1, Condition.RequiredItemQuantity));
				}
				else if (!Condition.ParamKey.IsNone())
				{
					// Fallback lookup by technical FName or primary asset name
					int32 TotalCount = 0;
					for (const FShadowSlaveItemInstance& Slot : InvComp->GetSlots())
					{
						if (Slot.IsValid() && Slot.ItemDefinition)
						{
							if (Slot.ItemDefinition->GetFName() == Condition.ParamKey ||
								Slot.ItemDefinition->GetPrimaryAssetId().PrimaryAssetName == Condition.ParamKey)
							{
								TotalCount += Slot.Quantity;
							}
						}
					}
					bResult = TotalCount >= FMath::Max(1, Condition.RequiredItemQuantity);
				}
				else
				{
					bResult = false;
				}
			}
			else
			{
				bResult = false;
			}
		}
		else
		{
			bResult = false;
		}
		break;

	case EShadowSlaveDialogueConditionType::CharacterRank:
		if (AActor* Interactor = CurrentInteractorActor.Get())
		{
			if (UShadowSlaveProgressionComponent* ProgComp = Interactor->FindComponentByClass<UShadowSlaveProgressionComponent>())
			{
				bResult = (static_cast<uint8>(ProgComp->GetCharacterRank()) >= static_cast<uint8>(Condition.RequiredRank));
			}
			else
			{
				bResult = false;
			}
		}
		else
		{
			bResult = false;
		}
		break;

	case EShadowSlaveDialogueConditionType::SoulCores:
		if (AActor* Interactor = CurrentInteractorActor.Get())
		{
			if (UShadowSlaveProgressionComponent* ProgComp = Interactor->FindComponentByClass<UShadowSlaveProgressionComponent>())
			{
				const float Cores = static_cast<float>(ProgComp->GetSoulCoreCount());
				bResult = CompareValues(Cores, Condition.ComparisonValue, Condition.ComparisonOperator);
			}
			else
			{
				bResult = false;
			}
		}
		else
		{
			bResult = false;
		}
		break;

	default:
		bResult = true;
		break;
	}

	return Condition.bInvertCondition ? !bResult : bResult;
}

bool UShadowSlaveConversationSubsystem::EvaluateConditions(const TArray<FShadowSlaveDialogueCondition>& Conditions) const
{
	for (const FShadowSlaveDialogueCondition& Cond : Conditions)
	{
		if (!EvaluateCondition(Cond))
		{
			return false;
		}
	}
	return true;
}

void UShadowSlaveConversationSubsystem::ExecuteConsequence(const FShadowSlaveDialogueConsequence& Consequence)
{
	switch (Consequence.ConsequenceType)
	{
	case EShadowSlaveDialogueConsequenceType::None:
		break;

	case EShadowSlaveDialogueConsequenceType::SetFlag:
		if (!Consequence.TargetKey.IsNone())
		{
			SetRuntimeFlag(Consequence.TargetKey, Consequence.BoolValue);
		}
		break;

	case EShadowSlaveDialogueConsequenceType::ModifyNumeric:
		if (!Consequence.TargetKey.IsNone())
		{
			const float CurrentVal = GetRuntimeNumericValue(Consequence.TargetKey, 0.0f);
			SetRuntimeNumericValue(Consequence.TargetKey, CurrentVal + Consequence.NumericValue);
		}
		break;

	case EShadowSlaveDialogueConsequenceType::GiveItem:
		if (AActor* Interactor = CurrentInteractorActor.Get())
		{
			if (UShadowSlaveInventoryComponent* InvComp = Interactor->FindComponentByClass<UShadowSlaveInventoryComponent>())
			{
				UShadowSlaveItemDefinition* ItemDef = Consequence.ItemDef.Get();
				if (!ItemDef && !Consequence.ItemDef.IsNull())
				{
					ItemDef = Consequence.ItemDef.LoadSynchronous();
				}

				if (ItemDef)
				{
					InvComp->AddItemSimple(ItemDef, FMath::Max(1, Consequence.ItemQuantity));
				}
			}
		}
		break;

	case EShadowSlaveDialogueConsequenceType::RemoveItem:
		if (AActor* Interactor = CurrentInteractorActor.Get())
		{
			if (UShadowSlaveInventoryComponent* InvComp = Interactor->FindComponentByClass<UShadowSlaveInventoryComponent>())
			{
				UShadowSlaveItemDefinition* ItemDef = Consequence.ItemDef.Get();
				if (!ItemDef && !Consequence.ItemDef.IsNull())
				{
					ItemDef = Consequence.ItemDef.LoadSynchronous();
				}

				if (ItemDef)
				{
					InvComp->RemoveItem(ItemDef, FMath::Max(1, Consequence.ItemQuantity));
				}
			}
		}
		break;

	case EShadowSlaveDialogueConsequenceType::TriggerEvent:
		if (!Consequence.EventId.IsNone())
		{
			OnDialogueEventTriggered.Broadcast(Consequence.EventId, CurrentInteractorActor.Get(), CurrentSpeakerActor.Get());
		}
		break;

	default:
		break;
	}
}

void UShadowSlaveConversationSubsystem::ExecuteConsequences(const TArray<FShadowSlaveDialogueConsequence>& Consequences)
{
	for (const FShadowSlaveDialogueConsequence& Cons : Consequences)
	{
		ExecuteConsequence(Cons);
	}
}

bool UShadowSlaveConversationSubsystem::GetRuntimeFlag(FName Key, bool DefaultValue) const
{
	if (const bool* Found = RuntimeFlags.Find(Key))
	{
		return *Found;
	}
	return DefaultValue;
}

void UShadowSlaveConversationSubsystem::SetRuntimeFlag(FName Key, bool Value)
{
	if (!Key.IsNone())
	{
		RuntimeFlags.Add(Key, Value);
	}
}

float UShadowSlaveConversationSubsystem::GetRuntimeNumericValue(FName Key, float DefaultValue) const
{
	if (const float* Found = RuntimeNumericValues.Find(Key))
	{
		return *Found;
	}
	return DefaultValue;
}

void UShadowSlaveConversationSubsystem::SetRuntimeNumericValue(FName Key, float Value)
{
	if (!Key.IsNone())
	{
		RuntimeNumericValues.Add(Key, Value);
	}
}

FString UShadowSlaveConversationSubsystem::GetRuntimeMetadata(FName Key) const
{
	if (const FString* Found = RuntimeMetadata.Find(Key))
	{
		return *Found;
	}
	return FString();
}

void UShadowSlaveConversationSubsystem::SetRuntimeMetadata(FName Key, const FString& Value)
{
	if (!Key.IsNone())
	{
		RuntimeMetadata.Add(Key, Value);
	}
}

void UShadowSlaveConversationSubsystem::ClearRuntimeVariables()
{
	RuntimeFlags.Empty();
	RuntimeNumericValues.Empty();
	RuntimeMetadata.Empty();
}

void UShadowSlaveConversationSubsystem::CaptureConversationState(FShadowSlaveConversationSaveData& OutSaveData) const
{
	OutSaveData.DialogueId = ActiveDialogueDef ? ActiveDialogueDef->DialogueId : NAME_None;
	OutSaveData.DialogueVersion = ActiveDialogueDef ? ActiveDialogueDef->Version : 1;
	OutSaveData.CurrentNodeId = CurrentNodeId;
	OutSaveData.RuntimeFlags = RuntimeFlags;
	OutSaveData.RuntimeNumericValues = RuntimeNumericValues;
	OutSaveData.RuntimeMetadata = RuntimeMetadata;
	OutSaveData.bIsActive = IsConversationActive();
}

bool UShadowSlaveConversationSubsystem::RestoreConversationState(const FShadowSlaveConversationSaveData& InSaveData, UShadowSlaveDialogueDefinition* InDialogueDef)
{
	RuntimeFlags = InSaveData.RuntimeFlags;
	RuntimeNumericValues = InSaveData.RuntimeNumericValues;
	RuntimeMetadata = InSaveData.RuntimeMetadata;

	if (!InSaveData.bIsActive)
	{
		ResetState();
		return true;
	}

	if (!InDialogueDef || InDialogueDef->DialogueId != InSaveData.DialogueId)
	{
		UE_LOG(LogShadowSlave, Warning, TEXT("RestoreConversationState: Mismatched or null dialogue definition for saved dialogue '%s'."),
			*InSaveData.DialogueId.ToString());
		ResetState();
		return false;
	}

	ActiveDialogueDef = InDialogueDef;
	const FShadowSlaveDialogueNode* TargetNode = InDialogueDef->FindNode(InSaveData.CurrentNodeId);
	if (!TargetNode)
	{
		TargetNode = InDialogueDef->FindNode(InDialogueDef->StartingNodeId);
	}

	if (TargetNode)
	{
		DisplayNode(*TargetNode);
		return true;
	}

	ResetState();
	return false;
}
