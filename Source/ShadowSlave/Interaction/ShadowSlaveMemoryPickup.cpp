// Copyright Epic Games, Inc. All Rights Reserved.

#include "Interaction/ShadowSlaveMemoryPickup.h"
#include "Memories/ShadowSlaveMemoryComponent.h"

AShadowSlaveMemoryPickup::AShadowSlaveMemoryPickup()
{
	MemoryDefinition = nullptr;
	InteractionPrompt = FText::FromString(TEXT("Acquire Memory"));
	InteractionPriority = 15; // Memory pickups are high-priority collectibles
}

void AShadowSlaveMemoryPickup::InitializeMemoryPickup(UShadowSlaveMemoryDefinition* InMemoryDef)
{
	MemoryDefinition = InMemoryDef;

	if (MemoryDefinition)
	{
		InteractionPrompt = FText::Format(
			NSLOCTEXT("ShadowSlave", "AcquireMemoryFormat", "Acquire Memory: {0}"),
			MemoryDefinition->DisplayName
		);
	}
}

bool AShadowSlaveMemoryPickup::CanInteract_Implementation(AActor* Interactor)
{
	return Super::CanInteract_Implementation(Interactor) && MemoryDefinition != nullptr;
}

FText AShadowSlaveMemoryPickup::GetInteractionPrompt_Implementation(AActor* Interactor)
{
	if (MemoryDefinition)
	{
		return FText::Format(
			NSLOCTEXT("ShadowSlave", "AcquireMemoryFormat", "Acquire Memory: {0}"),
			MemoryDefinition->DisplayName
		);
	}

	return InteractionPrompt;
}

FShadowSlaveInteractionResult AShadowSlaveMemoryPickup::ExecuteInteraction(AActor* Interactor)
{
	if (!Interactor)
	{
		return FShadowSlaveInteractionResult::Failure(FText::FromString(TEXT("Invalid interactor.")), InteractionId);
	}

	if (bIsProcessingAcquisition || PickupState != EShadowSlavePickupState::Available || bIsCollected)
	{
		return FShadowSlaveInteractionResult::Failure(FText::FromString(TEXT("Pickup is not available.")), InteractionId);
	}

	if (!MemoryDefinition)
	{
		return FShadowSlaveInteractionResult::Failure(FText::FromString(TEXT("Memory definition is missing.")), InteractionId);
	}

	UShadowSlaveMemoryComponent* MemoryComp = Interactor->FindComponentByClass<UShadowSlaveMemoryComponent>();
	if (!MemoryComp)
	{
		return FShadowSlaveInteractionResult::Failure(FText::FromString(TEXT("Interactor has no Memory Component.")), InteractionId);
	}

	bIsProcessingAcquisition = true;

	// Authoritatively acquire memory into component storage
	FShadowSlaveMemoryInstance OutInstance;
	const bool bAcquired = MemoryComp->AcquireMemory(MemoryDefinition, OutInstance);

	if (!bAcquired || !OutInstance.IsValid())
	{
		bIsProcessingAcquisition = false;
		return FShadowSlaveInteractionResult::Failure(FText::FromString(TEXT("Failed to acquire Memory into soul storage.")), InteractionId);
	}

	// Successful acquisition: consume and destroy world pickup
	OnCollected(Interactor);
	bIsProcessingAcquisition = false;
	return FShadowSlaveInteractionResult::Success(FName(TEXT("MemoryAcquired")));
}

bool AShadowSlaveMemoryPickup::CaptureSaveRecord_Implementation(FShadowSlaveWorldActorSaveRecord& OutRecord)
{
	if (!Super::CaptureSaveRecord_Implementation(OutRecord))
	{
		return false;
	}

	if (MemoryDefinition)
	{
		OutRecord.CustomStateData.Add(TEXT("MemoryDefinitionPath"), MemoryDefinition->GetPathName());
	}
	return true;
}

bool AShadowSlaveMemoryPickup::RestoreSaveRecord_Implementation(const FShadowSlaveWorldActorSaveRecord& InRecord)
{
	if (!Super::RestoreSaveRecord_Implementation(InRecord))
	{
		return false;
	}

	if (const FString* DefPath = InRecord.CustomStateData.Find(TEXT("MemoryDefinitionPath")))
	{
		if (!DefPath->IsEmpty())
		{
			if (UShadowSlaveMemoryDefinition* LoadedDef = Cast<UShadowSlaveMemoryDefinition>(StaticLoadObject(UShadowSlaveMemoryDefinition::StaticClass(), nullptr, **DefPath)))
			{
				MemoryDefinition = LoadedDef;
			}
		}
	}

	return true;
}
