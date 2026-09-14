// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Progression/ShadowSlaveProgressionTypes.h"
#include "ShadowSlaveProgressionComponent.generated.h"

class UShadowSlaveAttributeComponent;

/**
 * Reusable Actor Component managing an entity's character progression:
 * Nightmare Spell character rank, soul core count, and advancement hooks.
 * Designed event-driven without tick overhead.
 *
 * NOTE ON ATTRIBUTES & ESSENCE:
 * This component does NOT manage or duplicate Health, Stamina, or Essence pools.
 * All resource storage and modification remains strictly within UShadowSlaveAttributeComponent.
 */
UCLASS(ClassGroup = (ShadowSlave), meta = (BlueprintSpawnableComponent))
class SHADOWSLAVE_API UShadowSlaveProgressionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UShadowSlaveProgressionComponent();

	/* --- Character Rank API --- */

	/** Returns the current character rank */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Progression|Rank")
	EShadowSlaveCharacterRank GetCharacterRank() const { return CharacterRank; }

	/**
	 * Sets the character rank directly.
	 * Broadcasts OnCharacterRankChanged if the rank changed.
	 * Returns true if rank was successfully updated.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Progression|Rank")
	bool SetCharacterRank(EShadowSlaveCharacterRank NewRank);

	/** Returns whether this character has a verified/known rank */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Progression|Rank")
	bool HasKnownRank() const { return CharacterRank != EShadowSlaveCharacterRank::Unknown; }

	/**
	 * Extensibility hook checking whether this entity can advance to a target rank.
	 * Returns true if target rank is valid and higher than current rank.
	 * Does not enforce arbitrary XP/stat requirements.
	 */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Progression|Rank")
	bool CanAdvanceRank(EShadowSlaveCharacterRank TargetRank) const;

	/**
	 * Explicit advancement hook invoked when story/Nightmare trial completes.
	 * Returns true if rank was successfully advanced.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Progression|Rank")
	bool AdvanceRank(EShadowSlaveCharacterRank TargetRank);

	/* --- Soul Core API --- */

	/** Returns current active soul core count */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Progression|SoulCores")
	int32 GetSoulCoreCount() const { return SoulCoreState.CurrentSoulCores; }

	/** Sets soul core count clamped between 0 and MaximumSoulCores */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Progression|SoulCores")
	bool SetSoulCoreCount(int32 NewCount);

	/** Returns maximum soul cores cultivatable by this entity */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Progression|SoulCores")
	int32 GetMaxSoulCores() const { return SoulCoreState.MaximumSoulCores; }

	/** Sets maximum soul core limit (clamped to at least 1) */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Progression|SoulCores")
	bool SetMaxSoulCores(int32 NewMax);

	/** Increments soul core count by Count (clamped to MaximumSoulCores) */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Progression|SoulCores")
	bool AddSoulCore(int32 Count = 1);

	/** Decrements soul core count by Count (clamped to at least 0) */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Progression|SoulCores")
	bool RemoveSoulCore(int32 Count = 1);

	/** Returns copy of current soul core state struct */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Progression|SoulCores")
	FShadowSlaveSoulCoreState GetSoulCoreState() const { return SoulCoreState; }

	/* --- Progression Metadata --- */

	/** Sets arbitrary progression key-value metadata for quest/story hooks */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Progression|Metadata")
	void SetProgressionMetadata(FName Key, const FString& Value);

	/** Retrieves progression key-value metadata */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Progression|Metadata")
	bool GetProgressionMetadata(FName Key, FString& OutValue) const;

	/** Removes progression key-value metadata */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Progression|Metadata")
	bool RemoveProgressionMetadata(FName Key);

	/* --- Attribute Integration --- */

	/** Safe helper to locate owning actor's attribute component without duplicate ownership */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Progression|Attributes")
	UShadowSlaveAttributeComponent* GetAttributeComponent() const;

	/* --- Event Delegates --- */

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Progression|Events")
	FOnCharacterRankChangedSignature OnCharacterRankChanged;

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Progression|Events")
	FOnSoulCoreCountChangedSignature OnSoulCoreCountChanged;

	UPROPERTY(BlueprintAssignable, Category = "ShadowSlave|Progression|Events")
	FOnMaxSoulCoresChangedSignature OnMaxSoulCoresChanged;

protected:
	/** Nightmare Spell character rank */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Progression|State")
	EShadowSlaveCharacterRank CharacterRank = EShadowSlaveCharacterRank::Unknown;

	/** Soul core configuration state */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ShadowSlave|Progression|State")
	FShadowSlaveSoulCoreState SoulCoreState;

	/** Extensible metadata key-value storage for advancement tracking */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShadowSlave|Progression|State")
	TMap<FName, FString> ProgressionMetadata;
};
