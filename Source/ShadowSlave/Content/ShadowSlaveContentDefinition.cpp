// Copyright Epic Games, Inc. All Rights Reserved.

#include "Content/ShadowSlaveContentDefinition.h"

UShadowSlaveContentDefinition::UShadowSlaveContentDefinition()
	: ContentId(NAME_None)
	, ContentType(EShadowSlaveContentType::None)
	, DisplayName(FText::GetEmpty())
	, Description(FText::GetEmpty())
	, Version(1)
	, ProvenanceNote(FString())
{
}

FPrimaryAssetId UShadowSlaveContentDefinition::GetPrimaryAssetId() const
{
	FName TypeName;
	switch (ContentType)
	{
	case EShadowSlaveContentType::Character:
		TypeName = TEXT("ContentCharacter");
		break;
	case EShadowSlaveContentType::Item:
		TypeName = TEXT("ContentItem");
		break;
	case EShadowSlaveContentType::Memory:
		TypeName = TEXT("Memory");
		break;
	case EShadowSlaveContentType::Echo:
		TypeName = TEXT("Echo");
		break;
	case EShadowSlaveContentType::Quest:
		TypeName = TEXT("ContentQuest");
		break;
	case EShadowSlaveContentType::Dialogue:
		TypeName = TEXT("ContentDialogue");
		break;
	case EShadowSlaveContentType::Story:
		TypeName = TEXT("ContentStory");
		break;
	case EShadowSlaveContentType::Ability:
		TypeName = TEXT("ContentAbility");
		break;
	case EShadowSlaveContentType::World:
		TypeName = TEXT("ContentWorld");
		break;
	case EShadowSlaveContentType::Custom:
		TypeName = TEXT("ContentCustom");
		break;
	case EShadowSlaveContentType::None:
	default:
		TypeName = TEXT("ContentDefinition");
		break;
	}

	return FPrimaryAssetId(TypeName, ContentId.IsNone() ? GetFName() : ContentId);
}

bool UShadowSlaveContentDefinition::IsValidDefinition(FString* OutErrorMessage) const
{
	if (ContentId.IsNone())
	{
		if (OutErrorMessage)
		{
			*OutErrorMessage = TEXT("ContentId must not be None.");
		}
		return false;
	}

	if (Version < 1)
	{
		if (OutErrorMessage)
		{
			*OutErrorMessage = TEXT("Version must be at least 1.");
		}
		return false;
	}

	return true;
}

bool UShadowSlaveContentDefinition::ValidateDefinition(FString& OutErrorMessage) const
{
	return IsValidDefinition(&OutErrorMessage);
}
