// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Interaction/ShadowSlaveInteractableActor.h"
#include "ShadowSlavePickupActor.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPickupCollectedSignature, AShadowSlavePickupActor*, PickupActor, AActor*, Interactor);

/**
 * Reusable base actor for physical world pickups (Items, Memories, Collectibles).
 * Manages collected lifecycle state and safe actor cleanup upon successful acquisition.
 */
UCLASS(Abstract)
class SHADOWSLAVE_API AShadowSlavePickupActor : public AShadowSlaveInteractableActor
{
	GENERATED_BODY()

public:
	AShadowSlavePickupActor();

	virtual bool CanInteract_Implementation(AActor* Interactor) override;

	/** Returns current operational state of the pickup */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Interaction|Pickup")
	EShadowSlavePickupState GetPickupState() const { return PickupState; }

	/** Returns whether this pickup is currently available for collection */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Interaction|Pickup")
	bool IsAvailable() const { return PickupState == EShadowSlavePickupState::Available && !bIsCollected; }

	/** Returns whether this pickup has already been collected */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Interaction|Pickup")
	bool IsCollected() const { return bIsCollected || PickupState == EShadowSlavePickupState::Consumed; }

	/** Called when collection succeeds: deactivates or destroys the pickup actor */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Interaction|Pickup")
	virtual void OnCollected(AActor* Interactor);

	/* --- IShadowSlaveSaveableInterface Overrides --- */
	virtual bool CaptureSaveRecord_Implementation(FShadowSlaveWorldActorSaveRecord& OutRecord) override;
	virtual bool RestoreSaveRecord_Implementation(const FShadowSlaveWorldActorSaveRecord& InRecord) override;

	/* --- Event Delegates --- */

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Interaction|Pickup|Events")
	FOnPickupStateChangedSignature OnPickupStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Interaction|Pickup|Events")
	FOnPickupCollectedSignature OnPickupCollected;

protected:
	/** Operational runtime state of the pickup */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Interaction|Pickup")
	EShadowSlavePickupState PickupState = EShadowSlavePickupState::Available;

	/** Whether this pickup actor should be destroyed when collected */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Interaction|Pickup")
	bool bDestroyOnPickup = true;

	/** State tracking whether the pickup has been acquired */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Interaction|Pickup")
	bool bIsCollected = false;

	/** Re-entrancy guard to prevent multiple concurrent acquisition attempts in the same frame/event chain */
	UPROPERTY(Transient)
	bool bIsProcessingAcquisition = false;
};
