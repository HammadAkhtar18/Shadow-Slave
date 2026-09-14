// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/ShadowSlavePlayerStatusWidget.h"

UShadowSlavePlayerStatusWidget::UShadowSlavePlayerStatusWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	CurrentGait = EShadowSlaveGait::Walk;
	CurrentCombatState = ECombatState::Neutral;
	bIsAlive = true;
}

void UShadowSlavePlayerStatusWidget::UpdateGait(EShadowSlaveGait NewGait)
{
	CurrentGait = NewGait;
	OnGaitUpdated(CurrentGait);
}

void UShadowSlavePlayerStatusWidget::UpdateCombatState(ECombatState NewState)
{
	CurrentCombatState = NewState;
	OnCombatStateUpdated(CurrentCombatState);
}

void UShadowSlavePlayerStatusWidget::UpdateAliveStatus(bool bInIsAlive)
{
	bIsAlive = bInIsAlive;
	OnAliveStatusUpdated(bIsAlive);
}

void UShadowSlavePlayerStatusWidget::OnGaitUpdated_Implementation(EShadowSlaveGait NewGait)
{
	// Hook for Blueprint UMG logic
}

void UShadowSlavePlayerStatusWidget::OnCombatStateUpdated_Implementation(ECombatState NewState)
{
	// Hook for Blueprint UMG logic
}

void UShadowSlavePlayerStatusWidget::OnAliveStatusUpdated_Implementation(bool bInIsAlive)
{
	// Hook for Blueprint UMG logic
}
