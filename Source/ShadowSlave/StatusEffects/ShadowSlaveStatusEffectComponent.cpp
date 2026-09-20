// Copyright Epic Games, Inc. All Rights Reserved.

#include "StatusEffects/ShadowSlaveStatusEffectComponent.h"
#include "TimerManager.h"
#include "Engine/World.h"

UShadowSlaveStatusEffectComponent::UShadowSlaveStatusEffectComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	bIsProcessingEffectTransition = false;
}

FGuid UShadowSlaveStatusEffectComponent::ApplyEffect(
	UShadowSlaveStatusEffectDefinition* EffectDefinition,
	const FShadowSlaveStatusEffectSource& Source,
	const TMap<FName, FString>& DynamicProperties)
{
	if (!EffectDefinition || !EffectDefinition->IsValidDefinition())
	{
		UE_LOG(LogTemp, Warning, TEXT("UShadowSlaveStatusEffectComponent::ApplyEffect - Invalid effect definition."));
		return FGuid();
	}

	if (bIsProcessingEffectTransition)
	{
		UE_LOG(LogTemp, Warning, TEXT("UShadowSlaveStatusEffectComponent::ApplyEffect - Re-entrant call prevented."));
		return FGuid();
	}

	TGuardValue<bool> ReentrancyGuard(bIsProcessingEffectTransition, true);

	// Instant duration policy: executes and broadcasts, does not persist in ActiveEffects
	if (EffectDefinition->DurationPolicy == EStatusEffectDurationPolicy::Instant)
	{
		FShadowSlaveStatusEffectInstance InstantInstance(EffectDefinition, Source);
		InstantInstance.DynamicProperties = DynamicProperties;

		OnStatusEffectApplied.Broadcast(InstantInstance);
		OnStatusEffectRemoved.Broadcast(InstantInstance);
		return InstantInstance.InstanceId;
	}

	// For Timed or Persistent, check existing instances with the same definition
	int32 ExistingIndex = ActiveEffects.IndexOfByPredicate([EffectDefinition](const FShadowSlaveStatusEffectInstance& Inst)
	{
		return Inst.EffectDefinition == EffectDefinition;
	});

	if (ExistingIndex != INDEX_NONE)
	{
		switch (EffectDefinition->StackingPolicy)
		{
		case EStatusEffectStackingPolicy::IgnoreNew:
		{
			return ActiveEffects[ExistingIndex].InstanceId;
		}

		case EStatusEffectStackingPolicy::RefreshDuration:
		{
			if (EffectDefinition->DurationPolicy == EStatusEffectDurationPolicy::Timed)
			{
				ClearExpirationTimer(ActiveEffects[ExistingIndex]);
				ActiveEffects[ExistingIndex].TotalDuration = EffectDefinition->Duration;
				ActiveEffects[ExistingIndex].ApplicationWorldTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
				ScheduleExpirationTimer(ActiveEffects[ExistingIndex]);
			}
			return ActiveEffects[ExistingIndex].InstanceId;
		}

		case EStatusEffectStackingPolicy::AddStacks:
		{
			const int32 OldStacks = ActiveEffects[ExistingIndex].CurrentStacks;
			if (OldStacks < EffectDefinition->MaxStacks)
			{
				ActiveEffects[ExistingIndex].CurrentStacks = FMath::Clamp(OldStacks + 1, 1, EffectDefinition->MaxStacks);
			}

			if (EffectDefinition->DurationPolicy == EStatusEffectDurationPolicy::Timed)
			{
				ClearExpirationTimer(ActiveEffects[ExistingIndex]);
				ActiveEffects[ExistingIndex].TotalDuration = EffectDefinition->Duration;
				ActiveEffects[ExistingIndex].ApplicationWorldTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
				ScheduleExpirationTimer(ActiveEffects[ExistingIndex]);
			}

			if (ActiveEffects[ExistingIndex].CurrentStacks != OldStacks)
			{
				OnStatusEffectStackChanged.Broadcast(ActiveEffects[ExistingIndex], OldStacks);
			}
			return ActiveEffects[ExistingIndex].InstanceId;
		}

		case EStatusEffectStackingPolicy::Replace:
		{
			FShadowSlaveStatusEffectInstance OldInstance = ActiveEffects[ExistingIndex];
			ClearExpirationTimer(ActiveEffects[ExistingIndex]);
			ActiveEffects.RemoveAt(ExistingIndex);

			OnStatusEffectRemoved.Broadcast(OldInstance);
			break;
		}
		}
	}

	// Create new active effect instance
	FShadowSlaveStatusEffectInstance NewInstance(EffectDefinition, Source);
	NewInstance.DynamicProperties = DynamicProperties;
	NewInstance.CurrentStacks = 1;

	if (EffectDefinition->DurationPolicy == EStatusEffectDurationPolicy::Timed)
	{
		NewInstance.TotalDuration = EffectDefinition->Duration;
		NewInstance.ApplicationWorldTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
		ScheduleExpirationTimer(NewInstance);
	}

	ActiveEffects.Add(NewInstance);

	OnStatusEffectApplied.Broadcast(NewInstance);
	OnStatusEffectCollectionChanged.Broadcast();

	return NewInstance.InstanceId;
}

FGuid UShadowSlaveStatusEffectComponent::ApplyEffectSimple(UShadowSlaveStatusEffectDefinition* EffectDefinition)
{
	return ApplyEffect(EffectDefinition, FShadowSlaveStatusEffectSource(), TMap<FName, FString>());
}

bool UShadowSlaveStatusEffectComponent::RemoveEffect(const FGuid& InstanceId)
{
	if (!InstanceId.IsValid() || bIsProcessingEffectTransition)
	{
		return false;
	}

	int32 Index = ActiveEffects.IndexOfByPredicate([&InstanceId](const FShadowSlaveStatusEffectInstance& Inst)
	{
		return Inst.InstanceId == InstanceId;
	});

	if (Index == INDEX_NONE)
	{
		return false;
	}

	TGuardValue<bool> ReentrancyGuard(bIsProcessingEffectTransition, true);

	FShadowSlaveStatusEffectInstance RemovedInstance = ActiveEffects[Index];
	ClearExpirationTimer(ActiveEffects[Index]);
	ActiveEffects.RemoveAt(Index);

	OnStatusEffectRemoved.Broadcast(RemovedInstance);
	OnStatusEffectCollectionChanged.Broadcast();

	return true;
}

bool UShadowSlaveStatusEffectComponent::RemoveEffectByDefinition(const UShadowSlaveStatusEffectDefinition* EffectDefinition)
{
	if (!EffectDefinition || bIsProcessingEffectTransition)
	{
		return false;
	}

	int32 Index = ActiveEffects.IndexOfByPredicate([EffectDefinition](const FShadowSlaveStatusEffectInstance& Inst)
	{
		return Inst.EffectDefinition == EffectDefinition;
	});

	if (Index == INDEX_NONE)
	{
		return false;
	}

	return RemoveEffect(ActiveEffects[Index].InstanceId);
}

int32 UShadowSlaveStatusEffectComponent::RemoveAllEffectsByDefinition(const UShadowSlaveStatusEffectDefinition* EffectDefinition)
{
	if (!EffectDefinition || bIsProcessingEffectTransition)
	{
		return 0;
	}

	TArray<FGuid> IdsToRemove;
	for (const FShadowSlaveStatusEffectInstance& Inst : ActiveEffects)
	{
		if (Inst.EffectDefinition == EffectDefinition)
		{
			IdsToRemove.Add(Inst.InstanceId);
		}
	}

	int32 RemovedCount = 0;
	for (const FGuid& Id : IdsToRemove)
	{
		if (RemoveEffect(Id))
		{
			RemovedCount++;
		}
	}

	return RemovedCount;
}

int32 UShadowSlaveStatusEffectComponent::RemoveAllEffects()
{
	if (bIsProcessingEffectTransition)
	{
		return 0;
	}

	const int32 Count = ActiveEffects.Num();
	if (Count == 0)
	{
		return 0;
	}

	TGuardValue<bool> ReentrancyGuard(bIsProcessingEffectTransition, true);

	TArray<FShadowSlaveStatusEffectInstance> RemovedEffects = MoveTemp(ActiveEffects);
	ActiveEffects.Empty();

	for (FShadowSlaveStatusEffectInstance& Inst : RemovedEffects)
	{
		ClearExpirationTimer(Inst);
		OnStatusEffectRemoved.Broadcast(Inst);
	}

	OnStatusEffectCollectionChanged.Broadcast();
	return Count;
}

void UShadowSlaveStatusEffectComponent::ClearEffects()
{
	RemoveAllEffects();
}

bool UShadowSlaveStatusEffectComponent::HasEffect(const UShadowSlaveStatusEffectDefinition* EffectDefinition) const
{
	if (!EffectDefinition)
	{
		return false;
	}

	return ActiveEffects.ContainsByPredicate([EffectDefinition](const FShadowSlaveStatusEffectInstance& Inst)
	{
		return Inst.EffectDefinition == EffectDefinition;
	});
}

bool UShadowSlaveStatusEffectComponent::HasEffectById(FName EffectId) const
{
	if (EffectId.IsNone())
	{
		return false;
	}

	return ActiveEffects.ContainsByPredicate([EffectId](const FShadowSlaveStatusEffectInstance& Inst)
	{
		return Inst.EffectDefinition && Inst.EffectDefinition->GetEffectId() == EffectId;
	});
}

bool UShadowSlaveStatusEffectComponent::HasEffectByInstanceId(const FGuid& InstanceId) const
{
	if (!InstanceId.IsValid())
	{
		return false;
	}

	return ActiveEffects.ContainsByPredicate([&InstanceId](const FShadowSlaveStatusEffectInstance& Inst)
	{
		return Inst.InstanceId == InstanceId;
	});
}

bool UShadowSlaveStatusEffectComponent::FindEffect(const UShadowSlaveStatusEffectDefinition* EffectDefinition, FShadowSlaveStatusEffectInstance& OutEffect) const
{
	if (!EffectDefinition)
	{
		return false;
	}

	const FShadowSlaveStatusEffectInstance* Found = ActiveEffects.FindByPredicate([EffectDefinition](const FShadowSlaveStatusEffectInstance& Inst)
	{
		return Inst.EffectDefinition == EffectDefinition;
	});

	if (Found)
	{
		OutEffect = *Found;
		return true;
	}
	return false;
}

bool UShadowSlaveStatusEffectComponent::FindEffectByInstanceId(const FGuid& InstanceId, FShadowSlaveStatusEffectInstance& OutEffect) const
{
	if (!InstanceId.IsValid())
	{
		return false;
	}

	const FShadowSlaveStatusEffectInstance* Found = ActiveEffects.FindByPredicate([&InstanceId](const FShadowSlaveStatusEffectInstance& Inst)
	{
		return Inst.InstanceId == InstanceId;
	});

	if (Found)
	{
		OutEffect = *Found;
		return true;
	}
	return false;
}

bool UShadowSlaveStatusEffectComponent::FindEffectById(FName EffectId, FShadowSlaveStatusEffectInstance& OutEffect) const
{
	if (EffectId.IsNone())
	{
		return false;
	}

	const FShadowSlaveStatusEffectInstance* Found = ActiveEffects.FindByPredicate([EffectId](const FShadowSlaveStatusEffectInstance& Inst)
	{
		return Inst.EffectDefinition && Inst.EffectDefinition->GetEffectId() == EffectId;
	});

	if (Found)
	{
		OutEffect = *Found;
		return true;
	}
	return false;
}

void UShadowSlaveStatusEffectComponent::GetAllEffectsByDefinition(const UShadowSlaveStatusEffectDefinition* EffectDefinition, TArray<FShadowSlaveStatusEffectInstance>& OutEffects) const
{
	OutEffects.Empty();
	if (!EffectDefinition)
	{
		return;
	}

	for (const FShadowSlaveStatusEffectInstance& Inst : ActiveEffects)
	{
		if (Inst.EffectDefinition == EffectDefinition)
		{
			OutEffects.Add(Inst);
		}
	}
}

int32 UShadowSlaveStatusEffectComponent::GetStackCount(const UShadowSlaveStatusEffectDefinition* EffectDefinition) const
{
	if (!EffectDefinition)
	{
		return 0;
	}

	int32 TotalStacks = 0;
	for (const FShadowSlaveStatusEffectInstance& Inst : ActiveEffects)
	{
		if (Inst.EffectDefinition == EffectDefinition)
		{
			TotalStacks += Inst.CurrentStacks;
		}
	}
	return TotalStacks;
}

float UShadowSlaveStatusEffectComponent::GetRemainingDuration(const FGuid& InstanceId) const
{
	if (!InstanceId.IsValid())
	{
		return 0.0f;
	}

	const FShadowSlaveStatusEffectInstance* Found = ActiveEffects.FindByPredicate([&InstanceId](const FShadowSlaveStatusEffectInstance& Inst)
	{
		return Inst.InstanceId == InstanceId;
	});

	if (!Found || !Found->EffectDefinition)
	{
		return 0.0f;
	}

	if (Found->EffectDefinition->DurationPolicy == EStatusEffectDurationPolicy::Persistent)
	{
		return -1.0f;
	}

	if (Found->EffectDefinition->DurationPolicy == EStatusEffectDurationPolicy::Instant)
	{
		return 0.0f;
	}

	if (GetWorld() && Found->ExpirationTimerHandle.IsValid())
	{
		return GetWorld()->GetTimerManager().GetTimerRemaining(Found->ExpirationTimerHandle);
	}

	const double Elapsed = (GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0) - Found->ApplicationWorldTime;
	return FMath::Max(0.0f, Found->TotalDuration - static_cast<float>(Elapsed));
}

void UShadowSlaveStatusEffectComponent::RestoreEffects(const TArray<FShadowSlaveStatusEffectInstance>& InEffects)
{
	if (bIsProcessingEffectTransition)
	{
		return;
	}

	TGuardValue<bool> ReentrancyGuard(bIsProcessingEffectTransition, true);

	for (FShadowSlaveStatusEffectInstance& Inst : ActiveEffects)
	{
		ClearExpirationTimer(Inst);
	}
	ActiveEffects.Empty();

	for (const FShadowSlaveStatusEffectInstance& Restored : InEffects)
	{
		if (Restored.IsValid())
		{
			FShadowSlaveStatusEffectInstance NewInst = Restored;
			if (NewInst.EffectDefinition->DurationPolicy == EStatusEffectDurationPolicy::Timed && NewInst.TotalDuration > 0.0f)
			{
				NewInst.ApplicationWorldTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
				ScheduleExpirationTimer(NewInst);
			}
			ActiveEffects.Add(NewInst);
			// NOTE: No OnStatusEffectApplied broadcast during restoration.
			// Restoration is reconstitution from serialized state, not a new gameplay application.
			// This prevents duplicate gameplay reactions (e.g. attribute modifications, UI notifications)
			// that already occurred during the original application.
		}
	}

	// Single collection-changed notification for the entire restoration batch
	OnStatusEffectCollectionChanged.Broadcast();
}

void UShadowSlaveStatusEffectComponent::ScheduleExpirationTimer(FShadowSlaveStatusEffectInstance& Instance)
{
	if (!GetWorld() || Instance.TotalDuration <= 0.0f)
	{
		return;
	}

	FTimerDelegate TimerDelegate;
	TimerDelegate.BindUObject(this, &UShadowSlaveStatusEffectComponent::HandleEffectExpired, Instance.InstanceId);
	GetWorld()->GetTimerManager().SetTimer(Instance.ExpirationTimerHandle, TimerDelegate, Instance.TotalDuration, false);
}

void UShadowSlaveStatusEffectComponent::ClearExpirationTimer(FShadowSlaveStatusEffectInstance& Instance)
{
	if (GetWorld() && Instance.ExpirationTimerHandle.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(Instance.ExpirationTimerHandle);
	}
	Instance.ExpirationTimerHandle.Invalidate();
}

void UShadowSlaveStatusEffectComponent::HandleEffectExpired(FGuid InstanceId)
{
	if (bIsProcessingEffectTransition)
	{
		return;
	}

	int32 Index = ActiveEffects.IndexOfByPredicate([&InstanceId](const FShadowSlaveStatusEffectInstance& Inst)
	{
		return Inst.InstanceId == InstanceId;
	});

	if (Index == INDEX_NONE)
	{
		return;
	}

	TGuardValue<bool> ReentrancyGuard(bIsProcessingEffectTransition, true);

	FShadowSlaveStatusEffectInstance ExpiredInstance = ActiveEffects[Index];
	ClearExpirationTimer(ActiveEffects[Index]);
	ActiveEffects.RemoveAt(Index);

	OnStatusEffectExpired.Broadcast(ExpiredInstance);
	OnStatusEffectRemoved.Broadcast(ExpiredInstance);
	OnStatusEffectCollectionChanged.Broadcast();
}

void UShadowSlaveStatusEffectComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	for (FShadowSlaveStatusEffectInstance& Inst : ActiveEffects)
	{
		ClearExpirationTimer(Inst);
	}
	ActiveEffects.Empty();

	Super::EndPlay(EndPlayReason);
}
