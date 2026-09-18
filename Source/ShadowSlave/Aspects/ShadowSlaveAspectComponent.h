// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Aspects/ShadowSlaveAspectTypes.h"
#include "Aspects/ShadowSlaveAspectDefinition.h"
#include "Aspects/ShadowSlaveAspectAbilityDefinition.h"
#include "Aspects/ShadowSlaveFlawDefinition.h"
#include "ShadowSlaveAspectComponent.generated.h"

class UShadowSlaveAttributeComponent;

/**
 * Reusable Actor Component managing an entity's Aspect identity, runtime abilities, and Flaw.
 * Designed event-driven without tick overhead.
 * Usable by Player characters, generic enemies, NPCs, and future bosses.
 *
 * NOTE ON ATTRIBUTES & ESSENCE:
 * This component does NOT manage or duplicate Health, Stamina, or Essence pools.
 * All resource storage and modification remains strictly within UShadowSlaveAttributeComponent.
 */
UCLASS(ClassGroup = (ShadowSlave), meta = (BlueprintSpawnableComponent))
class SHADOWSLAVE_API UShadowSlaveAspectComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UShadowSlaveAspectComponent();

	/* --- Aspect Configuration --- */

	/**
	 * Sets or changes the active Aspect Definition.
	 * Initializes ability runtime instances and binds the Aspect's Flaw.
	 * Returns true if successfully updated.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Aspects|Operations")
	bool SetAspectDefinition(UShadowSlaveAspectDefinition* NewAspectDef);

	/** Returns the currently bound static Aspect Definition asset */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Aspects|Queries")
	UShadowSlaveAspectDefinition* GetAspectDefinition() const { return AspectDefinition; }

	/** Returns whether this component currently has an Aspect assigned */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Aspects|Queries")
	bool HasAspect() const { return AspectDefinition != nullptr; }

	/**
	 * Returns the Aspect Rank from the bound definition.
	 * Returns EShadowSlaveAspectRank::Unknown if no Aspect is assigned.
	 */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Aspects|Queries")
	EShadowSlaveAspectRank GetAspectRank() const;

	/* --- Flaw API --- */

	/** Returns the active Flaw Definition bound to this character */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Aspects|Flaw")
	UShadowSlaveFlawDefinition* GetFlawDefinition() const { return ActiveFlawDefinition; }

	/** Sets or overrides the active Flaw Definition directly */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Aspects|Flaw")
	bool SetFlawDefinition(UShadowSlaveFlawDefinition* NewFlawDef);

	/** Returns whether this character currently has an active Flaw */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Aspects|Flaw")
	bool HasFlaw() const { return ActiveFlawDefinition != nullptr; }

	/* --- Ability Queries & Management --- */

	/** Returns all static Ability Definitions available from the bound Aspect */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Aspects|Abilities")
	TArray<UShadowSlaveAspectAbilityDefinition*> GetAbilityDefinitions() const;

	/** Returns all runtime Ability Instances owned by this component */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Aspects|Abilities")
	const TArray<FShadowSlaveAspectAbilityInstance>& GetAbilityInstances() const { return AbilityInstances; }

	/** Returns whether an ability with the given identifier is unlocked */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Aspects|Abilities")
	bool IsAbilityUnlocked(FName AbilityId) const;

	/** Returns whether an ability with the given identifier is currently active. Active state is transient and is not saved. */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Aspects|Abilities")
	bool IsAbilityActive(FName AbilityId) const;

	/**
	 * Unlocks an ability by its unique AbilityId.
	 * Broadcasts OnAbilityUnlocked if the ability was newly unlocked.
	 * Returns true if successfully unlocked (or already unlocked).
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Aspects|Abilities")
	bool UnlockAbility(FName AbilityId);

	/** Finds an ability runtime instance by its AbilityId */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Aspects|Abilities")
	bool FindAbilityInstance(FName AbilityId, FShadowSlaveAspectAbilityInstance& OutInstance) const;

	/* --- Ability Execution Extensibility Boundary --- */

	/**
	 * Checks whether the specified ability can currently be activated.
	 * Verifies existence, unlock state, configured rank requirement, and available Essence when a
	 * data-driven cost is configured. Does not consume resources or mutate ability state.
	 */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Aspects|Execution")
	bool CanActivateAbility(FName AbilityId) const;

	/**
	 * Activates the generic runtime state for an unlocked Aspect ability.
	 * Existing data-driven Essence cost is consumed only after all validation passes. This does not
	 * implement a canon effect, combat action, range, montage, or cooldown; those remain future
	 * consumers of the activation event.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Aspects|Execution")
	bool ActivateAbility(FName AbilityId);

	/**
	 * Deactivates a currently active generic Aspect ability runtime state.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Aspects|Execution")
	bool DeactivateAbility(FName AbilityId);

	/* --- Dynamic Instance Properties --- */

	/** Sets dynamic property metadata on an individual ability instance */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Aspects|Abilities")
	bool SetAbilityDynamicProperty(FName AbilityId, FName Key, const FString& Value);

	/** Retrieves dynamic property metadata from an individual ability instance */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Aspects|Abilities")
	bool GetAbilityDynamicProperty(FName AbilityId, FName Key, FString& OutValue) const;

	/* --- Attribute Integration --- */

	/** Safe helper to locate owning actor's attribute component without duplicate ownership */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Aspects|Attributes")
	UShadowSlaveAttributeComponent* GetAttributeComponent() const;

	/* --- Event Delegates --- */

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Aspects|Events")
	FOnAspectChangedSignature OnAspectChanged;

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Aspects|Events")
	FOnAbilityUnlockedSignature OnAbilityUnlocked;

	/** Broadcast after an ability enters its transient active state and any configured Essence cost is committed. */
	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Aspects|Events")
	FOnAbilityActivatedSignature OnAbilityActivated;

	/** Broadcast after an active ability leaves its transient active state. */
	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Aspects|Events")
	FOnAbilityDeactivatedSignature OnAbilityDeactivated;

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Aspects|Events")
	FOnFlawChangedSignature OnFlawChanged;

protected:
	/** Static Aspect Definition currently bound to this character */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Aspects|State")
	TObjectPtr<UShadowSlaveAspectDefinition> AspectDefinition = nullptr;

	/** Active runtime instances of abilities granted by the Aspect */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Aspects|State")
	TArray<FShadowSlaveAspectAbilityInstance> AbilityInstances;

	/** Active Flaw definition bound to this character */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Aspects|State")
	TObjectPtr<UShadowSlaveFlawDefinition> ActiveFlawDefinition = nullptr;

	/** Guards activation/deactivation against re-entrant delegate callbacks. */
	bool bIsProcessingAbilityTransition = false;
};
