// Copyright Epic Games, Inc. All Rights Reserved.

#include "AI/ShadowSlaveTestEnemy.h"
#include "Attributes/ShadowSlaveAttributeComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"

AShadowSlaveTestEnemy::AShadowSlaveTestEnemy(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Default prototype test enemy stats
	AttackDamage = 15.0f;
	AttackRange = 160.0f;
	AttackAcceptanceRadius = 120.0f;
	AttackRecoveryDuration = 0.8f;
	WalkSpeed = 250.0f;
	SprintSpeed = 500.0f;
	StaggerDuration = 0.35f;

	// Create placeholder mesh attachment for testing when no skeletal mesh is provided
	PlaceholderMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaceholderMesh"));
	if (PlaceholderMesh)
	{
		PlaceholderMesh->SetupAttachment(GetCapsuleComponent());
		PlaceholderMesh->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
		PlaceholderMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -90.0f));

		static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMeshFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
		if (CylinderMeshFinder.Succeeded())
		{
			PlaceholderMesh->SetStaticMesh(CylinderMeshFinder.Object);
			PlaceholderMesh->SetRelativeScale3D(FVector(0.8f, 0.8f, 1.8f));
		}
	}
}

void AShadowSlaveTestEnemy::BeginPlay()
{
	Super::BeginPlay();

	// If a skeletal mesh is configured on this character, hide the placeholder mesh
	if (PlaceholderMesh)
	{
		if (GetMesh() && GetMesh()->GetSkeletalMeshAsset())
		{
			PlaceholderMesh->SetVisibility(false);
		}
		else
		{
			PlaceholderMesh->SetVisibility(true);
		}
	}
}

void AShadowSlaveTestEnemy::InitializeAttributes()
{
	Super::InitializeAttributes();

	if (AttributeComponent)
	{
		// Prototype test enemy attributes:
		// Generic non-canon values for AI verification.
		FAttributeInitConfig EnemyConfig;
		EnemyConfig.BaseMaxHealth = 80.0f;
		EnemyConfig.BaseMaxStamina = 50.0f;
		EnemyConfig.BaseMaxEssence = 0.0f; // Generic test enemy does not use essence
		EnemyConfig.bEnableStaminaRegen = false;

		AttributeComponent->InitializeAttributes(EnemyConfig);
	}
}

void AShadowSlaveTestEnemy::HandleDeath()
{
	Super::HandleDeath();

	// Visual indication of death for placeholder mesh if active
	if (PlaceholderMesh && PlaceholderMesh->IsVisible())
	{
		PlaceholderMesh->SetRelativeScale3D(FVector(0.8f, 0.8f, 0.25f));
		PlaceholderMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -80.0f));
	}
}
