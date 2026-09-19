// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "ShadowSlaveStatusEffectTypes.generated.h"

class UShadowSlaveStatusEffectDefinition;
class AActor;

/**
 * Duration policy defining how a status effect expires or persists over time.
 */
UENUM(BlueprintType)
enum class EStatusEffectDurationPolicy : uint8
{
	Instant    UMETA(DisplayName = "Instant"),    // One-off application, not retained over time
	Timed      UMETA(DisplayName = "Timed"),      // Finite duration in seconds; expires via timer
	Persistent UMETA(DisplayName = "Persistent")  // Remains active until explicitly removed by gameplay logic
};

/**
 * Stacking policy defining behavior when an effect with the same definition is reapplied.
 */
UENUM(BlueprintType)
enum class EStatusEffectStackingPolicy : uint8
{
	IgnoreNew       UMETA(DisplayName = "Ignore New"),       // Keep existing instance; discard new application
	RefreshDuration UMETA(DisplayName = "Refresh Duration"), // Reset remaining duration to full without changing stacks
	AddStacks       UMETA(DisplayName = "Add Stacks"),       // Increment stack count up to MaxStacks and refresh duration
	Replace         UMETA(DisplayName = "Replace")           // Remove older instance and apply new instance
};

/**
 * Generic polarity classification for status effects (informational and filtering metadata).
 */
UENUM(BlueprintType)
enum class EStatusEffectPolarity : uint8
{
	Neutral    UMETA(DisplayName = "Neutral"),
	Beneficial UMETA(DisplayName = "Beneficial"),
	Harmful    UMETA(DisplayName = "Harmful")
};

/**
 * Attribution source structure identifying what caused or applied this status effect.
 * Loose coupling: uses GUID, Name tag, and weak actor pointer without hard system dependencies.
 */
USTRUCT(BlueprintType)
struct SHADOWSLAVE_API FShadowSlaveStatusEffectSource
{
	GENERATED_BODY()

	/** Unique stable identifier for struct- or instance-based sources (e.g. Memory, Echo, Item GUID) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|StatusEffects")
	FGuid SourceId;

	/** Technical name of the source (e.g. AbilityId, ItemId, or encounter tag) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|StatusEffects")
	FName SourceName = NAME_None;

	/** Optional weak pointer to the instigator actor */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|StatusEffects")
	TWeakObjectPtr<AActor> SourceActor = nullptr;

	FShadowSlaveStatusEffectSource() = default;

	FShadowSlaveStatusEffectSource(const FGuid& InId, FName InName, AActor* InActor = nullptr)
		: SourceId(InId), SourceName(InName), SourceActor(InActor)
	{
	}
};

/**
 * Runtime instance of an active status effect on a character or target.
 * Belongs to the component managing it; holds mutable runtime state.
 * Static data (Name, Description, Policies, etc.) is NOT duplicated here.
 */
USTRUCT(BlueprintType)
struct SHADOWSLAVE_API FShadowSlaveStatusEffectInstance
{
	GENERATED_BODY()

	/** Unique instance GUID distinguishing this specific applied effect */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|StatusEffects")
	FGuid InstanceId = FGuid::NewGuid();

	/** Static archetype definition describing this effect */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|StatusEffects")
	TObjectPtr<UShadowSlaveStatusEffectDefinition> EffectDefinition = nullptr;

	/** Current stack count */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|StatusEffects", meta = (ClampMin = "1"))
	int32 CurrentStacks = 1;

	/** Total duration configured for this instance at application or refresh */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|StatusEffects", meta = (ClampMin = "0.0"))
	float TotalDuration = 0.0f;

	/** Attribution source information */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|StatusEffects")
	FShadowSlaveStatusEffectSource Source;

	/** Extensible dynamic properties for runtime modification */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|StatusEffects")
	TMap<FName, FString> DynamicProperties;

	/** Transient timer handle for timed expiration (never serialized) */
	FTimerHandle ExpirationTimerHandle;

	/** World time (in seconds) when this effect was applied or refreshed (for remaining duration queries) */
	double ApplicationWorldTime = 0.0;

	FShadowSlaveStatusEffectInstance() = default;

	FShadowSlaveStatusEffectInstance(UShadowSlaveStatusEffectDefinition* InDef, const FShadowSlaveStatusEffectSource& InSource = FShadowSlaveStatusEffectSource())
		: InstanceId(FGuid::NewGuid())
		, EffectDefinition(InDef)
		, CurrentStacks(1)
		, TotalDuration(0.0f)
		, Source(InSource)
		, ApplicationWorldTime(0.0)
	{
	}

	bool IsValid() const
	{
		return EffectDefinition != nullptr && InstanceId.IsValid();
	}
};

/* --- Status Effect Delegates --- */

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStatusEffectAppliedSignature, const FShadowSlaveStatusEffectInstance&, EffectInstance);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStatusEffectRemovedSignature, const FShadowSlaveStatusEffectInstance&, EffectInstance);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStatusEffectExpiredSignature, const FShadowSlaveStatusEffectInstance&, EffectInstance);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStatusEffectStackChangedSignature, const FShadowSlaveStatusEffectInstance&, EffectInstance, int32, OldStackCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStatusEffectCollectionChangedSignature);
