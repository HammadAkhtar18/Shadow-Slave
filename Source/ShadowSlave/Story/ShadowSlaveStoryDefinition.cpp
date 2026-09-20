// Copyright Epic Games, Inc. All Rights Reserved.

#include "Story/ShadowSlaveStoryDefinition.h"

UShadowSlaveStoryDefinition::UShadowSlaveStoryDefinition()
	: UShadowSlaveContentDefinition()
{
	ContentId = NAME_None;
	ContentType = EShadowSlaveContentType::Story;
	DisplayName = FText::GetEmpty();
	Description = FText::GetEmpty();
	Version = 1;
}

FPrimaryAssetId UShadowSlaveStoryDefinition::GetPrimaryAssetId() const
{
	return Super::GetPrimaryAssetId();
}

bool UShadowSlaveStoryDefinition::IsValidDefinition(FString* OutErrorMessage) const
{
	// 1. Generic content definition validation (valid ContentId, Version >= 1)
	if (!Super::IsValidDefinition(OutErrorMessage))
	{
		return false;
	}

	// 2. Generic content type must be Story
	if (ContentType != EShadowSlaveContentType::Story)
	{
		if (OutErrorMessage)
		{
			*OutErrorMessage = FString::Printf(TEXT("Story definition '%s' must have ContentType == EShadowSlaveContentType::Story."),
				*ContentId.ToString());
		}
		return false;
	}

	// 3. Generic display name must not be empty
	if (DisplayName.IsEmptyOrWhitespace())
	{
		if (OutErrorMessage)
		{
			*OutErrorMessage = FString::Printf(TEXT("Story definition '%s' must have a non-empty DisplayName."),
				*ContentId.ToString());
		}
		return false;
	}

	// 4. Story-specific validation
	TArray<FText> StoryErrors;
	if (!ValidateDefinition(StoryErrors))
	{
		if (OutErrorMessage && StoryErrors.Num() > 0)
		{
			*OutErrorMessage = StoryErrors[0].ToString();
		}
		return false;
	}

	return true;
}

bool UShadowSlaveStoryDefinition::ValidateDefinition(TArray<FText>& OutErrors) const
{
	OutErrors.Empty();

	if (ContentId.IsNone())
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
		else if (PrereqId == ContentId)
		{
			OutErrors.Add(FText::Format(
				NSLOCTEXT("ShadowSlave", "StoryDef_SelfPrereq", "Story '{0}' cannot list itself as a prerequisite."),
				FText::FromName(ContentId)
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
