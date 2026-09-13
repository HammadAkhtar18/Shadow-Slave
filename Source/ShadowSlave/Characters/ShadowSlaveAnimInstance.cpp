// Copyright Epic Games, Inc. All Rights Reserved.

#include "Characters/ShadowSlaveAnimInstance.h"
#include "Characters/ShadowSlaveCharacterBase.h"
#include "Combat/ShadowSlaveCombatComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

UShadowSlaveAnimInstance::UShadowSlaveAnimInstance()
{
}

void UShadowSlaveAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	Character = Cast<AShadowSlaveCharacterBase>(TryGetPawnOwner());
	if (Character)
	{
		MovementComponent = Character->GetCharacterMovement();
	}
}

void UShadowSlaveAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!Character)
	{
		Character = Cast<AShadowSlaveCharacterBase>(TryGetPawnOwner());
		if (Character)
		{
			MovementComponent = Character->GetCharacterMovement();
		}
	}

	if (!Character || !MovementComponent)
	{
		return;
	}

	// Extract velocity and ground speed
	Velocity = MovementComponent->Velocity;
	GroundSpeed = Velocity.Size2D();
	VelocityZ = Velocity.Z;

	// Extract movement intent and states
	bIsAccelerating = MovementComponent->GetCurrentAcceleration().SizeSquared2D() > 0.0f;
	bShouldMove = (GroundSpeed > 3.0f) && bIsAccelerating;

	// Grounded / airborne states
	bIsFalling = MovementComponent->IsFalling();
	bIsGrounded = !bIsFalling;

	// Extract locomotion gait directly from character
	CurrentGait = Character->GetGait();
	bIsSprinting = Character->IsSprinting();

	// Aim / character rotation
	AimRotation = Character->GetBaseAimRotation();

	// Calculate movement direction relative to character facing (-180 to 180 degrees)
	if (!Velocity.IsNearlyZero())
	{
		const FRotator ActorRot = Character->GetActorRotation();
		const FVector ForwardVector = ActorRot.Vector();
		const FVector RightVector = FRotationMatrix(ActorRot).GetScaledAxis(EAxis::Y);
		const FVector NormalizedVel = Velocity.GetSafeNormal2D();

		const float ForwardDot = FVector::DotProduct(ForwardVector, NormalizedVel);
		const float RightDot = FVector::DotProduct(RightVector, NormalizedVel);

		MovementDirection = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(ForwardDot, -1.0f, 1.0f)));
		if (RightDot < 0.0f)
		{
			MovementDirection = -MovementDirection;
		}
	}
	else
	{
		MovementDirection = 0.0f;
	}

	// Extract extensible lifecycle and control state
	bIsAlive = Character->IsAlive();
	bIsMovementControlEnabled = Character->IsMovementControlEnabled();

	if (UShadowSlaveCombatComponent* CombatComp = Character->GetCombatComponent())
	{
		CurrentCombatState = CombatComp->GetCombatState();
	}
	else
	{
		CurrentCombatState = ECombatState::Neutral;
	}
}

void UShadowSlaveAnimInstance::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeThreadSafeUpdateAnimation(DeltaSeconds);
}
