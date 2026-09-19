// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "NativeGameplayTags.h"

/**
 * Centralized native Gameplay Tag declarations for the Shadow Slave prototype.
 *
 * SCOPE & ARCHITECTURE:
 * These tags represent generic technical categories (State, Event, Ability, Combat, Interaction).
 * They contain ZERO canon elements (no named characters, abilities, or ranks).
 * Systems throughout the project can opt into these tags without coupling directly to each other.
 *
 * All container operations should use Unreal's native FGameplayTagContainer, FGameplayTag,
 * and FGameplayTagQuery APIs directly without project-specific wrapper abstractions.
 */
namespace ShadowSlaveGameplayTags
{
	/* --- State Category --- */
	SHADOWSLAVE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State);
	SHADOWSLAVE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Active);
	SHADOWSLAVE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Disabled);
	SHADOWSLAVE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Pending);

	/* --- Event Category --- */
	SHADOWSLAVE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event);
	SHADOWSLAVE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Trigger);
	SHADOWSLAVE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Complete);

	/* --- Ability Category --- */
	SHADOWSLAVE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability);
	SHADOWSLAVE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Action);

	/* --- Combat Category --- */
	SHADOWSLAVE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Combat);
	SHADOWSLAVE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Combat_Engaged);

	/* --- Interaction Category --- */
	SHADOWSLAVE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Interaction);
	SHADOWSLAVE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Interaction_Interactable);
}
