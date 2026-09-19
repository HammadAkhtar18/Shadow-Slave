// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "NativeGameplayTags.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ShadowSlaveGameplayTagTypes.generated.h"

/**
 * Centralized native Gameplay Tag declarations for the Shadow Slave prototype.
 *
 * SCOPE & ARCHITECTURE:
 * These tags represent generic technical categories (State, Event, Ability, Combat, Interaction).
 * They contain ZERO canon elements (no named characters, abilities, or ranks).
 * Systems throughout the project can opt into these tags without coupling directly to each other.
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

/**
 * Reusable utility library providing consistent, project-standard Gameplay Tag container queries
 * and mutation helpers for C++ and Blueprints.
 *
 * NOTE: The underlying authoritative state remains Unreal's native FGameplayTagContainer.
 */
UCLASS()
class SHADOWSLAVE_API UShadowSlaveGameplayTagLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Checks if Container has Tag. If bExactMatch is true, requires exact tag equality; otherwise allows hierarchical match. */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|GameplayTags")
	static bool HasTag(const FGameplayTagContainer& Container, const FGameplayTag& Tag, bool bExactMatch = false);

	/** Checks if Container contains any tags from OtherContainer. */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|GameplayTags")
	static bool HasAny(const FGameplayTagContainer& Container, const FGameplayTagContainer& OtherContainer, bool bExactMatch = false);

	/** Checks if Container contains all tags from OtherContainer. */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|GameplayTags")
	static bool HasAll(const FGameplayTagContainer& Container, const FGameplayTagContainer& OtherContainer, bool bExactMatch = false);

	/**
	 * Adds Tag to Container.
	 * Returns true if the tag was valid and added; returns false if invalid or already present.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|GameplayTags")
	static bool AddTag(UPARAM(ref) FGameplayTagContainer& Container, const FGameplayTag& Tag);

	/**
	 * Removes Tag from Container.
	 * Returns true if the tag was found and removed; returns false if not present or invalid.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|GameplayTags")
	static bool RemoveTag(UPARAM(ref) FGameplayTagContainer& Container, const FGameplayTag& Tag);

	/** Appends all tags from Source into Destination. */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|GameplayTags")
	static void AppendTags(UPARAM(ref) FGameplayTagContainer& Destination, const FGameplayTagContainer& Source);

	/** Evaluates a gameplay tag query against Container. */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|GameplayTags")
	static bool MatchesQuery(const FGameplayTagContainer& Container, const FGameplayTagQuery& Query);
};
