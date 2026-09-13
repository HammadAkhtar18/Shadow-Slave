// Copyright Epic Games, Inc. All Rights Reserved.

#include "Combat/ShadowSlaveCombatDummy.h"
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
}

void AShadowSlaveCombatDummy::BeginPlay()
{
	Super::BeginPlay();

	CurrentHealth = MaxHealth;
	bIsAlive = true;
}

float AShadowSlaveCombatDummy::TakeDamageCustom_Implementation(const FShadowSlaveDamageInfo& DamageInfo)
{
	if (!bIsAlive)
	{
		return 0.0f;
	}

	const float ActualDamage = FMath::Clamp(DamageInfo.DamageAmount, 0.0f, CurrentHealth);
	CurrentHealth -= ActualDamage;

	// Debug hit response
	if (UWorld* World = GetWorld())
	{
		const FVector IndicatorPos = (DamageInfo.HitLocation != FVector::ZeroVector) ? DamageInfo.HitLocation : (GetActorLocation() + FVector(0.0f, 0.0f, 50.0f));
		DrawDebugString(World, IndicatorPos, FString::Printf(TEXT("-%.0f HP (%.0f/%.0f)"), ActualDamage, CurrentHealth, MaxHealth), nullptr, FColor::Red, 1.5f, true, 1.2f);
		DrawDebugSphere(World, IndicatorPos, 15.0f, 8, FColor::Orange, false, 1.0f);
	}

	UE_LOG(LogShadowSlave, Log, TEXT("CombatDummy '%s' took %.1f damage. Remaining HP: %.1f / %.1f"), *GetName(), ActualDamage, CurrentHealth, MaxHealth);

	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);

	if (CurrentHealth <= 0.0f)
	{
		HandleDeath();
	}

	return ActualDamage;
}

float AShadowSlaveCombatDummy::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser)
{
	if (!bIsAlive)
	{
		return 0.0f;
	}

	// If TakeDamageCustom was already processed for this event, we return
	return Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
}

void AShadowSlaveCombatDummy::HandleDeath()
{
	bIsAlive = false;
	CurrentHealth = 0.0f;

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
