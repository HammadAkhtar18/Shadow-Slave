// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Memories/ShadowSlaveMemoryChronologyTypes.h"
#include "ShadowSlaveMemoryChronologyRegistry.generated.h"

class UShadowSlaveMemoryDefinition;

/**
 * Data-driven registry Primary Data Asset managing canonical Memory acquisition timelines,
 * story arc associations, ownership windows, and confidence ratings.
 * Keeps chronological story metadata completely separate from runtime instance state.
 */
UCLASS(BlueprintType)
class SHADOWSLAVE_API UShadowSlaveMemoryChronologyRegistry : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UShadowSlaveMemoryChronologyRegistry();

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	/* --- Registry Metadata --- */

	/** Unique identifier for this chronology registry */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Chronology")
	FName RegistryId = NAME_None;

	/** Human-readable title of this registry */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Chronology")
	FText RegistryName;

	/** Description and scope documentation */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Chronology", meta = (MultiLine = true))
	FText Description;

	/** Ordered collection of canonical Memory chronology entries */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShadowSlave|Chronology")
	TArray<FShadowSlaveMemoryChronologyEntry> ChronologyEntries;

	/* --- Query API --- */

	/** Returns all entries belonging to a specified story arc */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Chronology|Queries")
	TArray<FShadowSlaveMemoryChronologyEntry> GetEntriesByArc(EShadowSlaveStoryArc Arc) const;

	/** Returns all entries for a specific character (e.g. 'Sunless') */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Chronology|Queries")
	TArray<FShadowSlaveMemoryChronologyEntry> GetEntriesForCharacter(FName CharacterId) const;

	/** Finds an entry by its unique EntryId */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Chronology|Queries")
	bool FindEntryById(FName EntryId, FShadowSlaveMemoryChronologyEntry& OutEntry) const;

	/** Finds the chronology entry referencing the given static Memory definition */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Chronology|Queries")
	bool FindEntryByMemoryDefinition(const UShadowSlaveMemoryDefinition* MemoryDef, FShadowSlaveMemoryChronologyEntry& OutEntry) const;

	/** Returns only entries with 'Verified' canon confidence (for driving canon-critical progression) */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Chronology|Queries")
	TArray<FShadowSlaveMemoryChronologyEntry> GetVerifiedEntries() const;

	/** Returns total count of entries in the registry */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Chronology|Queries")
	int32 GetEntryCount() const { return ChronologyEntries.Num(); }

	/** Outputs formatted chronology entries to the debug log */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Chronology|Debug")
	void LogChronology() const;

	/* --- Baseline Factory --- */

	/**
	 * Creates a verified baseline chronology registry for Sunny covering the First Nightmare,
	 * Forgotten Shore, Chained Isles, and Antarctica narrative arcs.
	 */
	static UShadowSlaveMemoryChronologyRegistry* CreateSunnyBaselineChronologyRegistry(UObject* Outer = nullptr);
};
