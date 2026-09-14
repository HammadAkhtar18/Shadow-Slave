// Copyright Epic Games, Inc. All Rights Reserved.

#include "Aspects/ShadowSlaveAspectComponent.h"
#include "Attributes/ShadowSlaveAttributeComponent.h"
#include "GameFramework/Actor.h"

UShadowSlaveAspectComponent::UShadowSlaveAspectComponent()
{
	// Operates purely event-driven; no tick overhead
	PrimaryComponentTick.bCanEverTick = false;
	AspectDefinition = nullptr;
	ActiveFlawDefinition = nullptr;
}

bool UShadowSlaveAspectComponent::SetAspectDefinition(UShadowSlaveAspectDefinition* NewAspectDef)
{
	if (AspectDefinition == NewAspectDef)
	{
		return true;
	}

	UShadowSlaveAspectDefinition* OldAspectDef = AspectDefinition;
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
	if (FindAbilityInstance(AbilityId, FoundInstance))
	{
		return FoundInstance.bIsUnlocked && FoundInstance.IsValid();
	}
	return false;
}

bool UShadowSlaveAspectComponent::ActivateAbility(FName AbilityId)
{
	// Extensibility boundary: actual aspect ability gameplay execution will be integrated in future steps.
	if (!CanActivateAbility(AbilityId))
	{
		return false;
	}

	// Safe prototype stub: return false until concrete ability execution systems are attached
	return false;
}

bool UShadowSlaveAspectComponent::DeactivateAbility(FName AbilityId)
{
	// Extensibility boundary: safe prototype stub
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
