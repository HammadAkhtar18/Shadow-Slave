// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ShadowSlaveInteractionTypes.generated.h"

class AActor;

/**
 * Lightweight result payload describing the outcome of an interaction.
 */
USTRUCT(BlueprintType)
struct SHADOWSLAVE_API FShadowSlaveInteractionResult
{
	GENERATED_BODY()

	/** Whether the interaction succeeded */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Interaction")
	bool bSuccess = false;

	/** Reason for failure if interaction could not complete */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Interaction")
	FText FailureReason;

	/** Optional technical identifier representing the interaction outcome */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Interaction")
	FName InteractionId = NAME_None;

	/** Optional key-value metadata for story, quest, or analytics hooks */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Interaction")
	TMap<FName, FString> Metadata;

	FShadowSlaveInteractionResult() = default;

	FShadowSlaveInteractionResult(bool bInSuccess, const FText& InReason = FText::GetEmpty(), FName InId = NAME_None)
		: bSuccess(bInSuccess), FailureReason(InReason), InteractionId(InId)
	{
	}

	/** Helper creating a successful interaction result */
	static FShadowSlaveInteractionResult Success(FName InId = NAME_None)
	{
		return FShadowSlaveInteractionResult(true, FText::GetEmpty(), InId);
	}

	/** Helper creating a failed interaction result */
	static FShadowSlaveInteractionResult Failure(const FText& InReason, FName InId = NAME_None)
	{
		return FShadowSlaveInteractionResult(false, InReason, InId);
	}
};

/**
 * Operational runtime state for world pickup actors.
 * Distinguishes available interactive pickups from consumed/inactive pickups,
 * preventing double-acquisition and callback re-entrancy.
 */
UENUM(BlueprintType)
enum class EShadowSlavePickupState : uint8
{
	Available UMETA(DisplayName = "Available"),
	Consumed  UMETA(DisplayName = "Consumed")
};

/* --- Interaction Event Delegates --- */

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInteractionTargetChangedSignature, AActor*, NewTarget, AActor*, OldTarget);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnInteractedSignature, AActor*, Interactor, AActor*, InteractableObject, const FShadowSlaveInteractionResult&, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPickupStateChangedSignature, EShadowSlavePickupState, NewState, AActor*, Interactor);
