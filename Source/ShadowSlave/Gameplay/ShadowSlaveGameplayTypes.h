// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ShadowSlaveGameplayTypes.generated.h"

class APawn;

/**
 * High-level technical gameplay modes managed by the orchestration layer.
 * Strictly technical classification; not canon novel terminology.
 */
UENUM(BlueprintType)
enum class EShadowSlaveGameplayFlowState : uint8
{
	Unknown       UMETA(DisplayName = "Unknown"),
	None          UMETA(DisplayName = "None"),
	Exploration   UMETA(DisplayName = "Exploration"),
	Dialogue      UMETA(DisplayName = "Dialogue"),
	Combat        UMETA(DisplayName = "Combat"),
	Nightmare     UMETA(DisplayName = "Nightmare"),
	Transitioning UMETA(DisplayName = "Transitioning"),
	Paused        UMETA(DisplayName = "Paused")
};

/**
 * Generic transition request descriptor for future level, story, or map transitions.
 * Pure data boundary; does not invoke level streaming, OpenLevel, or teleportation.
 */
USTRUCT(BlueprintType)
struct SHADOWSLAVE_API FShadowSlaveGameplayTransitionRequest
{
	GENERATED_BODY()

	/** Associated story definition identifier (if triggered by a story beat) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Gameplay")
	FName StoryId = NAME_None;

	/** Optional associated story step identifier */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Gameplay")
	FName StoryStepId = NAME_None;

	/** Technical identifier of the destination level, world, or zone */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Gameplay")
	FName TargetWorldId = NAME_None;

	/** Optional spawn point or arrival anchor tag */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Gameplay")
	FName TargetSpawnPointId = NAME_None;

	/** Technical transition reason (e.g. "StoryProgression", "ScenarioExit", "Portal", "Death") */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Gameplay")
	FName Reason = NAME_None;

	/** Extensible metadata */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Gameplay")
	TMap<FName, FString> TransitionMetadata;

	bool IsValid() const
	{
		return !StoryId.IsNone() || !TargetWorldId.IsNone() || !Reason.IsNone();
	}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnShadowSlaveGameplayFlowStateChangedSignature,
	EShadowSlaveGameplayFlowState, NewState,
	EShadowSlaveGameplayFlowState, OldState
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnShadowSlaveGameplayTransitionRequestedSignature,
	const FShadowSlaveGameplayTransitionRequest&, Request
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnShadowSlavePlayerContextUpdatedSignature,
	APawn*, PlayerPawn
);
