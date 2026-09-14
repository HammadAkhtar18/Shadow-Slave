// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/ShadowSlaveResourceBarWidget.h"

UShadowSlaveResourceBarWidget::UShadowSlaveResourceBarWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ResourceBarType = EShadowSlaveResourceBarType::Health;
	BarLabel = FText::FromString(TEXT("Health"));
	BarColor = FLinearColor(0.85f, 0.15f, 0.15f, 1.0f); // Default red
	CurrentValue = 100.0f;
	MaxValue = 100.0f;
	Percent = 1.0f;
}

void UShadowSlaveResourceBarWidget::InitializeResourceBar(EShadowSlaveResourceBarType InType, const FText& InLabel, const FLinearColor& InBarColor)
{
	ResourceBarType = InType;
	BarLabel = InLabel;
	BarColor = InBarColor;

	OnBarColorChanged(BarColor);
	OnResourceUpdated(CurrentValue, MaxValue, Percent);
}

void UShadowSlaveResourceBarWidget::UpdateValues(float NewCurrentValue, float NewMaxValue, float NewPercent)
{
	CurrentValue = FMath::Max(0.0f, NewCurrentValue);
	MaxValue = FMath::Max(0.0f, NewMaxValue);
	Percent = FMath::Clamp(NewPercent, 0.0f, 1.0f);

	OnResourceUpdated(CurrentValue, MaxValue, Percent);
}

void UShadowSlaveResourceBarWidget::SetBarLabel(const FText& InLabel)
{
	BarLabel = InLabel;
}

void UShadowSlaveResourceBarWidget::SetBarColor(const FLinearColor& InColor)
{
	BarColor = InColor;
	OnBarColorChanged(BarColor);
}

void UShadowSlaveResourceBarWidget::OnResourceUpdated_Implementation(float InCurrent, float InMax, float InPercent)
{
	// Base C++ implementation: hook for Blueprint UMG bindings
}

void UShadowSlaveResourceBarWidget::OnBarColorChanged_Implementation(const FLinearColor& NewColor)
{
	// Base C++ implementation: hook for Blueprint UMG bindings
}
