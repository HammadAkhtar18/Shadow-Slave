// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Interaction/ShadowSlaveInteractionTypes.h"
#include "ShadowSlaveInteractionComponent.generated.h"

/**
 * Reusable Actor Component responsible for detecting interactable world objects,
 * managing current interaction candidate state, and executing interactions.
 * Performs lightweight, configurable forward traces without per-frame heavy scans.
 * Usable by player characters, companion NPCs, and AI agents.
 */
UCLASS(ClassGroup = (ShadowSlave), meta = (BlueprintSpawnableComponent))
class SHADOWSLAVE_API UShadowSlaveInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UShadowSlaveInteractionComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/* --- Detection API --- */

	/** Manually triggers an interactable detection sweep */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Interaction")
	void DetectInteractable();

	/** Enables or disables periodic interactable detection */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Interaction")
	void SetInteractionDetectionEnabled(bool bEnabled);

	/** Returns whether interaction detection is currently enabled */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Interaction")
	bool IsInteractionDetectionEnabled() const { return bEnableDetection; }

	/* --- Current Target Queries --- */

	/** Returns current candidate interactable actor, or nullptr if none in range */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Interaction")
	AActor* GetCurrentInteractable() const;

	/** Returns true if a valid interactable target is currently selected */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Interaction")
	bool HasInteractableTarget() const;

	/** Returns the interaction prompt text for the current candidate (e.g. "Open", "Talk", "Pick Up") */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Interaction")
	FText GetCurrentInteractionPrompt() const;

	/* --- Execution API --- */

	/**
	 * Attempts to interact with the current candidate interactable.
	 * Executes CanInteract validation, calls Interact on the target,
	 * and broadcasts OnInteracted. Returns true if interaction succeeded.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Interaction")
	bool TryInteract();

	/** Clears current interactable target and broadcasts change event if a target was set */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Interaction")
	void ClearCurrentInteractable();

	/* --- Configuration Parameters --- */

	/** Maximum forward trace distance in centimeters (configurable prototype tuning, not hardcoded canon) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Interaction|Detection", meta = (ClampMin = "50.0"))
	float InteractionDistance = 250.0f;

	/** Sphere sweep radius in centimeters for forgiving interaction cone detection */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Interaction|Detection", meta = (ClampMin = "0.0"))
	float InteractionRadius = 25.0f;

	/** Collision channel used for interaction traces */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Interaction|Detection")
	TEnumAsByte<ECollisionChannel> TraceCollisionChannel = ECC_Visibility;

	/** Whether line-of-sight is strictly required to target an interactable */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Interaction|Detection")
	bool bRequireLineOfSight = true;

	/**
	 * Interval in seconds between periodic detection traces.
	 * Default 0.1s (10 Hz) avoids expensive per-frame raycasting while maintaining responsiveness.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Interaction|Detection", meta = (ClampMin = "0.0"))
	float TraceInterval = 0.1f;

	/* --- Delegates --- */

	/** Broadcasts whenever candidate interactable changes (only on genuine target transitions) */
	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Interaction|Events")
	FOnInteractionTargetChangedSignature OnInteractionTargetChanged;

	/** Broadcasts when an interaction is executed */
	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Interaction|Events")
	FOnInteractedSignature OnInteracted;

protected:
	virtual void BeginPlay() override;

	/** Whether periodic detection is enabled */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Interaction|State")
	bool bEnableDetection = true;

	/** Current interactable candidate actor (weak reference for safety) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Interaction|State")
	TWeakObjectPtr<AActor> CurrentInteractableActor = nullptr;

private:
	/** Time accumulator for periodic trace intervals */
	float TraceTimeAccumulator = 0.0f;
};
