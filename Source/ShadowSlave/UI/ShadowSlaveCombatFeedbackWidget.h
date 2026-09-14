// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Combat/ShadowSlaveCombatTypes.h"
#include "ShadowSlaveCombatFeedbackWidget.generated.h"

class AActor;

/**
 * Reusable UMG widget presenting combat feedback:
 * Damage taken indicators, damage dealt/hit confirmation, and combat state transitions.
 * Purely presentation-driven: responds directly to combat/damage delegates.
 */
UCLASS(Abstract)
class SHADOWSLAVE_API UShadowSlaveCombatFeedbackWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UShadowSlaveCombatFeedbackWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** Called when player deals damage to a target actor (hit confirmation) */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|UI|Combat")
	void NotifyDamageDealt(AActor* Target, float Amount, bool bIsFatal);

	/** Called when player receives damage (vignette/directional indicator hook) */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|UI|Combat")
	void NotifyDamageTaken(float Amount, const FVector& HitDirection);

	/** Called when combat state transitions (Neutral, LightAttack, HeavyAttack, HitStun, etc.) */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|UI|Combat")
	void NotifyCombatStateChanged(ECombatState NewState, ECombatState OldState);

	/** Called when character dies */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|UI|Combat")
	void NotifyCharacterDied();

protected:
	/** Hook for Blueprint UMG implementations to animate crosshairs, hit markers, or sound cues */
	UFUNCTION(BlueprintNativeEvent, Category = "ShadowSlave|UI|Combat")
	void ReceiveDamageDealt(AActor* Target, float Amount, bool bIsFatal);
	virtual void ReceiveDamageDealt_Implementation(AActor* Target, float Amount, bool bIsFatal);

	/** Hook for Blueprint UMG implementations to trigger red screen edge / damage vignette */
	UFUNCTION(BlueprintNativeEvent, Category = "ShadowSlave|UI|Combat")
	void ReceiveDamageTaken(float Amount, const FVector& HitDirection);
	virtual void ReceiveDamageTaken_Implementation(float Amount, const FVector& HitDirection);

	/** Hook for Blueprint UMG implementations to update combat stance/state visuals */
	UFUNCTION(BlueprintNativeEvent, Category = "ShadowSlave|UI|Combat")
	void ReceiveCombatStateChanged(ECombatState NewState, ECombatState OldState);
	virtual void ReceiveCombatStateChanged_Implementation(ECombatState NewState, ECombatState OldState);

	/** Hook for Blueprint UMG implementations to trigger death screen/overlay */
	UFUNCTION(BlueprintNativeEvent, Category = "ShadowSlave|UI|Combat")
	void ReceiveCharacterDied();
	virtual void ReceiveCharacterDied_Implementation();
};
