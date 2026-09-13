// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Characters/ShadowSlaveCharacterTypes.h"
#include "Combat/ShadowSlaveCombatTypes.h"
#include "ShadowSlaveAnimInstance.generated.h"

class AShadowSlaveCharacterBase;
class UCharacterMovementComponent;

/**
 * Base Animation Instance class for Shadow Slave characters.
 * Extracts movement metrics from AShadowSlaveCharacterBase and UCharacterMovementComponent
 * to drive the Animation Blueprint without duplicating gameplay logic.
 */
UCLASS()
class SHADOWSLAVE_API UShadowSlaveAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	UShadowSlaveAnimInstance();

	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;

protected:
	/** Cached owning character reference */
	UPROPERTY(BlueprintReadOnly, Category = "ShadowSlave|Animation", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<AShadowSlaveCharacterBase> Character;

	/** Cached character movement component */
	UPROPERTY(BlueprintReadOnly, Category = "ShadowSlave|Animation", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCharacterMovementComponent> MovementComponent;

	/* --- Locomotion Metrics --- */

	/** 2D horizontal speed (cm/s) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Animation|Locomotion", meta = (AllowPrivateAccess = "true"))
	float GroundSpeed = 0.0f;

	/** Vertical velocity for jumping/falling arcs (cm/s) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Animation|Locomotion", meta = (AllowPrivateAccess = "true"))
	float VelocityZ = 0.0f;

	/** Current 3D velocity vector */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Animation|Locomotion", meta = (AllowPrivateAccess = "true"))
	FVector Velocity = FVector::ZeroVector;

	/** True when character is grounded, has significant velocity (> 3 cm/s), and active acceleration */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Animation|Locomotion", meta = (AllowPrivateAccess = "true"))
	bool bShouldMove = false;

	/** True if character is in the air (jumping or falling) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Animation|Locomotion", meta = (AllowPrivateAccess = "true"))
	bool bIsFalling = false;

	/** True if character is grounded on walkable surface */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Animation|Locomotion", meta = (AllowPrivateAccess = "true"))
	bool bIsGrounded = true;

	/** True if character has non-zero movement input acceleration */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Animation|Locomotion", meta = (AllowPrivateAccess = "true"))
	bool bIsAccelerating = false;

	/** Current movement gait mode (Walk, Sprint) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Animation|Locomotion", meta = (AllowPrivateAccess = "true"))
	EShadowSlaveGait CurrentGait = EShadowSlaveGait::Walk;

	/** Convenience boolean flag for sprinting state */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Animation|Locomotion", meta = (AllowPrivateAccess = "true"))
	bool bIsSprinting = false;

	/** Angle in degrees (-180 to 180) between character facing rotation and velocity vector */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Animation|Locomotion", meta = (AllowPrivateAccess = "true"))
	float MovementDirection = 0.0f;

	/** Character rotation / aim rotation */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Animation|Locomotion", meta = (AllowPrivateAccess = "true"))
	FRotator AimRotation = FRotator::ZeroRotator;

	/* --- Extensible Lifecycle & Combat Hooks --- */

	/** Reflects character alive status (future: death animations, ragdoll) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Animation|State", meta = (AllowPrivateAccess = "true"))
	bool bIsAlive = true;

	/** Reflects movement control status (future: stuns, root-motion attack locks) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Animation|State", meta = (AllowPrivateAccess = "true"))
	bool bIsMovementControlEnabled = true;

	/** Current combat state from the owning character's combat component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Animation|Combat", meta = (AllowPrivateAccess = "true"))
	ECombatState CurrentCombatState = ECombatState::Neutral;

public:
	/** Returns cached owning character */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Animation")
	AShadowSlaveCharacterBase* GetOwningCharacter() const { return Character; }

	/** Returns cached movement component */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Animation")
	UCharacterMovementComponent* GetMovementComponent() const { return MovementComponent; }
};
