// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Progression/ShadowSlaveProgressionTypes.h"
#include "Aspects/ShadowSlaveAspectTypes.h"
#include "ShadowSlaveProgressionStatusWidget.generated.h"

/**
 * Reusable UMG widget presenting high-level progression metrics:
 * Nightmare Spell character rank, soul core cultivation count, and Aspect identity/rank.
 * Purely presentation-driven: queries progression and aspect components directly without calculation.
 */
UCLASS(Abstract)
class SHADOWSLAVE_API UShadowSlaveProgressionStatusWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UShadowSlaveProgressionStatusWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** Updates the displayed character rank */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|UI|Progression")
	void UpdateRank(EShadowSlaveCharacterRank NewRank);

	/** Updates the displayed soul core count and maximum capacity */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|UI|Progression")
	void UpdateSoulCores(int32 InCurrentCores, int32 InMaxCores);

	/** Updates the displayed Aspect title and Aspect rank */
	UFUNCTION(BlueprintCallable, Category = "ShadowSlave|UI|Progression")
	void UpdateAspectInfo(const FText& InAspectName, EShadowSlaveAspectRank InAspectRank);

	UFUNCTION(BlueprintPure, Category = "ShadowSlave|UI|Progression")
	EShadowSlaveCharacterRank GetCharacterRank() const { return CharacterRank; }

	UFUNCTION(BlueprintPure, Category = "ShadowSlave|UI|Progression")
	int32 GetCurrentSoulCores() const { return CurrentSoulCores; }

	UFUNCTION(BlueprintPure, Category = "ShadowSlave|UI|Progression")
	int32 GetMaxSoulCores() const { return MaxSoulCores; }

	UFUNCTION(BlueprintPure, Category = "ShadowSlave|UI|Progression")
	FText GetAspectDisplayName() const { return AspectDisplayName; }

	UFUNCTION(BlueprintPure, Category = "ShadowSlave|UI|Progression")
	EShadowSlaveAspectRank GetAspectRank() const { return AspectRank; }

protected:
	/** Hook for Blueprint UMG implementations when character rank changes */
	UFUNCTION(BlueprintNativeEvent, Category = "ShadowSlave|UI|Progression")
	void OnRankUpdated(EShadowSlaveCharacterRank NewRank);
	virtual void OnRankUpdated_Implementation(EShadowSlaveCharacterRank NewRank);

	/** Hook for Blueprint UMG implementations when soul core count changes */
	UFUNCTION(BlueprintNativeEvent, Category = "ShadowSlave|UI|Progression")
	void OnSoulCoresUpdated(int32 InCurrentCores, int32 InMaxCores);
	virtual void OnSoulCoresUpdated_Implementation(int32 InCurrentCores, int32 InMaxCores);

	/** Hook for Blueprint UMG implementations when Aspect identity or rank changes */
	UFUNCTION(BlueprintNativeEvent, Category = "ShadowSlave|UI|Progression")
	void OnAspectInfoUpdated(const FText& InAspectName, EShadowSlaveAspectRank InAspectRank);
	virtual void OnAspectInfoUpdated_Implementation(const FText& InAspectName, EShadowSlaveAspectRank InAspectRank);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|UI|Progression")
	EShadowSlaveCharacterRank CharacterRank = EShadowSlaveCharacterRank::Unknown;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|UI|Progression")
	int32 CurrentSoulCores = 1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|UI|Progression")
	int32 MaxSoulCores = 1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|UI|Progression")
	FText AspectDisplayName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShadowSlave|UI|Progression")
	EShadowSlaveAspectRank AspectRank = EShadowSlaveAspectRank::Unknown;
};
