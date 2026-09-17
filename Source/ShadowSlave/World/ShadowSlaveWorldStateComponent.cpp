// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/ShadowSlaveWorldStateComponent.h"
#include "GameFramework/Actor.h"
#include "ShadowSlave.h"

UShadowSlaveWorldStateComponent::UShadowSlaveWorldStateComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

bool UShadowSlaveWorldStateComponent::SetStateValue(FName Key, const FShadowSlaveWorldValue& Value)
{
	if (Key.IsNone() || !Value.IsValid())
	{
		return false;
	}

	const FShadowSlaveWorldValue* Existing = StateEntries.Find(Key);
	if (Existing && *Existing == Value)
	{
		// Idempotent: identical value, do not emit redundant event
		return true;
	}

	const FShadowSlaveWorldValue OldValue = Existing ? *Existing : FShadowSlaveWorldValue();
	StateEntries.Add(Key, Value);

	OnWorldStateChanged.Broadcast(Key, Value, OldValue, GetOwner());
	return true;
}

bool UShadowSlaveWorldStateComponent::SetBoolValue(FName Key, bool Value)
{
	return SetStateValue(Key, FShadowSlaveWorldValue::MakeBool(Value));
}

bool UShadowSlaveWorldStateComponent::SetIntValue(FName Key, int32 Value)
{
	return SetStateValue(Key, FShadowSlaveWorldValue::MakeInt(Value));
}

bool UShadowSlaveWorldStateComponent::SetFloatValue(FName Key, float Value)
{
	return SetStateValue(Key, FShadowSlaveWorldValue::MakeFloat(Value));
}

bool UShadowSlaveWorldStateComponent::SetStringValue(FName Key, const FString& Value)
{
	return SetStateValue(Key, FShadowSlaveWorldValue::MakeString(Value));
}

bool UShadowSlaveWorldStateComponent::SetNameValue(FName Key, FName Value)
{
	return SetStateValue(Key, FShadowSlaveWorldValue::MakeName(Value));
}

bool UShadowSlaveWorldStateComponent::GetStateValue(FName Key, FShadowSlaveWorldValue& OutValue) const
{
	if (const FShadowSlaveWorldValue* Found = StateEntries.Find(Key))
	{
		OutValue = *Found;
		return true;
	}

	OutValue = FShadowSlaveWorldValue();
	return false;
}

bool UShadowSlaveWorldStateComponent::GetBoolValue(FName Key, bool DefaultValue) const
{
	if (const FShadowSlaveWorldValue* Found = StateEntries.Find(Key))
	{
		return Found->AsBool(DefaultValue);
	}
	return DefaultValue;
}

int32 UShadowSlaveWorldStateComponent::GetIntValue(FName Key, int32 DefaultValue) const
{
	if (const FShadowSlaveWorldValue* Found = StateEntries.Find(Key))
	{
		return Found->AsInt(DefaultValue);
	}
	return DefaultValue;
}

float UShadowSlaveWorldStateComponent::GetFloatValue(FName Key, float DefaultValue) const
{
	if (const FShadowSlaveWorldValue* Found = StateEntries.Find(Key))
	{
		return Found->AsFloat(DefaultValue);
	}
	return DefaultValue;
}

FString UShadowSlaveWorldStateComponent::GetStringValue(FName Key, const FString& DefaultValue) const
{
	if (const FShadowSlaveWorldValue* Found = StateEntries.Find(Key))
	{
		return Found->AsString(DefaultValue);
	}
	return DefaultValue;
}

FName UShadowSlaveWorldStateComponent::GetNameValue(FName Key, FName DefaultValue) const
{
	if (const FShadowSlaveWorldValue* Found = StateEntries.Find(Key))
	{
		return Found->AsName(DefaultValue);
	}
	return DefaultValue;
}

bool UShadowSlaveWorldStateComponent::HasStateValue(FName Key) const
{
	return StateEntries.Contains(Key);
}

bool UShadowSlaveWorldStateComponent::RemoveStateValue(FName Key)
{
	if (Key.IsNone())
	{
		return false;
	}

	FShadowSlaveWorldValue RemovedValue;
	if (StateEntries.RemoveAndCopyValue(Key, RemovedValue))
	{
		OnWorldStateChanged.Broadcast(Key, FShadowSlaveWorldValue(), RemovedValue, GetOwner());
		return true;
	}

	return false;
}

void UShadowSlaveWorldStateComponent::ClearState()
{
	if (StateEntries.Num() == 0)
	{
		return;
	}

	const TMap<FName, FShadowSlaveWorldValue> Previous = StateEntries;
	StateEntries.Empty();

	for (const auto& Pair : Previous)
	{
		OnWorldStateChanged.Broadcast(Pair.Key, FShadowSlaveWorldValue(), Pair.Value, GetOwner());
	}
}

TArray<FName> UShadowSlaveWorldStateComponent::GetAllStateKeys() const
{
	TArray<FName> Keys;
	StateEntries.GetKeys(Keys);
	return Keys;
}

FShadowSlaveWorldStateSnapshot UShadowSlaveWorldStateComponent::CaptureSnapshot() const
{
	FShadowSlaveWorldStateSnapshot Snapshot;
	Snapshot.PersistentId = PersistentStateId;
	Snapshot.States = StateEntries;
	Snapshot.Metadata = ComponentMetadata;
	Snapshot.bIsValid = !PersistentStateId.IsNone();
	return Snapshot;
}

bool UShadowSlaveWorldStateComponent::RestoreSnapshot(const FShadowSlaveWorldStateSnapshot& InSnapshot)
{
	if (!InSnapshot.bIsValid || InSnapshot.PersistentId.IsNone() || InSnapshot.PersistentId != PersistentStateId)
	{
		return false;
	}

	StateEntries = InSnapshot.States;
	ComponentMetadata = InSnapshot.Metadata;
	return true;
}

bool UShadowSlaveWorldStateComponent::CaptureSaveRecord(FShadowSlaveWorldActorSaveRecord& OutRecord) const
{
	if (PersistentStateId.IsNone())
	{
		return false;
	}

	OutRecord.PersistentId = PersistentStateId;
	if (const AActor* Owner = GetOwner())
	{
		OutRecord.ActorClass = Owner->GetClass();
		OutRecord.ActorTransform = Owner->GetActorTransform();
		OutRecord.bIsActive = !Owner->IsHidden();
	}
	else
	{
		OutRecord.ActorClass = nullptr;
		OutRecord.ActorTransform = FTransform::Identity;
		OutRecord.bIsActive = true;
	}

	OutRecord.CustomStateData.Empty();
	for (const auto& Pair : StateEntries)
	{
		OutRecord.CustomStateData.Add(Pair.Key, Pair.Value.ToString());
	}

	return true;
}

bool UShadowSlaveWorldStateComponent::RestoreSaveRecord(const FShadowSlaveWorldActorSaveRecord& InRecord)
{
	if (PersistentStateId.IsNone() || InRecord.PersistentId != PersistentStateId)
	{
		return false;
	}

	StateEntries.Empty();
	for (const auto& Pair : InRecord.CustomStateData)
	{
		StateEntries.Add(Pair.Key, FShadowSlaveWorldValue::FromString(Pair.Value));
	}

	if (AActor* Owner = GetOwner())
	{
		Owner->SetActorTransform(InRecord.ActorTransform);
		Owner->SetActorHiddenInGame(!InRecord.bIsActive);
		Owner->SetActorEnableCollision(InRecord.bIsActive);
	}

	return true;
}
