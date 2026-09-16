// Copyright Epic Games, Inc. All Rights Reserved.

#include "Characters/ShadowSlaveCharacterBase.h"
#include "Combat/ShadowSlaveCombatComponent.h"
#include "Attributes/ShadowSlaveAttributeComponent.h"
#include "Equipment/ShadowSlaveEquipmentComponent.h"
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

	// Create modular attribute component
	AttributeComponent = CreateDefaultSubobject<UShadowSlaveAttributeComponent>(TEXT("AttributeComponent"));

	// Create modular equipment component
	EquipmentComponent = CreateDefaultSubobject<UShadowSlaveEquipmentComponent>(TEXT("EquipmentComponent"));
}

void AShadowSlaveCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	bIsAlive = true;
	MovementSuppressionSources.Empty();
	UpdateMovementControlState();

	ApplyLocomotionSettings();
	InitializeAttributes();

	if (AttributeComponent)
	{
		AttributeComponent->OnHealthChanged.AddDynamic(this, &AShadowSlaveCharacterBase::HandleAttributeHealthChanged);
		AttributeComponent->OnDeath.AddDynamic(this, &AShadowSlaveCharacterBase::HandleDeath);
	}

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
	if (!IsAlive())
	{
		return 0.0f;
	}

	float ActualDamage = 0.0f;
	if (AttributeComponent)
	{
		ActualDamage = AttributeComponent->ApplyDamage(DamageInfo.DamageAmount, DamageInfo);
	}

	if (ActualDamage > 0.0f)
	{
		OnDamaged(DamageInfo);

		if (CombatComponent)
		{
			CombatComponent->NotifyDamageReceived(DamageInfo);
		}
	}

	return ActualDamage;
}

void AShadowSlaveCharacterBase::HandleAttributeHealthChanged(float NewHealth, float MaxHealth)
{
	OnHealthChanged.Broadcast(NewHealth, MaxHealth);
}

bool AShadowSlaveCharacterBase::IsAlive() const
{
	return AttributeComponent ? AttributeComponent->IsAlive() : bIsAlive;
}

float AShadowSlaveCharacterBase::GetCurrentHealth() const
{
	return AttributeComponent ? AttributeComponent->GetCurrentHealth() : 0.0f;
}

float AShadowSlaveCharacterBase::GetMaxHealth() const
{
	return AttributeComponent ? AttributeComponent->GetMaximumHealth() : 0.0f;
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
	if (!IsAlive() || !bCanMove)
	{
		return false;
	}

	// Must have stamina available if attribute component is attached
	if (AttributeComponent && AttributeComponent->GetCurrentStamina() <= 0.0f)
	{
		return false;
	}

	// Cannot sprint while attacking, dodging, or stunned
	if (CombatComponent && (CombatComponent->GetCombatState() == ECombatState::Attacking ||
		CombatComponent->GetCombatState() == ECombatState::Dodging ||
		CombatComponent->GetCombatState() == ECombatState::Stunned))
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
	SetMovementControlSuppressed(TEXT("Manual"), !bEnabled);
}

void AShadowSlaveCharacterBase::SetMovementControlSuppressed(FName Source, bool bSuppressed)
{
	if (bSuppressed)
	{
		MovementSuppressionSources.Add(Source);
	}
	else
	{
		MovementSuppressionSources.Remove(Source);
	}

	UpdateMovementControlState();
}

void AShadowSlaveCharacterBase::UpdateMovementControlState()
{
	bCanMove = (MovementSuppressionSources.Num() == 0) && bIsAlive;
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
	SetMovementControlSuppressed(TEXT("Death"), true);

	if (CombatComponent)
	{
		CombatComponent->HandleOwnerDeath();
	}

	if (GetCharacterMovement())
	{
		GetCharacterMovement()->DisableMovement();
	}
}
