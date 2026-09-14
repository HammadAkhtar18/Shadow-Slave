// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/ShadowSlaveUIManager.h"
#include "UI/ShadowSlaveHUDWidget.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Blueprint/UserWidget.h"
#include "ShadowSlave.h"

UShadowSlaveUIManager::UShadowSlaveUIManager()
{
	DefaultHUDWidgetClass = UShadowSlaveHUDWidget::StaticClass();
}

UShadowSlaveHUDWidget* UShadowSlaveUIManager::CreateRootHUD(APlayerController* PlayerController, TSubclassOf<UShadowSlaveHUDWidget> CustomWidgetClass)
{
	if (!PlayerController || !PlayerController->IsLocalController())
	{
		return nullptr;
	}

	// If a HUD widget already exists for this controller, return it
	if (UShadowSlaveHUDWidget* ExistingHUD = GetRootHUD(PlayerController))
	{
		return ExistingHUD;
	}

	TSubclassOf<UShadowSlaveHUDWidget> ClassToUse = CustomWidgetClass ? CustomWidgetClass : DefaultHUDWidgetClass;
	if (!ClassToUse)
	{
		ClassToUse = UShadowSlaveHUDWidget::StaticClass();
	}

	UShadowSlaveHUDWidget* NewHUD = CreateWidget<UShadowSlaveHUDWidget>(PlayerController, ClassToUse);
	if (!NewHUD)
	{
		UE_LOG(LogShadowSlave, Error, TEXT("UShadowSlaveUIManager: Failed to create root HUD widget for '%s'."), *PlayerController->GetName());
		return nullptr;
	}

	NewHUD->AddToViewport(0);

	// Connect to currently possessed pawn if one exists
	if (APawn* CurrentPawn = PlayerController->GetPawn())
	{
		NewHUD->InitializeHUD(CurrentPawn);
	}

	ActiveHUDs.Add(PlayerController, NewHUD);
	return NewHUD;
}

UShadowSlaveHUDWidget* UShadowSlaveUIManager::GetRootHUD(const APlayerController* PlayerController) const
{
	if (!PlayerController)
	{
		return nullptr;
	}

	if (const TObjectPtr<UShadowSlaveHUDWidget>* Found = ActiveHUDs.Find(const_cast<APlayerController*>(PlayerController)))
	{
		return Found->Get();
	}

	return nullptr;
}

bool UShadowSlaveUIManager::RemoveRootHUD(APlayerController* PlayerController)
{
	if (!PlayerController)
	{
		return false;
	}

	if (TObjectPtr<UShadowSlaveHUDWidget>* Found = ActiveHUDs.Find(PlayerController))
	{
		if (UShadowSlaveHUDWidget* HUD = Found->Get())
		{
			HUD->TeardownHUD();
			HUD->RemoveFromParent();
		}

		ActiveHUDs.Remove(PlayerController);
		return true;
	}

	return false;
}

void UShadowSlaveUIManager::SetHUDVisibility(APlayerController* PlayerController, bool bVisible)
{
	if (UShadowSlaveHUDWidget* HUD = GetRootHUD(PlayerController))
	{
		HUD->SetVisibility(bVisible ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UShadowSlaveUIManager::SendNotification(APlayerController* PlayerController, const FShadowSlaveNotificationMessage& Message)
{
	if (UShadowSlaveHUDWidget* HUD = GetRootHUD(PlayerController))
	{
		HUD->ShowNotification(Message.Message, Message.Type, Message.Duration);
	}
}
