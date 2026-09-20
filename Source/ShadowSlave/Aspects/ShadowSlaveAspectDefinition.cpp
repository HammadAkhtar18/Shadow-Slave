// Copyright Epic Games, Inc. All Rights Reserved.

#include "Aspects/ShadowSlaveAspectDefinition.h"

UShadowSlaveAspectDefinition::UShadowSlaveAspectDefinition()
	: UShadowSlaveContentDefinition()
{
	ContentId = NAME_None;
	ContentType = EShadowSlaveContentType::Custom;
	DisplayName = FText::FromString(TEXT("Generic Aspect"));
	Description = FText::FromString(TEXT("A prototype Aspect definition."));
	Version = 1;
	AspectRank = EShadowSlaveAspectRank::Unknown;
}

FPrimaryAssetId UShadowSlaveAspectDefinition::GetPrimaryAssetId() const
{
	return Super::GetPrimaryAssetId();
}

bool UShadowSlaveAspectDefinition::IsValidDefinition(FString* OutErrorMessage) const
{
	// 1. Generic content definition validation (valid ContentId, Version >= 1)
	if (!Super::IsValidDefinition(OutErrorMessage))
	{
		return false;
	}

	// 2. Generic content validation: DisplayName must not be empty
	if (DisplayName.IsEmpty())
	{
		if (OutErrorMessage)
		{
			*OutErrorMessage = FString::Printf(TEXT("Aspect definition '%s' must have a non-empty DisplayName."),
				*ContentId.ToString());
		}
		return false;
	}

	// 3. Generic content type validation: must be Custom (generic taxonomy has no Aspect member)
	if (ContentType != EShadowSlaveContentType::Custom)
	{
		if (OutErrorMessage)
		{
			*OutErrorMessage = FString::Printf(TEXT("Aspect definition '%s' must have ContentType == EShadowSlaveContentType::Custom."),
				*ContentId.ToString());
		}
		return false;
	}

	return true;
}

UShadowSlaveAspectAbilityDefinition* UShadowSlaveAspectDefinition::FindAbilityById(FName AbilityId) const
{
	if (AbilityId.IsNone())
	{
		return nullptr;
	}

	for (const TObjectPtr<UShadowSlaveAspectAbilityDefinition>& AbilityDef : AbilityDefinitions)
	{
		if (AbilityDef && AbilityDef->GetAbilityId() == AbilityId)
		{
			return AbilityDef.Get();
		}
	}

	return nullptr;
}
