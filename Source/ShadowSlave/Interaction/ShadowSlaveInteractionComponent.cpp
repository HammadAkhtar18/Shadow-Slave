// Copyright Epic Games, Inc. All Rights Reserved.

#include "Interaction/ShadowSlaveInteractionComponent.h"
#include "Interaction/ShadowSlaveInteractableInterface.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"

UShadowSlaveInteractionComponent::UShadowSlaveInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;

	InteractionDistance = 250.0f;
	InteractionRadius = 25.0f;
	TraceCollisionChannel = ECC_Visibility;
	bRequireLineOfSight = true;
	TraceInterval = 0.1f;
	bEnableDetection = true;
	TraceTimeAccumulator = 0.0f;
}

void UShadowSlaveInteractionComponent::BeginPlay()
{
	Super::BeginPlay();
	TraceTimeAccumulator = 0.0f;
}

void UShadowSlaveInteractionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bEnableDetection)
	{
		return;
	}

	if (TraceInterval <= 0.0f)
	{
		DetectInteractable();
		return;
	}

	TraceTimeAccumulator += DeltaTime;
	if (TraceTimeAccumulator >= TraceInterval)
	{
		TraceTimeAccumulator = 0.0f;
		DetectInteractable();
	}
}

void UShadowSlaveInteractionComponent::SetInteractionDetectionEnabled(bool bEnabled)
{
	bEnableDetection = bEnabled;
	if (!bEnabled)
	{
		ClearCurrentInteractable();
	}
}

AActor* UShadowSlaveInteractionComponent::GetCurrentInteractable() const
{
	return CurrentInteractableActor.IsValid() ? CurrentInteractableActor.Get() : nullptr;
}

bool UShadowSlaveInteractionComponent::HasInteractableTarget() const
{
	return CurrentInteractableActor.IsValid();
}

FText UShadowSlaveInteractionComponent::GetCurrentInteractionPrompt() const
{
	if (CurrentInteractableActor.IsValid() && CurrentInteractableActor->GetClass()->ImplementsInterface(UShadowSlaveInteractableInterface::StaticClass()))
	{
		return IShadowSlaveInteractableInterface::Execute_GetInteractionPrompt(CurrentInteractableActor.Get(), GetOwner());
	}

	return FText::GetEmpty();
}

void UShadowSlaveInteractionComponent::ClearCurrentInteractable()
{
	if (CurrentInteractableActor.IsValid())
	{
		AActor* OldTarget = CurrentInteractableActor.Get();
		CurrentInteractableActor = nullptr;
		OnInteractionTargetChanged.Broadcast(nullptr, OldTarget);
	}
	else
	{
		CurrentInteractableActor = nullptr;
	}
}

void UShadowSlaveInteractionComponent::DetectInteractable()
{
	UWorld* World = GetWorld();
	AActor* OwnerActor = GetOwner();
	if (!World || !OwnerActor)
	{
		ClearCurrentInteractable();
		return;
	}

	// Determine view source: prefer player camera if possessed by player controller, else actor forward
	FVector TraceStart = OwnerActor->GetActorLocation();
	FVector TraceDirection = OwnerActor->GetActorForwardVector();

	if (APawn* PawnOwner = Cast<APawn>(OwnerActor))
	{
		if (APlayerController* PC = Cast<APlayerController>(PawnOwner->GetController()))
		{
			if (PC->PlayerCameraManager)
			{
				TraceStart = PC->PlayerCameraManager->GetCameraLocation();
				TraceDirection = PC->PlayerCameraManager->GetCameraRotation().Vector();
			}
		}
	}

	const FVector TraceEnd = TraceStart + (TraceDirection * InteractionDistance);

	FCollisionQueryParams QueryParams(TEXT("ShadowSlaveInteractionTrace"), false, OwnerActor);
	QueryParams.AddIgnoredActor(OwnerActor);

	TArray<FHitResult> HitResults;
	if (InteractionRadius > 0.0f)
	{
		const FCollisionShape SphereShape = FCollisionShape::MakeSphere(InteractionRadius);
		World->SweepMultiByChannel(HitResults, TraceStart, TraceEnd, FQuat::Identity, TraceCollisionChannel, SphereShape, QueryParams);
	}
	else
	{
		World->LineTraceMultiByChannel(HitResults, TraceStart, TraceEnd, TraceCollisionChannel, QueryParams);
	}

	AActor* BestCandidate = nullptr;
	int32 BestPriority = MIN_int32;
	float BestDistanceSq = MAX_flt;

	TSet<AActor*> ProcessedActors;

	for (const FHitResult& Hit : HitResults)
	{
		AActor* HitActor = Hit.GetActor();
		if (!HitActor || HitActor == OwnerActor || ProcessedActors.Contains(HitActor))
		{
			continue;
		}

		ProcessedActors.Add(HitActor);

		if (HitActor->GetClass()->ImplementsInterface(UShadowSlaveInteractableInterface::StaticClass()))
		{
			if (IShadowSlaveInteractableInterface::Execute_CanInteract(HitActor, OwnerActor))
			{
				const int32 CandidatePriority = IShadowSlaveInteractableInterface::Execute_GetInteractionPriority(HitActor, OwnerActor);
				const float CandidateDistSq = FVector::DistSquared(OwnerActor->GetActorLocation(), HitActor->GetActorLocation());

				// Choose higher priority candidate, or closer candidate on tie
				if (CandidatePriority > BestPriority || (CandidatePriority == BestPriority && CandidateDistSq < BestDistanceSq))
				{
					BestCandidate = HitActor;
					BestPriority = CandidatePriority;
					BestDistanceSq = CandidateDistSq;
				}
			}
		}
	}

	// Only broadcast when target genuinely transitions (avoids per-frame redundant firing)
	AActor* CurrentTarget = CurrentInteractableActor.Get();
	if (BestCandidate != CurrentTarget)
	{
		CurrentInteractableActor = BestCandidate;
		OnInteractionTargetChanged.Broadcast(BestCandidate, CurrentTarget);
	}
}

bool UShadowSlaveInteractionComponent::TryInteract()
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !CurrentInteractableActor.IsValid())
	{
		return false;
	}

	AActor* TargetActor = CurrentInteractableActor.Get();
	if (!TargetActor->GetClass()->ImplementsInterface(UShadowSlaveInteractableInterface::StaticClass()))
	{
		ClearCurrentInteractable();
		return false;
	}

	// Validate availability immediately prior to execution
	if (!IShadowSlaveInteractableInterface::Execute_CanInteract(TargetActor, OwnerActor))
	{
		DetectInteractable();
		return false;
	}

	// Execute polymorphic interaction
	const FShadowSlaveInteractionResult Result = IShadowSlaveInteractableInterface::Execute_Interact(TargetActor, OwnerActor);

	OnInteracted.Broadcast(OwnerActor, TargetActor, Result);

	// Re-evaluate target in case the interaction consumed/destroyed the object or altered validity
	DetectInteractable();

	return Result.bSuccess;
}
