// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/ShadowSlaveUITypes.h"
#include "Characters/ShadowSlaveCharacterTypes.h"
#include "Combat/ShadowSlaveCombatTypes.h"
#include "Progression/ShadowSlaveProgressionTypes.h"
#include "Aspects/ShadowSlaveAspectTypes.h"
#include "Items/ShadowSlaveItemTypes.h"
#include "Memories/ShadowSlaveMemoryTypes.h"
#include "ShadowSlaveHUDWidget.generated.h"

class APawn;
class AShadowSlaveCharacterBase;
class UShadowSlaveAttributeComponent;
class UShadowSlaveInteractionComponent;
class UShadowSlaveCombatComponent;
class UShadowSlaveProgressionComponent;
class UShadowSlaveAspectComponent;
class UShadowSlaveInventoryComponent;
class UShadowSlaveMemoryComponent;
class UShadowSlaveAspectDefinition;
class UShadowSlaveAspectAbilityDefinition;
class UShadowSlaveResourceBarWidget;
class UShadowSlaveInteractionPromptWidget;
class UShadowSlaveCombatFeedbackWidget;
class UShadowSlaveNotificationWidget;
class UShadowSlavePlayerStatusWidget;
class UShadowSlaveProgressionStatusWidget;

/**
 * Root composition HUD widget for Shadow Slave.
 * Encapsulates resource bars, interaction prompt, combat feedback, player status,
 * progression overview, and transient notification layer.
 *
 * NOTE ON AUTHORITY & TICK:
 * This widget does NOT tick and never maintains authoritative gameplay state.
 * It subscribes to player pawn component delegates upon initialization and pushes updates event-driven.
 */
UCLASS(Abstract)
class SHADOWSLAVE_API UShadowSlaveHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UShadowSlaveHUDWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** Initializes HUD by binding to the possessed player character's components */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|UI|HUD")
	void InitializeHUD(APawn* InPlayerPawn);

	/** Tears down bindings and cleans up widget references */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|UI|HUD")
	void TeardownHUD();

	/** Posts a notification message to the notification layer */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|UI|HUD")
	void ShowNotification(const FText& Message, EShadowSlaveNotificationType Type = EShadowSlaveNotificationType::Info, float Duration = 3.0f);

	/* --- Child Widget Accessors --- */

	UFUNCTION(BlueprintPure, Category = "ShadowSlave|UI|HUD")
	UShadowSlaveResourceBarWidget* GetHealthBar() const { return HealthBar; }

	UFUNCTION(BlueprintPure, Category = "ShadowSlave|UI|HUD")
	UShadowSlaveResourceBarWidget* GetStaminaBar() const { return StaminaBar; }

	UFUNCTION(BlueprintPure, Category = "ShadowSlave|UI|HUD")
	UShadowSlaveResourceBarWidget* GetEssenceBar() const { return EssenceBar; }

	UFUNCTION(BlueprintPure, Category = "ShadowSlave|UI|HUD")
	UShadowSlaveInteractionPromptWidget* GetInteractionPrompt() const { return InteractionPrompt; }

	UFUNCTION(BlueprintPure, Category = "ShadowSlave|UI|HUD")
	UShadowSlaveCombatFeedbackWidget* GetCombatFeedback() const { return CombatFeedback; }

	UFUNCTION(BlueprintPure, Category = "ShadowSlave|UI|HUD")
	UShadowSlaveNotificationWidget* GetNotificationLayer() const { return NotificationLayer; }

	UFUNCTION(BlueprintPure, Category = "ShadowSlave|UI|HUD")
	UShadowSlavePlayerStatusWidget* GetPlayerStatus() const { return PlayerStatus; }

	UFUNCTION(BlueprintPure, Category = "ShadowSlave|UI|HUD")
	UShadowSlaveProgressionStatusWidget* GetProgressionStatus() const { return ProgressionStatus; }

	/* --- Sub-Widgets (Optional Blueprint Bindings) --- */

	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "ShadowSlave|UI|HUD")
	TObjectPtr<UShadowSlaveResourceBarWidget> HealthBar;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "ShadowSlave|UI|HUD")
	TObjectPtr<UShadowSlaveResourceBarWidget> StaminaBar;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "ShadowSlave|UI|HUD")
	TObjectPtr<UShadowSlaveResourceBarWidget> EssenceBar;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "ShadowSlave|UI|HUD")
	TObjectPtr<UShadowSlaveInteractionPromptWidget> InteractionPrompt;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "ShadowSlave|UI|HUD")
	TObjectPtr<UShadowSlaveCombatFeedbackWidget> CombatFeedback;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "ShadowSlave|UI|HUD")
	TObjectPtr<UShadowSlaveNotificationWidget> NotificationLayer;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "ShadowSlave|UI|HUD")
	TObjectPtr<UShadowSlavePlayerStatusWidget> PlayerStatus;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "ShadowSlave|UI|HUD")
	TObjectPtr<UShadowSlaveProgressionStatusWidget> ProgressionStatus;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/* --- Event Handlers for Component Subscriptions --- */

	UFUNCTION()
	void HandleHealthChanged(float CurrentHealth, float MaxHealth);

	UFUNCTION()
	void HandleStaminaChanged(float CurrentStamina, float MaxStamina);

	UFUNCTION()
	void HandleEssenceChanged(float CurrentEssence, float MaxEssence);

	UFUNCTION()
	void HandleInteractionTargetChanged(AActor* NewTarget, AActor* OldTarget);

	UFUNCTION()
	void HandleTargetHit(AActor* TargetActor, const FShadowSlaveDamageInfo& DamageInfo);

	UFUNCTION()
	void HandleCombatStateChanged(ECombatState OldState, ECombatState NewState);

	UFUNCTION()
	void HandleCharacterDamaged(const FShadowSlaveDamageInfo& DamageInfo);

	UFUNCTION()
	void HandleGaitChanged(EShadowSlaveGait OldGait, EShadowSlaveGait NewGait);

	UFUNCTION()
	void HandleCharacterRankChanged(EShadowSlaveCharacterRank NewRank, EShadowSlaveCharacterRank OldRank);

	UFUNCTION()
	void HandleSoulCoreCountChanged(int32 NewCount, int32 OldCount);

	UFUNCTION()
	void HandleMaxSoulCoresChanged(int32 NewMax, int32 OldMax);

	UFUNCTION()
	void HandleAspectChanged(UShadowSlaveAspectDefinition* NewAspectDef, UShadowSlaveAspectDefinition* OldAspectDef);

	UFUNCTION()
	void HandleAbilityUnlocked(FName AbilityId, UShadowSlaveAspectAbilityDefinition* AbilityDef);

	UFUNCTION()
	void HandleItemAdded(const FShadowSlaveItemInstance& ItemInstance, int32 QuantityAdded);

	UFUNCTION()
	void HandleMemoryAdded(const FShadowSlaveMemoryInstance& MemoryInstance);

	UFUNCTION()
	void HandleMemoryEquipped(const FShadowSlaveMemoryInstance& MemoryInstance);

	/** Helper resolving current input key prompt text for interaction */
	virtual FText GetInteractKeyPrompt() const;

	/* --- Extensibility Hooks for Future Menus & Ability Bars --- */

	/** Blueprint hook called when HUD finishes full initialization */
	UFUNCTION(BlueprintImplementableEvent, Category = "ShadowSlave|UI|HUD")
	void OnHUDInitialized(APawn* PlayerPawn);

	/** Blueprint hook called when HUD is torn down */
	UFUNCTION(BlueprintImplementableEvent, Category = "ShadowSlave|UI|HUD")
	void OnHUDTeardown();

	/** Extensibility hook for future Aspect Ability Panel */
	UFUNCTION(BlueprintImplementableEvent, Category = "ShadowSlave|UI|HUD|Hooks")
	void ReceiveAbilityPanelUpdated(UShadowSlaveAspectComponent* AspectComp);

	/** Extensibility hook for future Inventory Panel */
	UFUNCTION(BlueprintImplementableEvent, Category = "ShadowSlave|UI|HUD|Hooks")
	void ReceiveInventoryUpdated(UShadowSlaveInventoryComponent* InventoryComp);

	/** Extensibility hook for future Memory Panel */
	UFUNCTION(BlueprintImplementableEvent, Category = "ShadowSlave|UI|HUD|Hooks")
	void ReceiveMemoriesUpdated(UShadowSlaveMemoryComponent* MemoryComp);

protected:
	/** Tracked weak references to bound components */
	TWeakObjectPtr<APawn> BoundPawn;
	TWeakObjectPtr<AShadowSlaveCharacterBase> BoundCharacter;
	TWeakObjectPtr<UShadowSlaveAttributeComponent> BoundAttributes;
	TWeakObjectPtr<UShadowSlaveInteractionComponent> BoundInteraction;
	TWeakObjectPtr<UShadowSlaveCombatComponent> BoundCombat;
	TWeakObjectPtr<UShadowSlaveProgressionComponent> BoundProgression;
	TWeakObjectPtr<UShadowSlaveAspectComponent> BoundAspect;
	TWeakObjectPtr<UShadowSlaveInventoryComponent> BoundInventory;
	TWeakObjectPtr<UShadowSlaveMemoryComponent> BoundMemories;
};
