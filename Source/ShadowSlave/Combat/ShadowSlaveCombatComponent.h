// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Combat/ShadowSlaveCombatTypes.h"
#include "ShadowSlaveCombatComponent.generated.h"

class ACharacter;

/**
 * Modular Combat Component responsible for combat state, attack execution,
 * melee trace hit detection, and damage routing.
 * Reusable by both player characters and future AI enemies.
 */
UCLASS(ClassGroup = (ShadowSlave), meta = (BlueprintSpawnableComponent))
class SHADOWSLAVE_API UShadowSlaveCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UShadowSlaveCombatComponent();

	/* --- Public API --- */

	/** Returns current combat state */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Combat")
	ECombatState GetCombatState() const { return CurrentCombatState; }

	/** Sets combat state and broadcasts state change delegate */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Combat")
	void SetCombatState(ECombatState NewState);

	/** Checks if character can perform the requested attack */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Combat")
	virtual bool CanPerformAttack(EAttackType AttackType) const;

	/** Attempts to execute an attack (light or heavy) */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Combat")
	virtual bool ExecuteAttack(EAttackType AttackType);

	/** Opens the melee hit detection window (called via AnimNotifyState or fallback timer) */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Combat")
	void OpenHitWindow();

	/** Closes the hit detection window and begins recovery */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Combat")
	void CloseHitWindow();

	/** Performs sphere sweep hit detection along the attack path */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Combat")
	void PerformMeleeTrace();

	/** Notifies component of owner death to lock combat state */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Combat")
	void HandleOwnerDeath();

	/** Resets state back to Neutral */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Combat")
	void ResetToNeutral();

	/** Returns true if this combat component can apply damage to the specified target actor */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Combat")
	virtual bool CanDamageTarget(AActor* TargetActor) const;

	/** Retrieves the attack data struct for the given attack type */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Combat")
	const FShadowSlaveAttackData& GetAttackData(EAttackType AttackType) const;

	/** Sets attack data configuration for light attacks */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Combat")
	void SetLightAttackData(const FShadowSlaveAttackData& NewData) { LightAttackData = NewData; }

	/** Sets attack data configuration for heavy attacks */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Combat")
	void SetHeavyAttackData(const FShadowSlaveAttackData& NewData) { HeavyAttackData = NewData; }

	/** Returns attack data configuration for light attacks */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Combat")
	const FShadowSlaveAttackData& GetLightAttackData() const { return LightAttackData; }

	/** Returns attack data configuration for heavy attacks */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Combat")
	const FShadowSlaveAttackData& GetHeavyAttackData() const { return HeavyAttackData; }

	/* --- Delegates --- */

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Combat")
	FOnCombatStateChangedSignature OnCombatStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Combat")
	FOnAttackExecutedSignature OnAttackExecuted;

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Combat")
	FOnTargetHitSignature OnTargetHit;

protected:
	virtual void BeginPlay() override;

	/** Internal handler when recovery duration elapses */
	void OnRecoveryFinished();

protected:
	/** Current state of the actor in combat */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Combat")
	ECombatState CurrentCombatState = ECombatState::Neutral;

	/** Configuration for light attack */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Combat|Attacks")
	FShadowSlaveAttackData LightAttackData;

	/** Configuration for heavy attack */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Combat|Attacks")
	FShadowSlaveAttackData HeavyAttackData;

	/** Collision channel used for melee hit detection */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Combat|Detection")
	TEnumAsByte<ECollisionChannel> MeleeTraceChannel = ECC_Pawn;

	/** Whether friendly fire is allowed between actors of the same affiliation */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Combat|Detection")
	bool bAllowFriendlyFire = false;

	/** Enable debug visualization for melee sweep traces */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Combat|Debug")
	bool bDrawDebugTraces = true;

	/** Lifetime of debug trace spheres in seconds */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Combat|Debug", meta = (EditCondition = "bDrawDebugTraces"))
	float DebugTraceDuration = 1.0f;

private:
	/** Currently active attack data */
	FShadowSlaveAttackData ActiveAttackData;

	/** Whether the hit detection window is currently active */
	bool bHitWindowActive = false;

	/** Tracks actors hit during the current attack to prevent multi-hitting the same target */
	TArray<TWeakObjectPtr<AActor>> HitActorsThisAttack;

	/** Timer handles for hit window and recovery fallback transitions */
	FTimerHandle HitWindowTimerHandle;
	FTimerHandle RecoveryTimerHandle;
	FTimerHandle TraceLoopTimerHandle;

	/** Cached owning character reference */
	TWeakObjectPtr<ACharacter> OwningCharacter;
};
