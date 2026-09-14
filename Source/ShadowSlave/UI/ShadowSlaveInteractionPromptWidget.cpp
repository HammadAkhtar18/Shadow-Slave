// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/ShadowSlaveInteractionPromptWidget.h"

UShadowSlaveInteractionPromptWidget::UShadowSlaveInteractionPromptWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bIsPromptActive = false;
	CurrentActionPrompt = FText::GetEmpty();
	CurrentKeyPrompt = FText::FromString(TEXT("[E]"));
}

void UShadowSlaveInteractionPromptWidget::SetPrompt(const FText& InActionPrompt, const FText& InKeyPrompt)
{
	CurrentActionPrompt = InActionPrompt;
	CurrentKeyPrompt = InKeyPrompt.IsEmpty() ? FText::FromString(TEXT("[E]")) : InKeyPrompt;
	bIsPromptActive = !CurrentActionPrompt.IsEmpty();

	if (bIsPromptActive)
	{
		SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		OnPromptUpdated(CurrentActionPrompt, CurrentKeyPrompt);
	}
	else
	{
		ClearPrompt();
	}
}

void UShadowSlaveInteractionPromptWidget::ClearPrompt()
{
	bIsPromptActive = false;
	CurrentActionPrompt = FText::GetEmpty();
	SetVisibility(ESlateVisibility::Collapsed);
	OnPromptCleared();
}

void UShadowSlaveInteractionPromptWidget::OnPromptUpdated_Implementation(const FText& ActionPrompt, const FText& KeyPrompt)
{
	// Base implementation hook for Blueprint UMG logic
}

void UShadowSlaveInteractionPromptWidget::OnPromptCleared_Implementation()
{
	// Base implementation hook for Blueprint UMG logic
}
