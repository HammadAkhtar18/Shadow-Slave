// Copyright Epic Games, Inc. All Rights Reserved.

#include "StatusEffects/ShadowSlaveStatusEffectDefinition.h"

UShadowSlaveStatusEffectDefinition::UShadowSlaveStatusEffectDefinition()
	: EffectId(NAME_None)
	, DisplayName(FText::GetEmpty())
	, Description(FText::GetEmpty())
	, DurationPolicy(EStatusEffectDurationPolicy::Timed)
	, Duration(5.0f)
	, StackingPolicy(EStatusEffectStackingPolicy::RefreshDuration)
	, MaxStacks(1)
	, Polarity(EStatusEffectPolarity::Neutral)
	, bPersistAcrossSaveLoad(false)
{
}

FPrimaryAssetId UShadowSlaveStatusEffectDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("StatusEffectDefinition"), EffectId.IsNone() ? GetFName() : EffectId);
}

bool UShadowSlaveStatusEffectDefinition::IsValidDefinition() const
{
	if (EffectId.IsNone())
	{
		return false;
	}

	if (DurationPolicy == EStatusEffectDurationPolicy::Timed && Duration <= 0.0f)
	{
		return false;
	}

	if (MaxStacks < 1)
	{
		return false;
	}

	return true;
}
