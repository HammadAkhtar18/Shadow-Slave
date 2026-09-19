// Copyright Epic Games, Inc. All Rights Reserved.

#include "Core/ShadowSlaveGameplayTagTypes.h"

namespace ShadowSlaveGameplayTags
{
	/* --- State Category --- */
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State, "State", "Root technical state category");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Active, "State.Active", "Active technical state");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Disabled, "State.Disabled", "Disabled technical state");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Pending, "State.Pending", "Pending technical state");

	/* --- Event Category --- */
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event, "Event", "Root technical event category");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Trigger, "Event.Trigger", "Generic trigger event");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Complete, "Event.Complete", "Generic complete event");

	/* --- Ability Category --- */
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability, "Ability", "Root ability category");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Action, "Ability.Action", "Generic ability action tag");

	/* --- Combat Category --- */
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Combat, "Combat", "Root combat category");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Combat_Engaged, "Combat.Engaged", "Generic combat engagement state");

	/* --- Interaction Category --- */
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Interaction, "Interaction", "Root interaction category");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Interaction_Interactable, "Interaction.Interactable", "Generic interactable state tag");
}

bool UShadowSlaveGameplayTagLibrary::HasTag(const FGameplayTagContainer& Container, const FGameplayTag& Tag, bool bExactMatch)
{
	if (!Tag.IsValid())
	{
		return false;
	}

	return bExactMatch ? Container.HasTagExact(Tag) : Container.HasTag(Tag);
}

bool UShadowSlaveGameplayTagLibrary::HasAny(const FGameplayTagContainer& Container, const FGameplayTagContainer& OtherContainer, bool bExactMatch)
{
	return bExactMatch ? Container.HasAnyExact(OtherContainer) : Container.HasAny(OtherContainer);
}

bool UShadowSlaveGameplayTagLibrary::HasAll(const FGameplayTagContainer& Container, const FGameplayTagContainer& OtherContainer, bool bExactMatch)
{
	return bExactMatch ? Container.HasAllExact(OtherContainer) : Container.HasAll(OtherContainer);
}

bool UShadowSlaveGameplayTagLibrary::AddTag(FGameplayTagContainer& Container, const FGameplayTag& Tag)
{
	if (!Tag.IsValid() || Container.HasTagExact(Tag))
	{
		return false;
	}

	Container.AddTag(Tag);
	return true;
}

bool UShadowSlaveGameplayTagLibrary::RemoveTag(FGameplayTagContainer& Container, const FGameplayTag& Tag)
{
	if (!Tag.IsValid() || !Container.HasTagExact(Tag))
	{
		return false;
	}

	return Container.RemoveTag(Tag);
}

void UShadowSlaveGameplayTagLibrary::AppendTags(FGameplayTagContainer& Destination, const FGameplayTagContainer& Source)
{
	Destination.AppendTags(Source);
}

bool UShadowSlaveGameplayTagLibrary::MatchesQuery(const FGameplayTagContainer& Container, const FGameplayTagQuery& Query)
{
	return Query.Matches(Container);
}
