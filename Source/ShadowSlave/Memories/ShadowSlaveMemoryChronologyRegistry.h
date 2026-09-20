// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Content/ShadowSlaveContentDefinition.h"
#include "Memories/ShadowSlaveMemoryChronologyTypes.h"
#include "ShadowSlaveMemoryChronologyRegistry.generated.h"

class UShadowSlaveMemoryDefinition;

/**
 * Data-driven chronology catalog managing canonical Memory acquisition timelines,
 * story arc associations, ownership windows, and confidence ratings.
 * Specialization of UShadowSlaveContentDefinition for the generic content pipeline.
 *
 * ARCHITECTURAL PRINCIPLES:
 * 1. SPECIALIZED CONTENT DEFINITION: MemoryChronologyRegistry is a specialized static content
 *    definition deriving from UShadowSlaveContentDefinition. It is a chronology/timeline catalog,
 *    not a Memory archetype and not a replacement for UShadowSlaveContentRegistrySubsystem.
 * 2. GENERIC CONTENT CLASSIFICATION: Generic ContentType is EShadowSlaveContentType::Custom because
 *    the generic content pipeline taxonomy currently does not define a dedicated Chronology member.
 * 3. SPECIALIZED PRIMARY ASSET TYPE: Utilizes the GetCustomPrimaryAssetType() hook to produce
 *    deterministic PrimaryAssetId with PrimaryAssetType "MemoryChronologyRegistry":
 *    FPrimaryAssetId(TEXT("MemoryChronologyRegistry"), ContentId).
 * 4. SINGLE AUTHORITATIVE ID: ContentId is the sole stored stable identifier. GetRegistryId() and
 *    SetRegistryId() provide backward-compatible accessors without duplicate storage.
 * 5. TITLE MAPPING: RegistryName was a duplicate human-readable title of DisplayName. Title access
 *    is provided through inherited DisplayName and GetRegistryName().
 * 6. RUNTIME STATE SEPARATION: Chronological story metadata remains completely separate from
 *    Memory runtime instance state owned by UShadowSlaveMemoryComponent.
 * 7. CONTENT REGISTRY ROLE: UShadowSlaveContentRegistrySubsystem provides generic authored-definition
 *    lookup only. This catalog remains the Memory chronology/timeline table and is not merged into
 *    the generic content registry.
 */
UCLASS(BlueprintType)
class SHADOWSLAVE_API UShadowSlaveMemoryChronologyRegistry : public UShadowSlaveContentDefinition
{
	GENERATED_BODY()

public:
	UShadowSlaveMemoryChronologyRegistry();

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	/**
	 * Validates definition configuration.
	 * Combines base generic content validation (valid ContentId, valid DisplayName, Version >= 1, ContentType == Custom).
	 */
	virtual bool IsValidDefinition(FString* OutErrorMessage = nullptr) const override;

	/* --- Identification Compatibility Accessors --- */

	/** Compatibility accessor returning authoritative ContentId as RegistryId */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Chronology|Identity")
	FName GetRegistryId() const { return ContentId; }

	/** Compatibility accessor setting authoritative ContentId as RegistryId */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|Chronology|Identity")
	void SetRegistryId(FName InRegistryId) { ContentId = InRegistryId; }

	/** Compatibility accessor returning inherited DisplayName as RegistryName */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|Chronology|Identity")
	FText GetRegistryName() const { return DisplayName; }

	/* --- Chronology Data --- */

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

protected:
	/** Hook returning specialized PrimaryAssetType "MemoryChronologyRegistry" for ContentType == Custom */
	virtual FName GetCustomPrimaryAssetType() const override { return TEXT("MemoryChronologyRegistry"); }
};
