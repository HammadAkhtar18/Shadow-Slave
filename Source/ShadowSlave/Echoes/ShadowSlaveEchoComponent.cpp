// Copyright Epic Games, Inc. All Rights Reserved.

#include "Echoes/ShadowSlaveEchoComponent.h"
#include "Attributes/ShadowSlaveAttributeComponent.h"
#include "GameFramework/Actor.h"

UShadowSlaveEchoComponent::UShadowSlaveEchoComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

bool UShadowSlaveEchoComponent::AcquireEcho(UShadowSlaveEchoDefinition* EchoDef, FShadowSlaveEchoInstance& OutInstance)
{
	if (!EchoDef)
	{
		return false;
	}

	FShadowSlaveEchoInstance NewInstance(EchoDef);
	Echoes.Add(NewInstance);
	OutInstance = NewInstance;

	OnEchoAdded.Broadcast(NewInstance);
	OnEchoCollectionChanged.Broadcast();
	return true;
}

bool UShadowSlaveEchoComponent::AddEcho(UShadowSlaveEchoDefinition* EchoDef, FGuid& OutInstanceId)
{
	FShadowSlaveEchoInstance NewInstance;
	if (AcquireEcho(EchoDef, NewInstance))
	{
		OutInstanceId = NewInstance.InstanceId;
		return true;
	}

	OutInstanceId.Invalidate();
	return false;
}

bool UShadowSlaveEchoComponent::AddEchoSimple(UShadowSlaveEchoDefinition* EchoDef)
{
	FGuid DummyId;
	return AddEcho(EchoDef, DummyId);
}

bool UShadowSlaveEchoComponent::AddEchoInstance(const FShadowSlaveEchoInstance& InInstance)
{
	if (!InInstance.IsValid())
	{
		return false;
	}

	if (HasEchoByInstanceId(InInstance.InstanceId))
	{
		return false;
	}

	Echoes.Add(InInstance);
	OnEchoAdded.Broadcast(InInstance);
	OnEchoCollectionChanged.Broadcast();
	return true;
}

bool UShadowSlaveEchoComponent::RemoveEcho(const FGuid& InstanceId)
{
	if (!InstanceId.IsValid())
	{
		return false;
	}

	for (int32 Index = 0; Index < Echoes.Num(); ++Index)
	{
		if (Echoes[Index].InstanceId == InstanceId)
		{
			if (Echoes[Index].bIsSummoned)
			{
				DismissEcho(InstanceId);
			}

			FShadowSlaveEchoInstance RemovedInstance = Echoes[Index];
			Echoes.RemoveAt(Index);

			OnEchoRemoved.Broadcast(RemovedInstance);
			OnEchoCollectionChanged.Broadcast();
			return true;
		}
	}

	return false;
}

bool UShadowSlaveEchoComponent::RemoveEchoByDefinition(const UShadowSlaveEchoDefinition* EchoDef)
{
	if (!EchoDef)
	{
		return false;
	}

	for (int32 Index = 0; Index < Echoes.Num(); ++Index)
	{
		if (Echoes[Index].EchoDefinition == EchoDef)
		{
			return RemoveEcho(Echoes[Index].InstanceId);
		}
	}

	return false;
}

bool UShadowSlaveEchoComponent::DestroyEcho(const FGuid& InstanceId)
{
	if (!InstanceId.IsValid())
	{
		return false;
	}

	for (int32 Index = 0; Index < Echoes.Num(); ++Index)
	{
		if (Echoes[Index].InstanceId == InstanceId)
		{
			if (Echoes[Index].bIsSummoned)
			{
				DismissEcho(InstanceId);
			}

			FShadowSlaveEchoInstance DestroyedInstance = Echoes[Index];
			Echoes.RemoveAt(Index);

			OnEchoDestroyed.Broadcast(DestroyedInstance);
			OnEchoCollectionChanged.Broadcast();
			return true;
		}
	}

	return false;
}

void UShadowSlaveEchoComponent::ClearEchoes()
{
	if (Echoes.Num() == 0)
	{
		return;
	}

	for (FShadowSlaveEchoInstance& Echo : Echoes)
	{
		if (Echo.bIsSummoned)
		{
			Echo.bIsSummoned = false;
			Echo.State = EShadowSlaveEchoState::Dormant;
			OnEchoDismissed.Broadcast(Echo);
		}
	}

	Echoes.Empty();
	OnEchoCollectionChanged.Broadcast();
}

void UShadowSlaveEchoComponent::RestoreEchoes(const TArray<FShadowSlaveEchoInstance>& InInstances)
{
	Echoes.Empty();

	for (const FShadowSlaveEchoInstance& Instance : InInstances)
	{
		if (Instance.IsValid())
		{
			Echoes.Add(Instance);
		}
	}

	OnEchoCollectionChanged.Broadcast();
}

bool UShadowSlaveEchoComponent::SummonEcho(const FGuid& InstanceId)
{
	if (bIsProcessingEchoTransition || !InstanceId.IsValid())
	{
		return false;
	}

	for (int32 Index = 0; Index < Echoes.Num(); ++Index)
	{
		if (Echoes[Index].InstanceId == InstanceId)
		{
			if (Echoes[Index].bIsSummoned)
			{
				return true;
			}

			if (Echoes[Index].State == EShadowSlaveEchoState::Destroyed)
			{
				return false;
			}

			// Validate and consume essence if configured on definition
			if (Echoes[Index].EchoDefinition && Echoes[Index].EchoDefinition->SummonEssenceCost > 0.0f)
			{
				AActor* OwnerActor = GetOwner();
				if (!OwnerActor)
				{
					return false;
				}

				UShadowSlaveAttributeComponent* AttrComp = OwnerActor->FindComponentByClass<UShadowSlaveAttributeComponent>();
				if (!AttrComp || AttrComp->GetCurrentEssence() < Echoes[Index].EchoDefinition->SummonEssenceCost)
				{
					return false;
				}

				if (!AttrComp->ConsumeEssence(Echoes[Index].EchoDefinition->SummonEssenceCost))
				{
					return false;
				}
			}

			TGuardValue<bool> TransitionGuard(bIsProcessingEchoTransition, true);

			const EShadowSlaveEchoState OldState = Echoes[Index].State;
			Echoes[Index].bIsSummoned = true;
			Echoes[Index].State = EShadowSlaveEchoState::Summoned;

			OnEchoSummoned.Broadcast(Echoes[Index]);
			OnEchoStateChanged.Broadcast(Echoes[Index], OldState);
			return true;
		}
	}

	return false;
}

bool UShadowSlaveEchoComponent::DismissEcho(const FGuid& InstanceId)
{
	if (bIsProcessingEchoTransition || !InstanceId.IsValid())
	{
		return false;
	}

	for (int32 Index = 0; Index < Echoes.Num(); ++Index)
	{
		if (Echoes[Index].InstanceId == InstanceId)
		{
			if (!Echoes[Index].bIsSummoned)
			{
				return true;
			}

			TGuardValue<bool> TransitionGuard(bIsProcessingEchoTransition, true);

			const EShadowSlaveEchoState OldState = Echoes[Index].State;
			Echoes[Index].bIsSummoned = false;
			Echoes[Index].State = EShadowSlaveEchoState::Dormant;

			OnEchoDismissed.Broadcast(Echoes[Index]);
			OnEchoStateChanged.Broadcast(Echoes[Index], OldState);
			return true;
		}
	}

	return false;
}

bool UShadowSlaveEchoComponent::DismissAllEchoes()
{
	bool bAnyFailed = false;
	for (const FShadowSlaveEchoInstance& Echo : Echoes)
	{
		if (Echo.bIsSummoned)
		{
			if (!DismissEcho(Echo.InstanceId))
			{
				bAnyFailed = true;
			}
		}
	}

	return !bAnyFailed;
}

bool UShadowSlaveEchoComponent::SetEchoState(const FGuid& InstanceId, EShadowSlaveEchoState NewState)
{
	if (!InstanceId.IsValid())
	{
		return false;
	}

	for (int32 Index = 0; Index < Echoes.Num(); ++Index)
	{
		if (Echoes[Index].InstanceId == InstanceId)
		{
			if (Echoes[Index].State == NewState)
			{
				return true;
			}

			const EShadowSlaveEchoState OldState = Echoes[Index].State;
			Echoes[Index].State = NewState;
			Echoes[Index].bIsSummoned = (NewState == EShadowSlaveEchoState::Summoned);

			OnEchoStateChanged.Broadcast(Echoes[Index], OldState);
			return true;
		}
	}

	return false;
}

bool UShadowSlaveEchoComponent::GetEchoState(const FGuid& InstanceId, EShadowSlaveEchoState& OutState) const
{
	for (const FShadowSlaveEchoInstance& Echo : Echoes)
	{
		if (Echo.InstanceId == InstanceId)
		{
			OutState = Echo.State;
			return true;
		}
	}

	OutState = EShadowSlaveEchoState::Dormant;
	return false;
}

bool UShadowSlaveEchoComponent::SetEchoDynamicProperty(const FGuid& InstanceId, FName Key, const FString& Value)
{
	if (!InstanceId.IsValid() || Key.IsNone())
	{
		return false;
	}

	for (int32 Index = 0; Index < Echoes.Num(); ++Index)
	{
		if (Echoes[Index].InstanceId == InstanceId)
		{
			Echoes[Index].DynamicProperties.Add(Key, Value);
			OnEchoModified.Broadcast(Echoes[Index]);
			return true;
		}
	}

	return false;
}

bool UShadowSlaveEchoComponent::GetEchoDynamicProperty(const FGuid& InstanceId, FName Key, FString& OutValue) const
{
	if (!InstanceId.IsValid() || Key.IsNone())
	{
		return false;
	}

	for (const FShadowSlaveEchoInstance& Echo : Echoes)
	{
		if (Echo.InstanceId == InstanceId)
		{
			if (const FString* FoundValue = Echo.DynamicProperties.Find(Key))
			{
				OutValue = *FoundValue;
				return true;
			}
			return false;
		}
	}

	return false;
}

bool UShadowSlaveEchoComponent::RemoveEchoDynamicProperty(const FGuid& InstanceId, FName Key)
{
	if (!InstanceId.IsValid() || Key.IsNone())
	{
		return false;
	}

	for (int32 Index = 0; Index < Echoes.Num(); ++Index)
	{
		if (Echoes[Index].InstanceId == InstanceId)
		{
			if (Echoes[Index].DynamicProperties.Remove(Key) > 0)
			{
				OnEchoModified.Broadcast(Echoes[Index]);
				return true;
			}
			return false;
		}
	}

	return false;
}

bool UShadowSlaveEchoComponent::HasEcho(const UShadowSlaveEchoDefinition* EchoDef) const
{
	if (!EchoDef)
	{
		return false;
	}

	return Echoes.ContainsByPredicate([EchoDef](const FShadowSlaveEchoInstance& E)
	{
		return E.EchoDefinition == EchoDef;
	});
}

bool UShadowSlaveEchoComponent::HasEchoByInstanceId(const FGuid& InstanceId) const
{
	if (!InstanceId.IsValid())
	{
		return false;
	}

	return Echoes.ContainsByPredicate([InstanceId](const FShadowSlaveEchoInstance& E)
	{
		return E.InstanceId == InstanceId;
	});
}

bool UShadowSlaveEchoComponent::FindEcho(const FGuid& InstanceId, FShadowSlaveEchoInstance& OutInstance) const
{
	if (!InstanceId.IsValid())
	{
		return false;
	}

	for (const FShadowSlaveEchoInstance& Echo : Echoes)
	{
		if (Echo.InstanceId == InstanceId)
		{
			OutInstance = Echo;
			return true;
		}
	}

	return false;
}

bool UShadowSlaveEchoComponent::FindEchoByDefinition(const UShadowSlaveEchoDefinition* EchoDef, FShadowSlaveEchoInstance& OutInstance) const
{
	if (!EchoDef)
	{
		return false;
	}

	for (const FShadowSlaveEchoInstance& Echo : Echoes)
	{
		if (Echo.EchoDefinition == EchoDef)
		{
			OutInstance = Echo;
			return true;
		}
	}

	return false;
}

TArray<FShadowSlaveEchoInstance> UShadowSlaveEchoComponent::FindAllEchoesByDefinition(const UShadowSlaveEchoDefinition* EchoDef) const
{
	TArray<FShadowSlaveEchoInstance> Results;
	if (!EchoDef)
	{
		return Results;
	}

	for (const FShadowSlaveEchoInstance& Echo : Echoes)
	{
		if (Echo.EchoDefinition == EchoDef)
		{
			Results.Add(Echo);
		}
	}

	return Results;
}

bool UShadowSlaveEchoComponent::IsEchoSummoned(const FGuid& InstanceId) const
{
	if (!InstanceId.IsValid())
	{
		return false;
	}

	for (const FShadowSlaveEchoInstance& Echo : Echoes)
	{
		if (Echo.InstanceId == InstanceId)
		{
			return Echo.bIsSummoned;
		}
	}

	return false;
}

TArray<FShadowSlaveEchoInstance> UShadowSlaveEchoComponent::GetSummonedEchoes() const
{
	TArray<FShadowSlaveEchoInstance> Summoned;
	for (const FShadowSlaveEchoInstance& Echo : Echoes)
	{
		if (Echo.bIsSummoned)
		{
			Summoned.Add(Echo);
		}
	}

	return Summoned;
}

TArray<FShadowSlaveEchoInstance> UShadowSlaveEchoComponent::GetEchoesByRank(EShadowSlaveEchoRank Rank) const
{
	TArray<FShadowSlaveEchoInstance> Filtered;
	for (const FShadowSlaveEchoInstance& Echo : Echoes)
	{
		if (Echo.EchoDefinition && Echo.EchoDefinition->Rank == Rank)
		{
			Filtered.Add(Echo);
		}
	}

	return Filtered;
}

TArray<FShadowSlaveEchoInstance> UShadowSlaveEchoComponent::GetEchoesByClass(EShadowSlaveEchoClass Class) const
{
	TArray<FShadowSlaveEchoInstance> Filtered;
	for (const FShadowSlaveEchoInstance& Echo : Echoes)
	{
		if (Echo.EchoDefinition && Echo.EchoDefinition->Class == Class)
		{
			Filtered.Add(Echo);
		}
	}

	return Filtered;
}
