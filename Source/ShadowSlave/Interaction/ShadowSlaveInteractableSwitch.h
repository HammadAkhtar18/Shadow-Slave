// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Interaction/ShadowSlaveInteractableActor.h"
#include "ShadowSlaveInteractableSwitch.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSwitchStateChangedSignature, bool, bIsActivated, AActor*, Interactor);

/**
 * Reusable foundation for switches, levers, plates, and activation mechanisms.
 * Supports toggle switches or one-way triggers.
 */
UCLASS()
class SHADOWSLAVE_API AShadowSlaveInteractableSwitch : public AShadowSlaveInteractableActor
{
	GENERATED_BODY()

public:
	AShadowSlaveInteractableSwitch();

	virtual bool CanInteract_Implementation(AActor* Interactor) override;
	virtual FText GetInteractionPrompt_Implementation(AActor* Interactor) override;

	/** Returns whether this switch is currently activated */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Interaction|Switch")
	bool IsActivated() const { return bIsActivated; }

	/** Activates the switch */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Interaction|Switch")
	bool ActivateSwitch(AActor* Interactor);

	/** Deactivates the switch if permitted */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Interaction|Switch")
	bool DeactivateSwitch(AActor* Interactor);

	/** Toggles the switch state */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Interaction|Switch")
	bool ToggleSwitch(AActor* Interactor);

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Interaction|Switch")
	FOnSwitchStateChangedSignature OnSwitchStateChanged;

protected:
	virtual FShadowSlaveInteractionResult ExecuteInteraction(AActor* Interactor) override;

	/** Called when switch state changes; hook for Blueprint/Mechanism logic */
	UFUNCTION(BlueprintImplementableEvent, Category = "ShadowSlave|Interaction|Switch")
	void ReceiveSwitchStateChanged(bool bNewActivated, AActor* Interactor);

	/** Whether the switch is currently activated */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Interaction|Switch")
	bool bIsActivated = false;

	/** If false, the switch can only be activated once and cannot be turned off */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Interaction|Switch")
	bool bCanBeDeactivated = true;
};
