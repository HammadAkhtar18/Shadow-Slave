// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gameplay/ShadowSlaveQuestDefinition.h"

UShadowSlaveQuestDefinition::UShadowSlaveQuestDefinition()
{
	Version = 1;
	bAutoCompleteWhenObjectivesComplete = true;
}

FPrimaryAssetId UShadowSlaveQuestDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("ShadowSlaveQuest"), QuestId.IsNone() ? GetFName() : QuestId);
}

const FShadowSlaveObjectiveDefinition* UShadowSlaveQuestDefinition::FindObjective(FName ObjectiveId) const
{
	if (ObjectiveId.IsNone())
	{
		return nullptr;
	}

	for (const FShadowSlaveObjectiveDefinition& ObjDef : Objectives)
	{
		if (ObjDef.ObjectiveId == ObjectiveId)
		{
			return &ObjDef;
		}
	}

	return nullptr;
}

bool UShadowSlaveQuestDefinition::HasObjective(FName ObjectiveId) const
{
	return FindObjective(ObjectiveId) != nullptr;
}

bool UShadowSlaveQuestDefinition::ValidateDefinition(TArray<FText>& OutErrors) const
{
	OutErrors.Empty();

	if (QuestId.IsNone())
	{
		OutErrors.Add(NSLOCTEXT("ShadowSlave", "QuestDef_MissingId", "QuestId is None; a valid unique FName is required."));
	}

	if (Version < 1)
	{
		OutErrors.Add(NSLOCTEXT("ShadowSlave", "QuestDef_InvalidVersion", "Version must be at least 1."));
	}

	if (Objectives.Num() == 0)
	{
		OutErrors.Add(NSLOCTEXT("ShadowSlave", "QuestDef_NoObjectives", "Quest definition must contain at least one objective."));
	}

	TSet<FName> SeenPrerequisites;
	for (const FName& PrereqId : PrerequisiteQuestIds)
	{
		if (PrereqId.IsNone())
		{
			OutErrors.Add(NSLOCTEXT("ShadowSlave", "QuestDef_NonePrereq", "Prerequisite list contains an empty or None identifier."));
		}
		else if (PrereqId == QuestId)
		{
			OutErrors.Add(FText::Format(
				NSLOCTEXT("ShadowSlave", "QuestDef_SelfPrereq", "Quest '{0}' cannot list itself as a prerequisite."),
				FText::FromName(QuestId)
			));
		}
		else if (SeenPrerequisites.Contains(PrereqId))
		{
			OutErrors.Add(FText::Format(
				NSLOCTEXT("ShadowSlave", "QuestDef_DuplicatePrereq", "Duplicate prerequisite '{0}' detected."),
				FText::FromName(PrereqId)
			));
		}
		else
		{
			SeenPrerequisites.Add(PrereqId);
		}
	}

	TSet<FName> SeenObjectives;
	for (const FShadowSlaveObjectiveDefinition& ObjDef : Objectives)
	{
		if (ObjDef.ObjectiveId.IsNone())
		{
			OutErrors.Add(NSLOCTEXT("ShadowSlave", "QuestDef_NoneObjective", "Objective list contains an objective with an empty or None ObjectiveId."));
		}
		else if (SeenObjectives.Contains(ObjDef.ObjectiveId))
		{
			OutErrors.Add(FText::Format(
				NSLOCTEXT("ShadowSlave", "QuestDef_DuplicateObjective", "Duplicate objective ID '{0}' detected in quest '{1}'."),
				FText::FromName(ObjDef.ObjectiveId),
				FText::FromName(QuestId)
			));
		}
		else
		{
			SeenObjectives.Add(ObjDef.ObjectiveId);
		}

		if (ObjDef.RequiredQuantity < 1)
		{
			OutErrors.Add(FText::Format(
				NSLOCTEXT("ShadowSlave", "QuestDef_InvalidQuantity", "Objective '{0}' has invalid RequiredQuantity ({1}); must be at least 1."),
				FText::FromName(ObjDef.ObjectiveId),
				FText::AsNumber(ObjDef.RequiredQuantity)
			));
		}
	}

	return OutErrors.Num() == 0;
}
