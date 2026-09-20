// Copyright Epic Games, Inc. All Rights Reserved.

#include "Aspects/ShadowSlaveFlawDefinition.h"

UShadowSlaveFlawDefinition::UShadowSlaveFlawDefinition()
	: UShadowSlaveContentDefinition()
{
	ContentId = NAME_None;
	ContentType = EShadowSlaveContentType::Custom;
	DisplayName = FText::FromString(TEXT("Generic Flaw"));
	Description = FText::FromString(TEXT("A prototype Flaw definition."));
	Version = 1;
}

FPrimaryAssetId UShadowSlaveFlawDefinition::GetPrimaryAssetId() const
{
	return Super::GetPrimaryAssetId();
}

bool UShadowSlaveFlawDefinition::IsValidDefinition(FString* OutErrorMessage) const
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
			*OutErrorMessage = FString::Printf(TEXT("Flaw definition '%s' must have a non-empty DisplayName."),
				*ContentId.ToString());
		}
		return false;
	}

	// 3. Generic content type validation: must be Custom (generic taxonomy has no Flaw member)
	if (ContentType != EShadowSlaveContentType::Custom)
	{
		if (OutErrorMessage)
		{
			*OutErrorMessage = FString::Printf(TEXT("Flaw definition '%s' must have ContentType == EShadowSlaveContentType::Custom."),
				*ContentId.ToString());
		}
		return false;
	}

	return true;
}
