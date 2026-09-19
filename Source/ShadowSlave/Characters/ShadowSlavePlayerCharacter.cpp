// Copyright Epic Games, Inc. All Rights Reserved.

#include "Characters/ShadowSlavePlayerCharacter.h"
#include "Core/ShadowSlavePlayerController.h"
#include "Combat/ShadowSlaveCombatComponent.h"
#include "Attributes/ShadowSlaveAttributeComponent.h"
#include "Items/ShadowSlaveInventoryComponent.h"
#include "Memories/ShadowSlaveMemoryComponent.h"
#include "Progression/ShadowSlaveProgressionComponent.h"
#include "Aspects/ShadowSlaveAspectComponent.h"
#include "Echoes/ShadowSlaveEchoComponent.h"
#include "Interaction/ShadowSlaveInteractionComponent.h"
#include "Gameplay/ShadowSlaveQuestSubsystem.h"
#include "Gameplay/ShadowSlaveGameplaySubsystem.h"
#include "Engine/GameInstance.h"
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

	// Create modular inventory component
	InventoryComponent = CreateDefaultSubobject<UShadowSlaveInventoryComponent>(TEXT("InventoryComponent"));

	// Create modular memory component
	MemoryComponent = CreateDefaultSubobject<UShadowSlaveMemoryComponent>(TEXT("MemoryComponent"));

	// Create modular progression component
	ProgressionComponent = CreateDefaultSubobject<UShadowSlaveProgressionComponent>(TEXT("ProgressionComponent"));

	// Create modular aspect component
	AspectComponent = CreateDefaultSubobject<UShadowSlaveAspectComponent>(TEXT("AspectComponent"));

	// Create modular echo component
	EchoComponent = CreateDefaultSubobject<UShadowSlaveEchoComponent>(TEXT("EchoComponent"));

	// Create modular interaction component
	InteractionComponent = CreateDefaultSubobject<UShadowSlaveInteractionComponent>(TEXT("InteractionComponent"));
}

void AShadowSlavePlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	NotifyGameplaySubsystemContext();
}

void AShadowSlavePlayerCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearGameplaySubsystemContext();

	Super::EndPlay(EndPlayReason);
}

void AShadowSlavePlayerCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	NotifyGameplaySubsystemContext();
}

void AShadowSlavePlayerCharacter::UnPossessed()
{
	ClearGameplaySubsystemContext();

	Super::UnPossessed();
}

void AShadowSlavePlayerCharacter::OnRep_Controller()
{
	Super::OnRep_Controller();

	NotifyGameplaySubsystemContext();
}

void AShadowSlavePlayerCharacter::NotifyGameplaySubsystemContext()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		APlayerController* PC = Cast<APlayerController>(GetController());
		if (UShadowSlaveGameplaySubsystem* GameplaySub = GI->GetSubsystem<UShadowSlaveGameplaySubsystem>())
		{
			GameplaySub->SetPlayerContext(PC, this);
		}
		else if (UShadowSlaveQuestSubsystem* QuestSub = GI->GetSubsystem<UShadowSlaveQuestSubsystem>())
		{
			QuestSub->RegisterPlayerContext(this);
		}
	}
}

void AShadowSlavePlayerCharacter::ClearGameplaySubsystemContext()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UShadowSlaveGameplaySubsystem* GameplaySub = GI->GetSubsystem<UShadowSlaveGameplaySubsystem>())
		{
			if (GameplaySub->GetPlayerPawn() == this)
			{
				GameplaySub->SetPlayerContext(nullptr, nullptr);
			}
		}
		else if (UShadowSlaveQuestSubsystem* QuestSub = GI->GetSubsystem<UShadowSlaveQuestSubsystem>())
		{
			QuestSub->UnregisterPlayerContext();
		}
	}
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

		// Light Attack
		if (LightAttackAction)
		{
			EnhancedInputComponent->BindAction(LightAttackAction, ETriggerEvent::Started, this, &AShadowSlavePlayerCharacter::LightAttack);
		}

		// Heavy Attack
		if (HeavyAttackAction)
		{
			EnhancedInputComponent->BindAction(HeavyAttackAction, ETriggerEvent::Started, this, &AShadowSlavePlayerCharacter::HeavyAttack);
		}

		// Interact
		if (InteractAction)
		{
			EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &AShadowSlavePlayerCharacter::Interact);
		}

		// Dodge
		if (DodgeAction)
		{
			EnhancedInputComponent->BindAction(DodgeAction, ETriggerEvent::Started, this, &AShadowSlavePlayerCharacter::Dodge);
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

void AShadowSlavePlayerCharacter::LightAttack()
{
	if (bCanMove && CombatComponent)
	{
		CombatComponent->ExecuteAttack(EAttackType::Light);
	}
}

void AShadowSlavePlayerCharacter::HeavyAttack()
{
	if (bCanMove && CombatComponent)
	{
		CombatComponent->ExecuteAttack(EAttackType::Heavy);
	}
}

void AShadowSlavePlayerCharacter::Interact()
{
	if (bCanMove && InteractionComponent)
	{
		InteractionComponent->TryInteract();
	}
}

void AShadowSlavePlayerCharacter::Dodge()
{
	if (CombatComponent)
	{
		CombatComponent->RequestDodge();
	}
}

void AShadowSlavePlayerCharacter::InitializeAttributes()
{
	Super::InitializeAttributes();

	if (AttributeComponent)
	{
		// Prototype player attribute configuration
		// NOT final canon stats for Sunny (Sunless); temporary baseline values for testing only.
		FAttributeInitConfig PlayerConfig;
		PlayerConfig.BaseMaxHealth = 100.0f;
		PlayerConfig.BaseMaxStamina = 100.0f;
		PlayerConfig.BaseMaxEssence = 100.0f;
		PlayerConfig.bEnableStaminaRegen = true;
		PlayerConfig.StaminaRegenRate = 25.0f;
		PlayerConfig.StaminaRegenDelay = 1.0f;
		PlayerConfig.StaminaRegenTickInterval = 0.1f;

		AttributeComponent->InitializeAttributes(PlayerConfig);
	}
}

