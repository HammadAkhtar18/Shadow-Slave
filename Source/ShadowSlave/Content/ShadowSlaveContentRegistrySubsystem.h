// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Content/ShadowSlaveContentTypes.h"
#include "ShadowSlaveContentRegistrySubsystem.generated.h"

class UShadowSlaveContentDefinition;

/**
 * Generic GameInstance subsystem providing a central registry for static content definitions.
 *
 * ARCHITECTURAL PRINCIPLES:
 * 1. STATIC CONTENT REGISTRY: The registry manages static content definitions (Data Assets).
 *    It is NOT a gameplay-state store; mutable runtime state belongs exclusively in runtime components.
 * 2. STABLE IDENTIFIERS: Content is registered and queried via stable FName identifiers (ContentId).
 * 3. SOFT ASSET REFERENCES: Content definitions are referenced via TSoftObjectPtr / FPrimaryAssetId.
 *    Assets do not need to be loaded into memory simultaneously, preventing memory bloat.
 * 4. NO MASSIVE OBJECT SCANNING: Lookups are O(1) via hash map; no arbitrary UObject universe scanning
 *    or FindObject(ANY_PACKAGE, ...) is performed.
 * 5. SAFE RESOLUTION: Resolution is null-safe. Synchronous loading is strictly opt-in (bAllowSynchronousLoad)
 *    and never forced implicitly during gameplay lookups.
 * 6. DECOUPLED LIFETIME: As a UGameInstanceSubsystem, its lifecycle is tied to the game instance,
 *    avoiding mutable global singletons while remaining cleanly instantiable in unit tests.
 */
UCLASS()
class SHADOWSLAVE_API UShadowSlaveContentRegistrySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UShadowSlaveContentRegistrySubsystem();

	// ~USubsystem interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	// ~End USubsystem interface

	/* --- Registration --- */

	/**
	 * Registers a content entry by soft reference or pre-loaded definition.
	 * Returns true if registered successfully, false if duplicate ID or invalid entry.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|ContentRegistry")
	bool RegisterEntry(const FShadowSlaveContentRegistryEntry& Entry);

	/**
	 * Registers a pre-loaded in-memory content definition.
	 * Automatically extracts ContentId, ContentType, and soft reference.
	 * Returns true if registered successfully, false if duplicate ID or invalid definition.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|ContentRegistry")
	bool RegisterDefinition(UShadowSlaveContentDefinition* Definition);

	/**
	 * Unregisters an entry by stable content ID.
	 * Returns true if found and removed, false if not found.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|ContentRegistry")
	bool UnregisterEntry(FName ContentId);

	/**
	 * Clears all registered entries and category indices.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|ContentRegistry")
	void ClearRegistry();

	/* --- Query & Resolution --- */

	/** Returns true if an entry exists for the given stable ContentId */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|ContentRegistry")
	bool HasContent(FName ContentId) const;

	/** Finds the registry entry for the given stable ContentId */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|ContentRegistry")
	bool FindEntry(FName ContentId, FShadowSlaveContentRegistryEntry& OutEntry) const;

	/**
	 * Resolves the content definition for the given ContentId.
	 * If bAllowSynchronousLoad is false, returns the loaded definition if already in memory, or nullptr.
	 * If bAllowSynchronousLoad is true, loads the asset synchronously if not already loaded.
	 * Returns nullptr if the ID is unknown or resolution fails.
	 */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|ContentRegistry")
	UShadowSlaveContentDefinition* ResolveContentDefinition(FName ContentId, bool bAllowSynchronousLoad = false) const;

	/** Templated convenience helper for resolving to specific definition subclasses */
	template<typename T>
	T* ResolveContentDefinition(FName ContentId, bool bAllowSynchronousLoad = false) const
	{
		return Cast<T>(ResolveContentDefinition(ContentId, bAllowSynchronousLoad));
	}

	/** Checks whether the definition for a registered ContentId is currently loaded in memory */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|ContentRegistry")
	bool IsDefinitionLoaded(FName ContentId) const;

	/** Returns the total number of registered entries */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|ContentRegistry")
	int32 GetRegisteredContentCount() const;

	/** Returns all registered stable ContentIds */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|ContentRegistry")
	void GetAllRegisteredContentIds(TArray<FName>& OutContentIds) const;

	/** Returns the number of registered entries for a specific content type */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|ContentRegistry")
	int32 GetRegisteredContentCountByType(EShadowSlaveContentType Type) const;

	/** Returns all registered stable ContentIds for a specific content type */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|ContentRegistry")
	void GetContentIdsByType(EShadowSlaveContentType Type, TArray<FName>& OutContentIds) const;

	/** Returns all registered entries for a specific content type */
	UFUNCTION(BlueprintPure, Category = "ShadowSlave|ContentRegistry")
	TArray<FShadowSlaveContentRegistryEntry> GetEntriesByType(EShadowSlaveContentType Type) const;

private:
	/** Map of stable ContentId to registry entry */
	UPROPERTY(Transient)
	TMap<FName, FShadowSlaveContentRegistryEntry> RegistryEntries;

	/** Secondary lookup index mapping ContentType to array of ContentIds */
	UPROPERTY(Transient)
	TMap<EShadowSlaveContentType, TArray<FName>> EntriesByType;
};
