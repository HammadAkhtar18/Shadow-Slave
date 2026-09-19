// Copyright Epic Games, Inc. All Rights Reserved.

#include "Echoes/ShadowSlaveEchoDefinition.h"

UShadowSlaveEchoDefinition::UShadowSlaveEchoDefinition()
{
	EchoId = NAME_None;
	DisplayName = FText::FromString(TEXT("Echo"));
	Description = FText::GetEmpty();
	Rank = EShadowSlaveEchoRank::Unknown;
	Class = EShadowSlaveEchoClass::Unknown;
	SummonEssenceCost = 0.0f;
	EssenceUpkeepPerSecond = 0.0f;
}

FPrimaryAssetId UShadowSlaveEchoDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("ShadowSlaveEcho"), EchoId.IsNone() ? GetFName() : EchoId);
}
