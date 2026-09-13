// Copyright Epic Games, Inc. All Rights Reserved.

#include "Characters/ShadowSlaveCharacterBase.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

AShadowSlaveCharacterBase::AShadowSlaveCharacterBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;

	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.0f, 96.0f);

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement for responsive action RPG feel
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

	GetCharacterMovement()->JumpZVelocity = 700.0f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.0f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.0f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.0f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;
}

void AShadowSlaveCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	InitializeAttributes();
}

void AShadowSlaveCharacterBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AShadowSlaveCharacterBase::InitializeAttributes()
{
	// Foundation hook: Attributes component/values will initialize here in future steps
}

void AShadowSlaveCharacterBase::HandleDeath()
{
	bIsAlive = false;
	GetCharacterMovement()->DisableMovement();
	// Foundation hook: Death animations, ragdoll, and gameplay event broadcasting will be handled here
}
