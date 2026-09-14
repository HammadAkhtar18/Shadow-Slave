// Copyright Epic Games, Inc. All Rights Reserved.

#include "Interaction/ShadowSlaveInteractableActor.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"

AShadowSlaveInteractableActor::AShadowSlaveInteractableActor()
{
	// Operates purely event-driven; no tick overhead
	PrimaryActorTick.bCanEverTick = false;

	SceneRootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRootComponent"));
	RootComponent = SceneRootComponent;

	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMeshComponent"));
	StaticMeshComponent->SetupAttachment(RootComponent);
	StaticMeshComponent->SetCollisionProfileName(TEXT("BlockAllDynamic"));

	bIsInteractionEnabled = true;
	InteractionPrompt = FText::FromString(TEXT("Interact"));
	InteractionPriority = 0;
	InteractionId = NAME_None;
}

bool AShadowSlaveInteractableActor::CanInteract_Implementation(AActor* Interactor)
{
	return bIsInteractionEnabled && Interactor != nullptr && !IsPendingKillPending();
}

FText AShadowSlaveInteractableActor::GetInteractionPrompt_Implementation(AActor* Interactor)
{
	return InteractionPrompt;
}

FShadowSlaveInteractionResult AShadowSlaveInteractableActor::Interact_Implementation(AActor* Interactor)
{
	if (!CanInteract_Implementation(Interactor))
	{
		return FShadowSlaveInteractionResult::Failure(FText::FromString(TEXT("Interaction is not available.")), InteractionId);
	}

	const FShadowSlaveInteractionResult Result = ExecuteInteraction(Interactor);
	OnInteracted.Broadcast(Interactor, this, Result);
	return Result;
}

int32 AShadowSlaveInteractableActor::GetInteractionPriority_Implementation(AActor* Interactor)
{
	return InteractionPriority;
}

void AShadowSlaveInteractableActor::SetInteractionEnabled(bool bEnabled)
{
	bIsInteractionEnabled = bEnabled;
}

void AShadowSlaveInteractableActor::SetInteractionPrompt(const FText& NewPrompt)
{
	InteractionPrompt = NewPrompt;
}

FShadowSlaveInteractionResult AShadowSlaveInteractableActor::ExecuteInteraction(AActor* Interactor)
{
	// Base implementation succeeds generically; specialized subclasses override with custom logic
	return FShadowSlaveInteractionResult::Success(InteractionId);
}

FName AShadowSlaveInteractableActor::GetPersistentSaveId_Implementation() const
{
	return PersistentSaveId;
}

bool AShadowSlaveInteractableActor::CaptureSaveRecord_Implementation(FShadowSlaveWorldActorSaveRecord& OutRecord)
{
	if (PersistentSaveId.IsNone())
	{
		return false;
	}

	OutRecord.PersistentId = PersistentSaveId;
	OutRecord.ActorClass = GetClass();
	OutRecord.ActorTransform = GetActorTransform();
	OutRecord.bIsActive = !IsHidden();
	OutRecord.CustomStateData.Add(TEXT("bIsInteractionEnabled"), bIsInteractionEnabled ? TEXT("1") : TEXT("0"));
	return true;
}

bool AShadowSlaveInteractableActor::RestoreSaveRecord_Implementation(const FShadowSlaveWorldActorSaveRecord& InRecord)
{
	if (PersistentSaveId.IsNone() || InRecord.PersistentId != PersistentSaveId)
	{
		return false;
	}

	if (const FString* Val = InRecord.CustomStateData.Find(TEXT("bIsInteractionEnabled")))
	{
		SetInteractionEnabled(*Val == TEXT("1"));
	}

	return true;
}
