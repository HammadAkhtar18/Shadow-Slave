// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Characters/ShadowSlaveCharacterTypes.h"
#include "Combat/ShadowSlaveCombatTypes.h"
#include "ShadowSlavePlayerStatusWidget.generated.h"

/**
 * Reusable UMG widget presenting core character operational state:
 * Locomotion gait (Walk/Sprint), Combat state, and Life/Death status.
 * Purely presentation-driven without duplicating gameplay state.
 */
UCLASS(Abstract)
class SHADOWSLAVE_API UShadowSlavePlayerStatusWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UShadowSlavePlayerStatusWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** Updates the displayed locomotion gait */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|UI|Status")
	void UpdateGait(EShadowSlaveGait NewGait);

	/** Updates the displayed combat state */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|UI|Status")
	void UpdateCombatState(ECombatState NewState);

	/** Updates alive/dead status */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|UI|Status")
	void UpdateAliveStatus(bool bInIsAlive);

	UFUNCTION(BlueprintPure, Category = "ShadowSlave|UI|Status")
	EShadowSlaveGait GetCurrentGait() const { return CurrentGait; }

	UFUNCTION(BlueprintPure, Category = "ShadowSlave|UI|Status")
	ECombatState GetCurrentCombatState() const { return CurrentCombatState; }

	UFUNCTION(BlueprintPure, Category = "ShadowSlave|UI|Status")
	bool IsAlive() const { return bIsAlive; }

protected:
	/** Hook for Blueprint UMG implementations when gait changes */
	UFUNCTION(BlueprintNativeEvent, Category = "ShadowSlave|UI|Status")
	void OnGaitUpdated(EShadowSlaveGait NewGait);
	virtual void OnGaitUpdated_Implementation(EShadowSlaveGait NewGait);

	/** Hook for Blueprint UMG implementations when combat state changes */
	UFUNCTION(BlueprintNativeEvent, Category = "ShadowSlave|UI|Status")
	void OnCombatStateUpdated(ECombatState NewState);
	virtual void OnCombatStateUpdated_Implementation(ECombatState NewState);

	/** Hook for Blueprint UMG implementations when life status changes */
	UFUNCTION(BlueprintNativeEvent, Category = "ShadowSlave|UI|Status")
	void OnAliveStatusUpdated(bool bInIsAlive);
	virtual void OnAliveStatusUpdated_Implementation(bool bInIsAlive);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|UI|Status")
	EShadowSlaveGait CurrentGait = EShadowSlaveGait::Walk;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|UI|Status")
	ECombatState CurrentCombatState = ECombatState::Neutral;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|UI|Status")
	bool bIsAlive = true;
};
