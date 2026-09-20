// Copyright Epic Games, Inc. All Rights Reserved.

#include "Aspects/ShadowSlaveAspectDefinition.h"

UShadowSlaveAspectDefinition::UShadowSlaveAspectDefinition()
{
	AspectId = NAME_None;
	DisplayName = FText::FromString(TEXT("Generic Aspect"));
	Description = FText::FromString(TEXT("A prototype Aspect definition."));
	AspectRank = EShadowSlaveAspectRank::Unknown;
}

FPrimaryAssetId UShadowSlaveAspectDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("Aspect"), GetFName());
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
