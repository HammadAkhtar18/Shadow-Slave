// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/ShadowSlaveProgressionStatusWidget.h"

UShadowSlaveProgressionStatusWidget::UShadowSlaveProgressionStatusWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	CharacterRank = EShadowSlaveCharacterRank::Unknown;
	CurrentSoulCores = 1;
	MaxSoulCores = 1;
	AspectDisplayName = FText::FromString(TEXT("Unknown Aspect"));
	AspectRank = EShadowSlaveAspectRank::Unknown;
}

void UShadowSlaveProgressionStatusWidget::UpdateRank(EShadowSlaveCharacterRank NewRank)
{
	CharacterRank = NewRank;
	OnRankUpdated(CharacterRank);
}

void UShadowSlaveProgressionStatusWidget::UpdateSoulCores(int32 InCurrentCores, int32 InMaxCores)
{
	CurrentSoulCores = InCurrentCores;
	MaxSoulCores = InMaxCores;
	OnSoulCoresUpdated(CurrentSoulCores, MaxSoulCores);
}

void UShadowSlaveProgressionStatusWidget::UpdateAspectInfo(const FText& InAspectName, EShadowSlaveAspectRank InAspectRank)
{
	AspectDisplayName = InAspectName;
	AspectRank = InAspectRank;
	OnAspectInfoUpdated(AspectDisplayName, AspectRank);
}

void UShadowSlaveProgressionStatusWidget::OnRankUpdated_Implementation(EShadowSlaveCharacterRank NewRank)
{
	// Hook for Blueprint UMG logic
}

void UShadowSlaveProgressionStatusWidget::OnSoulCoresUpdated_Implementation(int32 InCurrentCores, int32 InMaxCores)
{
	// Hook for Blueprint UMG logic
}

void UShadowSlaveProgressionStatusWidget::OnAspectInfoUpdated_Implementation(const FText& InAspectName, EShadowSlaveAspectRank InAspectRank)
{
	// Hook for Blueprint UMG logic
}
