// Copyright Epic Games, Inc. All Rights Reserved.

#include "Combat/ShadowSlaveCombatDummy.h"
#include "Attributes/ShadowSlaveAttributeComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "ShadowSlave.h"

AShadowSlaveCombatDummy::AShadowSlaveCombatDummy()
{
	PrimaryActorTick.bCanEverTick = false;

	// Root collision capsule configured as Pawn channel for melee trace detection
	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComponent"));
	CapsuleComponent->InitCapsuleSize(45.0f, 96.0f);
	CapsuleComponent->SetCollisionProfileName(TEXT("Pawn"));
	CapsuleComponent->SetGenerateOverlapEvents(true);
	RootComponent = CapsuleComponent;

	// Visual mesh component
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(RootComponent);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Modular attribute component
	AttributeComponent = CreateDefaultSubobject<UShadowSlaveAttributeComponent>(TEXT("AttributeComponent"));
}

void AShadowSlaveCombatDummy::BeginPlay()
{
	Super::BeginPlay();

	bIsAlive = true;

	if (AttributeComponent)
	{
		AttributeComponent->OnHealthChanged.AddDynamic(this, &AShadowSlaveCombatDummy::HandleAttributeHealthChanged);
		AttributeComponent->OnDeath.AddDynamic(this, &AShadowSlaveCombatDummy::HandleDeath);
	}
}

float AShadowSlaveCombatDummy::TakeDamageCustom_Implementation(const FShadowSlaveDamageInfo& DamageInfo)
{
	if (!IsAlive_Implementation())
	{
		return 0.0f;
	}

	float ActualDamage = 0.0f;
	if (AttributeComponent)
	{
		ActualDamage = AttributeComponent->ApplyDamage(DamageInfo.DamageAmount, DamageInfo);
	}

	// Debug hit response
	if (UWorld* World = GetWorld())
	{
		const float RemainingHealth = AttributeComponent ? AttributeComponent->GetCurrentHealth() : 0.0f;
		const float MaxHealthVal = AttributeComponent ? AttributeComponent->GetMaximumHealth() : 0.0f;
		const FVector IndicatorPos = (DamageInfo.HitLocation != FVector::ZeroVector) ? DamageInfo.HitLocation : (GetActorLocation() + FVector(0.0f, 0.0f, 50.0f));
		DrawDebugString(World, IndicatorPos, FString::Printf(TEXT("-%.0f HP (%.0f/%.0f)"), ActualDamage, RemainingHealth, MaxHealthVal), nullptr, FColor::Red, 1.5f, true, 1.2f);
		DrawDebugSphere(World, IndicatorPos, 15.0f, 8, FColor::Orange, false, 1.0f);
	}

	UE_LOG(LogShadowSlave, Log, TEXT("CombatDummy '%s' took %.1f damage. Remaining HP: %.1f / %.1f"),
		*GetName(), ActualDamage,
		AttributeComponent ? AttributeComponent->GetCurrentHealth() : 0.0f,
		AttributeComponent ? AttributeComponent->GetMaximumHealth() : 0.0f);

	return ActualDamage;
}

bool AShadowSlaveCombatDummy::IsAlive_Implementation() const
{
	return AttributeComponent ? AttributeComponent->IsAlive() : bIsAlive;
}

float AShadowSlaveCombatDummy::GetHealthPercent() const
{
	return AttributeComponent ? AttributeComponent->GetHealthPercent() : 0.0f;
}

void AShadowSlaveCombatDummy::HandleAttributeHealthChanged(float NewHealth, float InMaxHealth)
{
	OnHealthChanged.Broadcast(NewHealth, InMaxHealth);
}

float AShadowSlaveCombatDummy::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser)
{
	if (!IsAlive_Implementation())
	{
		return 0.0f;
	}

	// If TakeDamageCustom was already processed for this event, we return
	return Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
}

void AShadowSlaveCombatDummy::HandleDeath()
{
	bIsAlive = false;

	// Disable collision on death
	if (CapsuleComponent)
	{
		CapsuleComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (UWorld* World = GetWorld())
	{
		DrawDebugString(World, GetActorLocation() + FVector(0.0f, 0.0f, 80.0f), TEXT("DEAD"), nullptr, FColor::Black, 3.0f, true, 1.5f);
	}

	UE_LOG(LogShadowSlave, Log, TEXT("CombatDummy '%s' has died."), *GetName());

	OnDummyDied.Broadcast();
}
