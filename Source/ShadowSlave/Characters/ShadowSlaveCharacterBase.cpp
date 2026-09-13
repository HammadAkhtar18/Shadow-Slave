// Copyright Epic Games, Inc. All Rights Reserved.

#include "Characters/ShadowSlaveCharacterBase.h"
#include "Combat/ShadowSlaveCombatComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Perception/AIPerceptionSystem.h"
#include "Perception/AISense_Sight.h"

AShadowSlaveCharacterBase::AShadowSlaveCharacterBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;

	// Set size for standard humanoid collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.0f, 96.0f);

	// Character rotation is decoupled from controller orientation (camera rotates independently)
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Orient character toward movement direction
	GetCharacterMovement()->bOrientRotationToMovement = true;

	// Apply base locomotion values
	ApplyLocomotionSettings();

	// Create modular combat component
	CombatComponent = CreateDefaultSubobject<UShadowSlaveCombatComponent>(TEXT("CombatComponent"));
}

void AShadowSlaveCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	CurrentHealth = MaxHealth;
	bIsAlive = true;

	ApplyLocomotionSettings();
	InitializeAttributes();

	// Register character as a sight perception stimulus source
	UAIPerceptionSystem::RegisterPerceptionStimuliSource(this, UAISense_Sight::StaticClass(), this);
}

void AShadowSlaveCharacterBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Auto-cancel sprint if character has decelerated or stopped moving forward
	if (CurrentGait == EShadowSlaveGait::Sprint && !CanSprint())
	{
		StopSprint();
	}
}

float AShadowSlaveCharacterBase::TakeDamageCustom_Implementation(const FShadowSlaveDamageInfo& DamageInfo)
{
	if (!bIsAlive)
	{
		return 0.0f;
	}

	const float ActualDamage = FMath::Clamp(DamageInfo.DamageAmount, 0.0f, CurrentHealth);
	CurrentHealth -= ActualDamage;

	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
	OnDamaged(DamageInfo);

	if (CurrentHealth <= 0.0f)
	{
		HandleDeath();
	}

	return ActualDamage;
}

float AShadowSlaveCharacterBase::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser)
{
	if (!bIsAlive)
	{
		return 0.0f;
	}

	return Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
}

void AShadowSlaveCharacterBase::ApplyLocomotionSettings()
{
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->RotationRate = BaseRotationRate;
		MoveComp->MaxAcceleration = BaseMaxAcceleration;
		MoveComp->BrakingDecelerationWalking = BaseBrakingDecelerationWalking;
		MoveComp->BrakingDecelerationFalling = BaseBrakingDecelerationFalling;
		MoveComp->FallingLateralFriction = BaseFallingLateralFriction;
		MoveComp->AirControl = BaseAirControl;
		MoveComp->AirControlBoostMultiplier = BaseAirControlBoostMultiplier;
		MoveComp->JumpZVelocity = BaseJumpZVelocity;
		MoveComp->bUseSeparateBrakingFriction = true;
		MoveComp->BrakingFrictionFactor = 1.0f;

		UpdateMaxSpeed();
	}
}

void AShadowSlaveCharacterBase::UpdateMaxSpeed()
{
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->MaxWalkSpeed = (CurrentGait == EShadowSlaveGait::Sprint) ? SprintSpeed : WalkSpeed;
	}
}

void AShadowSlaveCharacterBase::SetGait(EShadowSlaveGait NewGait)
{
	if (CurrentGait == NewGait)
	{
		return;
	}

	const EShadowSlaveGait OldGait = CurrentGait;
	CurrentGait = NewGait;
	UpdateMaxSpeed();
	OnGaitChanged.Broadcast(OldGait, NewGait);
}

bool AShadowSlaveCharacterBase::CanSprint() const
{
	// Must be alive, control enabled, grounded, and actually moving
	if (!bIsAlive || !bCanMove)
	{
		return false;
	}

	// Cannot sprint while attacking or stunned
	if (CombatComponent && (CombatComponent->GetCombatState() == ECombatState::Attacking || CombatComponent->GetCombatState() == ECombatState::Stunned))
	{
		return false;
	}

	const UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	if (!MoveComp || MoveComp->IsFalling())
	{
		return false;
	}

	// Moving forward check (horizontal speed must exceed 50 cm/s)
	return GetVelocity().SizeSquared2D() > 2500.0f;
}

void AShadowSlaveCharacterBase::StartSprint()
{
	if (CanSprint())
	{
		SetGait(EShadowSlaveGait::Sprint);
	}
}

void AShadowSlaveCharacterBase::StopSprint()
{
	if (CurrentGait == EShadowSlaveGait::Sprint)
	{
		SetGait(EShadowSlaveGait::Walk);
	}
}

void AShadowSlaveCharacterBase::SetMovementControlEnabled(bool bEnabled)
{
	bCanMove = bEnabled;
	if (!bCanMove)
	{
		StopSprint();
	}
}

void AShadowSlaveCharacterBase::OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PrevMovementMode, PreviousCustomMode);

	// Stop sprint if character enters falling state
	if (GetCharacterMovement() && GetCharacterMovement()->IsFalling())
	{
		StopSprint();
	}
}

void AShadowSlaveCharacterBase::InitializeAttributes()
{
	// Modular hook: attributes (health, soul essence) initialize here in future steps
}

void AShadowSlaveCharacterBase::OnDamaged(const FShadowSlaveDamageInfo& DamageInfo)
{
	OnCharacterDamaged.Broadcast(DamageInfo);
}

void AShadowSlaveCharacterBase::HandleDeath()
{
	bIsAlive = false;
	CurrentHealth = 0.0f;
	SetMovementControlEnabled(false);

	if (CombatComponent)
	{
		CombatComponent->HandleOwnerDeath();
	}

	if (GetCharacterMovement())
	{
		GetCharacterMovement()->DisableMovement();
	}
}
