// Copyright Epic Games, Inc. All Rights Reserved.

#include "StatusEffects/ShadowSlaveStatusEffectDefinition.h"

UShadowSlaveStatusEffectDefinition::UShadowSlaveStatusEffectDefinition()
	: UShadowSlaveContentDefinition()
{
	ContentId = NAME_None;
	ContentType = EShadowSlaveContentType::Custom;
	DisplayName = FText::FromString(TEXT("Status Effect"));
	Description = FText::GetEmpty();
	Version = 1;
	DurationPolicy = EStatusEffectDurationPolicy::Timed;
	Duration = 5.0f;
	StackingPolicy = EStatusEffectStackingPolicy::RefreshDuration;
	MaxStacks = 1;
	Polarity = EStatusEffectPolarity::Neutral;
	bPersistAcrossSaveLoad = false;
}

FPrimaryAssetId UShadowSlaveStatusEffectDefinition::GetPrimaryAssetId() const
{
	return Super::GetPrimaryAssetId();
}

bool UShadowSlaveStatusEffectDefinition::IsValidDefinition(FString* OutErrorMessage) const
{
	// 1. Generic content definition validation (valid ContentId, Version >= 1)
	if (!Super::IsValidDefinition(OutErrorMessage))
	{
		return false;
	}

	// 2. Generic content type validation: must be Custom
	if (ContentType != EShadowSlaveContentType::Custom)
	{
		if (OutErrorMessage)
		{
			*OutErrorMessage = FString::Printf(TEXT("StatusEffect definition '%s' must have ContentType == EShadowSlaveContentType::Custom."),
				*ContentId.ToString());
		}
		return false;
	}

	// 3. StatusEffect-specific validation: Timed effects require positive duration
	if (DurationPolicy == EStatusEffectDurationPolicy::Timed && Duration <= 0.0f)
	{
		if (OutErrorMessage)
		{
			*OutErrorMessage = FString::Printf(TEXT("StatusEffect definition '%s' has Timed DurationPolicy but non-positive Duration (%.2f)."),
				*ContentId.ToString(), Duration);
		}
		return false;
	}

	// 4. StatusEffect-specific validation: MaxStacks must be at least 1
	if (MaxStacks < 1)
	{
		if (OutErrorMessage)
		{
			*OutErrorMessage = FString::Printf(TEXT("StatusEffect definition '%s' has MaxStacks < 1 (%d)."),
				*ContentId.ToString(), MaxStacks);
		}
		return false;
	}

	return true;
}
