// Copyright Epic Games, Inc. All Rights Reserved.

#include "Attributes/ShadowSlaveAttributeComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "ShadowSlave.h"

UShadowSlaveAttributeComponent::UShadowSlaveAttributeComponent()
{
	// Component uses event-driven updates and timers; no tick overhead
	PrimaryComponentTick.bCanEverTick = false;

	EffectiveMaxHealth = AttributeConfig.BaseMaxHealth;
	CurrentHealth = EffectiveMaxHealth;

	EffectiveMaxStamina = AttributeConfig.BaseMaxStamina;
	CurrentStamina = EffectiveMaxStamina;

	EffectiveMaxEssence = AttributeConfig.BaseMaxEssence;
	CurrentEssence = EffectiveMaxEssence;
}

void UShadowSlaveAttributeComponent::BeginPlay()
{
	Super::BeginPlay();

	RecalculateMaxAttributes();

	CurrentHealth = EffectiveMaxHealth;
	CurrentStamina = EffectiveMaxStamina;
	CurrentEssence = EffectiveMaxEssence;
	bIsDead = false;
}

float UShadowSlaveAttributeComponent::ApplyDamage(float Amount, const FShadowSlaveDamageInfo& DamageInfo)
{
	// Edge cases: dead or non-positive damage
	if (bIsDead || Amount <= 0.0f)
	{
		return 0.0f;
	}

	// Clamp damage to remaining health
	const float DamageApplied = FMath::Clamp(Amount, 0.0f, CurrentHealth);
	CurrentHealth -= DamageApplied;

	OnHealthChanged.Broadcast(CurrentHealth, EffectiveMaxHealth);
	OnDamageReceived.Broadcast(DamageApplied, DamageInfo);

	if (CurrentHealth <= 0.0f)
	{
		CurrentHealth = 0.0f;
		bIsDead = true;

		// Immediately stop all regeneration upon death
		StopRegenTimers();

		OnDeath.Broadcast();
	}

	return DamageApplied;
}

float UShadowSlaveAttributeComponent::Heal(float Amount)
{
	// Cannot heal if dead or amount is non-positive
	if (bIsDead || Amount <= 0.0f)
	{
		return 0.0f;
	}

	const float MissingHealth = FMath::Max(EffectiveMaxHealth - CurrentHealth, 0.0f);
	const float ActualHealed = FMath::Min(Amount, MissingHealth);

	if (ActualHealed > 0.0f)
	{
		CurrentHealth += ActualHealed;
		OnHealthChanged.Broadcast(CurrentHealth, EffectiveMaxHealth);
		OnHealReceived.Broadcast(ActualHealed);
	}

	return ActualHealed;
}

void UShadowSlaveAttributeComponent::SetHealth(float NewHealth)
{
	if (bIsDead)
	{
		return;
	}

	CurrentHealth = FMath::Clamp(NewHealth, 0.0f, EffectiveMaxHealth);
	OnHealthChanged.Broadcast(CurrentHealth, EffectiveMaxHealth);

	if (CurrentHealth <= 0.0f)
	{
		CurrentHealth = 0.0f;
		bIsDead = true;
		StopRegenTimers();
		OnDeath.Broadcast();
	}
}

bool UShadowSlaveAttributeComponent::ConsumeStamina(float Amount)
{
	if (bIsDead || Amount <= 0.0f)
	{
		return false;
	}

	// Insufficient stamina check
	if (CurrentStamina < Amount)
	{
		return false;
	}

	CurrentStamina -= Amount;
	OnStaminaChanged.Broadcast(CurrentStamina, EffectiveMaxStamina);

	// Delay regeneration after consumption
	if (AttributeConfig.bEnableStaminaRegen)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(StaminaRegenTickHandle);
			World->GetTimerManager().SetTimer(
				StaminaRegenDelayHandle,
				this,
				&UShadowSlaveAttributeComponent::StartStaminaRegenTick,
				AttributeConfig.StaminaRegenDelay,
				false
			);
		}
	}

	return true;
}

float UShadowSlaveAttributeComponent::RestoreStamina(float Amount)
{
	if (bIsDead || Amount <= 0.0f)
	{
		return 0.0f;
	}

	const float MissingStamina = FMath::Max(EffectiveMaxStamina - CurrentStamina, 0.0f);
	const float Restored = FMath::Min(Amount, MissingStamina);

	if (Restored > 0.0f)
	{
		CurrentStamina += Restored;
		OnStaminaChanged.Broadcast(CurrentStamina, EffectiveMaxStamina);
	}

	return Restored;
}

void UShadowSlaveAttributeComponent::SetStamina(float NewStamina)
{
	CurrentStamina = FMath::Clamp(NewStamina, 0.0f, EffectiveMaxStamina);
	OnStaminaChanged.Broadcast(CurrentStamina, EffectiveMaxStamina);
}

void UShadowSlaveAttributeComponent::SetStaminaRegenEnabled(bool bEnabled)
{
	AttributeConfig.bEnableStaminaRegen = bEnabled;

	if (!bEnabled)
	{
		StopRegenTimers();
	}
	else if (CurrentStamina < EffectiveMaxStamina && !bIsDead)
	{
		StartStaminaRegenTick();
	}
}

bool UShadowSlaveAttributeComponent::ConsumeEssence(float Amount)
{
	if (bIsDead || Amount <= 0.0f)
	{
		return false;
	}

	// Insufficient essence check
	if (CurrentEssence < Amount)
	{
		return false;
	}

	CurrentEssence -= Amount;
	OnEssenceChanged.Broadcast(CurrentEssence, EffectiveMaxEssence);
	return true;
}

float UShadowSlaveAttributeComponent::RestoreEssence(float Amount)
{
	if (bIsDead || Amount <= 0.0f)
	{
		return 0.0f;
	}

	const float MissingEssence = FMath::Max(EffectiveMaxEssence - CurrentEssence, 0.0f);
	const float Restored = FMath::Min(Amount, MissingEssence);

	if (Restored > 0.0f)
	{
		CurrentEssence += Restored;
		OnEssenceChanged.Broadcast(CurrentEssence, EffectiveMaxEssence);
	}

	return Restored;
}

void UShadowSlaveAttributeComponent::SetEssence(float NewEssence)
{
	CurrentEssence = FMath::Clamp(NewEssence, 0.0f, EffectiveMaxEssence);
	OnEssenceChanged.Broadcast(CurrentEssence, EffectiveMaxEssence);
}

void UShadowSlaveAttributeComponent::AddModifier(const FAttributeModifier& Modifier)
{
	if (Modifier.ModifierId == NAME_None)
	{
		return;
	}

	// Remove any existing modifier with matching ID
	RemoveModifier(Modifier.ModifierId);

	ActiveModifiers.Add(Modifier);

	// Setup duration timer for temporary modifiers
	if (Modifier.Duration > 0.0f)
	{
		if (UWorld* World = GetWorld())
		{
			FTimerDelegate Delegate;
			Delegate.BindUObject(this, &UShadowSlaveAttributeComponent::OnModifierExpired, Modifier.ModifierId);

			FTimerHandle Handle;
			World->GetTimerManager().SetTimer(Handle, Delegate, Modifier.Duration, false);
			ModifierTimerHandles.Add(Modifier.ModifierId, Handle);
		}
	}

	RecalculateMaxAttributes();
}

bool UShadowSlaveAttributeComponent::RemoveModifier(FName ModifierId)
{
	const int32 RemovedCount = ActiveModifiers.RemoveAll([ModifierId](const FAttributeModifier& Mod)
	{
		return Mod.ModifierId == ModifierId;
	});

	if (FTimerHandle* Handle = ModifierTimerHandles.Find(ModifierId))
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(*Handle);
		}
		ModifierTimerHandles.Remove(ModifierId);
	}

	if (RemovedCount > 0)
	{
		RecalculateMaxAttributes();
		return true;
	}

	return false;
}

void UShadowSlaveAttributeComponent::RemoveModifiersFromSource(UObject* Source)
{
	if (!Source)
	{
		return;
	}

	TArray<FName> IdsToRemove;
	for (const FAttributeModifier& Mod : ActiveModifiers)
	{
		if (Mod.Source.Get() == Source)
		{
			IdsToRemove.Add(Mod.ModifierId);
		}
	}

	for (const FName& Id : IdsToRemove)
	{
		RemoveModifier(Id);
	}
}

void UShadowSlaveAttributeComponent::RemoveModifiersFromSourceId(const FGuid& SourceId)
{
	if (!SourceId.IsValid())
	{
		return;
	}

	TArray<FName> IdsToRemove;
	for (const FAttributeModifier& Mod : ActiveModifiers)
	{
		if (Mod.SourceId == SourceId)
		{
			IdsToRemove.Add(Mod.ModifierId);
		}
	}

	for (const FName& Id : IdsToRemove)
	{
		RemoveModifier(Id);
	}
}

bool UShadowSlaveAttributeComponent::HasModifierFromSourceId(const FGuid& SourceId) const
{
	if (!SourceId.IsValid())
	{
		return false;
	}

	for (const FAttributeModifier& Mod : ActiveModifiers)
	{
		if (Mod.SourceId == SourceId)
		{
			return true;
		}
	}

	return false;
}

void UShadowSlaveAttributeComponent::ClearAllModifiers()
{
	if (UWorld* World = GetWorld())
	{
		for (auto& Pair : ModifierTimerHandles)
		{
			World->GetTimerManager().ClearTimer(Pair.Value);
		}
	}

	ModifierTimerHandles.Empty();
	ActiveModifiers.Empty();
	RecalculateMaxAttributes();
}

void UShadowSlaveAttributeComponent::OnModifierExpired(FName ModifierId)
{
	RemoveModifier(ModifierId);
}

void UShadowSlaveAttributeComponent::RecalculateMaxAttributes()
{
	float FlatHealth = 0.0f;
	float PercentHealth = 0.0f;

	float FlatStamina = 0.0f;
	float PercentStamina = 0.0f;

	float FlatEssence = 0.0f;
	float PercentEssence = 0.0f;

	for (const FAttributeModifier& Mod : ActiveModifiers)
	{
		switch (Mod.TargetAttribute)
		{
		case EAttributeType::MaxHealth:
		case EAttributeType::Health:
			if (Mod.ModifierType == EAttributeModifierType::Flat) { FlatHealth += Mod.Value; }
			else { PercentHealth += Mod.Value; }
			break;

		case EAttributeType::MaxStamina:
		case EAttributeType::Stamina:
			if (Mod.ModifierType == EAttributeModifierType::Flat) { FlatStamina += Mod.Value; }
			else { PercentStamina += Mod.Value; }
			break;

		case EAttributeType::MaxEssence:
		case EAttributeType::Essence:
			if (Mod.ModifierType == EAttributeModifierType::Flat) { FlatEssence += Mod.Value; }
			else { PercentEssence += Mod.Value; }
			break;
		}
	}

	EffectiveMaxHealth = FMath::Max(1.0f, (AttributeConfig.BaseMaxHealth + FlatHealth) * (1.0f + PercentHealth));
	EffectiveMaxStamina = FMath::Max(0.0f, (AttributeConfig.BaseMaxStamina + FlatStamina) * (1.0f + PercentStamina));
	EffectiveMaxEssence = FMath::Max(0.0f, (AttributeConfig.BaseMaxEssence + FlatEssence) * (1.0f + PercentEssence));

	// Clamp current values within newly calculated maximums
	CurrentHealth = FMath::Clamp(CurrentHealth, 0.0f, EffectiveMaxHealth);
	CurrentStamina = FMath::Clamp(CurrentStamina, 0.0f, EffectiveMaxStamina);
	CurrentEssence = FMath::Clamp(CurrentEssence, 0.0f, EffectiveMaxEssence);

	OnHealthChanged.Broadcast(CurrentHealth, EffectiveMaxHealth);
	OnStaminaChanged.Broadcast(CurrentStamina, EffectiveMaxStamina);
	OnEssenceChanged.Broadcast(CurrentEssence, EffectiveMaxEssence);
}

void UShadowSlaveAttributeComponent::InitializeAttributes(const FAttributeInitConfig& NewConfig)
{
	AttributeConfig = NewConfig;
	RecalculateMaxAttributes();

	CurrentHealth = EffectiveMaxHealth;
	CurrentStamina = EffectiveMaxStamina;
	CurrentEssence = EffectiveMaxEssence;
	bIsDead = false;
}

void UShadowSlaveAttributeComponent::StartStaminaRegenTick()
{
	if (bIsDead || !AttributeConfig.bEnableStaminaRegen)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			StaminaRegenTickHandle,
			this,
			&UShadowSlaveAttributeComponent::TickStaminaRegeneration,
			AttributeConfig.StaminaRegenTickInterval,
			true
		);
	}
}

void UShadowSlaveAttributeComponent::TickStaminaRegeneration()
{
	if (bIsDead || !AttributeConfig.bEnableStaminaRegen)
	{
		StopRegenTimers();
		return;
	}

	if (CurrentStamina >= EffectiveMaxStamina)
	{
		CurrentStamina = EffectiveMaxStamina;
		StopRegenTimers();
		return;
	}

	const float RegenStep = AttributeConfig.StaminaRegenRate * AttributeConfig.StaminaRegenTickInterval;
	CurrentStamina = FMath::Min(CurrentStamina + RegenStep, EffectiveMaxStamina);

	OnStaminaChanged.Broadcast(CurrentStamina, EffectiveMaxStamina);

	if (CurrentStamina >= EffectiveMaxStamina)
	{
		StopRegenTimers();
	}
}

void UShadowSlaveAttributeComponent::StopRegenTimers()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(StaminaRegenDelayHandle);
		World->GetTimerManager().ClearTimer(StaminaRegenTickHandle);
	}
}

void UShadowSlaveAttributeComponent::LogAttributeStatus() const
{
	UE_LOG(LogShadowSlave, Log, TEXT("[%s] Attribute Status - Health: %.1f/%.1f, Stamina: %.1f/%.1f, Essence: %.1f/%.1f, Dead: %s"),
		*GetOwner()->GetName(),
		CurrentHealth, EffectiveMaxHealth,
		CurrentStamina, EffectiveMaxStamina,
		CurrentEssence, EffectiveMaxEssence,
		bIsDead ? TEXT("True") : TEXT("False")
	);
}
