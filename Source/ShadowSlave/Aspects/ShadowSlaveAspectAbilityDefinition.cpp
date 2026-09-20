// Copyright Epic Games, Inc. All Rights Reserved.

#include "Aspects/ShadowSlaveAspectAbilityDefinition.h"
#include "Aspects/ShadowSlaveAspectTypes.h"

FName FShadowSlaveAspectAbilityInstance::GetAbilityId() const
{
	return AbilityDefinition ? AbilityDefinition->GetAbilityId() : NAME_None;
}

UShadowSlaveAspectAbilityDefinition::UShadowSlaveAspectAbilityDefinition()
	: UShadowSlaveContentDefinition()
{
	ContentId = NAME_None;
	ContentType = EShadowSlaveContentType::Ability;
	DisplayName = FText::FromString(TEXT("Generic Aspect Ability"));
	Description = FText::FromString(TEXT("A prototype Aspect Ability definition."));
	Version = 1;
	RequiredCharacterRank = EShadowSlaveCharacterRank::Unknown;
	BaseEssenceCost = 0.0f;
}

FPrimaryAssetId UShadowSlaveAspectAbilityDefinition::GetPrimaryAssetId() const
{
	return Super::GetPrimaryAssetId();
}

bool UShadowSlaveAspectAbilityDefinition::IsValidDefinition(FString* OutErrorMessage) const
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
			*OutErrorMessage = FString::Printf(TEXT("Ability definition '%s' must have a non-empty DisplayName."),
				*ContentId.ToString());
		}
		return false;
	}

	// 3. Generic content type validation: must be Ability
	if (ContentType != EShadowSlaveContentType::Ability)
	{
		if (OutErrorMessage)
		{
			*OutErrorMessage = FString::Printf(TEXT("Ability definition '%s' must have ContentType == EShadowSlaveContentType::Ability."),
				*ContentId.ToString());
		}
		return false;
	}

	// 4. Ability-specific validation: BaseEssenceCost must be finite and non-negative
	if (!FMath::IsFinite(BaseEssenceCost) || BaseEssenceCost < 0.0f)
	{
		if (OutErrorMessage)
		{
			*OutErrorMessage = FString::Printf(TEXT("Ability definition '%s' has invalid BaseEssenceCost (%.2f)."),
				*ContentId.ToString(), BaseEssenceCost);
		}
		return false;
	}

	return true;
}
