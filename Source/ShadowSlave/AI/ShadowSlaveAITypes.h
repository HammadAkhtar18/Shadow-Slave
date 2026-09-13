// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ShadowSlaveAITypes.generated.h"

/**
 * AI State model for Nightmare Creature AI.
 * Follows the state flow:
 * Idle -> Investigating -> Chasing -> Attacking -> Recovering -> Returning/Searching -> Idle
 * Overridden unconditionally by Dead.
 */
UENUM(BlueprintType)
enum class EShadowSlaveAIState : uint8
{
	Idle          UMETA(DisplayName = "Idle"),
	Investigating UMETA(DisplayName = "Investigating"),
	Chasing       UMETA(DisplayName = "Chasing"),
	Attacking     UMETA(DisplayName = "Attacking"),
	Recovering    UMETA(DisplayName = "Recovering"),
	Searching     UMETA(DisplayName = "Searching"),
	Returning     UMETA(DisplayName = "Returning"),
	Dead          UMETA(DisplayName = "Dead")
};

/**
 * Helper to convert AI state enum to string for debugging.
 */
inline const TCHAR* LexToString(EShadowSlaveAIState State)
{
	switch (State)
	{
	case EShadowSlaveAIState::Idle:          return TEXT("Idle");
	case EShadowSlaveAIState::Investigating: return TEXT("Investigating");
	case EShadowSlaveAIState::Chasing:       return TEXT("Chasing");
	case EShadowSlaveAIState::Attacking:     return TEXT("Attacking");
	case EShadowSlaveAIState::Recovering:    return TEXT("Recovering");
	case EShadowSlaveAIState::Searching:     return TEXT("Searching");
	case EShadowSlaveAIState::Returning:     return TEXT("Returning");
	case EShadowSlaveAIState::Dead:          return TEXT("Dead");
	default:                                 return TEXT("Unknown");
	}
}

/**
 * Tunable parameters for AI perception, combat ranges, and state timeouts.
 */
USTRUCT(BlueprintType)
struct SHADOWSLAVE_API FShadowSlaveAIConfig
{
	GENERATED_BODY()

	/** Sight detection radius */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|AI|Perception", meta = (ClampMin = "100.0"))
	float SightRadius = 1500.0f;

	/** Lose sight detection radius */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|AI|Perception", meta = (ClampMin = "100.0"))
	float LoseSightRadius = 2000.0f;

	/** Half angle in degrees for sight cone (e.g. 70 deg gives 140 deg field of view) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|AI|Perception", meta = (ClampMin = "10.0", ClampMax = "180.0"))
	float PeripheralVisionAngleDegrees = 70.0f;

	/** Maximum age of sight stimulus in seconds */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|AI|Perception", meta = (ClampMin = "0.5"))
	float SightMaxAge = 5.0f;

	/** Attack range threshold in cm */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|AI|Combat", meta = (ClampMin = "50.0"))
	float AttackRange = 160.0f;

	/** Navigation acceptance radius when closing in on target */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|AI|Combat", meta = (ClampMin = "20.0"))
	float AttackAcceptanceRadius = 120.0f;

	/** Post-attack recovery cooldown in seconds before next action */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|AI|Combat", meta = (ClampMin = "0.1"))
	float AttackRecoveryDuration = 0.8f;

	/** Duration in seconds to investigate a stimulus location before returning */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|AI|Navigation", meta = (ClampMin = "0.5"))
	float InvestigationDuration = 3.0f;

	/** Duration in seconds to search at target's last known location before returning */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|AI|Navigation", meta = (ClampMin = "0.5"))
	float SearchDuration = 2.5f;

	/** Interval in seconds for checking distance and target state during chase */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|AI|Navigation", meta = (ClampMin = "0.05"))
	float ChaseUpdateInterval = 0.15f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAIStateChangedSignature, EShadowSlaveAIState, OldState, EShadowSlaveAIState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAITargetChangedSignature, AActor*, OldTarget, AActor*, NewTarget);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEnemyHitReactionSignature, const FVector&, HitDirection);
