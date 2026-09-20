// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_AUTOMATION_TESTS

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Gameplay/ShadowSlaveQuestSubsystem.h"
#include "Gameplay/ShadowSlaveQuestDefinition.h"
#include "Gameplay/ShadowSlaveQuestTypes.h"
#include "Content/ShadowSlaveContentRegistrySubsystem.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveQuestInvalidOrMissingDefinitionTest,
	"ShadowSlave.Quest.InvalidOrMissingDefinitionRejected",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveQuestInvalidOrMissingDefinitionTest::RunTest(const FString& Parameters)
{
	UShadowSlaveQuestSubsystem* QuestSub = NewObject<UShadowSlaveQuestSubsystem>();
	TestNotNull(TEXT("QuestSubsystem must instantiate"), QuestSub);
	if (!QuestSub)
	{
		return false;
	}

	// 1. Null definition rejected
	TestFalse(TEXT("RegisterQuestDefinition(nullptr) must fail"), QuestSub->RegisterQuestDefinition(nullptr));

	// 2. Definition with NAME_None QuestId rejected
	UShadowSlaveQuestDefinition* EmptyIdDef = NewObject<UShadowSlaveQuestDefinition>();
	EmptyIdDef->SetQuestId(NAME_None);
	TestFalse(TEXT("Definition with NAME_None QuestId must fail"), QuestSub->RegisterQuestDefinition(EmptyIdDef));

	// 3. Definition with Version < 1 rejected
	UShadowSlaveQuestDefinition* InvalidVersionDef = NewObject<UShadowSlaveQuestDefinition>();
	InvalidVersionDef->SetQuestId(FName("Quest_InvalidVersion"));
	InvalidVersionDef->Version = 0;
	TestFalse(TEXT("Definition with Version 0 must fail"), QuestSub->RegisterQuestDefinition(InvalidVersionDef));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveQuestDuplicateRegistrationTest,
	"ShadowSlave.Quest.DuplicateRegistrationDoesNotReplace",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveQuestDuplicateRegistrationTest::RunTest(const FString& Parameters)
{
	UShadowSlaveQuestSubsystem* QuestSub = NewObject<UShadowSlaveQuestSubsystem>();
	TestNotNull(TEXT("QuestSubsystem must instantiate"), QuestSub);
	if (!QuestSub)
	{
		return false;
	}

	const FName TestQuestId = FName("Quest_UniqueRegistration");

	UShadowSlaveQuestDefinition* DefA = NewObject<UShadowSlaveQuestDefinition>();
	DefA->SetQuestId(TestQuestId);
	DefA->Version = 1;

	FShadowSlaveObjectiveDefinition ObjA;
	ObjA.ObjectiveId = FName("Obj_A");
	ObjA.Type = EShadowSlaveObjectiveType::ReachLocation;
	ObjA.RequiredQuantity = 1;
	DefA->Objectives.Add(ObjA);

	// First registration succeeds
	TestTrue(TEXT("First registration of DefA must succeed"), QuestSub->RegisterQuestDefinition(DefA));
	TestTrue(TEXT("HasQuestDefinition must be true"), QuestSub->HasQuestDefinition(TestQuestId));

	// Idempotent re-registration with same instance succeeds
	TestTrue(TEXT("Re-registering same definition instance is idempotent"), QuestSub->RegisterQuestDefinition(DefA));

	// Attempting to register a different definition with the same QuestId must be rejected
	UShadowSlaveQuestDefinition* DefB = NewObject<UShadowSlaveQuestDefinition>();
	DefB->SetQuestId(TestQuestId);
	DefB->Version = 1;
	DefB->Objectives.Add(ObjA);

	TestFalse(TEXT("Registering distinct DefB with duplicate QuestId must fail"), QuestSub->RegisterQuestDefinition(DefB));
	TestEqual(TEXT("Registered definition must still be DefA"), QuestSub->GetQuestDefinition(TestQuestId), DefA);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveQuestMissingPrerequisitesTest,
	"ShadowSlave.Quest.MissingPrerequisitesFailClosed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveQuestMissingPrerequisitesTest::RunTest(const FString& Parameters)
{
	UShadowSlaveQuestSubsystem* QuestSub = NewObject<UShadowSlaveQuestSubsystem>();
	TestNotNull(TEXT("QuestSubsystem must instantiate"), QuestSub);
	if (!QuestSub)
	{
		return false;
	}

	const FName QuestAlpha = FName("Quest_Prereq_Alpha");
	const FName QuestBeta = FName("Quest_Prereq_Beta");

	UShadowSlaveQuestDefinition* DefAlpha = NewObject<UShadowSlaveQuestDefinition>();
	DefAlpha->SetQuestId(QuestAlpha);
	DefAlpha->Version = 1;

	UShadowSlaveQuestDefinition* DefBeta = NewObject<UShadowSlaveQuestDefinition>();
	DefBeta->SetQuestId(QuestBeta);
	DefBeta->Version = 1;
	DefBeta->PrerequisiteQuestIds.Add(QuestAlpha);

	TestTrue(TEXT("Register DefAlpha"), QuestSub->RegisterQuestDefinition(DefAlpha));
	TestTrue(TEXT("Register DefBeta"), QuestSub->RegisterQuestDefinition(DefBeta));

	// Prereq is not yet satisfied
	TestFalse(TEXT("Prerequisites for Beta must be unsatisfied initially"), QuestSub->ArePrerequisitesSatisfied(QuestBeta));

	// Attempting to activate Beta without completed prereqs must fail closed
	TestFalse(TEXT("ActivateQuest(Beta) must fail closed when prereq is incomplete"), QuestSub->ActivateQuest(QuestBeta));
	TestEqual(TEXT("Beta state must remain Locked"), QuestSub->GetQuestState(QuestBeta), EShadowSlaveQuestState::Locked);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveQuestPrerequisiteCompletionProgressionTest,
	"ShadowSlave.Quest.PrerequisiteCompletionAllowsProgression",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveQuestPrerequisiteCompletionProgressionTest::RunTest(const FString& Parameters)
{
	UShadowSlaveQuestSubsystem* QuestSub = NewObject<UShadowSlaveQuestSubsystem>();
	TestNotNull(TEXT("QuestSubsystem must instantiate"), QuestSub);
	if (!QuestSub)
	{
		return false;
	}

	const FName QuestAlpha = FName("Quest_Prog_Alpha");
	const FName QuestBeta = FName("Quest_Prog_Beta");

	UShadowSlaveQuestDefinition* DefAlpha = NewObject<UShadowSlaveQuestDefinition>();
	DefAlpha->SetQuestId(QuestAlpha);
	DefAlpha->Version = 1;

	UShadowSlaveQuestDefinition* DefBeta = NewObject<UShadowSlaveQuestDefinition>();
	DefBeta->SetQuestId(QuestBeta);
	DefBeta->Version = 1;
	DefBeta->PrerequisiteQuestIds.Add(QuestAlpha);

	QuestSub->RegisterQuestDefinition(DefAlpha);
	QuestSub->RegisterQuestDefinition(DefBeta);

	// Activate and complete prerequisite quest Alpha
	TestTrue(TEXT("Activate Alpha"), QuestSub->ActivateQuest(QuestAlpha));
	TestTrue(TEXT("Complete Alpha"), QuestSub->CompleteQuest(QuestAlpha));
	TestTrue(TEXT("Alpha is Completed"), QuestSub->IsQuestCompleted(QuestAlpha));

	// Now Beta prerequisites must be satisfied
	TestTrue(TEXT("Prerequisites for Beta are now satisfied"), QuestSub->ArePrerequisitesSatisfied(QuestBeta));

	// Beta can now be legally activated
	TestTrue(TEXT("Activate Beta should succeed now"), QuestSub->ActivateQuest(QuestBeta));
	TestTrue(TEXT("Beta is now Active"), QuestSub->IsQuestActive(QuestBeta));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveQuestIllegalLifecycleTransitionsTest,
	"ShadowSlave.Quest.IllegalLifecycleTransitionsRejected",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveQuestIllegalLifecycleTransitionsTest::RunTest(const FString& Parameters)
{
	UShadowSlaveQuestSubsystem* QuestSub = NewObject<UShadowSlaveQuestSubsystem>();
	TestNotNull(TEXT("QuestSubsystem must instantiate"), QuestSub);
	if (!QuestSub)
	{
		return false;
	}

	const FName QuestId = FName("Quest_Lifecycle");
	UShadowSlaveQuestDefinition* Def = NewObject<UShadowSlaveQuestDefinition>();
	Def->SetQuestId(QuestId);
	Def->Version = 1;
	QuestSub->RegisterQuestDefinition(Def);

	// Locked quest cannot transition directly to Completed or Failed
	TestFalse(TEXT("Locked cannot transition directly to Completed"), QuestSub->CompleteQuest(QuestId));
	TestFalse(TEXT("Locked cannot transition directly to Failed"), QuestSub->FailQuest(QuestId));
	TestEqual(TEXT("State must remain Locked"), QuestSub->GetQuestState(QuestId), EShadowSlaveQuestState::Locked);

	// Activate quest, then complete it
	TestTrue(TEXT("Activate quest"), QuestSub->ActivateQuest(QuestId));
	TestTrue(TEXT("Complete quest"), QuestSub->CompleteQuest(QuestId));
	TestEqual(TEXT("State is Completed"), QuestSub->GetQuestState(QuestId), EShadowSlaveQuestState::Completed);

	// Completed is terminal: cannot transition away to Active, Failed, or Abandoned
	TestFalse(TEXT("Completed quest cannot transition to Active"), QuestSub->ActivateQuest(QuestId));
	TestFalse(TEXT("Completed quest cannot transition to Failed"), QuestSub->FailQuest(QuestId));
	TestFalse(TEXT("Completed quest cannot transition to Abandoned"), QuestSub->AbandonQuest(QuestId));
	TestEqual(TEXT("State remains Completed"), QuestSub->GetQuestState(QuestId), EShadowSlaveQuestState::Completed);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveQuestObjectiveProgressCannotRegressTest,
	"ShadowSlave.Quest.ObjectiveProgressCannotRegress",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveQuestObjectiveProgressCannotRegressTest::RunTest(const FString& Parameters)
{
	UShadowSlaveQuestSubsystem* QuestSub = NewObject<UShadowSlaveQuestSubsystem>();
	TestNotNull(TEXT("QuestSubsystem must instantiate"), QuestSub);
	if (!QuestSub)
	{
		return false;
	}

	const FName QuestId = FName("Quest_ProgressRegress");
	const FName ObjId = FName("Obj_Count");

	UShadowSlaveQuestDefinition* Def = NewObject<UShadowSlaveQuestDefinition>();
	Def->SetQuestId(QuestId);
	Def->Version = 1;
	Def->bAutoCompleteWhenObjectivesComplete = false;

	FShadowSlaveObjectiveDefinition Obj;
	Obj.ObjectiveId = ObjId;
	Obj.Type = EShadowSlaveObjectiveType::CollectItem;
	Obj.RequiredQuantity = 10;
	Def->Objectives.Add(Obj);

	QuestSub->RegisterQuestDefinition(Def);
	QuestSub->ActivateQuest(QuestId);

	// Advance progress to 5
	TestTrue(TEXT("Add progress 5"), QuestSub->AddObjectiveProgress(QuestId, ObjId, 5));
	TestEqual(TEXT("Objective progress must be 5"), QuestSub->GetObjectiveProgress(QuestId, ObjId), 5);

	// Attempt negative progress: AddObjectiveProgress requires Amount > 0 and rejects regression
	TestFalse(TEXT("Negative progress addition must be rejected"), QuestSub->AddObjectiveProgress(QuestId, ObjId, -3));
	TestEqual(TEXT("Objective progress must not regress"), QuestSub->GetObjectiveProgress(QuestId, ObjId), 5);

	// Complete objective
	TestTrue(TEXT("Add progress 5 to reach 10"), QuestSub->AddObjectiveProgress(QuestId, ObjId, 5));
	TestEqual(TEXT("Objective progress must be 10"), QuestSub->GetObjectiveProgress(QuestId, ObjId), 10);
	TestEqual(TEXT("Objective must be Completed"), QuestSub->GetObjectiveState(QuestId, ObjId), EShadowSlaveObjectiveState::Completed);

	// Completed objective is terminal and cannot accept further modifications
	TestFalse(TEXT("Adding progress to a Completed objective must be rejected"), QuestSub->AddObjectiveProgress(QuestId, ObjId, 2));
	TestEqual(TEXT("Progress remains clamped at 10"), QuestSub->GetObjectiveProgress(QuestId, ObjId), 10);

	return true;
}

// -----------------------------------------------------------------------------
// Step 48: Quest Definition Content Pipeline Integration Tests
// -----------------------------------------------------------------------------

namespace
{
	UShadowSlaveQuestDefinition* CreateTestQuestDefinition(UObject* Outer = nullptr)
	{
		UShadowSlaveQuestDefinition* Def = NewObject<UShadowSlaveQuestDefinition>(Outer ? Outer : GetTransientPackage());
		Def->ContentId = FName(TEXT("Quest_Test_Archetype"));
		Def->DisplayName = FText::FromString(TEXT("Test Quest"));
		Def->Description = FText::FromString(TEXT("Test Quest Description"));
		Def->Version = 1;
		Def->bAutoCompleteWhenObjectivesComplete = true;

		FShadowSlaveObjectiveDefinition Obj;
		Obj.ObjectiveId = FName(TEXT("Obj_Test_Primary"));
		Obj.Description = FText::FromString(TEXT("Test Objective"));
		Obj.Type = EShadowSlaveObjectiveType::ReachLocation;
		Obj.RequiredQuantity = 1;
		Obj.bIsOptional = false;
		Def->Objectives.Add(Obj);

		return Def;
	}
}

// 1. DefinitionUsesGenericContentBase Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveQuestDefinitionGenericBaseTest,
	"ShadowSlave.QuestDefinition.DefinitionUsesGenericContentBase",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveQuestDefinitionGenericBaseTest::RunTest(const FString& Parameters)
{
	UShadowSlaveQuestDefinition* QuestDef = NewObject<UShadowSlaveQuestDefinition>();
	TestNotNull(TEXT("Quest definition must instantiate"), QuestDef);
	if (!QuestDef)
	{
		return false;
	}

	// 1. Must inherit from UShadowSlaveContentDefinition
	UShadowSlaveContentDefinition* ContentBase = Cast<UShadowSlaveContentDefinition>(QuestDef);
	TestNotNull(TEXT("UShadowSlaveQuestDefinition must inherit from UShadowSlaveContentDefinition"), ContentBase);

	// 2. Base content fields must be accessible and correctly initialized
	TestEqual(TEXT("Initial ContentId must be NAME_None"), QuestDef->ContentId, NAME_None);
	TestEqual(TEXT("Initial ContentType must be Quest"), QuestDef->ContentType, EShadowSlaveContentType::Quest);
	TestEqual(TEXT("Initial Version must be 1"), QuestDef->Version, 1);
	TestTrue(TEXT("Initial MetadataTags must be empty"), QuestDef->MetadataTags.IsEmpty());
	TestTrue(TEXT("Initial ProvenanceNote must be empty"), QuestDef->ProvenanceNote.IsEmpty());

	return true;
}

// 2. DefinitionUsesQuestContentType Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveQuestDefinitionContentTypeTest,
	"ShadowSlave.QuestDefinition.DefinitionUsesQuestContentType",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveQuestDefinitionContentTypeTest::RunTest(const FString& Parameters)
{
	UShadowSlaveQuestDefinition* QuestDef = NewObject<UShadowSlaveQuestDefinition>();
	TestNotNull(TEXT("Quest definition must instantiate"), QuestDef);
	if (!QuestDef)
	{
		return false;
	}

	// 1. Generic ContentType must be Quest
	TestEqual(TEXT("Generic ContentType must be EShadowSlaveContentType::Quest"),
		QuestDef->ContentType, EShadowSlaveContentType::Quest);

	// 2. Primary Asset Type must be "Quest"
	QuestDef->ContentId = FName(TEXT("Test_Quest_Identity"));
	const FPrimaryAssetId AssetId = QuestDef->GetPrimaryAssetId();
	TestEqual(TEXT("PrimaryAssetType must be 'Quest'"), AssetId.PrimaryAssetType, FPrimaryAssetType(TEXT("Quest")));
	TestEqual(TEXT("PrimaryAssetName must match ContentId"), AssetId.PrimaryAssetName, FName(TEXT("Test_Quest_Identity")));

	return true;
}

// 3. DefinitionHasSingleAuthoritativeId Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveQuestDefinitionSingleAuthoritativeIdTest,
	"ShadowSlave.QuestDefinition.DefinitionHasSingleAuthoritativeId",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveQuestDefinitionSingleAuthoritativeIdTest::RunTest(const FString& Parameters)
{
	UShadowSlaveQuestDefinition* QuestDef = NewObject<UShadowSlaveQuestDefinition>();
	TestNotNull(TEXT("Quest definition must instantiate"), QuestDef);
	if (!QuestDef)
	{
		return false;
	}

	const FName IdA(TEXT("Quest_First_Trial"));
	const FName IdB(TEXT("Quest_Shadow_Descent"));

	// 1. Direct ContentId mutation is reflected in GetQuestId()
	QuestDef->ContentId = IdA;
	TestEqual(TEXT("GetQuestId must reflect ContentId"), QuestDef->GetQuestId(), IdA);

	// 2. SetQuestId mutates ContentId
	QuestDef->SetQuestId(IdB);
	TestEqual(TEXT("ContentId must be updated by SetQuestId"), QuestDef->ContentId, IdB);
	TestEqual(TEXT("GetQuestId must return updated ID"), QuestDef->GetQuestId(), IdB);

	// 3. PrimaryAssetId uses the single authoritative ContentId
	const FPrimaryAssetId ExpectedAssetId(TEXT("Quest"), IdB);
	TestEqual(TEXT("PrimaryAssetId must match FPrimaryAssetId('Quest', ContentId)"),
		QuestDef->GetPrimaryAssetId(), ExpectedAssetId);

	return true;
}

// 4. DefinitionValidation Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveQuestDefinitionValidationTest,
	"ShadowSlave.QuestDefinition.DefinitionValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveQuestDefinitionValidationTest::RunTest(const FString& Parameters)
{
	UShadowSlaveQuestDefinition* ValidDef = CreateTestQuestDefinition();
	TestNotNull(TEXT("Test quest definition must instantiate"), ValidDef);
	if (!ValidDef)
	{
		return false;
	}

	FString ErrorMsg;
	TArray<FText> OutErrors;

	// 1. Valid definition passes IsValidDefinition and ValidateDefinition
	TestTrue(TEXT("Valid definition must pass IsValidDefinition"), ValidDef->IsValidDefinition(&ErrorMsg));
	TestTrue(TEXT("Valid definition must pass ValidateDefinition"), ValidDef->ValidateDefinition(OutErrors));
	TestEqual(TEXT("OutErrors must be empty on valid definition"), OutErrors.Num(), 0);

	// 2. Generic validation: ContentId None fails
	ValidDef->ContentId = NAME_None;
	TestFalse(TEXT("None ContentId must fail IsValidDefinition"), ValidDef->IsValidDefinition(&ErrorMsg));
	TestFalse(TEXT("Error message must not be empty on failure"), ErrorMsg.IsEmpty());
	ValidDef->ContentId = FName(TEXT("Quest_Test_Archetype"));

	// 3. Generic validation: Empty DisplayName fails
	ValidDef->DisplayName = FText::GetEmpty();
	TestFalse(TEXT("Empty DisplayName must fail IsValidDefinition"), ValidDef->IsValidDefinition(&ErrorMsg));
	ValidDef->DisplayName = FText::FromString(TEXT("Test Quest"));

	// 4. Generic validation: Version < 1 fails
	ValidDef->Version = 0;
	TestFalse(TEXT("Version 0 must fail IsValidDefinition"), ValidDef->IsValidDefinition(&ErrorMsg));
	ValidDef->Version = 1;

	// 5. Generic validation: Wrong ContentType fails
	ValidDef->ContentType = EShadowSlaveContentType::Item;
	TestFalse(TEXT("Wrong ContentType must fail IsValidDefinition"), ValidDef->IsValidDefinition(&ErrorMsg));
	ValidDef->ContentType = EShadowSlaveContentType::Quest;

	// 6. Quest-specific validation: Empty Objectives fails
	TArray<FShadowSlaveObjectiveDefinition> SavedObjectives = ValidDef->Objectives;
	ValidDef->Objectives.Empty();
	TestFalse(TEXT("Empty objectives must fail IsValidDefinition"), ValidDef->IsValidDefinition(&ErrorMsg));
	ValidDef->Objectives = SavedObjectives;

	// 7. Quest-specific validation: Objective with None ID fails
	ValidDef->Objectives[0].ObjectiveId = NAME_None;
	TestFalse(TEXT("Objective with None ID must fail IsValidDefinition"), ValidDef->IsValidDefinition(&ErrorMsg));
	ValidDef->Objectives = SavedObjectives;

	// 8. Quest-specific validation: Duplicate objective IDs fail
	ValidDef->Objectives.Add(ValidDef->Objectives[0]);
	TestFalse(TEXT("Duplicate objective ID must fail IsValidDefinition"), ValidDef->IsValidDefinition(&ErrorMsg));
	ValidDef->Objectives = SavedObjectives;

	// 9. Quest-specific validation: Objective RequiredQuantity < 1 fails
	ValidDef->Objectives[0].RequiredQuantity = 0;
	TestFalse(TEXT("Objective with 0 RequiredQuantity must fail IsValidDefinition"), ValidDef->IsValidDefinition(&ErrorMsg));
	ValidDef->Objectives = SavedObjectives;

	// 10. Quest-specific validation: Prerequisite with None ID fails
	ValidDef->PrerequisiteQuestIds.Add(NAME_None);
	TestFalse(TEXT("Prerequisite with None ID must fail IsValidDefinition"), ValidDef->IsValidDefinition(&ErrorMsg));
	ValidDef->PrerequisiteQuestIds.Empty();

	// 11. Quest-specific validation: Self-prerequisite fails
	ValidDef->PrerequisiteQuestIds.Add(ValidDef->ContentId);
	TestFalse(TEXT("Self-prerequisite must fail IsValidDefinition"), ValidDef->IsValidDefinition(&ErrorMsg));
	ValidDef->PrerequisiteQuestIds.Empty();

	// 12. Quest-specific validation: Duplicate prerequisite fails
	ValidDef->PrerequisiteQuestIds.Add(FName(TEXT("Other_Quest")));
	ValidDef->PrerequisiteQuestIds.Add(FName(TEXT("Other_Quest")));
	TestFalse(TEXT("Duplicate prerequisite must fail IsValidDefinition"), ValidDef->IsValidDefinition(&ErrorMsg));
	ValidDef->PrerequisiteQuestIds.Empty();

	// Restored definition passes again
	TestTrue(TEXT("Restored definition passes IsValidDefinition"), ValidDef->IsValidDefinition(&ErrorMsg));

	return true;
}

// 5. DefinitionRegistryIntegration Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveQuestDefinitionRegistryIntegrationTest,
	"ShadowSlave.QuestDefinition.DefinitionRegistryIntegration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveQuestDefinitionRegistryIntegrationTest::RunTest(const FString& Parameters)
{
	UShadowSlaveContentRegistrySubsystem* Registry = NewObject<UShadowSlaveContentRegistrySubsystem>();
	TestNotNull(TEXT("Registry subsystem must instantiate"), Registry);
	if (!Registry)
	{
		return false;
	}

	UShadowSlaveQuestDefinition* QuestDef = CreateTestQuestDefinition();
	TestNotNull(TEXT("Quest definition must instantiate"), QuestDef);
	if (!QuestDef)
	{
		return false;
	}
	QuestDef->ContentId = FName(TEXT("Quest_Registry_Test"));

	// 1. Register with generic registry subsystem
	const bool bRegistered = Registry->RegisterDefinition(QuestDef);
	TestTrue(TEXT("RegisterDefinition must succeed for UShadowSlaveQuestDefinition"), bRegistered);

	// 2. Query existence & count
	TestTrue(TEXT("HasContent must return true for registered QuestDef"), Registry->HasContent(QuestDef->ContentId));
	TestEqual(TEXT("Total registered count must be 1"), Registry->GetRegisteredContentCount(), 1);
	TestEqual(TEXT("Quest count by type must be 1"), Registry->GetRegisteredContentCountByType(EShadowSlaveContentType::Quest), 1);
	TestEqual(TEXT("Story count by type must be 0"), Registry->GetRegisteredContentCountByType(EShadowSlaveContentType::Story), 0);
	TestEqual(TEXT("Item count by type must be 0"), Registry->GetRegisteredContentCountByType(EShadowSlaveContentType::Item), 0);

	// 3. Generic resolution
	UShadowSlaveContentDefinition* ResolvedGeneric = Registry->ResolveContentDefinition(QuestDef->ContentId);
	TestNotNull(TEXT("Resolved generic definition must not be null"), ResolvedGeneric);
	TestEqual(TEXT("Resolved generic definition must match QuestDef"), ResolvedGeneric, Cast<UShadowSlaveContentDefinition>(QuestDef));

	// 4. Typed resolution
	UShadowSlaveQuestDefinition* ResolvedQuest = Registry->ResolveContentDefinition<UShadowSlaveQuestDefinition>(QuestDef->ContentId);
	TestNotNull(TEXT("Resolved typed quest definition must not be null"), ResolvedQuest);
	TestEqual(TEXT("Resolved typed quest must match original QuestDef"), ResolvedQuest, QuestDef);

	// 5. PrimaryAssetId verification: uses "Quest" type
	const FPrimaryAssetId ExpectedAssetId(TEXT("Quest"), QuestDef->ContentId);
	TestEqual(TEXT("GetPrimaryAssetId must match expected PrimaryAssetId with Quest type"),
		QuestDef->GetPrimaryAssetId(), ExpectedAssetId);

	return true;
}

// 6. ExistingRuntimeCompatibility Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveQuestDefinitionExistingRuntimeCompatibilityTest,
	"ShadowSlave.QuestDefinition.ExistingRuntimeCompatibility",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveQuestDefinitionExistingRuntimeCompatibilityTest::RunTest(const FString& Parameters)
{
	UShadowSlaveQuestSubsystem* QuestSub = NewObject<UShadowSlaveQuestSubsystem>();
	TestNotNull(TEXT("QuestSubsystem must instantiate"), QuestSub);
	if (!QuestSub)
	{
		return false;
	}

	UShadowSlaveQuestDefinition* QuestDef = CreateTestQuestDefinition();
	TestNotNull(TEXT("Test quest definition must instantiate"), QuestDef);
	if (!QuestDef)
	{
		return false;
	}

	QuestDef->SetQuestId(FName(TEXT("Quest_Runtime_Compat")));

	// 1. Register definition with QuestSubsystem
	TestTrue(TEXT("RegisterQuestDefinition must succeed"), QuestSub->RegisterQuestDefinition(QuestDef));
	TestTrue(TEXT("HasQuestDefinition must return true"), QuestSub->HasQuestDefinition(QuestDef->GetQuestId()));
	TestEqual(TEXT("GetQuestDefinition must return QuestDef"), QuestSub->GetQuestDefinition(QuestDef->GetQuestId()), QuestDef);

	// 2. Initial state without prerequisites is Available
	TestEqual(TEXT("Initial quest state must be Available"), QuestSub->GetQuestState(QuestDef->GetQuestId()), EShadowSlaveQuestState::Available);

	// 3. State transitions
	TestTrue(TEXT("ActivateQuest must succeed"), QuestSub->ActivateQuest(QuestDef->GetQuestId()));
	TestTrue(TEXT("IsQuestActive must return true"), QuestSub->IsQuestActive(QuestDef->GetQuestId()));

	// 4. Objective progress tracking
	const FName ObjId = QuestDef->Objectives[0].ObjectiveId;
	TestTrue(TEXT("AddObjectiveProgress must succeed"), QuestSub->AddObjectiveProgress(QuestDef->GetQuestId(), ObjId, 1));
	TestEqual(TEXT("Objective state must be Completed"), QuestSub->GetObjectiveState(QuestDef->GetQuestId(), ObjId), EShadowSlaveObjectiveState::Completed);

	// 5. Complete quest
	TestTrue(TEXT("CompleteQuest must succeed"), QuestSub->CompleteQuest(QuestDef->GetQuestId()));
	TestEqual(TEXT("Quest state must be Completed"), QuestSub->GetQuestState(QuestDef->GetQuestId()), EShadowSlaveQuestState::Completed);
	TestTrue(TEXT("IsQuestCompleted must return true"), QuestSub->IsQuestCompleted(QuestDef->GetQuestId()));

	return true;
}

#endif // WITH_AUTOMATION_TESTS
