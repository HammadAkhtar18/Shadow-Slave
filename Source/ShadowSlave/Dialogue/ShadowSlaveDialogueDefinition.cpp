// Copyright Epic Games, Inc. All Rights Reserved.

#include "Dialogue/ShadowSlaveDialogueDefinition.h"

UShadowSlaveDialogueDefinition::UShadowSlaveDialogueDefinition()
{
	DialogueId = NAME_None;
	DisplayName = FText::FromString(TEXT("Dialogue Definition"));
	Description = FText::GetEmpty();
	StartingNodeId = NAME_None;
	Version = 1;
}

FPrimaryAssetId UShadowSlaveDialogueDefinition::GetPrimaryAssetId() const
{
	const FName AssetName = DialogueId.IsNone() ? GetFName() : DialogueId;
	return FPrimaryAssetId(TEXT("ShadowSlaveDialogue"), AssetName);
}

const FShadowSlaveDialogueNode* UShadowSlaveDialogueDefinition::FindNode(FName InNodeId) const
{
	if (InNodeId.IsNone())
	{
		return nullptr;
	}

	for (const FShadowSlaveDialogueNode& Node : Nodes)
	{
		if (Node.NodeId == InNodeId)
		{
			return &Node;
		}
	}

	return nullptr;
}

bool UShadowSlaveDialogueDefinition::GetNodeById(FName InNodeId, FShadowSlaveDialogueNode& OutNode) const
{
	const FShadowSlaveDialogueNode* FoundNode = FindNode(InNodeId);
	if (FoundNode)
	{
		OutNode = *FoundNode;
		return true;
	}

	OutNode = FShadowSlaveDialogueNode();
	return false;
}

bool UShadowSlaveDialogueDefinition::ValidateDefinition(TArray<FText>& OutErrors) const
{
	OutErrors.Empty();

	if (DialogueId.IsNone())
	{
		OutErrors.Add(NSLOCTEXT("ShadowSlave", "DialogueError_MissingId", "DialogueId is empty or NAME_None."));
	}

	if (Version < 1)
	{
		OutErrors.Add(NSLOCTEXT("ShadowSlave", "DialogueError_InvalidVersion", "Dialogue version must be at least 1."));
	}

	if (StartingNodeId.IsNone())
	{
		OutErrors.Add(NSLOCTEXT("ShadowSlave", "DialogueError_MissingStartingNode", "StartingNodeId is empty or NAME_None."));
	}

	if (Nodes.Num() == 0)
	{
		OutErrors.Add(NSLOCTEXT("ShadowSlave", "DialogueError_NoNodes", "Dialogue definition contains no nodes."));
		return false;
	}

	TSet<FName> KnownNodeIds;
	bool bFoundStartingNode = false;

	// Phase 1: Validate node uniqueness and starting node existence
	for (int32 i = 0; i < Nodes.Num(); ++i)
	{
		const FShadowSlaveDialogueNode& Node = Nodes[i];

		if (Node.NodeId.IsNone())
		{
			OutErrors.Add(FText::Format(
				NSLOCTEXT("ShadowSlave", "DialogueError_NodeMissingId", "Node at index {0} has an empty NodeId."),
				FText::AsNumber(i)
			));
			continue;
		}

		if (KnownNodeIds.Contains(Node.NodeId))
		{
			OutErrors.Add(FText::Format(
				NSLOCTEXT("ShadowSlave", "DialogueError_DuplicateNodeId", "Duplicate NodeId '{0}' detected."),
				FText::FromName(Node.NodeId)
			));
		}
		else
		{
			KnownNodeIds.Add(Node.NodeId);
		}

		if (Node.NodeId == StartingNodeId)
		{
			bFoundStartingNode = true;
		}

		if (Node.DialogueText.IsEmpty())
		{
			OutErrors.Add(FText::Format(
				NSLOCTEXT("ShadowSlave", "DialogueError_EmptyDialogueText", "Node '{0}' has empty DialogueText."),
				FText::FromName(Node.NodeId)
			));
		}
	}

	if (!StartingNodeId.IsNone() && !bFoundStartingNode)
	{
		OutErrors.Add(FText::Format(
			NSLOCTEXT("ShadowSlave", "DialogueError_StartingNodeNotFound", "StartingNodeId '{0}' does not exist in the node collection."),
			FText::FromName(StartingNodeId)
		));
	}

	// Phase 2: Validate choices within each node
	for (const FShadowSlaveDialogueNode& Node : Nodes)
	{
		TSet<FName> ChoiceIdsInNode;

		for (int32 ChoiceIdx = 0; ChoiceIdx < Node.Choices.Num(); ++ChoiceIdx)
		{
			const FShadowSlaveDialogueChoice& Choice = Node.Choices[ChoiceIdx];

			if (Choice.ChoiceId.IsNone())
			{
				OutErrors.Add(FText::Format(
					NSLOCTEXT("ShadowSlave", "DialogueError_ChoiceMissingId", "Node '{0}' choice at index {1} has empty ChoiceId."),
					FText::FromName(Node.NodeId),
					FText::AsNumber(ChoiceIdx)
				));
			}
			else if (ChoiceIdsInNode.Contains(Choice.ChoiceId))
			{
				OutErrors.Add(FText::Format(
					NSLOCTEXT("ShadowSlave", "DialogueError_DuplicateChoiceId", "Node '{0}' contains duplicate ChoiceId '{1}'."),
					FText::FromName(Node.NodeId),
					FText::FromName(Choice.ChoiceId)
				));
			}
			else
			{
				ChoiceIdsInNode.Add(Choice.ChoiceId);
			}

			if (Choice.ChoiceText.IsEmpty())
			{
				OutErrors.Add(FText::Format(
					NSLOCTEXT("ShadowSlave", "DialogueError_EmptyChoiceText", "Node '{0}' choice '{1}' has empty ChoiceText."),
					FText::FromName(Node.NodeId),
					FText::FromName(Choice.ChoiceId)
				));
			}

			// TargetNodeId of NAME_None is valid and represents an intentional dialogue completion choice.
			// If a non-None target is specified, it MUST exist in the graph.
			if (!Choice.TargetNodeId.IsNone() && !KnownNodeIds.Contains(Choice.TargetNodeId))
			{
				OutErrors.Add(FText::Format(
					NSLOCTEXT("ShadowSlave", "DialogueError_TargetNodeNotFound", "Node '{0}' choice '{1}' references nonexistent TargetNodeId '{2}'."),
					FText::FromName(Node.NodeId),
					FText::FromName(Choice.ChoiceId),
					FText::FromName(Choice.TargetNodeId)
				));
			}
		}
	}

	return OutErrors.Num() == 0;
}

UShadowSlaveDialogueDefinition* UShadowSlaveDialogueDefinition::CreateTestDialogueDefinition(UObject* Outer)
{
	UObject* EffectiveOuter = Outer ? Outer : GetTransientPackage();
	UShadowSlaveDialogueDefinition* NewDef = NewObject<UShadowSlaveDialogueDefinition>(EffectiveOuter);

	NewDef->DialogueId = FName(TEXT("Test_Dialogue_Generic"));
	NewDef->DisplayName = FText::FromString(TEXT("Generic Test Dialogue"));
	NewDef->Description = FText::FromString(TEXT("A prototype branching dialogue definition for automated testing."));
	NewDef->StartingNodeId = FName(TEXT("Node_Greeting"));
	NewDef->Version = 1;

	// Node 1: Greeting
	FShadowSlaveDialogueNode NodeGreeting;
	NodeGreeting.NodeId = FName(TEXT("Node_Greeting"));
	NodeGreeting.SpeakerId = FName(TEXT("NPC_Guide"));
	NodeGreeting.SpeakerDisplayName = FText::FromString(TEXT("Guide"));
	NodeGreeting.DialogueText = FText::FromString(TEXT("Greetings, traveler. How may I assist you today?"));

	// Choice 1 -> Node_Info
	FShadowSlaveDialogueChoice ChoiceInfo;
	ChoiceInfo.ChoiceId = FName(TEXT("Choice_AskInfo"));
	ChoiceInfo.ChoiceText = FText::FromString(TEXT("Tell me about this area."));
	ChoiceInfo.TargetNodeId = FName(TEXT("Node_Info"));
	NodeGreeting.Choices.Add(ChoiceInfo);

	// Choice 2 -> Node_Supplies
	FShadowSlaveDialogueChoice ChoiceSupplies;
	ChoiceSupplies.ChoiceId = FName(TEXT("Choice_AskSupplies"));
	ChoiceSupplies.ChoiceText = FText::FromString(TEXT("Do you have any supplies?"));
	ChoiceSupplies.TargetNodeId = FName(TEXT("Node_Supplies"));
	NodeGreeting.Choices.Add(ChoiceSupplies);

	// Choice 3 -> Exit (TargetNodeId = None)
	FShadowSlaveDialogueChoice ChoiceLeave;
	ChoiceLeave.ChoiceId = FName(TEXT("Choice_LeaveGreeting"));
	ChoiceLeave.ChoiceText = FText::FromString(TEXT("Farewell."));
	ChoiceLeave.TargetNodeId = NAME_None;
	NodeGreeting.Choices.Add(ChoiceLeave);

	NewDef->Nodes.Add(NodeGreeting);

	// Node 2: Info (Branches back or exits)
	FShadowSlaveDialogueNode NodeInfo;
	NodeInfo.NodeId = FName(TEXT("Node_Info"));
	NodeInfo.SpeakerId = FName(TEXT("NPC_Guide"));
	NodeInfo.SpeakerDisplayName = FText::FromString(TEXT("Guide"));
	NodeInfo.DialogueText = FText::FromString(TEXT("This domain is fraught with danger. Tread carefully."));

	FShadowSlaveDialogueChoice ChoiceBackFromInfo;
	ChoiceBackFromInfo.ChoiceId = FName(TEXT("Choice_BackFromInfo"));
	ChoiceBackFromInfo.ChoiceText = FText::FromString(TEXT("I have another question."));
	ChoiceBackFromInfo.TargetNodeId = FName(TEXT("Node_Greeting"));
	NodeInfo.Choices.Add(ChoiceBackFromInfo);

	FShadowSlaveDialogueChoice ChoiceLeaveFromInfo;
	ChoiceLeaveFromInfo.ChoiceId = FName(TEXT("Choice_LeaveFromInfo"));
	ChoiceLeaveFromInfo.ChoiceText = FText::FromString(TEXT("Understood. Farewell."));
	ChoiceLeaveFromInfo.TargetNodeId = NAME_None;
	NodeInfo.Choices.Add(ChoiceLeaveFromInfo);

	NewDef->Nodes.Add(NodeInfo);

	// Node 3: Supplies (Sets a flag and branches back or exits)
	FShadowSlaveDialogueNode NodeSupplies;
	NodeSupplies.NodeId = FName(TEXT("Node_Supplies"));
	NodeSupplies.SpeakerId = FName(TEXT("NPC_Guide"));
	NodeSupplies.SpeakerDisplayName = FText::FromString(TEXT("Guide"));
	NodeSupplies.DialogueText = FText::FromString(TEXT("I have shared what little I can spare."));

	FShadowSlaveDialogueConsequence ConsequenceSetFlag;
	ConsequenceSetFlag.ConsequenceType = EShadowSlaveDialogueConsequenceType::SetFlag;
	ConsequenceSetFlag.TargetKey = FName(TEXT("HasAskedSupplies"));
	ConsequenceSetFlag.BoolValue = true;
	NodeSupplies.NodeConsequences.Add(ConsequenceSetFlag);

	FShadowSlaveDialogueChoice ChoiceBackFromSupplies;
	ChoiceBackFromSupplies.ChoiceId = FName(TEXT("Choice_BackFromSupplies"));
	ChoiceBackFromSupplies.ChoiceText = FText::FromString(TEXT("Thank you. I have another question."));
	ChoiceBackFromSupplies.TargetNodeId = FName(TEXT("Node_Greeting"));
	NodeSupplies.Choices.Add(ChoiceBackFromSupplies);

	FShadowSlaveDialogueChoice ChoiceLeaveFromSupplies;
	ChoiceLeaveFromSupplies.ChoiceId = FName(TEXT("Choice_LeaveFromSupplies"));
	ChoiceLeaveFromSupplies.ChoiceText = FText::FromString(TEXT("Thank you. Farewell."));
	ChoiceLeaveFromSupplies.TargetNodeId = NAME_None;
	NodeSupplies.Choices.Add(ChoiceLeaveFromSupplies);

	NewDef->Nodes.Add(NodeSupplies);

	return NewDef;
}
