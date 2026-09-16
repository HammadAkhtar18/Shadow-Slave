// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/ShadowSlaveHUDWidget.h"
#include "UI/ShadowSlaveResourceBarWidget.h"
#include "UI/ShadowSlaveInteractionPromptWidget.h"
#include "UI/ShadowSlaveCombatFeedbackWidget.h"
#include "UI/ShadowSlaveNotificationWidget.h"
#include "UI/ShadowSlavePlayerStatusWidget.h"
#include "UI/ShadowSlaveProgressionStatusWidget.h"
#include "Characters/ShadowSlaveCharacterBase.h"
#include "Attributes/ShadowSlaveAttributeComponent.h"
#include "Interaction/ShadowSlaveInteractionComponent.h"
#include "Combat/ShadowSlaveCombatComponent.h"
#include "Combat/ShadowSlaveDamageableInterface.h"
#include "Progression/ShadowSlaveProgressionComponent.h"
#include "Aspects/ShadowSlaveAspectComponent.h"
#include "Aspects/ShadowSlaveAspectDefinition.h"
#include "Aspects/ShadowSlaveAspectAbilityDefinition.h"
#include "Items/ShadowSlaveInventoryComponent.h"
#include "Items/ShadowSlaveItemDefinition.h"
#include "Memories/ShadowSlaveMemoryComponent.h"
#include "Memories/ShadowSlaveMemoryDefinition.h"
#include "GameFramework/Pawn.h"
#include "ShadowSlave.h"

UShadowSlaveHUDWidget::UShadowSlaveHUDWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void UShadowSlaveHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Initialize default appearance for resource bars if bound
	if (HealthBar)
	{
		HealthBar->InitializeResourceBar(
			EShadowSlaveResourceBarType::Health,
			FText::FromString(TEXT("Health")),
			FLinearColor(0.85f, 0.15f, 0.15f, 1.0f) // Crimson red
		);
	}

	if (StaminaBar)
	{
		StaminaBar->InitializeResourceBar(
			EShadowSlaveResourceBarType::Stamina,
			FText::FromString(TEXT("Stamina")),
			FLinearColor(0.15f, 0.75f, 0.25f, 1.0f) // Emerald green
		);
	}

	if (EssenceBar)
	{
		EssenceBar->InitializeResourceBar(
			EShadowSlaveResourceBarType::Essence,
			FText::FromString(TEXT("Essence")),
			FLinearColor(0.20f, 0.45f, 0.90f, 1.0f) // Soul blue
		);
	}

	if (InteractionPrompt)
	{
		InteractionPrompt->ClearPrompt();
	}
}

void UShadowSlaveHUDWidget::NativeDestruct()
{
	TeardownHUD();
	Super::NativeDestruct();
}

void UShadowSlaveHUDWidget::InitializeHUD(APawn* InPlayerPawn)
{
	if (!InPlayerPawn)
	{
		return;
	}

	// Clean up any previous pawn subscriptions before binding new ones
	TeardownHUD();

	BoundPawn = InPlayerPawn;
	BoundCharacter = Cast<AShadowSlaveCharacterBase>(InPlayerPawn);

	// 1. Bind Attributes Component
	BoundAttributes = InPlayerPawn->FindComponentByClass<UShadowSlaveAttributeComponent>();
	if (BoundAttributes.IsValid())
	{
		BoundAttributes->OnHealthChanged.AddDynamic(this, &UShadowSlaveHUDWidget::HandleHealthChanged);
		BoundAttributes->OnStaminaChanged.AddDynamic(this, &UShadowSlaveHUDWidget::HandleStaminaChanged);
		BoundAttributes->OnEssenceChanged.AddDynamic(this, &UShadowSlaveHUDWidget::HandleEssenceChanged);

		// Push initial attribute values (event-driven, no polling)
		HandleHealthChanged(BoundAttributes->GetCurrentHealth(), BoundAttributes->GetMaximumHealth());
		HandleStaminaChanged(BoundAttributes->GetCurrentStamina(), BoundAttributes->GetMaximumStamina());
		HandleEssenceChanged(BoundAttributes->GetCurrentEssence(), BoundAttributes->GetMaximumEssence());
	}

	// 2. Bind Interaction Component
	BoundInteraction = InPlayerPawn->FindComponentByClass<UShadowSlaveInteractionComponent>();
	if (BoundInteraction.IsValid())
	{
		BoundInteraction->OnInteractionTargetChanged.AddDynamic(this, &UShadowSlaveHUDWidget::HandleInteractionTargetChanged);

		if (InteractionPrompt)
		{
			if (BoundInteraction->HasInteractableTarget())
			{
				InteractionPrompt->SetPrompt(BoundInteraction->GetCurrentInteractionPrompt(), GetInteractKeyPrompt());
			}
			else
			{
				InteractionPrompt->ClearPrompt();
			}
		}
	}

	// 3. Bind Combat Component
	BoundCombat = InPlayerPawn->FindComponentByClass<UShadowSlaveCombatComponent>();
	if (BoundCombat.IsValid())
	{
		BoundCombat->OnTargetHit.AddDynamic(this, &UShadowSlaveHUDWidget::HandleTargetHit);
		BoundCombat->OnCombatStateChanged.AddDynamic(this, &UShadowSlaveHUDWidget::HandleCombatStateChanged);

		if (PlayerStatus)
		{
			PlayerStatus->UpdateCombatState(BoundCombat->GetCombatState());
		}
	}

	// 4. Bind Character Locomotion & Damage
	if (BoundCharacter.IsValid())
	{
		BoundCharacter->OnGaitChanged.AddDynamic(this, &UShadowSlaveHUDWidget::HandleGaitChanged);
		BoundCharacter->OnCharacterDamaged.AddDynamic(this, &UShadowSlaveHUDWidget::HandleCharacterDamaged);

		if (PlayerStatus)
		{
			PlayerStatus->UpdateGait(BoundCharacter->GetGait());
			PlayerStatus->UpdateAliveStatus(BoundCharacter->IsAlive());
		}
	}

	// 5. Bind Progression Component
	BoundProgression = InPlayerPawn->FindComponentByClass<UShadowSlaveProgressionComponent>();
	if (BoundProgression.IsValid())
	{
		BoundProgression->OnCharacterRankChanged.AddDynamic(this, &UShadowSlaveHUDWidget::HandleCharacterRankChanged);
		BoundProgression->OnSoulCoreCountChanged.AddDynamic(this, &UShadowSlaveHUDWidget::HandleSoulCoreCountChanged);
		BoundProgression->OnMaxSoulCoresChanged.AddDynamic(this, &UShadowSlaveHUDWidget::HandleMaxSoulCoresChanged);

		if (ProgressionStatus)
		{
			ProgressionStatus->UpdateRank(BoundProgression->GetCharacterRank());
			ProgressionStatus->UpdateSoulCores(BoundProgression->GetSoulCoreCount(), BoundProgression->GetMaxSoulCores());
		}
	}

	// 6. Bind Aspect Component
	BoundAspect = InPlayerPawn->FindComponentByClass<UShadowSlaveAspectComponent>();
	if (BoundAspect.IsValid())
	{
		BoundAspect->OnAspectChanged.AddDynamic(this, &UShadowSlaveHUDWidget::HandleAspectChanged);
		BoundAspect->OnAbilityUnlocked.AddDynamic(this, &UShadowSlaveHUDWidget::HandleAbilityUnlocked);

		if (ProgressionStatus)
		{
			const FText AspectTitle = BoundAspect->GetAspectDefinition() ? BoundAspect->GetAspectDefinition()->DisplayName : FText::FromString(TEXT("None"));
			ProgressionStatus->UpdateAspectInfo(AspectTitle, BoundAspect->GetAspectRank());
		}

		ReceiveAbilityPanelUpdated(BoundAspect.Get());
	}

	// 7. Bind Inventory Component
	BoundInventory = InPlayerPawn->FindComponentByClass<UShadowSlaveInventoryComponent>();
	if (BoundInventory.IsValid())
	{
		BoundInventory->OnItemAdded.AddDynamic(this, &UShadowSlaveHUDWidget::HandleItemAdded);
		ReceiveInventoryUpdated(BoundInventory.Get());
	}

	// 8. Bind Memory Component
	BoundMemories = InPlayerPawn->FindComponentByClass<UShadowSlaveMemoryComponent>();
	if (BoundMemories.IsValid())
	{
		BoundMemories->OnMemoryAdded.AddDynamic(this, &UShadowSlaveHUDWidget::HandleMemoryAdded);
		BoundMemories->OnMemoryEquipped.AddDynamic(this, &UShadowSlaveHUDWidget::HandleMemoryEquipped);
		ReceiveMemoriesUpdated(BoundMemories.Get());
	}

	OnHUDInitialized(InPlayerPawn);
}

void UShadowSlaveHUDWidget::TeardownHUD()
{
	if (BoundAttributes.IsValid())
	{
		BoundAttributes->OnHealthChanged.RemoveDynamic(this, &UShadowSlaveHUDWidget::HandleHealthChanged);
		BoundAttributes->OnStaminaChanged.RemoveDynamic(this, &UShadowSlaveHUDWidget::HandleStaminaChanged);
		BoundAttributes->OnEssenceChanged.RemoveDynamic(this, &UShadowSlaveHUDWidget::HandleEssenceChanged);
	}

	if (BoundInteraction.IsValid())
	{
		BoundInteraction->OnInteractionTargetChanged.RemoveDynamic(this, &UShadowSlaveHUDWidget::HandleInteractionTargetChanged);
	}

	if (BoundCombat.IsValid())
	{
		BoundCombat->OnTargetHit.RemoveDynamic(this, &UShadowSlaveHUDWidget::HandleTargetHit);
		BoundCombat->OnCombatStateChanged.RemoveDynamic(this, &UShadowSlaveHUDWidget::HandleCombatStateChanged);
	}

	if (BoundCharacter.IsValid())
	{
		BoundCharacter->OnGaitChanged.RemoveDynamic(this, &UShadowSlaveHUDWidget::HandleGaitChanged);
		BoundCharacter->OnCharacterDamaged.RemoveDynamic(this, &UShadowSlaveHUDWidget::HandleCharacterDamaged);
	}

	if (BoundProgression.IsValid())
	{
		BoundProgression->OnCharacterRankChanged.RemoveDynamic(this, &UShadowSlaveHUDWidget::HandleCharacterRankChanged);
		BoundProgression->OnSoulCoreCountChanged.RemoveDynamic(this, &UShadowSlaveHUDWidget::HandleSoulCoreCountChanged);
		BoundProgression->OnMaxSoulCoresChanged.RemoveDynamic(this, &UShadowSlaveHUDWidget::HandleMaxSoulCoresChanged);
	}

	if (BoundAspect.IsValid())
	{
		BoundAspect->OnAspectChanged.RemoveDynamic(this, &UShadowSlaveHUDWidget::HandleAspectChanged);
		BoundAspect->OnAbilityUnlocked.RemoveDynamic(this, &UShadowSlaveHUDWidget::HandleAbilityUnlocked);
	}

	if (BoundInventory.IsValid())
	{
		BoundInventory->OnItemAdded.RemoveDynamic(this, &UShadowSlaveHUDWidget::HandleItemAdded);
	}

	if (BoundMemories.IsValid())
	{
		BoundMemories->OnMemoryAdded.RemoveDynamic(this, &UShadowSlaveHUDWidget::HandleMemoryAdded);
		BoundMemories->OnMemoryEquipped.RemoveDynamic(this, &UShadowSlaveHUDWidget::HandleMemoryEquipped);
	}

	BoundPawn = nullptr;
	BoundCharacter = nullptr;
	BoundAttributes = nullptr;
	BoundInteraction = nullptr;
	BoundCombat = nullptr;
	BoundProgression = nullptr;
	BoundAspect = nullptr;
	BoundInventory = nullptr;
	BoundMemories = nullptr;

	OnHUDTeardown();
}

void UShadowSlaveHUDWidget::ShowNotification(const FText& Message, EShadowSlaveNotificationType Type, float Duration)
{
	if (NotificationLayer)
	{
		NotificationLayer->PostNotification(FShadowSlaveNotificationMessage(Message, Type, Duration));
	}
}

/* --- Event Handlers --- */

void UShadowSlaveHUDWidget::HandleHealthChanged(float CurrentHealth, float MaxHealth)
{
	if (HealthBar)
	{
		const float Pct = (MaxHealth > 0.0f) ? (CurrentHealth / MaxHealth) : 0.0f;
		HealthBar->UpdateValues(CurrentHealth, MaxHealth, Pct);
	}

	if (PlayerStatus)
	{
		PlayerStatus->UpdateAliveStatus(CurrentHealth > 0.0f);
	}

	if (CurrentHealth <= 0.0f && CombatFeedback)
	{
		CombatFeedback->NotifyCharacterDied();
	}
}

void UShadowSlaveHUDWidget::HandleStaminaChanged(float CurrentStamina, float MaxStamina)
{
	if (StaminaBar)
	{
		const float Pct = (MaxStamina > 0.0f) ? (CurrentStamina / MaxStamina) : 0.0f;
		StaminaBar->UpdateValues(CurrentStamina, MaxStamina, Pct);
	}
}

void UShadowSlaveHUDWidget::HandleEssenceChanged(float CurrentEssence, float MaxEssence)
{
	if (EssenceBar)
	{
		const float Pct = (MaxEssence > 0.0f) ? (CurrentEssence / MaxEssence) : 0.0f;
		EssenceBar->UpdateValues(CurrentEssence, MaxEssence, Pct);
	}
}

void UShadowSlaveHUDWidget::HandleInteractionTargetChanged(AActor* NewTarget, AActor* OldTarget)
{
	if (!InteractionPrompt)
	{
		return;
	}

	if (NewTarget && BoundInteraction.IsValid())
	{
		const FText ActionPrompt = BoundInteraction->GetCurrentInteractionPrompt();
		InteractionPrompt->SetPrompt(ActionPrompt, GetInteractKeyPrompt());
	}
	else
	{
		InteractionPrompt->ClearPrompt();
	}
}

void UShadowSlaveHUDWidget::HandleTargetHit(AActor* TargetActor, const FShadowSlaveDamageInfo& DamageInfo)
{
	if (CombatFeedback)
	{
		bool bIsFatal = false;
		if (TargetActor && TargetActor->GetClass()->ImplementsInterface(UShadowSlaveDamageableInterface::StaticClass()))
		{
			bIsFatal = !IShadowSlaveDamageableInterface::Execute_IsAlive(TargetActor);
		}

		CombatFeedback->NotifyDamageDealt(TargetActor, DamageInfo.DamageAmount, bIsFatal);
	}
}

void UShadowSlaveHUDWidget::HandleCombatStateChanged(ECombatState OldState, ECombatState NewState)
{
	if (CombatFeedback)
	{
		CombatFeedback->NotifyCombatStateChanged(NewState, OldState);
	}

	if (PlayerStatus)
	{
		PlayerStatus->UpdateCombatState(NewState);
	}
}

void UShadowSlaveHUDWidget::HandleCharacterDamaged(const FShadowSlaveDamageInfo& DamageInfo)
{
	if (CombatFeedback)
	{
		CombatFeedback->NotifyDamageTaken(DamageInfo.DamageAmount, DamageInfo.HitDirection);
	}
}

void UShadowSlaveHUDWidget::HandleGaitChanged(EShadowSlaveGait OldGait, EShadowSlaveGait NewGait)
{
	if (PlayerStatus)
	{
		PlayerStatus->UpdateGait(NewGait);
	}
}

void UShadowSlaveHUDWidget::HandleCharacterRankChanged(EShadowSlaveCharacterRank NewRank, EShadowSlaveCharacterRank OldRank)
{
	if (ProgressionStatus)
	{
		ProgressionStatus->UpdateRank(NewRank);
	}

	ShowNotification(FText::FromString(TEXT("Character Rank Advanced")), EShadowSlaveNotificationType::RankAdvancement);
}

void UShadowSlaveHUDWidget::HandleSoulCoreCountChanged(int32 NewCount, int32 OldCount)
{
	if (ProgressionStatus && BoundProgression.IsValid())
	{
		ProgressionStatus->UpdateSoulCores(NewCount, BoundProgression->GetMaxSoulCores());
	}
}

void UShadowSlaveHUDWidget::HandleMaxSoulCoresChanged(int32 NewMax, int32 OldMax)
{
	if (ProgressionStatus && BoundProgression.IsValid())
	{
		ProgressionStatus->UpdateSoulCores(BoundProgression->GetSoulCoreCount(), NewMax);
	}
}

void UShadowSlaveHUDWidget::HandleAspectChanged(UShadowSlaveAspectDefinition* NewAspectDef, UShadowSlaveAspectDefinition* OldAspectDef)
{
	if (ProgressionStatus && BoundAspect.IsValid())
	{
		const FText AspectTitle = NewAspectDef ? NewAspectDef->DisplayName : FText::FromString(TEXT("None"));
		ProgressionStatus->UpdateAspectInfo(AspectTitle, BoundAspect->GetAspectRank());
	}

	ReceiveAbilityPanelUpdated(BoundAspect.Get());
}

void UShadowSlaveHUDWidget::HandleAbilityUnlocked(FName AbilityId, UShadowSlaveAspectAbilityDefinition* AbilityDef)
{
	if (AbilityDef)
	{
		const FText Msg = FText::Format(FText::FromString(TEXT("Ability Unlocked: {0}")), AbilityDef->DisplayName);
		ShowNotification(Msg, EShadowSlaveNotificationType::AbilityUnlocked);
	}

	ReceiveAbilityPanelUpdated(BoundAspect.Get());
}

void UShadowSlaveHUDWidget::HandleItemAdded(const FShadowSlaveItemInstance& ItemInstance, int32 QuantityAdded)
{
	if (ItemInstance.ItemDefinition)
	{
		const FText Msg = FText::Format(
			FText::FromString(TEXT("Acquired: {0} x{1}")),
			ItemInstance.ItemDefinition->DisplayName,
			FText::AsNumber(QuantityAdded)
		);
		ShowNotification(Msg, EShadowSlaveNotificationType::ItemAcquired);
	}

	ReceiveInventoryUpdated(BoundInventory.Get());
}

void UShadowSlaveHUDWidget::HandleMemoryAdded(const FShadowSlaveMemoryInstance& MemoryInstance)
{
	if (MemoryInstance.MemoryDefinition)
	{
		const FText Msg = FText::Format(
			FText::FromString(TEXT("Memory Acquired: {0}")),
			MemoryInstance.MemoryDefinition->DisplayName
		);
		ShowNotification(Msg, EShadowSlaveNotificationType::MemoryAcquired);
	}

	ReceiveMemoriesUpdated(BoundMemories.Get());
}

void UShadowSlaveHUDWidget::HandleMemoryEquipped(const FShadowSlaveMemoryInstance& MemoryInstance)
{
	ReceiveMemoriesUpdated(BoundMemories.Get());
}

FText UShadowSlaveHUDWidget::GetInteractKeyPrompt() const
{
	// Input abstraction: defaults to [E], ready for future dynamic Enhanced Input key mapping
	return FText::FromString(TEXT("[E]"));
}
