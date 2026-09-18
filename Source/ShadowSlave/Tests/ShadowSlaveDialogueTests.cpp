// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_AUTOMATION_TESTS

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Dialogue/ShadowSlaveConversationSubsystem.h"
#include "Dialogue/ShadowSlaveDialogueTypes.h"
#include "Items/ShadowSlaveInventoryComponent.h"
#include "Items/ShadowSlaveItemDefinition.h"
#include "Progression/ShadowSlaveProgressionTypes.h"

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

#endif // WITH_AUTOMATION_TESTS
