// Copyright Epic Games, Inc. All Rights Reserved.

#include "Echoes/ShadowSlaveEchoDefinition.h"

UShadowSlaveEchoDefinition::UShadowSlaveEchoDefinition()
	: UShadowSlaveContentDefinition()
{
	ContentId = NAME_None;
	ContentType = EShadowSlaveContentType::Echo;
	DisplayName = FText::FromString(TEXT("Echo"));
	Description = FText::GetEmpty();
	Version = 1;
	Rank = EShadowSlaveEchoRank::Unknown;
	Class = EShadowSlaveEchoClass::Unknown;
	SummonEssenceCost = 0.0f;
}

FPrimaryAssetId UShadowSlaveEchoDefinition::GetPrimaryAssetId() const
{
	return Super::GetPrimaryAssetId();
}

bool UShadowSlaveEchoDefinition::IsValidDefinition(FString* OutErrorMessage) const
{
	// 1. Generic content definition validation (valid ContentId, Version >= 1)
	if (!Super::IsValidDefinition(OutErrorMessage))
	{
		return false;
	}

	// 2. Generic content type validation: must be Echo
	if (ContentType != EShadowSlaveContentType::Echo)
	{
		if (OutErrorMessage)
		{
			*OutErrorMessage = FString::Printf(TEXT("Echo definition '%s' must have ContentType == EShadowSlaveContentType::Echo."),
				*ContentId.ToString());
		}
		return false;
	}

	// 3. Echo-specific validation: summon essence cost must not be negative
	if (SummonEssenceCost < 0.0f)
	{
		if (OutErrorMessage)
		{
			*OutErrorMessage = FString::Printf(TEXT("Echo definition '%s' has negative SummonEssenceCost (%.2f)."),
				*ContentId.ToString(), SummonEssenceCost);
		}
		return false;
	}

	return true;
}
