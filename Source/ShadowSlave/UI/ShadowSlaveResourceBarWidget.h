// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/ShadowSlaveUITypes.h"
#include "ShadowSlaveResourceBarWidget.generated.h"

/**
 * Reusable UMG widget for displaying resource pools (Health, Stamina, Essence).
 * Purely presentation-driven: receives value updates from owning HUD without maintaining authoritative state.
 * Designed without per-frame Tick polling: updates strictly on delegate broadcasts.
 */
UCLASS(Abstract)
class SHADOWSLAVE_API UShadowSlaveResourceBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UShadowSlaveResourceBarWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** Initializes resource bar configuration */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|UI|Resource")
	void InitializeResourceBar(EShadowSlaveResourceBarType InType, const FText& InLabel, const FLinearColor& InBarColor);

	/** Updates the current, maximum, and computed percentage values */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|UI|Resource")
	void UpdateValues(float NewCurrentValue, float NewMaxValue, float NewPercent);

	/** Sets bar label text */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|UI|Resource")
	void SetBarLabel(const FText& InLabel);

	/** Sets bar accent/fill color */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|UI|Resource")
	void SetBarColor(const FLinearColor& InColor);

	UFUNCTION(BlueprintPure, Category = "ShadowSlave|UI|Resource")
	EShadowSlaveResourceBarType GetResourceBarType() const { return ResourceBarType; }

	UFUNCTION(BlueprintPure, Category = "ShadowSlave|UI|Resource")
	float GetCurrentValue() const { return CurrentValue; }

	UFUNCTION(BlueprintPure, Category = "ShadowSlave|UI|Resource")
	float GetMaxValue() const { return MaxValue; }

	UFUNCTION(BlueprintPure, Category = "ShadowSlave|UI|Resource")
	float GetPercent() const { return Percent; }

	UFUNCTION(BlueprintPure, Category = "ShadowSlave|UI|Resource")
	FText GetBarLabel() const { return BarLabel; }

	UFUNCTION(BlueprintPure, Category = "ShadowSlave|UI|Resource")
	FLinearColor GetBarColor() const { return BarColor; }

protected:
	/** Hook for Blueprint UMG implementations to animate or update visual progress bars / text */
	UFUNCTION(BlueprintNativeEvent, Category = "ShadowSlave|UI|Resource")
	void OnResourceUpdated(float InCurrent, float InMax, float InPercent);
	virtual void OnResourceUpdated_Implementation(float InCurrent, float InMax, float InPercent);

	/** Hook for Blueprint UMG implementations when accent color changes */
	UFUNCTION(BlueprintNativeEvent, Category = "ShadowSlave|UI|Resource")
	void OnBarColorChanged(const FLinearColor& NewColor);
	virtual void OnBarColorChanged_Implementation(const FLinearColor& NewColor);

	/** Classification of this resource bar */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|UI|Resource")
	EShadowSlaveResourceBarType ResourceBarType = EShadowSlaveResourceBarType::Health;

	/** Display label (e.g. "Health", "Stamina", "Essence") */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|UI|Resource")
	FText BarLabel;

	/** Visual accent color */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|UI|Resource")
	FLinearColor BarColor = FLinearColor::Red;

	/** Cached presentation values (never authoritative) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|UI|Resource")
	float CurrentValue = 100.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|UI|Resource")
	float MaxValue = 100.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|UI|Resource")
	float Percent = 1.0f;
};
