// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StatusEffects/ShadowSlaveStatusEffectTypes.h"
#include "StatusEffects/ShadowSlaveStatusEffectDefinition.h"
#include "ShadowSlaveStatusEffectComponent.generated.h"

/**
 * Component managing active status effects / conditions on an actor.
 * Generic technical foundation with zero canon mechanics.
 * Purely event-driven and timer-driven: NO Tick, NO polling loops.
 * Strictly respects AttributeComponent authority: does NOT duplicate health/stamina/essence.
 */
UCLASS(ClassGroup = (ShadowSlave), meta = (BlueprintSpawnableComponent))
class SHADOWSLAVE_API UShadowSlaveStatusEffectComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UShadowSlaveStatusEffectComponent();

	/* --- Application & Removal API --- */

	/**
	 * Applies a status effect based on its definition archetype.
	 * Handles stacking policy (IgnoreNew, RefreshDuration, AddStacks, Replace).
	 * Handles duration policy (Instant, Timed, Persistent).
	 * Returns the unique instance GUID if applied or updated, or invalid GUID if rejected.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|StatusEffects")
	FGuid ApplyEffect(UShadowSlaveStatusEffectDefinition* EffectDefinition, const FShadowSlaveStatusEffectSource& Source = FShadowSlaveStatusEffectSource(), const TMap<FName, FString>& DynamicProperties = TMap<FName, FString>());

	/** Convenience overload for applying an effect without source attribution or properties */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|StatusEffects")
	FGuid ApplyEffectSimple(UShadowSlaveStatusEffectDefinition* EffectDefinition);

	/**
	 * Removes a specific status effect instance by its unique GUID.
	 * Returns true if found and removed.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|StatusEffects")
	bool RemoveEffect(const FGuid& InstanceId);

	/**
	 * Removes the first active status effect matching the specified definition.
	 * Returns true if an effect was removed.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|StatusEffects")
	bool RemoveEffectByDefinition(const UShadowSlaveStatusEffectDefinition* EffectDefinition);

	/**
	 * Removes all active status effects matching the specified definition.
	 * Returns the number of removed instances.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|StatusEffects")
	int32 RemoveAllEffectsByDefinition(const UShadowSlaveStatusEffectDefinition* EffectDefinition);

	/**
	 * Removes all active status effects from this component.
	 * Returns the number of removed instances.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|StatusEffects")
	int32 RemoveAllEffects();

	/** Clears all active status effects (alias for RemoveAllEffects) */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|StatusEffects")
	void ClearEffects();

	/* --- Query API --- */

	/** Returns true if any active effect matches the given definition */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|StatusEffects")
	bool HasEffect(const UShadowSlaveStatusEffectDefinition* EffectDefinition) const;

	/** Returns true if any active effect matches the given EffectId */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|StatusEffects")
	bool HasEffectById(FName EffectId) const;

	/** Returns true if an active effect with the given InstanceId exists */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|StatusEffects")
	bool HasEffectByInstanceId(const FGuid& InstanceId) const;

	/** Finds the first active effect matching the given definition */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|StatusEffects")
	bool FindEffect(const UShadowSlaveStatusEffectDefinition* EffectDefinition, FShadowSlaveStatusEffectInstance& OutEffect) const;

	/** Finds an active effect by its unique InstanceId */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|StatusEffects")
	bool FindEffectByInstanceId(const FGuid& InstanceId, FShadowSlaveStatusEffectInstance& OutEffect) const;

	/** Finds an active effect by its EffectId */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|StatusEffects")
	bool FindEffectById(FName EffectId, FShadowSlaveStatusEffectInstance& OutEffect) const;

	/** Retrieves all active effects matching the given definition */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|StatusEffects")
	void GetAllEffectsByDefinition(const UShadowSlaveStatusEffectDefinition* EffectDefinition, TArray<FShadowSlaveStatusEffectInstance>& OutEffects) const;

	/** Returns the total stack count for all active instances of the given definition */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|StatusEffects")
	int32 GetStackCount(const UShadowSlaveStatusEffectDefinition* EffectDefinition) const;

	/** Returns the number of active effects */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|StatusEffects")
	int32 GetEffectCount() const { return ActiveEffects.Num(); }

	/** Returns the remaining duration for the given instance (in seconds, -1.0 for persistent, 0.0 if not found or expired) */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|StatusEffects")
	float GetRemainingDuration(const FGuid& InstanceId) const;

	/** Const reference to active effects for C++ consumers */
	const TArray<FShadowSlaveStatusEffectInstance>& GetActiveEffects() const { return ActiveEffects; }

	/** Copy of active effects for Blueprint consumers */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|StatusEffects")
	TArray<FShadowSlaveStatusEffectInstance> GetActiveEffectsCopy() const { return ActiveEffects; }

	/* --- Persistence / Restoration Support --- */

	/**
	 * Restores an array of status effect instances from serialized state.
	 * Re-evaluates timers and delegates deterministically.
	 */
	void RestoreEffects(const TArray<FShadowSlaveStatusEffectInstance>& InEffects);

	/* --- Delegates --- */

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|StatusEffects|Events")
	FOnStatusEffectAppliedSignature OnStatusEffectApplied;

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|StatusEffects|Events")
	FOnStatusEffectRemovedSignature OnStatusEffectRemoved;

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|StatusEffects|Events")
	FOnStatusEffectExpiredSignature OnStatusEffectExpired;

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|StatusEffects|Events")
	FOnStatusEffectStackChangedSignature OnStatusEffectStackChanged;

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|StatusEffects|Events")
	FOnStatusEffectCollectionChangedSignature OnStatusEffectCollectionChanged;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Timer callback when a timed effect expires */
	UFUNCTION()
	void HandleEffectExpired(FGuid InstanceId);

	/** Internal helper to schedule an expiration timer for a timed effect instance */
	void ScheduleExpirationTimer(FShadowSlaveStatusEffectInstance& Instance);

	/** Internal helper to clear and invalidate an expiration timer */
	void ClearExpirationTimer(FShadowSlaveStatusEffectInstance& Instance);

protected:
	/** Collection of currently active status effect instances */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|StatusEffects")
	TArray<FShadowSlaveStatusEffectInstance> ActiveEffects;

	/** Re-entrancy guard flag to prevent recursive modifications during delegate broadcasts */
	bool bIsProcessingEffectTransition = false;
};
