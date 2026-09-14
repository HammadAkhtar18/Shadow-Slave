// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/ShadowSlaveHUD.h"
#include "UI/ShadowSlaveHUDWidget.h"
#include "UI/ShadowSlaveUIManager.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Engine/GameInstance.h"

AShadowSlaveHUD::AShadowSlaveHUD()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AShadowSlaveHUD::PostInitializeComponents()
{
	Super::PostInitializeComponents();
}

void AShadowSlaveHUD::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PC = Cast<APlayerController>(GetOwningPlayerController());
	if (PC && PC->IsLocalController())
	{
		if (UGameInstance* GI = GetGameInstance())
		{
			if (UShadowSlaveUIManager* UIMgr = GI->GetSubsystem<UShadowSlaveUIManager>())
			{
				UShadowSlaveHUDWidget* CreatedHUD = UIMgr->CreateRootHUD(PC, HUDWidgetClass);
				RootHUDWidget = CreatedHUD;

				if (CreatedHUD && PC->GetPawn())
				{
					CreatedHUD->InitializeHUD(PC->GetPawn());
				}
			}
		}
	}
}

void AShadowSlaveHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (APlayerController* PC = Cast<APlayerController>(GetOwningPlayerController()))
	{
		if (UGameInstance* GI = GetGameInstance())
		{
			if (UShadowSlaveUIManager* UIMgr = GI->GetSubsystem<UShadowSlaveUIManager>())
			{
				UIMgr->RemoveRootHUD(PC);
			}
		}
	}

	RootHUDWidget.Reset();

	Super::EndPlay(EndPlayReason);
}

UShadowSlaveHUDWidget* AShadowSlaveHUD::GetRootHUDWidget() const
{
	return RootHUDWidget.Get();
}

void AShadowSlaveHUD::NotifyPawnChanged(APawn* NewPawn)
{
	if (UShadowSlaveHUDWidget* HUD = RootHUDWidget.Get())
	{
		HUD->InitializeHUD(NewPawn);
	}
	else if (APlayerController* PC = Cast<APlayerController>(GetOwningPlayerController()))
	{
		if (PC->IsLocalController())
		{
			if (UGameInstance* GI = GetGameInstance())
			{
				if (UShadowSlaveUIManager* UIMgr = GI->GetSubsystem<UShadowSlaveUIManager>())
				{
					UShadowSlaveHUDWidget* CreatedHUD = UIMgr->CreateRootHUD(PC, HUDWidgetClass);
					RootHUDWidget = CreatedHUD;
					if (CreatedHUD && NewPawn)
					{
						CreatedHUD->InitializeHUD(NewPawn);
					}
				}
			}
		}
	}
}
