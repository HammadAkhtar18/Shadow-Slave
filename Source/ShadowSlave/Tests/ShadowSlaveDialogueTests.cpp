// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_AUTOMATION_TESTS

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Dialogue/ShadowSlaveConversationSubsystem.h"
#include "Dialogue/ShadowSlaveDialogueDefinition.h"
#include "Dialogue/ShadowSlaveDialogueTypes.h"
#include "Items/ShadowSlaveInventoryComponent.h"
#include "Items/ShadowSlaveItemDefinition.h"
#include "Progression/ShadowSlaveProgressionTypes.h"
#include "Content/ShadowSlaveContentRegistrySubsystem.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveDialogueSuccessfulConsequenceTest,
	"ShadowSlave.Dialogue.SuccessfulConsequenceAdvancesState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveDialogueSuccessfulConsequenceTest::RunTest(const FString& Parameters)
{
	UShadowSlaveConversationSubsystem* ConvSub = NewObject<UShadowSlaveConversationSubsystem>();
	TestNotNull(TEXT("ConversationSubsystem must instantiate"), ConvSub);
	if (!ConvSub)
	{
		return false;
	}

	const FName FlagKey = FName("Flag_TestGuardMet");
	const FName NumKey = FName("Affection_Score");

	// Test SetFlag consequence
	FShadowSlaveDialogueConsequence FlagCons;
	FlagCons.ConsequenceType = EShadowSlaveDialogueConsequenceType::SetFlag;
	FlagCons.TargetKey = FlagKey;
	FlagCons.BoolValue = true;

	TestTrue(TEXT("ExecuteConsequence(SetFlag) must succeed"), ConvSub->ExecuteConsequence(FlagCons));
	TestTrue(TEXT("GetRuntimeFlag must report true"), ConvSub->GetRuntimeFlag(FlagKey));

	// Test ModifyNumeric consequence
	FShadowSlaveDialogueConsequence NumCons;
	NumCons.ConsequenceType = EShadowSlaveDialogueConsequenceType::ModifyNumeric;
	NumCons.TargetKey = NumKey;
	NumCons.NumericValue = 15.5f;

	TestTrue(TEXT("ExecuteConsequence(ModifyNumeric) must succeed"), ConvSub->ExecuteConsequence(NumCons));
	TestNearlyEqual(TEXT("GetRuntimeNumericValue must report 15.5"), ConvSub->GetRuntimeNumericValue(NumKey), 15.5f, 0.001f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveDialogueFailedConsequenceHaltsSequenceTest,
	"ShadowSlave.Dialogue.FailedConsequenceHaltsSequence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveDialogueFailedConsequenceHaltsSequenceTest::RunTest(const FString& Parameters)
{
	UShadowSlaveConversationSubsystem* ConvSub = NewObject<UShadowSlaveConversationSubsystem>();
	TestNotNull(TEXT("ConversationSubsystem must instantiate"), ConvSub);
	if (!ConvSub)
	{
		return false;
	}

	const FName PreFlag = FName("Flag_ExecutedPre");
	const FName PostFlag = FName("Flag_NeverReachedPost");

	TArray<FShadowSlaveDialogueConsequence> Consequences;

	// 1. Valid consequence: SetFlag PreFlag = true
	FShadowSlaveDialogueConsequence Cons1;
	Cons1.ConsequenceType = EShadowSlaveDialogueConsequenceType::SetFlag;
	Cons1.TargetKey = PreFlag;
	Cons1.BoolValue = true;
	Consequences.Add(Cons1);

	// 2. Failing consequence: RemoveItem without an active interactor actor (fails safely)
	FShadowSlaveDialogueConsequence Cons2;
	Cons2.ConsequenceType = EShadowSlaveDialogueConsequenceType::RemoveItem;
	Cons2.ItemQuantity = 1;
	Consequences.Add(Cons2);

	// 3. Downstream consequence: SetFlag PostFlag = true
	FShadowSlaveDialogueConsequence Cons3;
	Cons3.ConsequenceType = EShadowSlaveDialogueConsequenceType::SetFlag;
	Cons3.TargetKey = PostFlag;
	Cons3.BoolValue = true;
	Consequences.Add(Cons3);

	// Execution must return false because Cons2 fails
	const bool bAllSucceeded = ConvSub->ExecuteConsequences(Consequences);
	TestFalse(TEXT("ExecuteConsequences must return false when an intermediate consequence fails"), bAllSucceeded);

	// Pre-flag must be set
	TestTrue(TEXT("First consequence was executed"), ConvSub->GetRuntimeFlag(PreFlag));

	// Post-flag must NOT be set because the chain halted immediately at Cons2
	TestFalse(TEXT("Downstream consequence was skipped and must not be executed"), ConvSub->GetRuntimeFlag(PostFlag));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveDialogueUnknownCharacterRankTest,
	"ShadowSlave.Dialogue.UnknownCharacterRankFailsRequirement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveDialogueUnknownCharacterRankTest::RunTest(const FString& Parameters)
{
	UShadowSlaveConversationSubsystem* ConvSub = NewObject<UShadowSlaveConversationSubsystem>();
	TestNotNull(TEXT("ConversationSubsystem must instantiate"), ConvSub);
	if (!ConvSub)
	{
		return false;
	}

	// Condition requiring Unknown rank must fail closed
	FShadowSlaveDialogueCondition CondUnknown;
	CondUnknown.ConditionType = EShadowSlaveDialogueConditionType::CharacterRank;
	CondUnknown.RequiredRank = EShadowSlaveCharacterRank::Unknown;

	TestFalse(TEXT("Condition with RequiredRank=Unknown must evaluate to false"), ConvSub->EvaluateCondition(CondUnknown));

	// Condition requiring Mundane rank when no interactor is present must fail closed
	FShadowSlaveDialogueCondition CondMundane;
	CondMundane.ConditionType = EShadowSlaveDialogueConditionType::CharacterRank;
	CondMundane.RequiredRank = EShadowSlaveCharacterRank::Mundane;

	TestFalse(TEXT("Condition requiring rank without interactor must evaluate to false"), ConvSub->EvaluateCondition(CondMundane));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveDialoguePassiveSaveRoundTripTest,
	"ShadowSlave.Dialogue.PassiveSaveRestoresDurableRuntimeVariables",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveDialoguePassiveSaveRoundTripTest::RunTest(const FString& Parameters)
{
	UShadowSlaveConversationSubsystem* Source = NewObject<UShadowSlaveConversationSubsystem>();
	UShadowSlaveConversationSubsystem* Destination = NewObject<UShadowSlaveConversationSubsystem>();
	TestNotNull(TEXT("Source conversation subsystem must instantiate"), Source);
	TestNotNull(TEXT("Destination conversation subsystem must instantiate"), Destination);
	if (!Source || !Destination)
	{
		return false;
	}

	Source->SetRuntimeFlag(FName(TEXT("Flag_GateOpened")), true);
	Source->SetRuntimeNumericValue(FName(TEXT("Reputation")), 12.5f);
	Source->SetRuntimeMetadata(FName(TEXT("LastContact")), TEXT("TestNPC"));

	FShadowSlaveConversationSaveData SaveData;
	Source->CaptureConversationState(SaveData);
	TestTrue(TEXT("Captured passive dialogue data must be valid"), SaveData.bIsValid);
	TestFalse(TEXT("Captured dialogue data must never request active-session restoration"), SaveData.bIsActive);
	TestTrue(TEXT("Captured dialogue data must omit transient dialogue identity"), SaveData.DialogueId.IsNone());
	TestTrue(TEXT("Captured dialogue data must omit transient node identity"), SaveData.CurrentNodeId.IsNone());

	Destination->SetRuntimeFlag(FName(TEXT("Flag_GateOpened")), false);
	TestTrue(TEXT("Passive dialogue data must restore"), Destination->RestoreConversationState(SaveData));
	TestFalse(TEXT("Restore must leave the conversation inactive"), Destination->IsConversationActive());
	TestTrue(TEXT("Restored flag must match the captured value"), Destination->GetRuntimeFlag(FName(TEXT("Flag_GateOpened"))));
	TestNearlyEqual(TEXT("Restored numeric value must match the captured value"), Destination->GetRuntimeNumericValue(FName(TEXT("Reputation"))), 12.5f, 0.001f);
	TestEqual(TEXT("Restored metadata must match the captured value"), Destination->GetRuntimeMetadata(FName(TEXT("LastContact"))), FString(TEXT("TestNPC")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveDialogueActiveSaveRejectionTest,
	"ShadowSlave.Dialogue.ActiveSaveDataIsRejectedWithoutMutation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveDialogueActiveSaveRejectionTest::RunTest(const FString& Parameters)
{
	UShadowSlaveConversationSubsystem* ConvSub = NewObject<UShadowSlaveConversationSubsystem>();
	TestNotNull(TEXT("Conversation subsystem must instantiate"), ConvSub);
	if (!ConvSub)
	{
		return false;
	}

	const FName ExistingFlag(TEXT("Flag_Existing"));
	ConvSub->SetRuntimeFlag(ExistingFlag, true);

	FShadowSlaveConversationSaveData UnsupportedActiveSave;
	UnsupportedActiveSave.bIsValid = true;
	UnsupportedActiveSave.bIsActive = true;
	UnsupportedActiveSave.RuntimeFlags.Add(FName(TEXT("Flag_Unexpected")), true);

	TestFalse(TEXT("Active dialogue save data must be rejected"), ConvSub->RestoreConversationState(UnsupportedActiveSave));
	TestTrue(TEXT("Rejected data must not mutate existing runtime variables"), ConvSub->GetRuntimeFlag(ExistingFlag));
	TestFalse(TEXT("Rejected data must not introduce new runtime variables"), ConvSub->GetRuntimeFlag(FName(TEXT("Flag_Unexpected"))));

	return true;
}

// -----------------------------------------------------------------------------
// Step 46: Dialogue Definition Content Pipeline Integration Tests
// -----------------------------------------------------------------------------

// 1. DefinitionUsesGenericContentBase Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveDialogueDefinitionGenericBaseTest,
	"ShadowSlave.DialogueDefinition.DefinitionUsesGenericContentBase",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveDialogueDefinitionGenericBaseTest::RunTest(const FString& Parameters)
{
	UShadowSlaveDialogueDefinition* DialogueDef = NewObject<UShadowSlaveDialogueDefinition>();
	TestNotNull(TEXT("Dialogue definition must instantiate"), DialogueDef);
	if (!DialogueDef)
	{
		return false;
	}

	// 1. Must inherit from UShadowSlaveContentDefinition
	UShadowSlaveContentDefinition* ContentBase = Cast<UShadowSlaveContentDefinition>(DialogueDef);
	TestNotNull(TEXT("UShadowSlaveDialogueDefinition must inherit from UShadowSlaveContentDefinition"), ContentBase);

	// 2. Base content fields must be accessible and correctly initialized
	TestEqual(TEXT("Initial ContentId must be NAME_None"), DialogueDef->ContentId, NAME_None);
	TestEqual(TEXT("Initial ContentType must be Dialogue"), DialogueDef->ContentType, EShadowSlaveContentType::Dialogue);
	TestEqual(TEXT("Initial Version must be 1"), DialogueDef->Version, 1);
	TestTrue(TEXT("Initial MetadataTags must be empty"), DialogueDef->MetadataTags.IsEmpty());
	TestTrue(TEXT("Initial ProvenanceNote must be empty"), DialogueDef->ProvenanceNote.IsEmpty());

	return true;
}

// 2. DefinitionUsesDialogueContentType Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveDialogueDefinitionContentTypeTest,
	"ShadowSlave.DialogueDefinition.DefinitionUsesDialogueContentType",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveDialogueDefinitionContentTypeTest::RunTest(const FString& Parameters)
{
	UShadowSlaveDialogueDefinition* DialogueDef = NewObject<UShadowSlaveDialogueDefinition>();
	TestNotNull(TEXT("Dialogue definition must instantiate"), DialogueDef);
	if (!DialogueDef)
	{
		return false;
	}

	// 1. Broad generic classification must be Dialogue
	TestEqual(TEXT("Generic ContentType must be EShadowSlaveContentType::Dialogue"),
		DialogueDef->ContentType, EShadowSlaveContentType::Dialogue);

	// 2. Dialogue-specific structures (nodes, choices, conditions, consequences) remain separate
	UShadowSlaveDialogueDefinition* TestDef = UShadowSlaveDialogueDefinition::CreateTestDialogueDefinition();
	TestNotNull(TEXT("Test dialogue definition must be created"), TestDef);
	if (TestDef)
	{
		TestEqual(TEXT("TestDef ContentType must be Dialogue"), TestDef->ContentType, EShadowSlaveContentType::Dialogue);
		TestTrue(TEXT("TestDef has dialogue nodes"), TestDef->Nodes.Num() > 0);
		TestEqual(TEXT("Starting node matches expected"), TestDef->StartingNodeId, FName(TEXT("Node_Greeting")));
	}

	return true;
}

// 3. DefinitionHasSingleAuthoritativeId Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveDialogueDefinitionSingleAuthoritativeIdTest,
	"ShadowSlave.DialogueDefinition.DefinitionHasSingleAuthoritativeId",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveDialogueDefinitionSingleAuthoritativeIdTest::RunTest(const FString& Parameters)
{
	UShadowSlaveDialogueDefinition* DialogueDef = NewObject<UShadowSlaveDialogueDefinition>();
	TestNotNull(TEXT("Dialogue definition must instantiate"), DialogueDef);
	if (!DialogueDef)
	{
		return false;
	}

	const FName IdA(TEXT("Dialogue_Tutorial_Intro"));
	const FName IdB(TEXT("Dialogue_Merchant_Trade"));

	// 1. Direct ContentId mutation is reflected in GetDialogueId()
	DialogueDef->ContentId = IdA;
	TestEqual(TEXT("GetDialogueId must reflect ContentId"), DialogueDef->GetDialogueId(), IdA);

	// 2. SetDialogueId mutates ContentId
	DialogueDef->SetDialogueId(IdB);
	TestEqual(TEXT("ContentId must be updated by SetDialogueId"), DialogueDef->ContentId, IdB);
	TestEqual(TEXT("GetDialogueId must return updated ID"), DialogueDef->GetDialogueId(), IdB);

	// 3. PrimaryAssetId uses the single authoritative ContentId
	const FPrimaryAssetId ExpectedAssetId(TEXT("Dialogue"), IdB);
	TestEqual(TEXT("PrimaryAssetId must match FPrimaryAssetId('Dialogue', ContentId)"),
		DialogueDef->GetPrimaryAssetId(), ExpectedAssetId);

	return true;
}

// 4. DefinitionValidation Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveDialogueDefinitionValidationTest,
	"ShadowSlave.DialogueDefinition.DefinitionValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveDialogueDefinitionValidationTest::RunTest(const FString& Parameters)
{
	UShadowSlaveDialogueDefinition* ValidDef = UShadowSlaveDialogueDefinition::CreateTestDialogueDefinition();
	TestNotNull(TEXT("Test dialogue definition must instantiate"), ValidDef);
	if (!ValidDef)
	{
		return false;
	}

	FString ErrorMsg;
	TArray<FText> GraphErrors;

	// 1. Valid definition passes IsValidDefinition and ValidateDefinition
	TestTrue(TEXT("Valid definition must pass IsValidDefinition"), ValidDef->IsValidDefinition(&ErrorMsg));
	TestTrue(TEXT("Valid definition must pass ValidateDefinition"), ValidDef->ValidateDefinition(GraphErrors));
	TestEqual(TEXT("GraphErrors must be empty"), GraphErrors.Num(), 0);

	// 2. Generic validation: ContentId None fails
	ValidDef->ContentId = NAME_None;
	TestFalse(TEXT("None ContentId must fail IsValidDefinition"), ValidDef->IsValidDefinition(&ErrorMsg));
	TestFalse(TEXT("Error message must not be empty on failure"), ErrorMsg.IsEmpty());
	ValidDef->ContentId = FName(TEXT("Test_Dialogue_Generic"));

	// 3. Generic validation: Empty DisplayName fails
	ValidDef->DisplayName = FText::GetEmpty();
	TestFalse(TEXT("Empty DisplayName must fail IsValidDefinition"), ValidDef->IsValidDefinition(&ErrorMsg));
	ValidDef->DisplayName = FText::FromString(TEXT("Generic Test Dialogue"));

	// 4. Generic validation: Version < 1 fails
	ValidDef->Version = 0;
	TestFalse(TEXT("Version 0 must fail IsValidDefinition"), ValidDef->IsValidDefinition(&ErrorMsg));
	ValidDef->Version = 1;

	// 5. Generic validation: Wrong ContentType fails
	ValidDef->ContentType = EShadowSlaveContentType::Story;
	TestFalse(TEXT("Wrong ContentType must fail IsValidDefinition"), ValidDef->IsValidDefinition(&ErrorMsg));
	ValidDef->ContentType = EShadowSlaveContentType::Dialogue;

	// 6. Dialogue-specific validation: Missing starting node fails
	ValidDef->StartingNodeId = FName(TEXT("Nonexistent_Node"));
	TestFalse(TEXT("Nonexistent starting node must fail IsValidDefinition"), ValidDef->IsValidDefinition(&ErrorMsg));
	ValidDef->StartingNodeId = FName(TEXT("Node_Greeting"));

	// 7. Dialogue-specific validation: Empty nodes array fails
	TArray<FShadowSlaveDialogueNode> SavedNodes = ValidDef->Nodes;
	ValidDef->Nodes.Empty();
	TestFalse(TEXT("Empty nodes must fail IsValidDefinition"), ValidDef->IsValidDefinition(&ErrorMsg));
	ValidDef->Nodes = SavedNodes;

	// Restored definition passes again
	TestTrue(TEXT("Restored definition passes IsValidDefinition"), ValidDef->IsValidDefinition(&ErrorMsg));

	return true;
}

// 5. DefinitionRegistryIntegration Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveDialogueDefinitionRegistryIntegrationTest,
	"ShadowSlave.DialogueDefinition.DefinitionRegistryIntegration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveDialogueDefinitionRegistryIntegrationTest::RunTest(const FString& Parameters)
{
	UShadowSlaveContentRegistrySubsystem* Registry = NewObject<UShadowSlaveContentRegistrySubsystem>();
	TestNotNull(TEXT("Registry subsystem must instantiate"), Registry);

	UShadowSlaveDialogueDefinition* DialogueDef = UShadowSlaveDialogueDefinition::CreateTestDialogueDefinition();
	TestNotNull(TEXT("Dialogue definition must instantiate"), DialogueDef);
	DialogueDef->ContentId = FName(TEXT("Dialogue_Registry_Test"));

	// 1. Register with generic registry subsystem
	const bool bRegistered = Registry->RegisterDefinition(DialogueDef);
	TestTrue(TEXT("RegisterDefinition must succeed for UShadowSlaveDialogueDefinition"), bRegistered);

	// 2. Query existence & count
	TestTrue(TEXT("HasContent must return true for registered DialogueDef"), Registry->HasContent(DialogueDef->ContentId));
	TestEqual(TEXT("Total registered count must be 1"), Registry->GetRegisteredContentCount(), 1);
	TestEqual(TEXT("Dialogue count by type must be 1"), Registry->GetRegisteredContentCountByType(EShadowSlaveContentType::Dialogue), 1);
	TestEqual(TEXT("Story count by type must be 0"), Registry->GetRegisteredContentCountByType(EShadowSlaveContentType::Story), 0);
	TestEqual(TEXT("Quest count by type must be 0"), Registry->GetRegisteredContentCountByType(EShadowSlaveContentType::Quest), 0);
	TestEqual(TEXT("Item count by type must be 0"), Registry->GetRegisteredContentCountByType(EShadowSlaveContentType::Item), 0);

	// 3. Generic resolution
	UShadowSlaveContentDefinition* ResolvedGeneric = Registry->ResolveContentDefinition(DialogueDef->ContentId);
	TestNotNull(TEXT("Resolved generic definition must not be null"), ResolvedGeneric);
	TestEqual(TEXT("Resolved generic definition must match DialogueDef"), ResolvedGeneric, Cast<UShadowSlaveContentDefinition>(DialogueDef));

	// 4. Typed resolution
	UShadowSlaveDialogueDefinition* ResolvedDialogue = Registry->ResolveContentDefinition<UShadowSlaveDialogueDefinition>(DialogueDef->ContentId);
	TestNotNull(TEXT("Resolved typed dialogue definition must not be null"), ResolvedDialogue);
	TestEqual(TEXT("Resolved typed dialogue must match original DialogueDef"), ResolvedDialogue, DialogueDef);

	// 5. PrimaryAssetId verification
	const FPrimaryAssetId ExpectedAssetId(TEXT("Dialogue"), DialogueDef->ContentId);
	TestEqual(TEXT("GetPrimaryAssetId must match expected PrimaryAssetId with Dialogue type"),
		DialogueDef->GetPrimaryAssetId(), ExpectedAssetId);

	return true;
}

// 6. ExistingRuntimeCompatibility Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveDialogueDefinitionExistingRuntimeCompatibilityTest,
	"ShadowSlave.DialogueDefinition.ExistingRuntimeCompatibility",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveDialogueDefinitionExistingRuntimeCompatibilityTest::RunTest(const FString& Parameters)
{
	UShadowSlaveConversationSubsystem* ConvSub = NewObject<UShadowSlaveConversationSubsystem>();
	TestNotNull(TEXT("ConversationSubsystem must instantiate"), ConvSub);
	if (!ConvSub)
	{
		return false;
	}

	UShadowSlaveDialogueDefinition* DialogueDef = UShadowSlaveDialogueDefinition::CreateTestDialogueDefinition();
	TestNotNull(TEXT("Test dialogue definition must instantiate"), DialogueDef);
	if (!DialogueDef)
	{
		return false;
	}

	// 1. Start conversation with migrated definition
	const bool bStarted = ConvSub->StartConversation(DialogueDef, nullptr, nullptr);
	TestTrue(TEXT("StartConversation must succeed with migrated DialogueDefinition"), bStarted);
	TestTrue(TEXT("Conversation must be active"), ConvSub->IsConversationActive());
	TestEqual(TEXT("Active dialogue definition must match"), ConvSub->GetActiveDialogue(), DialogueDef);
	TestEqual(TEXT("Current node ID must match starting node"), ConvSub->GetCurrentNodeId(), FName(TEXT("Node_Greeting")));
	TestEqual(TEXT("State must be WaitingForChoice"), ConvSub->GetConversationState(), EShadowSlaveConversationState::WaitingForChoice);

	// 2. Verify available choices from greeting node
	const TArray<FShadowSlaveDialogueChoice>& Choices = ConvSub->GetAvailableChoices();
	TestEqual(TEXT("Greeting node must have 3 available choices"), Choices.Num(), 3);

	// 3. Select Choice 2 (Ask Supplies) -> transitions to Node_Supplies and executes consequence
	const bool bSelected = ConvSub->SelectChoiceById(FName(TEXT("Choice_AskSupplies")));
	TestTrue(TEXT("SelectChoiceById must succeed"), bSelected);
	TestEqual(TEXT("Current node is now Node_Supplies"), ConvSub->GetCurrentNodeId(), FName(TEXT("Node_Supplies")));
	TestTrue(TEXT("Node_Supplies consequence set HasAskedSupplies flag"), ConvSub->GetRuntimeFlag(FName(TEXT("HasAskedSupplies"))));

	// 4. Select Choice to exit (TargetNodeId = None)
	const bool bExited = ConvSub->SelectChoiceById(FName(TEXT("Choice_LeaveFromSupplies")));
	TestTrue(TEXT("SelectChoiceById for exit choice must succeed"), bExited);
	TestFalse(TEXT("Conversation must no longer be active after exit choice"), ConvSub->IsConversationActive());
	TestEqual(TEXT("Conversation state must be Completed"), ConvSub->GetConversationState(), EShadowSlaveConversationState::Completed);

	return true;
}

#endif // WITH_AUTOMATION_TESTS
