// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Characters/ShadowSlaveCharacterTypes.h"
#include "Combat/ShadowSlaveDamageableInterface.h"
#include "Combat/ShadowSlaveCombatTypes.h"
#include "ShadowSlaveCharacterBase.generated.h"

class UShadowSlaveCombatComponent;
class UShadowSlaveAttributeComponent;
class UShadowSlaveEquipmentComponent;

class AShadowSlaveCharacterBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCharacterHealthChangedSignature, float, CurrentHealth, float, MaxHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCharacterDamagedSignature, const FShadowSlaveDamageInfo&, DamageInfo);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCharacterDeathSignature, AShadowSlaveCharacterBase*, DeadCharacter, AActor*, KillerActor);

/**
 * Base character class for all characters in Shadow Slave (player, companions, enemies).
 * Encapsulates core locomotion configuration, combat component integration,
 * damage receiving interface implementation, and lifecycle hooks.
 */
UCLASS(Abstract)
class SHADOWSLAVE_API AShadowSlaveCharacterBase : public ACharacter, public IShadowSlaveDamageableInterface
{
	GENERATED_BODY()

public:
	AShadowSlaveCharacterBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void Tick(float DeltaTime) override;

	/* --- IShadowSlaveDamageableInterface --- */
	virtual float TakeDamageCustom_Implementation(const FShadowSlaveDamageInfo& DamageInfo) override;
	virtual bool IsAlive_Implementation() const override { return bIsAlive; }

	// Standard engine damage handling
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	/** Delegate triggered whenever gait changes */
	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Locomotion")
	FOnGaitChangedSignature OnGaitChanged;

	/** Delegate triggered whenever character health changes */
	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Combat")
	FOnCharacterHealthChangedSignature OnHealthChanged;

	/** Delegate triggered whenever character receives damage */
	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Combat")
	FOnCharacterDamagedSignature OnCharacterDamaged;

	/** Delegate triggered whenever character dies */
	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Character|Events")
	FOnCharacterDeathSignature OnCharacterDied;

	/** Stable technical identifier for quest, dialogue, or encounter targeting */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Character")
	FName CharacterId = NAME_None;

	/** Returns stable technical identifier */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Character")
	FName GetCharacterId() const { return CharacterId; }

	/** Sets stable technical identifier */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Character")
	void SetCharacterId(FName InId) { CharacterId = InId; }

	/** Returns whether this character is currently alive */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Character")
	virtual bool IsAlive() const;

	/** Returns current gait mode */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Locomotion")
	EShadowSlaveGait GetGait() const { return CurrentGait; }

	/** Returns true if character is sprinting */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Locomotion")
	bool IsSprinting() const { return CurrentGait == EShadowSlaveGait::Sprint; }

	/** Changes current gait mode */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Locomotion")
	virtual void SetGait(EShadowSlaveGait NewGait);

	/** Initiates sprint if valid */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Locomotion")
	virtual void StartSprint();

	/** Stops sprint and returns to walking */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Locomotion")
	virtual void StopSprint();

	/** Condition hook for checking if sprint is currently allowed */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Locomotion")
	virtual bool CanSprint() const;

	/** Enables or disables player/AI movement control */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Locomotion")
	virtual void SetMovementControlEnabled(bool bEnabled);

	/** Sets movement control suppression for a specific named source (e.g. "Dodge", "Death") */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Locomotion")
	virtual void SetMovementControlSuppressed(FName Source, bool bSuppressed);

	/** Returns whether movement control is currently suppressed by a specific source */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Locomotion")
	bool IsMovementControlSuppressedBy(FName Source) const { return MovementSuppressionSources.Contains(Source); }

	/** Returns whether movement control is enabled */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Locomotion")
	bool IsMovementControlEnabled() const { return bCanMove; }

	/** Returns the modular combat component */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Combat")
	UShadowSlaveCombatComponent* GetCombatComponent() const { return CombatComponent; }

	/** Returns current health */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Attributes")
	float GetCurrentHealth() const;

	/** Returns maximum health */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Attributes")
	float GetMaxHealth() const;

	/** Returns the modular attribute component */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Attributes")
	UShadowSlaveAttributeComponent* GetAttributeComponent() const { return AttributeComponent; }

	/** Returns the modular equipment component */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Equipment")
	UShadowSlaveEquipmentComponent* GetEquipmentComponent() const { return EquipmentComponent; }

protected:
	virtual void BeginPlay() override;

	virtual void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode = 0) override;

	/** Applies tunable locomotion variables to CharacterMovementComponent */
	virtual void ApplyLocomotionSettings();

	/** Updates CharacterMovementComponent MaxWalkSpeed based on current gait */
	virtual void UpdateMaxSpeed();

	/** Lifecycle hook for initializing attributes (health, soul essence, etc.) */
	virtual void InitializeAttributes();

	/** Lifecycle hook triggered when character receives valid damage */
	virtual void OnDamaged(const FShadowSlaveDamageInfo& DamageInfo);

	/** Lifecycle hook for death handling */
	virtual void HandleDeath();

protected:
	/** Modular Combat Component responsible for combat state, attacks, and traces */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Combat", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UShadowSlaveCombatComponent> CombatComponent;

	/** Modular Attribute Component managing health, stamina, and essence */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Attributes", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UShadowSlaveAttributeComponent> AttributeComponent;

	/** Modular Equipment Component managing items and Memories across equipment slots */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Equipment", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UShadowSlaveEquipmentComponent> EquipmentComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Character")
	bool bIsAlive = true;

	/** Weak pointer to last known damage dealer for death attribution */
	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> LastDamageAttacker = nullptr;

	/** Callback when attribute component broadcasts health changes */
	UFUNCTION()
	virtual void HandleAttributeHealthChanged(float NewHealth, float MaxHealth);

	/** Updates bCanMove from active suppression sources and alive status */
	virtual void UpdateMovementControlState();

	/** Active named systems currently suppressing movement control */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Locomotion")
	TSet<FName> MovementSuppressionSources;

	/** Controls whether movement input is accepted */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Locomotion")
	bool bCanMove = true;

	/** Current movement gait */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Locomotion")
	EShadowSlaveGait CurrentGait = EShadowSlaveGait::Walk;

	/* --- Locomotion Tunables --- */

	/** Maximum walk speed in cm/s */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Locomotion", meta = (ClampMin = "50.0"))
	float WalkSpeed = 450.0f;

	/** Maximum sprint speed in cm/s */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Locomotion", meta = (ClampMin = "100.0"))
	float SprintSpeed = 750.0f;

	/** Maximum acceleration for smooth responsiveness */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Locomotion", meta = (ClampMin = "100.0"))
	float BaseMaxAcceleration = 2048.0f;

	/** Braking deceleration when walking */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Locomotion", meta = (ClampMin = "0.0"))
	float BaseBrakingDecelerationWalking = 2048.0f;

	/** Braking deceleration when falling/in-air */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Locomotion", meta = (ClampMin = "0.0"))
	float BaseBrakingDecelerationFalling = 1500.0f;

	/** Lateral friction while falling */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Locomotion", meta = (ClampMin = "0.0"))
	float BaseFallingLateralFriction = 0.5f;

	/** Air control responsiveness multiplier */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Locomotion", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float BaseAirControl = 0.35f;

	/** Boost factor for air control */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Locomotion", meta = (ClampMin = "0.0"))
	float BaseAirControlBoostMultiplier = 2.0f;

	/** Jump velocity in Z direction */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Locomotion", meta = (ClampMin = "100.0"))
	float BaseJumpZVelocity = 650.0f;

	/** Character rotation rate when turning towards movement */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Locomotion")
	FRotator BaseRotationRate = FRotator(0.0f, 540.0f, 0.0f);
};
