// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Characters/ShadowSlaveCharacterTypes.h"
#include "ShadowSlaveCharacterBase.generated.h"

/**
 * Base character class for all characters in Shadow Slave (player, companions, enemies).
 * Encapsulates core locomotion configuration, movement state transitions, and lifecycle hooks.
 */
UCLASS(Abstract)
class SHADOWSLAVE_API AShadowSlaveCharacterBase : public ACharacter
{
	GENERATED_BODY()

public:
	AShadowSlaveCharacterBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void Tick(float DeltaTime) override;

	/** Delegate triggered whenever gait changes (e.g. for stamina drain or animation states) */
	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Locomotion")
	FOnGaitChangedSignature OnGaitChanged;

	/** Returns whether this character is currently alive */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Character")
	virtual bool IsAlive() const { return bIsAlive; }

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

	/** Condition hook for checking if sprint is currently allowed (future: stamina, status effects) */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Locomotion")
	virtual bool CanSprint() const;

	/** Enables or disables player/AI movement control (future: stuns, root motion attacks) */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Locomotion")
	virtual void SetMovementControlEnabled(bool bEnabled);

	/** Returns whether movement control is enabled */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Locomotion")
	bool IsMovementControlEnabled() const { return bCanMove; }

protected:
	virtual void BeginPlay() override;

	virtual void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode = 0) override;

	/** Applies tunable locomotion variables to CharacterMovementComponent */
	virtual void ApplyLocomotionSettings();

	/** Updates CharacterMovementComponent MaxWalkSpeed based on current gait */
	virtual void UpdateMaxSpeed();

	/** Lifecycle hook for initializing attributes (health, soul essence, etc.) */
	virtual void InitializeAttributes();

	/** Lifecycle hook for death handling */
	virtual void HandleDeath();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Character")
	bool bIsAlive = true;

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

	/** Maximum acceleration for smooth and responsive responsiveness */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Locomotion", meta = (ClampMin = "100.0"))
	float BaseMaxAcceleration = 2048.0f;

	/** Braking deceleration when walking (prevents ice-skating) */
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
