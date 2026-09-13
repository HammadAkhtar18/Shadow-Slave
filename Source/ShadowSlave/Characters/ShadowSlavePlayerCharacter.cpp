// Copyright Epic Games, Inc. All Rights Reserved.

#include "Characters/ShadowSlavePlayerCharacter.h"
#include "Core/ShadowSlavePlayerController.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Engine/LocalPlayer.h"
#include "ShadowSlave.h"

AShadowSlavePlayerCharacter::AShadowSlavePlayerCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Create collision-aware spring arm boom
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = DefaultTargetArmLength;
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bDoCollisionTest = true;
	CameraBoom->ProbeSize = 12.0f;
	CameraBoom->ProbeChannel = ECC_Camera;
	CameraBoom->SocketOffset = CameraSocketOffset;
	CameraBoom->TargetOffset = CameraTargetOffset;
	CameraBoom->bEnableCameraLag = bEnableCameraLag;
	CameraBoom->CameraLagSpeed = CameraLagSpeed;
	CameraBoom->bEnableCameraRotationLag = bEnableCameraRotationLag;
	CameraBoom->CameraRotationLagSpeed = CameraRotationLagSpeed;

	// Create follow camera positioned at spring arm socket
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
}

void AShadowSlavePlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Register default Input Mapping Context if defined on character
	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			if (DefaultMappingContext)
			{
				Subsystem->AddMappingContext(DefaultMappingContext, 0);
			}
		}
	}

	// Bind Enhanced Input actions
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Jump
		if (JumpAction)
		{
			EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &AShadowSlavePlayerCharacter::JumpStarted);
			EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &AShadowSlavePlayerCharacter::JumpStopped);
		}

		// Move (WASD / Left Stick)
		if (MoveAction)
		{
			EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AShadowSlavePlayerCharacter::Move);
		}

		// Look (Mouse / Right Stick)
		if (LookAction)
		{
			EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AShadowSlavePlayerCharacter::Look);
		}

		// Sprint (Left Shift / Gamepad L3)
		if (SprintAction)
		{
			EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &AShadowSlavePlayerCharacter::SprintStarted);
			EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &AShadowSlavePlayerCharacter::SprintStopped);
		}
	}
	else
	{
		UE_LOG(LogShadowSlave, Error, TEXT("'%s' Failed to find an Enhanced Input component! Enhanced Input is required."), *GetNameSafe(this));
	}
}

void AShadowSlavePlayerCharacter::Move(const FInputActionValue& Value)
{
	if (!bCanMove)
	{
		return;
	}

	// Value is a Vector2D (X = Right/Left, Y = Forward/Backward)
	const FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// Compute camera-relative forward and right directions
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0.0f, Rotation.Yaw, 0.0f);

		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// Apply movement input
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void AShadowSlavePlayerCharacter::Look(const FInputActionValue& Value)
{
	// Value is a Vector2D (X = Yaw, Y = Pitch)
	const FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		float SensitivityYaw = 1.0f;
		float SensitivityPitch = 1.0f;

		if (const AShadowSlavePlayerController* PC = Cast<AShadowSlavePlayerController>(Controller))
		{
			SensitivityYaw = PC->GetLookSensitivityYaw();
			SensitivityPitch = PC->GetLookSensitivityPitch();
		}

		AddControllerYawInput(LookAxisVector.X * SensitivityYaw);
		AddControllerPitchInput(LookAxisVector.Y * SensitivityPitch);
	}
}

void AShadowSlavePlayerCharacter::JumpStarted()
{
	if (bCanMove)
	{
		Jump();
	}
}

void AShadowSlavePlayerCharacter::JumpStopped()
{
	StopJumping();
}

void AShadowSlavePlayerCharacter::SprintStarted()
{
	StartSprint();
}

void AShadowSlavePlayerCharacter::SprintStopped()
{
	StopSprint();
}
