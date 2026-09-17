// Copyright Epic Games, Inc. All Rights Reserved.

#include "Story/ShadowSlaveStoryDefinition.h"

UShadowSlaveStoryDefinition::UShadowSlaveStoryDefinition()
{
	Version = 1;
}

FPrimaryAssetId UShadowSlaveStoryDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("ShadowSlaveStory"), StoryId.IsNone() ? GetFName() : StoryId);
}

bool UShadowSlaveStoryDefinition::ValidateDefinition(TArray<FText>& OutErrors) const
{
	OutErrors.Empty();

	if (StoryId.IsNone())
	{
		OutErrors.Add(NSLOCTEXT("ShadowSlave", "StoryDef_MissingId", "StoryId is None; a valid unique FName is required."));
	}

	if (Version < 1)
	{
		OutErrors.Add(NSLOCTEXT("ShadowSlave", "StoryDef_InvalidVersion", "Version must be at least 1."));
	}

	TSet<FName> SeenPrerequisites;
	for (const FName& PrereqId : PrerequisiteStoryIds)
	{
		if (PrereqId.IsNone())
		{
			OutErrors.Add(NSLOCTEXT("ShadowSlave", "StoryDef_NonePrereq", "Prerequisite list contains an empty or None identifier."));
		}
		else if (PrereqId == StoryId)
		{
			OutErrors.Add(FText::Format(
				NSLOCTEXT("ShadowSlave", "StoryDef_SelfPrereq", "Story '{0}' cannot list itself as a prerequisite."),
				FText::FromName(StoryId)
			));
		}
		else if (SeenPrerequisites.Contains(PrereqId))
		{
			OutErrors.Add(FText::Format(
				NSLOCTEXT("ShadowSlave", "StoryDef_DuplicatePrereq", "Duplicate prerequisite '{0}' detected."),
				FText::FromName(PrereqId)
			));
		}
		else
		{
			SeenPrerequisites.Add(PrereqId);
		}
	}

	TSet<FName> SeenSteps;
	for (const FName& StepId : StepIds)
	{
		if (StepId.IsNone())
		{
			OutErrors.Add(NSLOCTEXT("ShadowSlave", "StoryDef_NoneStep", "Step list contains an empty or None identifier."));
		}
		else if (SeenSteps.Contains(StepId))
		{
			OutErrors.Add(FText::Format(
				NSLOCTEXT("ShadowSlave", "StoryDef_DuplicateStep", "Duplicate step ID '{0}' detected."),
				FText::FromName(StepId)
			));
		}
		else
		{
			SeenSteps.Add(StepId);
		}
	}

	return OutErrors.Num() == 0;
}
