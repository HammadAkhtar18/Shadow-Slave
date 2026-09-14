// Copyright Epic Games, Inc. All Rights Reserved.

#include "Nightmares/ShadowSlaveNightmareScenarioDefinition.h"
#include "UObject/Package.h"

UShadowSlaveNightmareScenarioDefinition::UShadowSlaveNightmareScenarioDefinition()
{
	ScenarioId = NAME_None;
	DisplayName = FText::GetEmpty();
	Description = FText::GetEmpty();
	ScenarioVersion = 1;
	CompletionRule = EShadowSlaveScenarioCompletionRule::AllRequiredObjectives;
}

FPrimaryAssetId UShadowSlaveNightmareScenarioDefinition::GetPrimaryAssetId() const
{
	const FName AssetIdentifier = ScenarioId.IsNone() ? GetFName() : ScenarioId;
	return FPrimaryAssetId(TEXT("NightmareScenario"), AssetIdentifier);
}

bool UShadowSlaveNightmareScenarioDefinition::ValidateScenario(FText& OutError) const
{
	if (ScenarioId.IsNone())
	{
		OutError = FText::FromString(TEXT("Validation Failed: ScenarioId cannot be None."));
		return false;
	}

	if (ScenarioVersion <= 0)
	{
		OutError = FText::FromString(TEXT("Validation Failed: ScenarioVersion must be greater than zero."));
		return false;
	}

	if (DisplayName.IsEmpty())
	{
		OutError = FText::FromString(TEXT("Validation Failed: DisplayName cannot be empty."));
		return false;
	}

	TSet<FName> SeenObjectiveIds;
	int32 RequiredObjectiveCount = 0;

	for (int32 Index = 0; Index < Objectives.Num(); ++Index)
	{
		const FShadowSlaveNightmareObjectiveDefinition& Obj = Objectives[Index];

		if (Obj.ObjectiveId.IsNone())
		{
			OutError = FText::Format(
				FText::FromString(TEXT("Validation Failed: Objective at index {0} has None as ObjectiveId.")),
				FText::AsNumber(Index)
			);
			return false;
		}

		if (Obj.DisplayName.IsEmpty())
		{
			OutError = FText::Format(
				FText::FromString(TEXT("Validation Failed: Objective '{0}' has an empty DisplayName.")),
				FText::FromName(Obj.ObjectiveId)
			);
			return false;
		}

		if (Obj.TargetProgress <= 0.0f)
		{
			OutError = FText::Format(
				FText::FromString(TEXT("Validation Failed: Objective '{0}' must have TargetProgress greater than zero.")),
				FText::FromName(Obj.ObjectiveId)
			);
			return false;
		}

		if (SeenObjectiveIds.Contains(Obj.ObjectiveId))
		{
			OutError = FText::Format(
				FText::FromString(TEXT("Validation Failed: Duplicate ObjectiveId '{0}' detected.")),
				FText::FromName(Obj.ObjectiveId)
			);
			return false;
		}

		SeenObjectiveIds.Add(Obj.ObjectiveId);

		if (Obj.bIsRequired)
		{
			++RequiredObjectiveCount;
		}
	}

	if ((CompletionRule == EShadowSlaveScenarioCompletionRule::AllRequiredObjectives ||
		 CompletionRule == EShadowSlaveScenarioCompletionRule::AnyRequiredObjective) &&
		RequiredObjectiveCount == 0 && Objectives.Num() > 0)
	{
		OutError = FText::FromString(TEXT("Validation Failed: Scenario has objectives and requires completion, but zero required objectives are defined."));
		return false;
	}

	OutError = FText::GetEmpty();
	return true;
}

const FShadowSlaveNightmareObjectiveDefinition* UShadowSlaveNightmareScenarioDefinition::FindObjectiveDefinition(FName InObjectiveId) const
{
	if (InObjectiveId.IsNone())
	{
		return nullptr;
	}

	for (const FShadowSlaveNightmareObjectiveDefinition& Obj : Objectives)
	{
		if (Obj.ObjectiveId == InObjectiveId)
		{
			return &Obj;
		}
	}

	return nullptr;
}

UShadowSlaveNightmareScenarioDefinition* UShadowSlaveNightmareScenarioDefinition::CreateTestScenarioDefinition(UObject* Outer)
{
	UObject* EffectiveOuter = Outer ? Outer : GetTransientPackage();
	UShadowSlaveNightmareScenarioDefinition* TestDef = NewObject<UShadowSlaveNightmareScenarioDefinition>(EffectiveOuter);

	TestDef->ScenarioId = FName(TEXT("Scenario_Dev_Test"));
	TestDef->DisplayName = FText::FromString(TEXT("Development Test Scenario"));
	TestDef->Description = FText::FromString(TEXT("Generic test scenario used for headless validation of session and objective state machines."));
	TestDef->ScenarioVersion = 1;
	TestDef->CompletionRule = EShadowSlaveScenarioCompletionRule::AllRequiredObjectives;

	// Objective 1: Generic navigation objective
	FShadowSlaveNightmareObjectiveDefinition Obj1(
		FName(TEXT("Obj_Test_ReachWaypoint")),
		FText::FromString(TEXT("Reach Designated Waypoint")),
		FText::FromString(TEXT("Navigate to the designated tactical rally point.")),
		EShadowSlaveNightmareObjectiveType::ReachLocation,
		true,
		1.0f
	);
	TestDef->Objectives.Add(Obj1);

	// Objective 2: Generic interaction/collection objective
	FShadowSlaveNightmareObjectiveDefinition Obj2(
		FName(TEXT("Obj_Test_SecureArtifact")),
		FText::FromString(TEXT("Secure Test Artifact")),
		FText::FromString(TEXT("Locate and secure the test artifact.")),
		EShadowSlaveNightmareObjectiveType::Interact,
		true,
		1.0f
	);
	TestDef->Objectives.Add(Obj2);

	// Objective 3: Optional secondary objective
	FShadowSlaveNightmareObjectiveDefinition Obj3(
		FName(TEXT("Obj_Test_OptionalSurvey")),
		FText::FromString(TEXT("Survey Perimeter")),
		FText::FromString(TEXT("Perform optional environmental survey.")),
		EShadowSlaveNightmareObjectiveType::ReachLocation,
		false,
		3.0f
	);
	TestDef->Objectives.Add(Obj3);

	return TestDef;
}
