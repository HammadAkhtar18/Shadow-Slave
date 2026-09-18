// Copyright Epic Games, Inc. All Rights Reserved.

#include "Aspects/ShadowSlaveAspectComponent.h"
#include "Attributes/ShadowSlaveAttributeComponent.h"
#include "Progression/ShadowSlaveProgressionComponent.h"
#include "Core/ShadowSlaveLogChannels.h"
#include "GameFramework/Actor.h"

UShadowSlaveAspectComponent::UShadowSlaveAspectComponent()
{
	// Operates purely event-driven; no tick overhead
	PrimaryComponentTick.bCanEverTick = false;
	AspectDefinition = nullptr;
	ActiveFlawDefinition = nullptr;
	bIsProcessingAbilityTransition = false;
}

bool UShadowSlaveAspectComponent::SetAspectDefinition(UShadowSlaveAspectDefinition* NewAspectDef)
{
	if (AspectDefinition == NewAspectDef)
	{
		return true;
	}

	if (bIsProcessingAbilityTransition)
	{
		return false;
	}

	TGuardValue<bool> TransitionGuard(bIsProcessingAbilityTransition, true);
	UShadowSlaveAspectDefinition* OldAspectDef = AspectDefinition;
	for (FShadowSlaveAspectAbilityInstance& Instance : AbilityInstances)
	{
		if (Instance.bIsActive)
		{
			Instance.bIsActive = false;
			OnAbilityDeactivated.Broadcast(Instance.GetAbilityId(), Instance.AbilityDefinition);
		}
	}

	AspectDefinition = NewAspectDef;

	// Populate runtime ability instances matching the definition
	AbilityInstances.Empty();
	if (NewAspectDef)
	{
		for (const TObjectPtr<UShadowSlaveAspectAbilityDefinition>& AbilityDef : NewAspectDef->AbilityDefinitions)
		{
			if (AbilityDef)
			{
				AbilityInstances.Add(FShadowSlaveAspectAbilityInstance(AbilityDef.Get(), false));
			}
		}
	}

	// Update active flaw if the aspect specifies one
	UShadowSlaveFlawDefinition* OldFlaw = ActiveFlawDefinition;
	ActiveFlawDefinition = NewAspectDef ? NewAspectDef->FlawDefinition.Get() : nullptr;

	OnAspectChanged.Broadcast(NewAspectDef, OldAspectDef);

	if (ActiveFlawDefinition != OldFlaw)
	{
		OnFlawChanged.Broadcast(ActiveFlawDefinition, OldFlaw);
	}

	return true;
}

EShadowSlaveAspectRank UShadowSlaveAspectComponent::GetAspectRank() const
{
	return AspectDefinition ? AspectDefinition->AspectRank : EShadowSlaveAspectRank::Unknown;
}

bool UShadowSlaveAspectComponent::SetFlawDefinition(UShadowSlaveFlawDefinition* NewFlawDef)
{
	if (ActiveFlawDefinition == NewFlawDef)
	{
		return true;
	}

	UShadowSlaveFlawDefinition* OldFlaw = ActiveFlawDefinition;
	ActiveFlawDefinition = NewFlawDef;

	OnFlawChanged.Broadcast(NewFlawDef, OldFlaw);
	return true;
}

TArray<UShadowSlaveAspectAbilityDefinition*> UShadowSlaveAspectComponent::GetAbilityDefinitions() const
{
	TArray<UShadowSlaveAspectAbilityDefinition*> Definitions;
	if (!AspectDefinition)
	{
		return Definitions;
	}

	for (const TObjectPtr<UShadowSlaveAspectAbilityDefinition>& AbilityDef : AspectDefinition->AbilityDefinitions)
	{
		if (AbilityDef)
		{
			Definitions.Add(AbilityDef.Get());
		}
	}
	return Definitions;
}

bool UShadowSlaveAspectComponent::IsAbilityUnlocked(FName AbilityId) const
{
	if (AbilityId.IsNone())
	{
		return false;
	}

	for (const FShadowSlaveAspectAbilityInstance& Instance : AbilityInstances)
	{
		if (Instance.GetAbilityId() == AbilityId)
		{
			return Instance.bIsUnlocked;
		}
	}

	return false;
}

bool UShadowSlaveAspectComponent::IsAbilityActive(FName AbilityId) const
{
	if (AbilityId.IsNone())
	{
		return false;
	}

	for (const FShadowSlaveAspectAbilityInstance& Instance : AbilityInstances)
	{
		if (Instance.GetAbilityId() == AbilityId)
		{
			return Instance.bIsActive;
		}
	}

	return false;
}

bool UShadowSlaveAspectComponent::UnlockAbility(FName AbilityId)
{
	if (AbilityId.IsNone())
	{
		return false;
	}

	for (FShadowSlaveAspectAbilityInstance& Instance : AbilityInstances)
	{
		if (Instance.GetAbilityId() == AbilityId)
		{
			if (Instance.bIsUnlocked)
			{
				return true; // Already unlocked; avoid redundant event broadcast
			}

			Instance.bIsUnlocked = true;
			OnAbilityUnlocked.Broadcast(AbilityId, Instance.AbilityDefinition);
			return true;
		}
	}

	return false;
}

bool UShadowSlaveAspectComponent::FindAbilityInstance(FName AbilityId, FShadowSlaveAspectAbilityInstance& OutInstance) const
{
	if (AbilityId.IsNone())
	{
		OutInstance = FShadowSlaveAspectAbilityInstance();
		return false;
	}

	for (const FShadowSlaveAspectAbilityInstance& Instance : AbilityInstances)
	{
		if (Instance.GetAbilityId() == AbilityId)
		{
			OutInstance = Instance;
			return true;
		}
	}

	OutInstance = FShadowSlaveAspectAbilityInstance();
	return false;
}

bool UShadowSlaveAspectComponent::CanActivateAbility(FName AbilityId) const
{
	FShadowSlaveAspectAbilityInstance FoundInstance;
	if (!FindAbilityInstance(AbilityId, FoundInstance) || !FoundInstance.bIsUnlocked || !FoundInstance.IsValid())
	{
		return false;
	}

	const UShadowSlaveAspectAbilityDefinition* AbilityDef = FoundInstance.AbilityDefinition;
	if (!FMath::IsFinite(AbilityDef->BaseEssenceCost) || AbilityDef->BaseEssenceCost < 0.0f)
	{
		return false;
	}

	if (AbilityDef->HasRankRequirement())
	{
		const AActor* OwnerActor = GetOwner();
		const UShadowSlaveProgressionComponent* Progression = OwnerActor ? OwnerActor->FindComponentByClass<UShadowSlaveProgressionComponent>() : nullptr;
		if (!Progression || !Progression->HasKnownRank() || static_cast<uint8>(Progression->GetCharacterRank()) < static_cast<uint8>(AbilityDef->RequiredCharacterRank))
		{
			return false;
		}
	}

	if (AbilityDef->BaseEssenceCost > 0.0f)
	{
		const UShadowSlaveAttributeComponent* Attributes = GetAttributeComponent();
		if (!Attributes || Attributes->GetCurrentEssence() < AbilityDef->BaseEssenceCost)
		{
			return false;
		}
	}

	return true;
}

bool UShadowSlaveAspectComponent::ActivateAbility(FName AbilityId)
{
	if (bIsProcessingAbilityTransition || AbilityId.IsNone())
	{
		return false;
	}

	for (FShadowSlaveAspectAbilityInstance& Instance : AbilityInstances)
	{
		if (Instance.GetAbilityId() != AbilityId)
		{
			continue;
		}

		if (Instance.bIsActive)
		{
			return true;
		}

		if (!CanActivateAbility(AbilityId))
		{
			UE_LOG(LogShadowSlave, Verbose, TEXT("UShadowSlaveAspectComponent::ActivateAbility rejected '%s' because its runtime prerequisites are not satisfied."), *AbilityId.ToString());
			return false;
		}

		TGuardValue<bool> TransitionGuard(bIsProcessingAbilityTransition, true);
		const float EssenceCost = Instance.AbilityDefinition->BaseEssenceCost;
		if (EssenceCost > 0.0f)
		{
			UShadowSlaveAttributeComponent* Attributes = GetAttributeComponent();
			if (!Attributes || !Attributes->ConsumeEssence(EssenceCost))
			{
				return false;
			}
		}

		Instance.bIsActive = true;
		OnAbilityActivated.Broadcast(AbilityId, Instance.AbilityDefinition);
		return true;
	}

	return false;
}

bool UShadowSlaveAspectComponent::DeactivateAbility(FName AbilityId)
{
	if (bIsProcessingAbilityTransition || AbilityId.IsNone())
	{
		return false;
	}

	for (FShadowSlaveAspectAbilityInstance& Instance : AbilityInstances)
	{
		if (Instance.GetAbilityId() != AbilityId)
		{
			continue;
		}

		if (!Instance.bIsActive)
		{
			return true;
		}

		TGuardValue<bool> TransitionGuard(bIsProcessingAbilityTransition, true);
		Instance.bIsActive = false;
		OnAbilityDeactivated.Broadcast(AbilityId, Instance.AbilityDefinition);
		return true;
	}

	return false;
}

bool UShadowSlaveAspectComponent::SetAbilityDynamicProperty(FName AbilityId, FName Key, const FString& Value)
{
	if (AbilityId.IsNone() || Key.IsNone())
	{
		return false;
	}

	for (FShadowSlaveAspectAbilityInstance& Instance : AbilityInstances)
	{
		if (Instance.GetAbilityId() == AbilityId)
		{
			Instance.DynamicProperties.Add(Key, Value);
			return true;
		}
	}

	return false;
}

bool UShadowSlaveAspectComponent::GetAbilityDynamicProperty(FName AbilityId, FName Key, FString& OutValue) const
{
	if (AbilityId.IsNone() || Key.IsNone())
	{
		return false;
	}

	for (const FShadowSlaveAspectAbilityInstance& Instance : AbilityInstances)
	{
		if (Instance.GetAbilityId() == AbilityId)
		{
			if (const FString* Found = Instance.DynamicProperties.Find(Key))
			{
				OutValue = *Found;
				return true;
			}
			return false;
		}
	}

	return false;
}

UShadowSlaveAttributeComponent* UShadowSlaveAspectComponent::GetAttributeComponent() const
{
	AActor* OwnerActor = GetOwner();
	return OwnerActor ? OwnerActor->FindComponentByClass<UShadowSlaveAttributeComponent>() : nullptr;
}
