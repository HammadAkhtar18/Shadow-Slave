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
	if (bIsProcessingEchoTransition || bIsTearingDown || !EchoDef)
	{
		return false;
	}

	TGuardValue<bool> TransitionGuard(bIsProcessingEchoTransition, true);

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
	if (bIsProcessingEchoTransition || bIsTearingDown || !InInstance.IsValid())
	{
		return false;
	}

	if (HasEchoByInstanceId(InInstance.InstanceId))
	{
		return false;
	}

	TGuardValue<bool> TransitionGuard(bIsProcessingEchoTransition, true);

	Echoes.Add(InInstance);
	OnEchoAdded.Broadcast(InInstance);
	OnEchoCollectionChanged.Broadcast();
	return true;
}

bool UShadowSlaveEchoComponent::RemoveEcho(const FGuid& InstanceId)
{
	if (bIsProcessingEchoTransition || bIsTearingDown || !InstanceId.IsValid())
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

			TGuardValue<bool> TransitionGuard(bIsProcessingEchoTransition, true);

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
	if (bIsProcessingEchoTransition || bIsTearingDown || !InstanceId.IsValid())
	{
		return false;
	}

	for (int32 Index = 0; Index < Echoes.Num(); ++Index)
	{
		if (Echoes[Index].InstanceId == InstanceId)
		{
			if (Echoes[Index].State == EShadowSlaveEchoState::Destroyed)
			{
				return true;
			}

			TGuardValue<bool> TransitionGuard(bIsProcessingEchoTransition, true);

			// Clean up transient world actor if present
			if (Echoes[Index].TransientActor.IsValid())
			{
				AActor* ActorToDestroy = Echoes[Index].TransientActor.Get();
				Echoes[Index].TransientActor = nullptr;
				if (ActorToDestroy)
				{
					ActorToDestroy->OnDestroyed.RemoveDynamic(this, &UShadowSlaveEchoComponent::HandleSummonedActorDestroyed);
					ActorToDestroy->Destroy();
				}
			}

			// Auto-dismiss if currently summoned so no stale summon state remains
			if (Echoes[Index].bIsSummoned)
			{
				Echoes[Index].bIsSummoned = false;
				OnEchoDismissed.Broadcast(Echoes[Index]);
			}

			const EShadowSlaveEchoState OldState = Echoes[Index].State;
			Echoes[Index].State = EShadowSlaveEchoState::Destroyed;

			OnEchoDestroyed.Broadcast(Echoes[Index]);
			OnEchoStateChanged.Broadcast(Echoes[Index], OldState);
			return true;
		}
	}

	return false;
}

void UShadowSlaveEchoComponent::ClearEchoes()
{
	if (bIsProcessingEchoTransition || bIsTearingDown || Echoes.Num() == 0)
	{
		return;
	}

	TGuardValue<bool> TransitionGuard(bIsProcessingEchoTransition, true);

	for (FShadowSlaveEchoInstance& Echo : Echoes)
	{
		if (Echo.TransientActor.IsValid())
		{
			AActor* ActorToDestroy = Echo.TransientActor.Get();
			Echo.TransientActor = nullptr;
			if (ActorToDestroy)
			{
				ActorToDestroy->OnDestroyed.RemoveDynamic(this, &UShadowSlaveEchoComponent::HandleSummonedActorDestroyed);
				ActorToDestroy->Destroy();
			}
		}
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
	if (bIsProcessingEchoTransition || bIsTearingDown)
	{
		return;
	}

	TGuardValue<bool> TransitionGuard(bIsProcessingEchoTransition, true);

	// Clean up any existing transient world actors before restoring
	for (FShadowSlaveEchoInstance& Echo : Echoes)
	{
		if (Echo.TransientActor.IsValid())
		{
			AActor* ActorToDestroy = Echo.TransientActor.Get();
			Echo.TransientActor = nullptr;
			if (ActorToDestroy)
			{
				ActorToDestroy->OnDestroyed.RemoveDynamic(this, &UShadowSlaveEchoComponent::HandleSummonedActorDestroyed);
				ActorToDestroy->Destroy();
			}
		}
	}
	Echoes.Empty();

	for (const FShadowSlaveEchoInstance& Instance : InInstances)
	{
		if (Instance.IsValid())
		{
			FShadowSlaveEchoInstance RestoredInstance = Instance;
			// INVARIANT: Transient world summon state is NEVER restored from disk.
			RestoredInstance.bIsSummoned = false;
			RestoredInstance.TransientActor = nullptr;
			if (RestoredInstance.State == EShadowSlaveEchoState::Summoned)
			{
				RestoredInstance.State = EShadowSlaveEchoState::Dormant;
			}
			Echoes.Add(RestoredInstance);
		}
	}

	OnEchoCollectionChanged.Broadcast();
}

bool UShadowSlaveEchoComponent::SummonEcho(const FGuid& InstanceId, AActor* InTransientActor)
{
	if (bIsProcessingEchoTransition || bIsTearingDown || !InstanceId.IsValid())
	{
		return false;
	}

	for (int32 Index = 0; Index < Echoes.Num(); ++Index)
	{
		if (Echoes[Index].InstanceId == InstanceId)
		{
			// Destroyed Echoes can NEVER be summoned
			if (Echoes[Index].State == EShadowSlaveEchoState::Destroyed)
			{
				return false;
			}

			// Duplicate summon rejected: the same Echo instance cannot have two simultaneous summoned representations.
			if (Echoes[Index].bIsSummoned)
			{
				return false;
			}

			// If a transient actor representation is provided, verify it is not already used by another Echo
			if (InTransientActor)
			{
				for (const FShadowSlaveEchoInstance& OtherEcho : Echoes)
				{
					if (OtherEcho.InstanceId != InstanceId && OtherEcho.TransientActor.Get() == InTransientActor)
					{
						return false;
					}
				}
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

			if (InTransientActor)
			{
				InTransientActor->OnDestroyed.AddDynamic(this, &UShadowSlaveEchoComponent::HandleSummonedActorDestroyed);
				Echoes[Index].TransientActor = InTransientActor;
			}

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
	if (bIsProcessingEchoTransition || bIsTearingDown || !InstanceId.IsValid())
	{
		return false;
	}

	for (int32 Index = 0; Index < Echoes.Num(); ++Index)
	{
		if (Echoes[Index].InstanceId == InstanceId)
		{
			if (!Echoes[Index].bIsSummoned)
			{
				// Repeated dismissal is harmless
				return true;
			}

			TGuardValue<bool> TransitionGuard(bIsProcessingEchoTransition, true);

			// Destroy and unbind transient actor if present
			if (Echoes[Index].TransientActor.IsValid())
			{
				AActor* ActorToDestroy = Echoes[Index].TransientActor.Get();
				Echoes[Index].TransientActor = nullptr;
				if (ActorToDestroy)
				{
					ActorToDestroy->OnDestroyed.RemoveDynamic(this, &UShadowSlaveEchoComponent::HandleSummonedActorDestroyed);
					ActorToDestroy->Destroy();
				}
			}

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
	if (bIsProcessingEchoTransition || bIsTearingDown)
	{
		return false;
	}

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

bool UShadowSlaveEchoComponent::SetSummonedActor(const FGuid& InstanceId, AActor* InTransientActor)
{
	if (bIsProcessingEchoTransition || bIsTearingDown || !InstanceId.IsValid() || !InTransientActor)
	{
		return false;
	}

	// Verify actor is not already registered to another Echo
	for (const FShadowSlaveEchoInstance& OtherEcho : Echoes)
	{
		if (OtherEcho.InstanceId != InstanceId && OtherEcho.TransientActor.Get() == InTransientActor)
		{
			return false;
		}
	}

	for (int32 Index = 0; Index < Echoes.Num(); ++Index)
	{
		if (Echoes[Index].InstanceId == InstanceId)
		{
			if (!Echoes[Index].bIsSummoned)
			{
				return false; // Cannot bind actor to dormant Echo
			}

			TGuardValue<bool> TransitionGuard(bIsProcessingEchoTransition, true);

			// Unbind from prior actor if different
			if (Echoes[Index].TransientActor.IsValid() && Echoes[Index].TransientActor.Get() != InTransientActor)
			{
				Echoes[Index].TransientActor->OnDestroyed.RemoveDynamic(this, &UShadowSlaveEchoComponent::HandleSummonedActorDestroyed);
			}

			InTransientActor->OnDestroyed.AddDynamic(this, &UShadowSlaveEchoComponent::HandleSummonedActorDestroyed);
			Echoes[Index].TransientActor = InTransientActor;
			return true;
		}
	}

	return false;
}

AActor* UShadowSlaveEchoComponent::GetSummonedActor(const FGuid& InstanceId) const
{
	if (!InstanceId.IsValid())
	{
		return nullptr;
	}

	for (const FShadowSlaveEchoInstance& Echo : Echoes)
	{
		if (Echo.InstanceId == InstanceId)
		{
			return Echo.bIsSummoned ? Echo.TransientActor.Get() : nullptr;
		}
	}

	return nullptr;
}

void UShadowSlaveEchoComponent::HandleSummonedActorDestroyed(AActor* DestroyedActor)
{
	if (!DestroyedActor || bIsProcessingEchoTransition || bIsTearingDown)
	{
		return;
	}

	for (int32 Index = 0; Index < Echoes.Num(); ++Index)
		if (Echoes[Index].TransientActor.Get() == DestroyedActor)
		{
			TGuardValue<bool> TransitionGuard(bIsProcessingEchoTransition, true);

			Echoes[Index].TransientActor = nullptr;

			if (Echoes[Index].bIsSummoned)
			{
				const EShadowSlaveEchoState OldState = Echoes[Index].State;
				Echoes[Index].bIsSummoned = false;
				Echoes[Index].State = EShadowSlaveEchoState::Dormant;

				OnEchoDismissed.Broadcast(Echoes[Index]);
				OnEchoStateChanged.Broadcast(Echoes[Index], OldState);
			}
			return;
		}
}

bool UShadowSlaveEchoComponent::SetEchoState(const FGuid& InstanceId, EShadowSlaveEchoState NewState)
{
	if (bIsProcessingEchoTransition || bIsTearingDown || !InstanceId.IsValid())
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

			TGuardValue<bool> TransitionGuard(bIsProcessingEchoTransition, true);

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
	if (bIsProcessingEchoTransition || bIsTearingDown || !InstanceId.IsValid() || Key.IsNone())
	{
		return false;
	}

	for (int32 Index = 0; Index < Echoes.Num(); ++Index)
	{
		if (Echoes[Index].InstanceId == InstanceId)
		{
			TGuardValue<bool> TransitionGuard(bIsProcessingEchoTransition, true);

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
	if (bIsProcessingEchoTransition || bIsTearingDown || !InstanceId.IsValid() || Key.IsNone())
	{
		return false;
	}

	for (int32 Index = 0; Index < Echoes.Num(); ++Index)
	{
		if (Echoes[Index].InstanceId == InstanceId)
		{
			TGuardValue<bool> TransitionGuard(bIsProcessingEchoTransition, true);

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

void UShadowSlaveEchoComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	TGuardValue<bool> TeardownGuard(bIsTearingDown, true);
	TGuardValue<bool> TransitionGuard(bIsProcessingEchoTransition, true);

	for (FShadowSlaveEchoInstance& Echo : Echoes)
	{
		if (Echo.TransientActor.IsValid())
		{
			AActor* ActorToDestroy = Echo.TransientActor.Get();
			Echo.TransientActor = nullptr;
			if (ActorToDestroy)
			{
				ActorToDestroy->OnDestroyed.RemoveDynamic(this, &UShadowSlaveEchoComponent::HandleSummonedActorDestroyed);
				ActorToDestroy->Destroy();
			}
		}
		Echo.bIsSummoned = false;
		if (Echo.State == EShadowSlaveEchoState::Summoned)
		{
			Echo.State = EShadowSlaveEchoState::Dormant;
		}
	}

	Super::EndPlay(EndPlayReason);
}
