// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ShadowSlaveInteractionPromptWidget.generated.h"

/**
 * Reusable UMG widget displaying contextual world interaction prompts (e.g. "[E] Open Door", "[E] Talk").
 * Subscribes to UShadowSlaveInteractionComponent target changes and updates presentation.
 */
UCLASS(Abstract)
class SHADOWSLAVE_API UShadowSlaveInteractionPromptWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UShadowSlaveInteractionPromptWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** Displays an active prompt with action description and input key display */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|UI|Interaction")
	void SetPrompt(const FText& InActionPrompt, const FText& InKeyPrompt);

	/** Clears and hides the active prompt */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|UI|Interaction")
	void ClearPrompt();

	UFUNCTION(BlueprintPure, Category = "ShadowSlave|UI|Interaction")
	bool IsPromptActive() const { return bIsPromptActive; }

	UFUNCTION(BlueprintPure, Category = "ShadowSlave|UI|Interaction")
	FText GetActionPrompt() const { return CurrentActionPrompt; }

	UFUNCTION(BlueprintPure, Category = "ShadowSlave|UI|Interaction")
	FText GetKeyPrompt() const { return CurrentKeyPrompt; }

protected:
	/** Hook for Blueprint UMG implementations to animate or show prompt elements */
	UFUNCTION(BlueprintNativeEvent, Category = "ShadowSlave|UI|Interaction")
	void OnPromptUpdated(const FText& ActionPrompt, const FText& KeyPrompt);
	virtual void OnPromptUpdated_Implementation(const FText& ActionPrompt, const FText& KeyPrompt);

	/** Hook for Blueprint UMG implementations to hide prompt elements */
	UFUNCTION(BlueprintNativeEvent, Category = "ShadowSlave|UI|Interaction")
	void OnPromptCleared();
	virtual void OnPromptCleared_Implementation();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|UI|Interaction")
	bool bIsPromptActive = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|UI|Interaction")
	FText CurrentActionPrompt;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|UI|Interaction")
	FText CurrentKeyPrompt;
};
