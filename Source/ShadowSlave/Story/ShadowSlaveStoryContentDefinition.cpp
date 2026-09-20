// Copyright Epic Games, Inc. All Rights Reserved.

#include "Story/ShadowSlaveStoryContentDefinition.h"

UShadowSlaveStoryContentDefinition::UShadowSlaveStoryContentDefinition()
	: UShadowSlaveContentDefinition()
{
	ContentId = NAME_None;
	ContentType = EShadowSlaveContentType::Story;
	DisplayName = FText::FromString(TEXT("Story Arc"));
	Description = FText::GetEmpty();
	Version = 1;
}

FPrimaryAssetId UShadowSlaveStoryContentDefinition::GetPrimaryAssetId() const
{
	return Super::GetPrimaryAssetId();
}

bool UShadowSlaveStoryContentDefinition::IsValidDefinition(FString* OutErrorMessage) const
{
	// 1. Generic content definition validation (valid ContentId, Version >= 1)
	if (!Super::IsValidDefinition(OutErrorMessage))
	{
		return false;
	}

	// 2. Generic content type validation: must be Story
	if (ContentType != EShadowSlaveContentType::Story)
	{
		if (OutErrorMessage)
		{
			*OutErrorMessage = FString::Printf(TEXT("Story content definition '%s' must have ContentType == EShadowSlaveContentType::Story."),
				*ContentId.ToString());
		}
		return false;
	}

	// 3. Story-specific validation
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

const FShadowSlaveStoryContentEntry* UShadowSlaveStoryContentDefinition::FindContentEntry(FName InContentId) const
{
	if (InContentId.IsNone())
	{
		return nullptr;
	}

	for (const FShadowSlaveStoryContentEntry& Entry : ContentEntries)
	{
		if (Entry.ContentId == InContentId)
		{
			return &Entry;
		}
	}

	return nullptr;
}

int32 UShadowSlaveStoryContentDefinition::FindContentEntryIndex(FName InContentId) const
{
	if (InContentId.IsNone())
	{
		return INDEX_NONE;
	}

	for (int32 Index = 0; Index < ContentEntries.Num(); ++Index)
	{
		if (ContentEntries[Index].ContentId == InContentId)
		{
			return Index;
		}
	}

	return INDEX_NONE;
}

bool UShadowSlaveStoryContentDefinition::HasContentEntry(FName InContentId) const
{
	return FindContentEntryIndex(InContentId) != INDEX_NONE;
}

bool UShadowSlaveStoryContentDefinition::ValidateDefinition(TArray<FText>& OutErrors) const
{
	OutErrors.Empty();

	if (ContentId.IsNone())
	{
		OutErrors.Add(NSLOCTEXT("ShadowSlave", "StoryContentDef_MissingId", "ContentId is None; a valid unique FName is required."));
	}

	if (Version < 1)
	{
		OutErrors.Add(NSLOCTEXT("ShadowSlave", "StoryContentDef_InvalidVersion", "Version must be at least 1."));
	}

	if (ContentType != EShadowSlaveContentType::Story)
	{
		OutErrors.Add(NSLOCTEXT("ShadowSlave", "StoryContentDef_InvalidContentType", "ContentType must be EShadowSlaveContentType::Story."));
	}

	// Arc-level prerequisites validation
	TSet<FName> SeenPrerequisites;
	for (const FName& PrereqId : PrerequisiteStoryContentIds)
	{
		if (PrereqId.IsNone())
		{
			OutErrors.Add(NSLOCTEXT("ShadowSlave", "StoryContentDef_NonePrereq", "Prerequisite list contains an empty or None identifier."));
		}
		else if (PrereqId == ContentId)
		{
			OutErrors.Add(FText::Format(
				NSLOCTEXT("ShadowSlave", "StoryContentDef_SelfPrereq", "Story content '{0}' cannot list itself as a prerequisite."),
				FText::FromName(ContentId)
			));
		}
		else if (SeenPrerequisites.Contains(PrereqId))
		{
			OutErrors.Add(FText::Format(
				NSLOCTEXT("ShadowSlave", "StoryContentDef_DuplicatePrereq", "Duplicate prerequisite '{0}' detected."),
				FText::FromName(PrereqId)
			));
		}
		else
		{
			SeenPrerequisites.Add(PrereqId);
		}
	}

	// Content entries validation
	TSet<FName> SeenEntries;
	for (int32 Index = 0; Index < ContentEntries.Num(); ++Index)
	{
		const FShadowSlaveStoryContentEntry& Entry = ContentEntries[Index];

		if (Entry.ContentId.IsNone())
		{
			OutErrors.Add(FText::Format(
				NSLOCTEXT("ShadowSlave", "StoryContentDef_NoneEntryId", "Content entry at index {0} has an empty or None ContentId."),
				FText::AsNumber(Index)
			));
			continue;
		}

		if (SeenEntries.Contains(Entry.ContentId))
		{
			OutErrors.Add(FText::Format(
				NSLOCTEXT("ShadowSlave", "StoryContentDef_DuplicateEntryId", "Duplicate ContentId '{0}' detected in content entries."),
				FText::FromName(Entry.ContentId)
			));
		}
		else
		{
			SeenEntries.Add(Entry.ContentId);
		}

		if (Entry.Version < 1)
		{
			OutErrors.Add(FText::Format(
				NSLOCTEXT("ShadowSlave", "StoryContentDef_InvalidEntryVersion", "Content entry '{0}' has invalid Version (must be >= 1)."),
				FText::FromName(Entry.ContentId)
			));
		}

		if (Entry.ContentType == EShadowSlaveStoryContentType::None)
		{
			OutErrors.Add(FText::Format(
				NSLOCTEXT("ShadowSlave", "StoryContentDef_NoneContentType", "Content entry '{0}' has None ContentType."),
				FText::FromName(Entry.ContentId)
			));
		}

		// Content type to reference combination check
		const bool bRequiresTarget = (Entry.ContentType == EShadowSlaveStoryContentType::Quest ||
		                              Entry.ContentType == EShadowSlaveStoryContentType::Dialogue ||
		                              Entry.ContentType == EShadowSlaveStoryContentType::Nightmare ||
		                              Entry.ContentType == EShadowSlaveStoryContentType::WorldState ||
		                              Entry.ContentType == EShadowSlaveStoryContentType::Location ||
		                              Entry.ContentType == EShadowSlaveStoryContentType::Transition);

		if (bRequiresTarget && Entry.TargetId.IsNone())
		{
			OutErrors.Add(FText::Format(
				NSLOCTEXT("ShadowSlave", "StoryContentDef_MissingTargetId", "Content entry '{0}' requires a non-None TargetId."),
				FText::FromName(Entry.ContentId)
			));
		}

		// Entry prerequisite checks
		TSet<FName> SeenEntryPrereqs;
		for (const FName& EntryPrereqId : Entry.PrerequisiteContentIds)
		{
			if (EntryPrereqId.IsNone())
			{
				OutErrors.Add(FText::Format(
					NSLOCTEXT("ShadowSlave", "StoryContentDef_NoneEntryPrereq", "Content entry '{0}' contains a None prerequisite."),
					FText::FromName(Entry.ContentId)
				));
			}
			else if (EntryPrereqId == Entry.ContentId)
			{
				OutErrors.Add(FText::Format(
					NSLOCTEXT("ShadowSlave", "StoryContentDef_SelfEntryPrereq", "Content entry '{0}' cannot list itself as a prerequisite."),
					FText::FromName(Entry.ContentId)
				));
			}
			else if (SeenEntryPrereqs.Contains(EntryPrereqId))
			{
				OutErrors.Add(FText::Format(
					NSLOCTEXT("ShadowSlave", "StoryContentDef_DuplicateEntryPrereq", "Content entry '{0}' has duplicate prerequisite '{1}'."),
					FText::FromName(Entry.ContentId),
					FText::FromName(EntryPrereqId)
				));
			}
			else
			{
				SeenEntryPrereqs.Add(EntryPrereqId);
			}
		}
	}

	// Verify all entry prerequisites resolve within this definition
	for (const FShadowSlaveStoryContentEntry& Entry : ContentEntries)
	{
		for (const FName& EntryPrereqId : Entry.PrerequisiteContentIds)
		{
			if (!EntryPrereqId.IsNone() && EntryPrereqId != Entry.ContentId && !SeenEntries.Contains(EntryPrereqId))
			{
				OutErrors.Add(FText::Format(
					NSLOCTEXT("ShadowSlave", "StoryContentDef_UnresolvedEntryPrereq", "Content entry '{0}' references unknown prerequisite '{1}' within the same definition."),
					FText::FromName(Entry.ContentId),
					FText::FromName(EntryPrereqId)
				));
			}
		}
	}

	// Associated IDs validation
	auto ValidateUniqueIdList = [&OutErrors](const TArray<FName>& IdList, const TCHAR* ListCategory)
	{
		TSet<FName> SeenIds;
		for (const FName& Id : IdList)
		{
			if (Id.IsNone())
			{
				OutErrors.Add(FText::Format(
					NSLOCTEXT("ShadowSlave", "StoryContentDef_NoneAssociatedId", "{0} list contains an empty or None identifier."),
					FText::FromString(ListCategory)
				));
			}
			else if (SeenIds.Contains(Id))
			{
				OutErrors.Add(FText::Format(
					NSLOCTEXT("ShadowSlave", "StoryContentDef_DuplicateAssociatedId", "Duplicate identifier '{0}' detected in {1} list."),
					FText::FromName(Id),
					FText::FromString(ListCategory)
				));
			}
			else
			{
				SeenIds.Add(Id);
			}
		}
	};

	ValidateUniqueIdList(AssociatedQuestIds, TEXT("AssociatedQuestIds"));
	ValidateUniqueIdList(AssociatedDialogueIds, TEXT("AssociatedDialogueIds"));
	ValidateUniqueIdList(AssociatedNightmareScenarioIds, TEXT("AssociatedNightmareScenarioIds"));
	ValidateUniqueIdList(AssociatedWorldStateKeys, TEXT("AssociatedWorldStateKeys"));

	return OutErrors.Num() == 0;
}
