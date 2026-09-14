// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/ShadowSlaveNotificationWidget.h"
#include "TimerManager.h"
#include "Engine/World.h"

UShadowSlaveNotificationWidget::UShadowSlaveNotificationWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bHasActiveNotification = false;
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void UShadowSlaveNotificationWidget::PostNotification(const FShadowSlaveNotificationMessage& Message)
{
	if (!bHasActiveNotification)
	{
		ActiveNotification = Message;
		bHasActiveNotification = true;
		ReceiveNotification(ActiveNotification);

		if (UWorld* World = GetWorld())
		{
			const float Duration = FMath::Max(0.5f, Message.Duration);
			World->GetTimerManager().SetTimer(
				DismissTimerHandle,
				this,
				&UShadowSlaveNotificationWidget::DismissNotification,
				Duration,
				false
			);
		}
	}
	else
	{
		PendingQueue.Add(Message);
	}
}

void UShadowSlaveNotificationWidget::DismissNotification()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DismissTimerHandle);
	}

	bHasActiveNotification = false;
	OnNotificationDismissed();

	ProcessNextNotification();
}

void UShadowSlaveNotificationWidget::ClearAllNotifications()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DismissTimerHandle);
	}

	PendingQueue.Empty();
	bHasActiveNotification = false;
	OnNotificationDismissed();
}

void UShadowSlaveNotificationWidget::ProcessNextNotification()
{
	if (PendingQueue.Num() > 0)
	{
		const FShadowSlaveNotificationMessage NextMsg = PendingQueue[0];
		PendingQueue.RemoveAt(0);
		PostNotification(NextMsg);
	}
}

void UShadowSlaveNotificationWidget::ReceiveNotification_Implementation(const FShadowSlaveNotificationMessage& Message)
{
	// Base implementation hook for Blueprint UMG logic
}

void UShadowSlaveNotificationWidget::OnNotificationDismissed_Implementation()
{
	// Base implementation hook for Blueprint UMG logic
}
