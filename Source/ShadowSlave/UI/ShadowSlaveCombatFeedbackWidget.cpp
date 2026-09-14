// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/ShadowSlaveCombatFeedbackWidget.h"
#include "GameFramework/Actor.h"

UShadowSlaveCombatFeedbackWidget::UShadowSlaveCombatFeedbackWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void UShadowSlaveCombatFeedbackWidget::NotifyDamageDealt(AActor* Target, float Amount, bool bIsFatal)
{
	ReceiveDamageDealt(Target, Amount, bIsFatal);
}

void UShadowSlaveCombatFeedbackWidget::NotifyDamageTaken(float Amount, const FVector& HitDirection)
{
	ReceiveDamageTaken(Amount, HitDirection);
}

void UShadowSlaveCombatFeedbackWidget::NotifyCombatStateChanged(ECombatState NewState, ECombatState OldState)
{
	ReceiveCombatStateChanged(NewState, OldState);
}

void UShadowSlaveCombatFeedbackWidget::NotifyCharacterDied()
{
	ReceiveCharacterDied();
}

void UShadowSlaveCombatFeedbackWidget::ReceiveDamageDealt_Implementation(AActor* Target, float Amount, bool bIsFatal)
{
	// Hook for Blueprint UMG logic
}

void UShadowSlaveCombatFeedbackWidget::ReceiveDamageTaken_Implementation(float Amount, const FVector& HitDirection)
{
	// Hook for Blueprint UMG logic
}

void UShadowSlaveCombatFeedbackWidget::ReceiveCombatStateChanged_Implementation(ECombatState NewState, ECombatState OldState)
{
	// Hook for Blueprint UMG logic
}

void UShadowSlaveCombatFeedbackWidget::ReceiveCharacterDied_Implementation()
{
	// Hook for Blueprint UMG logic
}
