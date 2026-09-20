// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_AUTOMATION_TESTS

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Nightmares/ShadowSlaveNightmareScenarioDefinition.h"
#include "Nightmares/ShadowSlaveNightmareSubsystem.h"
#include "Nightmares/ShadowSlaveNightmareObjectiveTracker.h"
#include "Nightmares/ShadowSlaveNightmareTypes.h"
#include "Nightmares/ShadowSlaveNightmareObjectiveTypes.h"
#include "Nightmares/ShadowSlaveNightmareSaveTypes.h"
#include "Content/ShadowSlaveContentRegistrySubsystem.h"

// -----------------------------------------------------------------------------
// Step 47: Nightmare Scenario Definition Content Pipeline Integration Tests
// -----------------------------------------------------------------------------

// 1. DefinitionUsesGenericContentBase Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveNightmareScenarioDefinitionGenericBaseTest,
	"ShadowSlave.NightmareScenarioDefinition.DefinitionUsesGenericContentBase",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveNightmareScenarioDefinitionGenericBaseTest::RunTest(const FString& Parameters)
{
	UShadowSlaveNightmareScenarioDefinition* ScenarioDef = NewObject<UShadowSlaveNightmareScenarioDefinition>();
	TestNotNull(TEXT("Scenario definition must instantiate"), ScenarioDef);
	if (!ScenarioDef)
	{
		return false;
	}

	// 1. Must inherit from UShadowSlaveContentDefinition
	UShadowSlaveContentDefinition* ContentBase = Cast<UShadowSlaveContentDefinition>(ScenarioDef);
	TestNotNull(TEXT("UShadowSlaveNightmareScenarioDefinition must inherit from UShadowSlaveContentDefinition"), ContentBase);

	// 2. Base content fields must be accessible and correctly initialized
	TestEqual(TEXT("Initial ContentId must be NAME_None"), ScenarioDef->ContentId, NAME_None);
	TestEqual(TEXT("Initial ContentType must be Custom"), ScenarioDef->ContentType, EShadowSlaveContentType::Custom);
	TestEqual(TEXT("Initial Version must be 1"), ScenarioDef->Version, 1);
	TestTrue(TEXT("Initial MetadataTags must be empty"), ScenarioDef->MetadataTags.IsEmpty());
	TestTrue(TEXT("Initial ProvenanceNote must be empty"), ScenarioDef->ProvenanceNote.IsEmpty());

	return true;
}

// 2. DefinitionUsesCustomContentType Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveNightmareScenarioDefinitionContentTypeTest,
	"ShadowSlave.NightmareScenarioDefinition.DefinitionUsesCustomContentType",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveNightmareScenarioDefinitionContentTypeTest::RunTest(const FString& Parameters)
{
	UShadowSlaveNightmareScenarioDefinition* ScenarioDef = NewObject<UShadowSlaveNightmareScenarioDefinition>();
	TestNotNull(TEXT("Scenario definition must instantiate"), ScenarioDef);
	if (!ScenarioDef)
	{
		return false;
	}

	// 1. Generic ContentType must be Custom (no new enum value added)
	TestEqual(TEXT("Generic ContentType must be EShadowSlaveContentType::Custom"),
		ScenarioDef->ContentType, EShadowSlaveContentType::Custom);

	// 2. Specialized Primary Asset Type must be "NightmareScenario"
	ScenarioDef->ContentId = FName(TEXT("Test_Nightmare_Identity"));
	const FPrimaryAssetId AssetId = ScenarioDef->GetPrimaryAssetId();
	TestEqual(TEXT("PrimaryAssetType must be 'NightmareScenario'"), AssetId.PrimaryAssetType, FPrimaryAssetType(TEXT("NightmareScenario")));
	TestEqual(TEXT("PrimaryAssetName must match ContentId"), AssetId.PrimaryAssetName, FName(TEXT("Test_Nightmare_Identity")));

	// 3. Test factory produces definition with Custom ContentType
	UShadowSlaveNightmareScenarioDefinition* TestDef = UShadowSlaveNightmareScenarioDefinition::CreateTestScenarioDefinition();
	TestNotNull(TEXT("CreateTestScenarioDefinition must return valid definition"), TestDef);
	if (TestDef)
	{
		TestEqual(TEXT("TestDef ContentType must be Custom"), TestDef->ContentType, EShadowSlaveContentType::Custom);
		TestEqual(TEXT("TestDef PrimaryAssetType must be 'NightmareScenario'"),
			TestDef->GetPrimaryAssetId().PrimaryAssetType, FPrimaryAssetType(TEXT("NightmareScenario")));
	}

	return true;
}

// 3. DefinitionHasSingleAuthoritativeId Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveNightmareScenarioDefinitionSingleAuthoritativeIdTest,
	"ShadowSlave.NightmareScenarioDefinition.DefinitionHasSingleAuthoritativeId",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveNightmareScenarioDefinitionSingleAuthoritativeIdTest::RunTest(const FString& Parameters)
{
	UShadowSlaveNightmareScenarioDefinition* ScenarioDef = NewObject<UShadowSlaveNightmareScenarioDefinition>();
	TestNotNull(TEXT("Scenario definition must instantiate"), ScenarioDef);
	if (!ScenarioDef)
	{
		return false;
	}

	const FName IdA(TEXT("Scenario_First_Trial"));
	const FName IdB(TEXT("Scenario_Colosseum_Descent"));

	// 1. Direct ContentId mutation is reflected in GetScenarioId()
	ScenarioDef->ContentId = IdA;
	TestEqual(TEXT("GetScenarioId must reflect ContentId"), ScenarioDef->GetScenarioId(), IdA);

	// 2. SetScenarioId mutates ContentId
	ScenarioDef->SetScenarioId(IdB);
	TestEqual(TEXT("ContentId must be updated by SetScenarioId"), ScenarioDef->ContentId, IdB);
	TestEqual(TEXT("GetScenarioId must return updated ID"), ScenarioDef->GetScenarioId(), IdB);

	// 3. Version accessor compatibility
	ScenarioDef->Version = 3;
	TestEqual(TEXT("GetScenarioVersion must reflect Version"), ScenarioDef->GetScenarioVersion(), 3);
	ScenarioDef->SetScenarioVersion(5);
	TestEqual(TEXT("Version must be updated by SetScenarioVersion"), ScenarioDef->Version, 5);
	TestEqual(TEXT("GetScenarioVersion must return updated version"), ScenarioDef->GetScenarioVersion(), 5);

	// 4. PrimaryAssetId uses the single authoritative ContentId
	const FPrimaryAssetId ExpectedAssetId(TEXT("NightmareScenario"), IdB);
	TestEqual(TEXT("PrimaryAssetId must match FPrimaryAssetId('NightmareScenario', ContentId)"),
		ScenarioDef->GetPrimaryAssetId(), ExpectedAssetId);

	return true;
}

// 4. DefinitionValidation Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveNightmareScenarioDefinitionValidationTest,
	"ShadowSlave.NightmareScenarioDefinition.DefinitionValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveNightmareScenarioDefinitionValidationTest::RunTest(const FString& Parameters)
{
	UShadowSlaveNightmareScenarioDefinition* ValidDef = UShadowSlaveNightmareScenarioDefinition::CreateTestScenarioDefinition();
	TestNotNull(TEXT("Test scenario definition must instantiate"), ValidDef);
	if (!ValidDef)
	{
		return false;
	}

	FString ErrorMsg;
	FText ScenarioError;

	// 1. Valid definition passes IsValidDefinition and ValidateScenario
	TestTrue(TEXT("Valid definition must pass IsValidDefinition"), ValidDef->IsValidDefinition(&ErrorMsg));
	TestTrue(TEXT("Valid definition must pass ValidateScenario"), ValidDef->ValidateScenario(ScenarioError));
	TestTrue(TEXT("ScenarioError must be empty on valid definition"), ScenarioError.IsEmpty());

	// 2. Generic validation: ContentId None fails
	ValidDef->ContentId = NAME_None;
	TestFalse(TEXT("None ContentId must fail IsValidDefinition"), ValidDef->IsValidDefinition(&ErrorMsg));
	TestFalse(TEXT("Error message must not be empty on failure"), ErrorMsg.IsEmpty());
	ValidDef->ContentId = FName(TEXT("Scenario_Dev_Test"));

	// 3. Generic validation: Empty DisplayName fails
	ValidDef->DisplayName = FText::GetEmpty();
	TestFalse(TEXT("Empty DisplayName must fail IsValidDefinition"), ValidDef->IsValidDefinition(&ErrorMsg));
	ValidDef->DisplayName = FText::FromString(TEXT("Development Test Scenario"));

	// 4. Generic validation: Version < 1 fails
	ValidDef->Version = 0;
	TestFalse(TEXT("Version 0 must fail IsValidDefinition"), ValidDef->IsValidDefinition(&ErrorMsg));
	ValidDef->Version = 1;

	// 5. Generic validation: Wrong ContentType fails
	ValidDef->ContentType = EShadowSlaveContentType::Story;
	TestFalse(TEXT("Wrong ContentType must fail IsValidDefinition"), ValidDef->IsValidDefinition(&ErrorMsg));
	ValidDef->ContentType = EShadowSlaveContentType::Custom;

	// 6. Scenario-specific validation: Objective with None ID fails
	TArray<FShadowSlaveNightmareObjectiveDefinition> SavedObjectives = ValidDef->Objectives;
	ValidDef->Objectives[0].ObjectiveId = NAME_None;
	TestFalse(TEXT("Objective with None ID must fail IsValidDefinition"), ValidDef->IsValidDefinition(&ErrorMsg));
	ValidDef->Objectives = SavedObjectives;

	// 7. Scenario-specific validation: Duplicate objective IDs fail
	ValidDef->Objectives.Add(ValidDef->Objectives[0]);
	TestFalse(TEXT("Duplicate objective ID must fail IsValidDefinition"), ValidDef->IsValidDefinition(&ErrorMsg));
	ValidDef->Objectives = SavedObjectives;

	// 8. Scenario-specific validation: Zero required objectives when required fails
	for (FShadowSlaveNightmareObjectiveDefinition& Obj : ValidDef->Objectives)
	{
		Obj.bIsRequired = false;
	}
	TestFalse(TEXT("Zero required objectives with AllRequiredObjectives rule must fail"), ValidDef->IsValidDefinition(&ErrorMsg));
	ValidDef->Objectives = SavedObjectives;

	// Restored definition passes again
	TestTrue(TEXT("Restored definition passes IsValidDefinition"), ValidDef->IsValidDefinition(&ErrorMsg));

	return true;
}

// 5. DefinitionRegistryIntegration Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveNightmareScenarioDefinitionRegistryIntegrationTest,
	"ShadowSlave.NightmareScenarioDefinition.DefinitionRegistryIntegration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveNightmareScenarioDefinitionRegistryIntegrationTest::RunTest(const FString& Parameters)
{
	UShadowSlaveContentRegistrySubsystem* Registry = NewObject<UShadowSlaveContentRegistrySubsystem>();
	TestNotNull(TEXT("Registry subsystem must instantiate"), Registry);

	UShadowSlaveNightmareScenarioDefinition* ScenarioDef = UShadowSlaveNightmareScenarioDefinition::CreateTestScenarioDefinition();
	TestNotNull(TEXT("Scenario definition must instantiate"), ScenarioDef);
	ScenarioDef->ContentId = FName(TEXT("Scenario_Registry_Test"));

	// 1. Register with generic registry subsystem
	const bool bRegistered = Registry->RegisterDefinition(ScenarioDef);
	TestTrue(TEXT("RegisterDefinition must succeed for UShadowSlaveNightmareScenarioDefinition"), bRegistered);

	// 2. Query existence & count
	TestTrue(TEXT("HasContent must return true for registered ScenarioDef"), Registry->HasContent(ScenarioDef->ContentId));
	TestEqual(TEXT("Total registered count must be 1"), Registry->GetRegisteredContentCount(), 1);
	TestEqual(TEXT("Custom count by type must be 1"), Registry->GetRegisteredContentCountByType(EShadowSlaveContentType::Custom), 1);
	TestEqual(TEXT("Story count by type must be 0"), Registry->GetRegisteredContentCountByType(EShadowSlaveContentType::Story), 0);
	TestEqual(TEXT("Quest count by type must be 0"), Registry->GetRegisteredContentCountByType(EShadowSlaveContentType::Quest), 0);

	// 3. Generic resolution
	UShadowSlaveContentDefinition* ResolvedGeneric = Registry->ResolveContentDefinition(ScenarioDef->ContentId);
	TestNotNull(TEXT("Resolved generic definition must not be null"), ResolvedGeneric);
	TestEqual(TEXT("Resolved generic definition must match ScenarioDef"), ResolvedGeneric, Cast<UShadowSlaveContentDefinition>(ScenarioDef));

	// 4. Typed resolution
	UShadowSlaveNightmareScenarioDefinition* ResolvedScenario = Registry->ResolveContentDefinition<UShadowSlaveNightmareScenarioDefinition>(ScenarioDef->ContentId);
	TestNotNull(TEXT("Resolved typed scenario definition must not be null"), ResolvedScenario);
	TestEqual(TEXT("Resolved typed scenario must match original ScenarioDef"), ResolvedScenario, ScenarioDef);

	// 5. PrimaryAssetId verification: uses specialized "NightmareScenario" type
	const FPrimaryAssetId ExpectedAssetId(TEXT("NightmareScenario"), ScenarioDef->ContentId);
	TestEqual(TEXT("GetPrimaryAssetId must match expected PrimaryAssetId with NightmareScenario type"),
		ScenarioDef->GetPrimaryAssetId(), ExpectedAssetId);

	return true;
}

// 6. ExistingRuntimeCompatibility Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveNightmareScenarioDefinitionExistingRuntimeCompatibilityTest,
	"ShadowSlave.NightmareScenarioDefinition.ExistingRuntimeCompatibility",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveNightmareScenarioDefinitionExistingRuntimeCompatibilityTest::RunTest(const FString& Parameters)
{
	UShadowSlaveNightmareSubsystem* NightmareSub = NewObject<UShadowSlaveNightmareSubsystem>();
	TestNotNull(TEXT("NightmareSubsystem must instantiate"), NightmareSub);
	if (!NightmareSub)
	{
		return false;
	}

	UShadowSlaveNightmareScenarioDefinition* ScenarioDef = UShadowSlaveNightmareScenarioDefinition::CreateTestScenarioDefinition();
	TestNotNull(TEXT("Test scenario definition must instantiate"), ScenarioDef);
	if (!ScenarioDef)
	{
		return false;
	}

	FText OutError;

	// 1. CanStartScenario verifies definition integrity
	TestTrue(TEXT("CanStartScenario must return true for valid definition"), NightmareSub->CanStartScenario(ScenarioDef, OutError));

	// 2. StartScenario transitions session to Active
	const bool bStarted = NightmareSub->StartScenario(ScenarioDef);
	TestTrue(TEXT("StartScenario must succeed with migrated definition"), bStarted);
	TestTrue(TEXT("IsScenarioActive must return true"), NightmareSub->IsScenarioActive());
	TestEqual(TEXT("GetCurrentScenario must return ScenarioDef"), NightmareSub->GetCurrentScenario(), ScenarioDef);
	TestEqual(TEXT("GetCurrentScenarioId must match ScenarioDef GetScenarioId"), NightmareSub->GetCurrentScenarioId(), ScenarioDef->GetScenarioId());
	TestEqual(TEXT("GetCurrentScenarioVersion must match ScenarioDef GetScenarioVersion"), NightmareSub->GetCurrentScenarioVersion(), ScenarioDef->GetScenarioVersion());

	// 3. Objective tracker initialized with scenario objectives
	UShadowSlaveNightmareObjectiveTracker* Tracker = NightmareSub->GetObjectiveTracker();
	TestNotNull(TEXT("Objective tracker must be valid"), Tracker);
	if (Tracker)
	{
		TestEqual(TEXT("Objective count matches scenario definition"), Tracker->GetAllObjectiveRuntimeStates().Num(), ScenarioDef->Objectives.Num());
	}

	// 4. Save/Load state captures ContentId as ScenarioId
	FShadowSlaveNightmareSaveData SaveData = NightmareSub->CaptureNightmareState();
	TestTrue(TEXT("SaveData must be valid"), SaveData.bIsValid);
	TestTrue(TEXT("SaveData must report active"), SaveData.bIsActive);
	TestEqual(TEXT("Saved ScenarioId must match ScenarioDef GetScenarioId"), SaveData.ScenarioId, ScenarioDef->GetScenarioId());
	TestEqual(TEXT("Saved ScenarioVersion must match ScenarioDef GetScenarioVersion"), SaveData.ScenarioVersion, ScenarioDef->GetScenarioVersion());

	// 5. Clean session conclusion and exit
	TestTrue(TEXT("CompleteScenario must succeed"), NightmareSub->CompleteScenario());
	TestTrue(TEXT("BeginExitScenario must succeed"), NightmareSub->BeginExitScenario());
	TestTrue(TEXT("FinishExitScenario must succeed"), NightmareSub->FinishExitScenario());
	TestEqual(TEXT("Session state must be Inactive after exit"), NightmareSub->GetSessionState(), EShadowSlaveNightmareSessionState::Inactive);
	TestNull(TEXT("Active scenario definition must be cleared"), NightmareSub->GetCurrentScenario());

	return true;
}

#endif // WITH_AUTOMATION_TESTS
