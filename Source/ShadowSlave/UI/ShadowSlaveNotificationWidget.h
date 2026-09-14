// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/ShadowSlaveUITypes.h"
#include "ShadowSlaveNotificationWidget.generated.h"

/**
 * Reusable UMG notification widget presenting transient banner/toast messages
 * (e.g. Item Acquired, Memory Acquired, Ability Unlocked, Rank Advancement).
 * Strictly generic presentation; does not hardcode canon lore or story scripts.
 */
UCLASS(Abstract)
class SHADOWSLAVE_API UShadowSlaveNotificationWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UShadowSlaveNotificationWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** Posts a new notification message to be presented */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|UI|Notification")
	void PostNotification(const FShadowSlaveNotificationMessage& Message);

	/** Dismisses the currently active notification */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|UI|Notification")
	void DismissNotification();

	/** Clears all pending notifications */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|UI|Notification")
	void ClearAllNotifications();

	UFUNCTION(BlueprintPure, Category = "ShadowSlave|UI|Notification")
	bool HasActiveNotification() const { return bHasActiveNotification; }

	UFUNCTION(BlueprintPure, Category = "ShadowSlave|UI|Notification")
	FShadowSlaveNotificationMessage GetActiveNotification() const { return ActiveNotification; }

protected:
	/** Hook for Blueprint UMG implementations to display and animate notification banner */
	UFUNCTION(BlueprintNativeEvent, Category = "ShadowSlave|UI|Notification")
	void ReceiveNotification(const FShadowSlaveNotificationMessage& Message);
	virtual void ReceiveNotification_Implementation(const FShadowSlaveNotificationMessage& Message);

	/** Hook for Blueprint UMG implementations to fade out or dismiss notification */
	UFUNCTION(BlueprintNativeEvent, Category = "ShadowSlave|UI|Notification")
	void OnNotificationDismissed();
	virtual void OnNotificationDismissed_Implementation();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|UI|Notification")
	bool bHasActiveNotification = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|UI|Notification")
	FShadowSlaveNotificationMessage ActiveNotification;

	/** Queue of pending notification messages */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|UI|Notification")
	TArray<FShadowSlaveNotificationMessage> PendingQueue;

	/** Timer handle for automatic notification dismissal */
	FTimerHandle DismissTimerHandle;

private:
	void ProcessNextNotification();
};
