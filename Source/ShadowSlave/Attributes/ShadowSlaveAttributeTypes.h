// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Combat/ShadowSlaveCombatTypes.h"
#include "ShadowSlaveAttributeTypes.generated.h"

/**
 * Supported character attributes / resources.
 */
UENUM(BlueprintType)
enum class EAttributeType : uint8
{
	Health     UMETA(DisplayName = "Health"),
	MaxHealth  UMETA(DisplayName = "Maximum Health"),
	Stamina    UMETA(DisplayName = "Stamina"),
	MaxStamina UMETA(DisplayName = "Maximum Stamina"),
	Essence    UMETA(DisplayName = "Essence"),
	MaxEssence UMETA(DisplayName = "Maximum Essence")
};

/**
 * Modifier calculation types for extensible buff/debuff/equipment systems.
 */
UENUM(BlueprintType)
enum class EAttributeModifierType : uint8
{
	Flat    UMETA(DisplayName = "Flat Additive (+/-)"),
	Percent UMETA(DisplayName = "Percentage Multiplier (+/- %)")
};

/**
 * Lightweight, extensible modifier struct for temporary buffs, debuffs, or equipment bonuses.
 */
USTRUCT(BlueprintType)
struct SHADOWSLAVE_API FAttributeModifier
{
	GENERATED_BODY()

	/** Unique identifier for this modifier instance */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Attributes")
	FName ModifierId = NAME_None;

	/** Which attribute this modifier affects */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Attributes")
	EAttributeType TargetAttribute = EAttributeType::MaxHealth;

	/** Whether this modifier adds flat points or percentage multiplier */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Attributes")
	EAttributeModifierType ModifierType = EAttributeModifierType::Flat;

	/** Numeric value of the modification */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Attributes")
	float Value = 0.0f;

	/** Duration in seconds. 0.0 indicates indefinite until explicitly removed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Attributes", meta = (ClampMin = "0.0"))
	float Duration = 0.0f;

	/** Optional source object (equipment item, effect causer, ability) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Attributes")
	TWeakObjectPtr<UObject> Source = nullptr;

	FAttributeModifier() = default;

	FAttributeModifier(FName InId, EAttributeType InTarget, EAttributeModifierType InType, float InValue, float InDuration = 0.0f, UObject* InSource = nullptr)
		: ModifierId(InId), TargetAttribute(InTarget), ModifierType(InType), Value(InValue), Duration(InDuration), Source(InSource)
	{
	}
};

/**
 * Tunable baseline configuration for character attributes.
 */
USTRUCT(BlueprintType)
struct SHADOWSLAVE_API FAttributeInitConfig
{
	GENERATED_BODY()

	/** Base maximum health points */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Attributes", meta = (ClampMin = "1.0"))
	float BaseMaxHealth = 100.0f;

	/** Base maximum stamina points */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Attributes", meta = (ClampMin = "0.0"))
	float BaseMaxStamina = 100.0f;

	/** Base maximum soul essence points */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Attributes", meta = (ClampMin = "0.0"))
	float BaseMaxEssence = 100.0f;

	/** Whether stamina automatically regenerates when depleted */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Attributes|Stamina")
	bool bEnableStaminaRegen = true;

	/** Stamina points restored per second during regeneration */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Attributes|Stamina", meta = (ClampMin = "0.0", EditCondition = "bEnableStaminaRegen"))
	float StaminaRegenRate = 20.0f;

	/** Delay in seconds after consuming stamina before regeneration begins */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Attributes|Stamina", meta = (ClampMin = "0.0", EditCondition = "bEnableStaminaRegen"))
	float StaminaRegenDelay = 1.0f;

	/** Interval in seconds between regeneration timer ticks */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Attributes|Stamina", meta = (ClampMin = "0.02", EditCondition = "bEnableStaminaRegen"))
	float StaminaRegenTickInterval = 0.1f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAttributeChangedSignature, float, CurrentValue, float, MaxValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDamageReceivedSignature, float, DamageApplied, const FShadowSlaveDamageInfo&, DamageInfo);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHealReceivedSignature, float, HealAmount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDeathSignature);
