// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_AUTOMATION_TESTS

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Gameplay/ShadowSlaveQuestSubsystem.h"
#include "Gameplay/ShadowSlaveQuestDefinition.h"
#include "Gameplay/ShadowSlaveQuestTypes.h"

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
	EmptyIdDef->QuestId = NAME_None;
	TestFalse(TEXT("Definition with NAME_None QuestId must fail"), QuestSub->RegisterQuestDefinition(EmptyIdDef));

	// 3. Definition with Version < 1 rejected
	UShadowSlaveQuestDefinition* InvalidVersionDef = NewObject<UShadowSlaveQuestDefinition>();
	InvalidVersionDef->QuestId = FName("Quest_InvalidVersion");
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
	DefA->QuestId = TestQuestId;
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
	DefB->QuestId = TestQuestId;
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
	DefAlpha->QuestId = QuestAlpha;
	DefAlpha->Version = 1;

	UShadowSlaveQuestDefinition* DefBeta = NewObject<UShadowSlaveQuestDefinition>();
	DefBeta->QuestId = QuestBeta;
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
	DefAlpha->QuestId = QuestAlpha;
	DefAlpha->Version = 1;

	UShadowSlaveQuestDefinition* DefBeta = NewObject<UShadowSlaveQuestDefinition>();
	DefBeta->QuestId = QuestBeta;
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
	Def->QuestId = QuestId;
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
	Def->QuestId = QuestId;
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

#endif // WITH_AUTOMATION_TESTS
