// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ShadowSlaveUITypes.generated.h"

/**
 * Classification for generic HUD notifications.
 * Kept strictly generic without hardcoding canon story events.
 */
UENUM(BlueprintType)
enum class EShadowSlaveNotificationType : uint8
{
	Info            UMETA(DisplayName = "Information"),
	Warning         UMETA(DisplayName = "Warning"),
	Success         UMETA(DisplayName = "Success"),
	ItemAcquired    UMETA(DisplayName = "Item Acquired"),
	MemoryAcquired  UMETA(DisplayName = "Memory Acquired"),
	AbilityUnlocked UMETA(DisplayName = "Ability Unlocked"),
	RankAdvancement UMETA(DisplayName = "Rank Advancement")
};

/**
 * Generic notification payload for transient UI messages.
 */
USTRUCT(BlueprintType)
struct SHADOWSLAVE_API FShadowSlaveNotificationMessage
{
	GENERATED_BODY()

	/** Primary display text */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|UI")
	FText Message;

	/** Notification classification */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|UI")
	EShadowSlaveNotificationType Type = EShadowSlaveNotificationType::Info;

	/** Display duration in seconds */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|UI", meta = (ClampMin = "0.5"))
	float Duration = 3.0f;

	/** Real-world timestamp when the notification was created */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|UI")
	FDateTime Timestamp = FDateTime::UtcNow();

	FShadowSlaveNotificationMessage() = default;

	FShadowSlaveNotificationMessage(const FText& InMessage, EShadowSlaveNotificationType InType = EShadowSlaveNotificationType::Info, float InDuration = 3.0f)
		: Message(InMessage), Type(InType), Duration(InDuration), Timestamp(FDateTime::UtcNow())
	{
	}
};

/**
 * Classification of resource bar types for generic resource widgets.
 */
UENUM(BlueprintType)
enum class EShadowSlaveResourceBarType : uint8
{
	Health  UMETA(DisplayName = "Health"),
	Stamina UMETA(DisplayName = "Stamina"),
	Essence UMETA(DisplayName = "Essence"),
	Custom  UMETA(DisplayName = "Custom")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNotificationReceivedSignature, const FShadowSlaveNotificationMessage&, Notification);
