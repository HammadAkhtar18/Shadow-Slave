// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Interaction/ShadowSlaveInteractableActor.h"
#include "ShadowSlavePickupActor.generated.h"

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

	/** Returns whether this pickup has already been collected */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Interaction|Pickup")
	bool IsCollected() const { return bIsCollected; }

	/** Called when collection succeeds: deactivates or destroys the pickup actor */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Interaction|Pickup")
	virtual void OnCollected(AActor* Interactor);

protected:
	/** Whether this pickup actor should be destroyed when collected */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Interaction|Pickup")
	bool bDestroyOnPickup = true;

	/** State tracking whether the pickup has been acquired */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Interaction|Pickup")
	bool bIsCollected = false;
};
