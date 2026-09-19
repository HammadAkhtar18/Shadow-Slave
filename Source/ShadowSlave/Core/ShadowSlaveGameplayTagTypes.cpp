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
