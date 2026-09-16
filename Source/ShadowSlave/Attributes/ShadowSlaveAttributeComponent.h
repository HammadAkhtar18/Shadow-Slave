// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Attributes/ShadowSlaveAttributeTypes.h"
#include "ShadowSlaveAttributeComponent.generated.h"

/**
 * Reusable Actor Component responsible for managing character attributes and resources:
 * Health, Stamina, and Soul Essence.
 * Usable by Player characters, generic enemies, NPCs, and future bosses.
 * Handles event-driven stamina regeneration and extensible attribute modifiers without tick overhead.
 */
UCLASS(ClassGroup = (ShadowSlave), meta = (BlueprintSpawnableComponent))
class SHADOWSLAVE_API UShadowSlaveAttributeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UShadowSlaveAttributeComponent();

	/* --- Health API --- */

	/** Returns current health */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Attributes|Health")
	float GetCurrentHealth() const { return CurrentHealth; }

	/** Returns effective maximum health (base + active modifiers) */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Attributes|Health")
	float GetMaximumHealth() const { return EffectiveMaxHealth; }

	/** Returns baseline maximum health without modifiers */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Attributes|Health")
	float GetBaseMaxHealth() const { return AttributeConfig.BaseMaxHealth; }

	/** Returns health percentage (0.0 to 1.0) */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Attributes|Health")
	float GetHealthPercent() const { return (EffectiveMaxHealth > 0.0f) ? (CurrentHealth / EffectiveMaxHealth) : 0.0f; }

	/** Applies incoming damage, clamps to remaining health, and broadcasts damage/death events */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Attributes|Health")
	float ApplyDamage(float Amount, const FShadowSlaveDamageInfo& DamageInfo = FShadowSlaveDamageInfo());

	/** Restores health clamped to maximum; returns actual health healed */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Attributes|Health")
	float Heal(float Amount);

	/** Sets health directly (clamped between 0 and maximum health) */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Attributes|Health")
	void SetHealth(float NewHealth);

	/** Returns whether this character is dead */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Attributes|Health")
	bool IsDead() const { return bIsDead; }

	/** Returns whether this character is alive */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Attributes|Health")
	bool IsAlive() const { return !bIsDead; }

	/* --- Stamina API --- */

	/** Returns current stamina */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Attributes|Stamina")
	float GetCurrentStamina() const { return CurrentStamina; }

	/** Returns effective maximum stamina (base + active modifiers) */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Attributes|Stamina")
	float GetMaximumStamina() const { return EffectiveMaxStamina; }

	/** Returns baseline maximum stamina without modifiers */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Attributes|Stamina")
	float GetBaseMaxStamina() const { return AttributeConfig.BaseMaxStamina; }

	/** Returns stamina percentage (0.0 to 1.0) */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Attributes|Stamina")
	float GetStaminaPercent() const { return (EffectiveMaxStamina > 0.0f) ? (CurrentStamina / EffectiveMaxStamina) : 0.0f; }

	/** Attempts to consume stamina; returns true if sufficient stamina was available */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Attributes|Stamina")
	bool ConsumeStamina(float Amount);

	/** Restores stamina clamped to maximum; returns actual amount restored */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Attributes|Stamina")
	float RestoreStamina(float Amount);

	/** Sets current stamina directly */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Attributes|Stamina")
	void SetStamina(float NewStamina);

	/** Enables or disables automatic stamina regeneration */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Attributes|Stamina")
	void SetStaminaRegenEnabled(bool bEnabled);

	/** Returns whether stamina regeneration is enabled */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Attributes|Stamina")
	bool IsStaminaRegenEnabled() const { return AttributeConfig.bEnableStaminaRegen; }

	/* --- Essence API --- */

	/** Returns current essence */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Attributes|Essence")
	float GetCurrentEssence() const { return CurrentEssence; }

	/** Returns effective maximum essence (base + active modifiers) */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Attributes|Essence")
	float GetMaximumEssence() const { return EffectiveMaxEssence; }

	/** Returns baseline maximum essence without modifiers */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Attributes|Essence")
	float GetBaseMaxEssence() const { return AttributeConfig.BaseMaxEssence; }

	/** Returns essence percentage (0.0 to 1.0) */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Attributes|Essence")
	float GetEssencePercent() const { return (EffectiveMaxEssence > 0.0f) ? (CurrentEssence / EffectiveMaxEssence) : 0.0f; }

	/** Attempts to consume essence; returns true if sufficient essence was available */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Attributes|Essence")
	bool ConsumeEssence(float Amount);

	/** Restores essence clamped to maximum; returns actual amount restored */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Attributes|Essence")
	float RestoreEssence(float Amount);

	/** Sets current essence directly */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Attributes|Essence")
	void SetEssence(float NewEssence);

	/* --- Modifiers API --- */

	/** Adds an attribute modifier (flat or percent bonus) */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Attributes|Modifiers")
	void AddModifier(const FAttributeModifier& Modifier);

	/** Removes an active modifier by its unique identifier */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Attributes|Modifiers")
	bool RemoveModifier(FName ModifierId);

	/** Removes all active modifiers originating from a specific source object */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Attributes|Modifiers")
	void RemoveModifiersFromSource(UObject* Source);

	/** Removes all active modifiers originating from a specific source GUID */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Attributes|Modifiers")
	void RemoveModifiersFromSourceId(const FGuid& SourceId);

	/** Returns true if any active modifier originates from the specified source GUID */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Attributes|Modifiers")
	bool HasModifierFromSourceId(const FGuid& SourceId) const;

	/** Returns all active modifiers currently applied */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Attributes|Modifiers")
	const TArray<FAttributeModifier>& GetActiveModifiers() const { return ActiveModifiers; }

	/** Clears all active modifiers */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Attributes|Modifiers")
	void ClearAllModifiers();

	/* --- Configuration & Initialization --- */

	/** Configures baseline attributes */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Attributes")
	void InitializeAttributes(const FAttributeInitConfig& NewConfig);

	/** Logs current status of all attributes to the output log */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Attributes|Debug")
	void LogAttributeStatus() const;

	/* --- Events & Delegates --- */

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Attributes|Events")
	FOnAttributeChangedSignature OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Attributes|Events")
	FOnAttributeChangedSignature OnStaminaChanged;

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Attributes|Events")
	FOnAttributeChangedSignature OnEssenceChanged;

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Attributes|Events")
	FOnDamageReceivedSignature OnDamageReceived;

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Attributes|Events")
	FOnHealReceivedSignature OnHealReceived;

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Attributes|Events")
	FOnDeathSignature OnDeath;

protected:
	virtual void BeginPlay() override;

	/** Recalculates effective max attributes factoring in active flat and percentage modifiers */
	void RecalculateMaxAttributes();

	/** Initiates stamina regeneration timer tick */
	void StartStaminaRegenTick();

	/** Ticks stamina regeneration periodically until stamina is full */
	void TickStaminaRegeneration();

	/** Callback when a temporary modifier duration expires */
	void OnModifierExpired(FName ModifierId);

	/** Stops all regeneration timers */
	void StopRegenTimers();

protected:
	/** Baseline attribute configuration */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Attributes|Config")
	FAttributeInitConfig AttributeConfig;

	/* --- Current Runtime Values --- */

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Attributes|Health")
	float CurrentHealth = 100.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Attributes|Health")
	float EffectiveMaxHealth = 100.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Attributes|Stamina")
	float CurrentStamina = 100.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Attributes|Stamina")
	float EffectiveMaxStamina = 100.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Attributes|Essence")
	float CurrentEssence = 100.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Attributes|Essence")
	float EffectiveMaxEssence = 100.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Attributes|Health")
	bool bIsDead = false;

	/* --- Active Modifiers --- */

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Attributes|Modifiers")
	TArray<FAttributeModifier> ActiveModifiers;

	TMap<FName, FTimerHandle> ModifierTimerHandles;

	/* --- Regeneration Timer Handles --- */

	FTimerHandle StaminaRegenDelayHandle;
	FTimerHandle StaminaRegenTickHandle;
};
