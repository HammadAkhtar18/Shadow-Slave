// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Combat/ShadowSlaveCombatTypes.h"
#include "ShadowSlaveCombatComponent.generated.h"

class ACharacter;
class AShadowSlaveCharacterBase;
class UShadowSlaveAttributeComponent;

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

	/** Validates if transition from CurrentCombatState to NewState is legal */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Combat")
	bool CanTransitionToState(ECombatState NewState) const;

	/** Sets combat state and broadcasts state change delegate */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Combat")
	void SetCombatState(ECombatState NewState);

	/** Checks if character can perform the requested attack */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Combat")
	virtual bool CanPerformAttack(EAttackType AttackType) const;

	/** Attempts to execute an attack (light or heavy) */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Combat")
	virtual bool ExecuteAttack(EAttackType AttackType);

	/** Cancels active attack immediately, halting montages, hit windows, and returning to Neutral */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Combat")
	void CancelAttack();

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

	/** Resets state back to Neutral if currently living */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Combat")
	void ResetToNeutral();

	/** Returns true if this combat component can apply damage to the specified target actor */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Combat")
	virtual bool CanDamageTarget(AActor* TargetActor) const;

	/** Returns the unique runtime instance ID of the active/most recent attack */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Combat")
	int32 GetCurrentAttackInstanceId() const { return CurrentAttackInstanceId; }

	/** Returns whether the specified actor has already been damaged by the current attack instance */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Combat")
	bool HasHitTargetThisAttack(AActor* TargetActor) const;

	/** Returns how many times the specified actor has been damaged by the current attack instance */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Combat")
	int32 GetHitCountForTargetThisAttack(AActor* TargetActor) const;

	/** Retrieves the owner's AttributeComponent if present */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Combat")
	UShadowSlaveAttributeComponent* GetOwnerAttributeComponent() const;

	/** Notifies combat component of damage received by the owning character */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Combat")
	void NotifyDamageReceived(const FShadowSlaveDamageInfo& DamageInfo);

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

	/** Attempts to execute a dodge in the specified direction (or direction derived from character movement/facing) */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Combat|Dodge")
	virtual bool RequestDodge(const FVector& Direction = FVector::ZeroVector);

	/** Returns true if character can perform a dodge right now (grounded, sufficient stamina, valid combat state) */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Combat|Dodge")
	virtual bool CanPerformDodge() const;

	/** Returns the active cardinal dodge direction */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Combat|Dodge")
	EDodgeDirection GetCurrentDodgeDirection() const { return CurrentDodgeCardinalDirection; }

	/** Returns the active 2D dodge movement vector */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Combat|Dodge")
	FVector GetCurrentDodgeVector() const { return ActiveDodgeDirection; }

	/** Returns current dodge configuration data */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Combat|Dodge")
	const FShadowSlaveDodgeData& GetDodgeData() const { return DodgeData; }

	/** Sets dodge configuration data */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Combat|Dodge")
	void SetDodgeData(const FShadowSlaveDodgeData& NewData) { DodgeData = NewData; }

	/** Returns whether dodge requires character to be grounded */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Combat|Dodge")
	bool IsDodgeGroundedOnly() const { return bRequireGrounded; }

	/** Sets whether dodge requires character to be grounded */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Combat|Dodge")
	void SetRequireGrounded(bool bRequire) { bRequireGrounded = bRequire; }

	/* --- Delegates --- */

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Combat")
	FOnCombatStateChangedSignature OnCombatStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Combat")
	FOnAttackExecutedSignature OnAttackExecuted;

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Combat")
	FOnAttackStartedSignature OnAttackStarted;

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Combat")
	FOnAttackEndedSignature OnAttackEnded;

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Combat")
	FOnTargetHitSignature OnTargetHit;

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Combat")
	FOnDamageDealtSignature OnDamageDealt;

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Combat")
	FOnCombatDamageReceivedSignature OnDamageReceived;

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Combat|Dodge")
	FOnDodgeStartedSignature OnDodgeStarted;

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Combat|Dodge")
	FOnDodgeEndedSignature OnDodgeEnded;

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Combat|Dodge")
	FOnDodgeRejectedSignature OnDodgeRejected;

protected:
	virtual void BeginPlay() override;

	/** Internal handler when recovery duration elapses */
	void OnRecoveryFinished();

	/** Internal handler bound to montage completion/interruption */
	void HandleMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	/** Executes physical launch, montage playback, and starts duration timer */
	virtual void ExecuteDodge(const FVector& Direction);

	/** Internal handler when dodge duration elapses */
	void OnDodgeFinished();

	/** Resolves input/movement/facing into a normalized 2D world direction vector */
	FVector ResolveDodgeDirection(const FVector& InputDirection) const;

	/** Determines cardinal direction relative to character facing */
	EDodgeDirection CalculateDodgeCardinalDirection(const FVector& Direction) const;

	/** Resolves the appropriate animation montage for the cardinal dodge direction */
	UAnimMontage* GetDodgeMontageForDirection(EDodgeDirection Direction) const;

	/** Applies dodge launch impulse and temporarily suppresses movement input */
	void ApplyDodgeMovement(const FVector& Direction);

	/** Restores character movement control after dodge finishes or cancels */
	void RestoreDodgeMovement();

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

	/** Configuration for dodge actions */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Combat|Dodge")
	FShadowSlaveDodgeData DodgeData;

	/** Whether dodge requires the character to be grounded on walkable surface */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Combat|Dodge")
	bool bRequireGrounded = true;

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

	/** Unique sequence counter for active/last executed attack */
	int32 CurrentAttackInstanceId = 0;

	/** Whether the hit detection window is currently active */
	bool bHitWindowActive = false;

	/** True if the active attack is driven by montage anim notifies; false if using timer fallback */
	bool bIsMontageDriven = false;

	/** Re-entrancy guard flag to prevent recursive state transitions during callbacks or delegate broadcasts */
	bool bIsTransitioningState = false;

	/** Active 2D unit vector of the current dodge */
	FVector ActiveDodgeDirection = FVector::ForwardVector;

	/** Active cardinal direction of the current dodge */
	EDodgeDirection CurrentDodgeCardinalDirection = EDodgeDirection::Forward;

	/** Whether movement control was suppressed specifically by active dodge */
	bool bMovementControlSuppressedByDodge = false;

	/** Tracks hit counts per target actor during the current attack instance to enforce MaxHitsPerTarget */
	TMap<TWeakObjectPtr<AActor>, int32> HitCountsThisAttack;

	/** Timer handles for hit window and recovery fallback transitions */
	FTimerHandle HitWindowTimerHandle;
	FTimerHandle RecoveryTimerHandle;
	FTimerHandle TraceLoopTimerHandle;
	FTimerHandle DodgeTimerHandle;

	/** Cached owning character reference */
	TWeakObjectPtr<AShadowSlaveCharacterBase> OwningCharacter;
};
