// Copyright Epic Games, Inc. All Rights Reserved.

#include "Aspects/ShadowSlaveAspectAbilityDefinition.h"
#include "Aspects/ShadowSlaveAspectTypes.h"

FName FShadowSlaveAspectAbilityInstance::GetAbilityId() const
{
	return AbilityDefinition ? AbilityDefinition->AbilityId : NAME_None;
}

UShadowSlaveAspectAbilityDefinition::UShadowSlaveAspectAbilityDefinition()
{
	AbilityId = NAME_None;
	DisplayName = FText::FromString(TEXT("Generic Aspect Ability"));
	Description = FText::FromString(TEXT("A prototype Aspect Ability definition."));
	RequiredCharacterRank = EShadowSlaveCharacterRank::Unknown;
	BaseEssenceCost = 0.0f;
}

FPrimaryAssetId UShadowSlaveAspectAbilityDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("AspectAbility"), GetFName());
}
