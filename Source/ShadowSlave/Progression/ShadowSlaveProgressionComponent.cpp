// Copyright Epic Games, Inc. All Rights Reserved.

#include "Progression/ShadowSlaveProgressionComponent.h"
#include "Attributes/ShadowSlaveAttributeComponent.h"
#include "GameFramework/Actor.h"

UShadowSlaveProgressionComponent::UShadowSlaveProgressionComponent()
{
	// Operates purely event-driven; no tick overhead
	PrimaryComponentTick.bCanEverTick = false;
	CharacterRank = EShadowSlaveCharacterRank::Unknown;
	SoulCoreState = FShadowSlaveSoulCoreState(1, 1);
}

bool UShadowSlaveProgressionComponent::SetCharacterRank(EShadowSlaveCharacterRank NewRank)
{
	if (CharacterRank == NewRank)
	{
		return true;
	}

	const EShadowSlaveCharacterRank OldRank = CharacterRank;
	CharacterRank = NewRank;

	OnCharacterRankChanged.Broadcast(NewRank, OldRank);
	return true;
}

bool UShadowSlaveProgressionComponent::CanAdvanceRank(EShadowSlaveCharacterRank TargetRank) const
{
	if (TargetRank == EShadowSlaveCharacterRank::Unknown || TargetRank == CharacterRank)
	{
		return false;
	}

	// Technical check that target rank is higher than current rank
	return static_cast<uint8>(TargetRank) > static_cast<uint8>(CharacterRank);
}

bool UShadowSlaveProgressionComponent::AdvanceRank(EShadowSlaveCharacterRank TargetRank)
{
	if (!CanAdvanceRank(TargetRank))
	{
		return false;
	}

	return SetCharacterRank(TargetRank);
}

bool UShadowSlaveProgressionComponent::SetSoulCoreCount(int32 NewCount)
{
	const int32 ClampedCount = FMath::Clamp(NewCount, 0, SoulCoreState.MaximumSoulCores);
	if (SoulCoreState.CurrentSoulCores == ClampedCount)
	{
		return true;
	}

	const int32 OldCount = SoulCoreState.CurrentSoulCores;
	SoulCoreState.CurrentSoulCores = ClampedCount;

	OnSoulCoreCountChanged.Broadcast(ClampedCount, OldCount);
	return true;
}

bool UShadowSlaveProgressionComponent::SetMaxSoulCores(int32 NewMax)
{
	const int32 ClampedMax = FMath::Max(1, NewMax);
	if (SoulCoreState.MaximumSoulCores == ClampedMax)
	{
		return true;
	}

	const int32 OldMax = SoulCoreState.MaximumSoulCores;
	SoulCoreState.MaximumSoulCores = ClampedMax;

	OnMaxSoulCoresChanged.Broadcast(ClampedMax, OldMax);

	// If current cores exceed the new maximum, clamp down
	if (SoulCoreState.CurrentSoulCores > ClampedMax)
	{
		SetSoulCoreCount(ClampedMax);
	}

	return true;
}

bool UShadowSlaveProgressionComponent::AddSoulCore(int32 Count)
{
	if (Count <= 0)
	{
		return false;
	}

	return SetSoulCoreCount(SoulCoreState.CurrentSoulCores + Count);
}

bool UShadowSlaveProgressionComponent::RemoveSoulCore(int32 Count)
{
	if (Count <= 0)
	{
		return false;
	}

	return SetSoulCoreCount(SoulCoreState.CurrentSoulCores - Count);
}

void UShadowSlaveProgressionComponent::SetProgressionMetadata(FName Key, const FString& Value)
{
	ProgressionMetadata.Add(Key, Value);
}

bool UShadowSlaveProgressionComponent::GetProgressionMetadata(FName Key, FString& OutValue) const
{
	if (const FString* Found = ProgressionMetadata.Find(Key))
	{
		OutValue = *Found;
		return true;
	}
	return false;
}

bool UShadowSlaveProgressionComponent::RemoveProgressionMetadata(FName Key)
{
	return ProgressionMetadata.Remove(Key) > 0;
}

UShadowSlaveAttributeComponent* UShadowSlaveProgressionComponent::GetAttributeComponent() const
{
	AActor* OwnerActor = GetOwner();
	return OwnerActor ? OwnerActor->FindComponentByClass<UShadowSlaveAttributeComponent>() : nullptr;
}
