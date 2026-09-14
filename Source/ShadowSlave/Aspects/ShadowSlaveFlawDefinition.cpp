// Copyright Epic Games, Inc. All Rights Reserved.

#include "Aspects/ShadowSlaveFlawDefinition.h"

UShadowSlaveFlawDefinition::UShadowSlaveFlawDefinition()
{
	FlawId = NAME_None;
	DisplayName = FText::FromString(TEXT("Generic Flaw"));
	Description = FText::FromString(TEXT("A prototype Flaw definition."));
}

FPrimaryAssetId UShadowSlaveFlawDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("Flaw"), GetFName());
}
