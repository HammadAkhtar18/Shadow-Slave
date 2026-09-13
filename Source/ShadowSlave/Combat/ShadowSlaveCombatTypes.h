// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ShadowSlaveCombatTypes.generated.h"

class UAnimMontage;

/**
 * Combat states representing the actor's current combat activity.
 */
UENUM(BlueprintType)
enum class ECombatState : uint8
{
	Neutral UMETA(DisplayName = "Neutral"),
	Attacking UMETA(DisplayName = "Attacking"),
	Recovering UMETA(DisplayName = "Recovering"),
	Dodging UMETA(DisplayName = "Dodging"),
	Stunned UMETA(DisplayName = "Stunned"),
	Dead UMETA(DisplayName = "Dead")
};

/**
 * Basic melee attack classifications.
 */
UENUM(BlueprintType)
enum class EAttackType : uint8
{
	Light UMETA(DisplayName = "Light Attack"),
	Heavy UMETA(DisplayName = "Heavy Attack")
};

/**
 * Payload struct containing detailed damage event information.
 */
USTRUCT(BlueprintType)
struct SHADOWSLAVE_API FShadowSlaveDamageInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Combat")
	float DamageAmount = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Combat")
	TWeakObjectPtr<AActor> Attacker = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Combat")
	TWeakObjectPtr<AActor> DamageCauser = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Combat")
	FVector HitLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Combat")
	FVector HitNormal = FVector::ZeroVector;

	FShadowSlaveDamageInfo() = default;

	FShadowSlaveDamageInfo(float InDamage, AActor* InAttacker, AActor* InCauser, const FVector& InLocation = FVector::ZeroVector, const FVector& InNormal = FVector::ZeroVector)
		: DamageAmount(InDamage), Attacker(InAttacker), DamageCauser(InCauser), HitLocation(InLocation), HitNormal(InNormal)
	{
	}
};

/**
 * Data-driven configuration for an attack action.
 */
USTRUCT(BlueprintType)
struct SHADOWSLAVE_API FShadowSlaveAttackData
{
	GENERATED_BODY()

	/** Category of attack */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Combat")
	EAttackType AttackType = EAttackType::Light;

	/** Base physical damage dealt by this attack */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Combat", meta = (ClampMin = "0.0"))
	float Damage = 25.0f;

	/** Radius of the sphere sweep used for hit detection */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Combat", meta = (ClampMin = "5.0"))
	float TraceRadius = 40.0f;

	/** Forward reach of the melee attack sweep from character origin */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Combat", meta = (ClampMin = "20.0"))
	float TraceDistance = 150.0f;

	/** Fallback duration in seconds for hit window if montage notify is not used */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Combat", meta = (ClampMin = "0.05"))
	float HitWindowDuration = 0.35f;

	/** Recovery duration in seconds after attack finishes before returning to Neutral */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Combat", meta = (ClampMin = "0.0"))
	float RecoveryDuration = 0.25f;

	/** Animation montage played for this attack */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Combat")
	TObjectPtr<UAnimMontage> AttackMontage = nullptr;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCombatStateChangedSignature, ECombatState, OldState, ECombatState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAttackExecutedSignature, EAttackType, AttackType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTargetHitSignature, AActor*, TargetActor, const FShadowSlaveDamageInfo&, DamageInfo);
