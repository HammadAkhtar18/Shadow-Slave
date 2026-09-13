// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Combat/ShadowSlaveDamageableInterface.h"
#include "ShadowSlaveCombatDummy.generated.h"

class UCapsuleComponent;
class UStaticMeshComponent;
class UShadowSlaveAttributeComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDummyHealthChangedSignature, float, CurrentHealth, float, MaxHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDummyDiedSignature);

/**
 * Temporary combat test dummy for validating melee hit detection, damage application,
 * and death transitions without requiring full AI.
 */
UCLASS()
class SHADOWSLAVE_API AShadowSlaveCombatDummy : public AActor, public IShadowSlaveDamageableInterface
{
	GENERATED_BODY()

public:
	AShadowSlaveCombatDummy();

	/* --- IShadowSlaveDamageableInterface --- */
	virtual float TakeDamageCustom_Implementation(const FShadowSlaveDamageInfo& DamageInfo) override;
	virtual bool IsAlive_Implementation() const override;

	// Standard engine damage handling
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	/** Returns current health percentage (0.0 - 1.0) */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Combat|Dummy")
	float GetHealthPercent() const;

	/** Returns AttributeComponent */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Combat|Dummy")
	UShadowSlaveAttributeComponent* GetAttributeComponent() const { return AttributeComponent; }

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Combat|Dummy")
	FOnDummyHealthChangedSignature OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Combat|Dummy")
	FOnDummyDiedSignature OnDummyDied;

protected:
	virtual void BeginPlay() override;

	/** Handles death transition */
	virtual void HandleDeath();

	/** Callback when attribute component broadcasts health changes */
	UFUNCTION()
	virtual void HandleAttributeHealthChanged(float NewHealth, float InMaxHealth);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Combat|Dummy")
	TObjectPtr<UCapsuleComponent> CapsuleComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Combat|Dummy")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	/** Modular Attribute Component managing health */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Combat|Dummy")
	TObjectPtr<UShadowSlaveAttributeComponent> AttributeComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Combat|Dummy")
	bool bIsAlive = true;
};
