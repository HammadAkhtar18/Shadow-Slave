// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ShadowSlaveWorldTypes.generated.h"

class UWorld;

/**
 * Supported underlying primitive and technical types for generic world state.
 */
UENUM(BlueprintType)
enum class EShadowSlaveWorldValueType : uint8
{
	None   UMETA(DisplayName = "None"),
	Bool   UMETA(DisplayName = "Boolean"),
	Int    UMETA(DisplayName = "Integer"),
	Float  UMETA(DisplayName = "Float"),
	String UMETA(DisplayName = "String"),
	Name   UMETA(DisplayName = "Name")
};

/**
 * Generic, serialization-friendly value container for world-state variables.
 * Stores primitive and string-like values without creating a bloated UObject property system.
 */
USTRUCT(BlueprintType)
struct SHADOWSLAVE_API FShadowSlaveWorldValue
{
	GENERATED_BODY()

	/** The active type contained in this value wrapper */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|World")
	EShadowSlaveWorldValueType ValueType = EShadowSlaveWorldValueType::None;

	/** Boolean payload */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|World")
	bool BoolValue = false;

	/** Integer payload */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|World")
	int32 IntValue = 0;

	/** Floating-point payload */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|World")
	float FloatValue = 0.0f;

	/** String payload */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|World")
	FString StringValue;

	/** Name / technical identifier payload */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|World")
	FName NameValue = NAME_None;

	FShadowSlaveWorldValue() = default;

	explicit FShadowSlaveWorldValue(bool InVal)
		: ValueType(EShadowSlaveWorldValueType::Bool), BoolValue(InVal)
	{
	}

	explicit FShadowSlaveWorldValue(int32 InVal)
		: ValueType(EShadowSlaveWorldValueType::Int), IntValue(InVal)
	{
	}

	explicit FShadowSlaveWorldValue(float InVal)
		: ValueType(EShadowSlaveWorldValueType::Float), FloatValue(InVal)
	{
	}

	explicit FShadowSlaveWorldValue(const FString& InVal)
		: ValueType(EShadowSlaveWorldValueType::String), StringValue(InVal)
	{
	}

	explicit FShadowSlaveWorldValue(FName InVal)
		: ValueType(EShadowSlaveWorldValueType::Name), NameValue(InVal)
	{
	}

	static FShadowSlaveWorldValue MakeBool(bool InVal) { return FShadowSlaveWorldValue(InVal); }
	static FShadowSlaveWorldValue MakeInt(int32 InVal) { return FShadowSlaveWorldValue(InVal); }
	static FShadowSlaveWorldValue MakeFloat(float InVal) { return FShadowSlaveWorldValue(InVal); }
	static FShadowSlaveWorldValue MakeString(const FString& InVal) { return FShadowSlaveWorldValue(InVal); }
	static FShadowSlaveWorldValue MakeName(FName InVal) { return FShadowSlaveWorldValue(InVal); }

	bool AsBool(bool DefaultValue = false) const
	{
		return (ValueType == EShadowSlaveWorldValueType::Bool) ? BoolValue : DefaultValue;
	}

	int32 AsInt(int32 DefaultValue = 0) const
	{
		return (ValueType == EShadowSlaveWorldValueType::Int) ? IntValue : DefaultValue;
	}

	float AsFloat(float DefaultValue = 0.0f) const
	{
		return (ValueType == EShadowSlaveWorldValueType::Float) ? FloatValue : DefaultValue;
	}

	FString AsString(const FString& DefaultValue = TEXT("")) const
	{
		return (ValueType == EShadowSlaveWorldValueType::String) ? StringValue : DefaultValue;
	}

	FName AsName(FName DefaultValue = NAME_None) const
	{
		return (ValueType == EShadowSlaveWorldValueType::Name) ? NameValue : DefaultValue;
	}

	bool IsValid() const
	{
		return ValueType != EShadowSlaveWorldValueType::None;
	}

	bool operator==(const FShadowSlaveWorldValue& Other) const
	{
		if (ValueType != Other.ValueType)
		{
			return false;
		}

		switch (ValueType)
		{
		case EShadowSlaveWorldValueType::Bool:
			return BoolValue == Other.BoolValue;
		case EShadowSlaveWorldValueType::Int:
			return IntValue == Other.IntValue;
		case EShadowSlaveWorldValueType::Float:
			return FMath::IsNearlyEqual(FloatValue, Other.FloatValue);
		case EShadowSlaveWorldValueType::String:
			return StringValue.Equals(Other.StringValue, ESearchCase::CaseSensitive);
		case EShadowSlaveWorldValueType::Name:
			return NameValue == Other.NameValue;
		default:
			return true;
		}
	}

	bool operator!=(const FShadowSlaveWorldValue& Other) const
	{
		return !(*this == Other);
	}

	FString ToString() const
	{
		switch (ValueType)
		{
		case EShadowSlaveWorldValueType::Bool:
			return FString::Printf(TEXT("b:%s"), BoolValue ? TEXT("1") : TEXT("0"));
		case EShadowSlaveWorldValueType::Int:
			return FString::Printf(TEXT("i:%d"), IntValue);
		case EShadowSlaveWorldValueType::Float:
			return FString::Printf(TEXT("f:%f"), FloatValue);
		case EShadowSlaveWorldValueType::String:
			return FString::Printf(TEXT("s:%s"), *StringValue);
		case EShadowSlaveWorldValueType::Name:
			return FString::Printf(TEXT("n:%s"), *NameValue.ToString());
		default:
			return TEXT("none");
		}
	}

	static FShadowSlaveWorldValue FromString(const FString& InSerialized)
	{
		if (InSerialized.IsEmpty() || InSerialized.Equals(TEXT("none"), ESearchCase::IgnoreCase))
		{
			return FShadowSlaveWorldValue();
		}

		if (InSerialized.Len() >= 2 && InSerialized[1] == TEXT(':'))
		{
			const TCHAR Prefix = InSerialized[0];
			const FString Payload = InSerialized.RightChop(2);

			switch (Prefix)
			{
			case TEXT('b'):
				return FShadowSlaveWorldValue(Payload.Equals(TEXT("1")) || Payload.Equals(TEXT("true"), ESearchCase::IgnoreCase));
			case TEXT('i'):
				return FShadowSlaveWorldValue(FCString::Atoi(*Payload));
			case TEXT('f'):
				return FShadowSlaveWorldValue(FCString::Atof(*Payload));
			case TEXT('s'):
				return FShadowSlaveWorldValue(Payload);
			case TEXT('n'):
				return FShadowSlaveWorldValue(FName(*Payload));
			default:
				break;
			}
		}

		// Fallback heuristic if no prefix present
		if (InSerialized.Equals(TEXT("true"), ESearchCase::IgnoreCase) || InSerialized.Equals(TEXT("false"), ESearchCase::IgnoreCase))
		{
			return FShadowSlaveWorldValue(InSerialized.Equals(TEXT("true"), ESearchCase::IgnoreCase));
		}
		if (InSerialized.IsNumeric())
		{
			if (InSerialized.Contains(TEXT(".")))
			{
				return FShadowSlaveWorldValue(FCString::Atof(*InSerialized));
			}
			return FShadowSlaveWorldValue(FCString::Atoi(*InSerialized));
		}

		return FShadowSlaveWorldValue(InSerialized);
	}
};

/**
 * Individual world-state record containing key, value, and optional metadata.
 */
USTRUCT(BlueprintType)
struct SHADOWSLAVE_API FShadowSlaveWorldStateRecord
{
	GENERATED_BODY()

	/** Stable key for this state entry */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|World")
	FName Key = NAME_None;

	/** Stored value */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|World")
	FShadowSlaveWorldValue Value;

	/** Optional technical metadata for this state entry */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|World")
	TMap<FName, FString> Metadata;

	bool IsValid() const
	{
		return !Key.IsNone() && Value.IsValid();
	}
};

/**
 * Snapshot of a persistent entity's complete world state.
 */
USTRUCT(BlueprintType)
struct SHADOWSLAVE_API FShadowSlaveWorldStateSnapshot
{
	GENERATED_BODY()

	/** Persistent ID of the owning entity/actor */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|World")
	FName PersistentId = NAME_None;

	/** Collection of state entries mapped by Key */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|World")
	TMap<FName, FShadowSlaveWorldValue> States;

	/** Optional metadata associated with this snapshot */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|World")
	TMap<FName, FString> Metadata;

	/** Validity flag */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|World")
	bool bIsValid = false;
};

/**
 * Lightweight transition boundary request for future world/level travel.
 * Architectural boundary only: does not execute level loading, streaming, or map travel.
 */
USTRUCT(BlueprintType)
struct SHADOWSLAVE_API FShadowSlaveLevelTransitionRequest
{
	GENERATED_BODY()

	/** Stable identifier of destination world/level/area */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|World")
	FName DestinationLevelId = NAME_None;

	/** Target spawn point or entrance tag within destination level */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|World")
	FName TargetSpawnPointId = NAME_None;

	/** Optional soft reference to the target level package */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|World")
	TSoftObjectPtr<UWorld> TargetLevel;

	/** Optional transition metadata (fade durations, narrative flags, etc.) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|World")
	TMap<FName, FString> TransitionMetadata;

	bool IsValid() const
	{
		return !DestinationLevelId.IsNone() || !TargetLevel.IsNull();
	}
};
