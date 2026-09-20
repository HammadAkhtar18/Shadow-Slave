// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_AUTOMATION_TESTS

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Story/ShadowSlaveStorySubsystem.h"
#include "Story/ShadowSlaveStoryContentDefinition.h"
#include "Story/ShadowSlaveStoryContentTypes.h"
#include "Story/ShadowSlaveStoryTypes.h"
#include "Content/ShadowSlaveContentTypes.h"
#include "Content/ShadowSlaveContentDefinition.h"
#include "Content/ShadowSlaveContentRegistrySubsystem.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveStoryMalformedSaveDataTest,
	"ShadowSlave.Story.MalformedOrUnresolvedSaveDataFailsClosed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveStoryMalformedSaveDataTest::RunTest(const FString& Parameters)
{
	UShadowSlaveStorySubsystem* StorySub = NewObject<UShadowSlaveStorySubsystem>();
	TestNotNull(TEXT("StorySubsystem must instantiate"), StorySub);
	if (!StorySub)
	{
		return false;
	}

	// 1. Invalid save data (bIsValid == false) must fail closed
	FShadowSlaveStorySaveData InvalidSave;
	InvalidSave.bIsValid = false;
	TestFalse(TEXT("ImportSaveData must return false for bIsValid=false"), StorySub->ImportSaveData(InvalidSave));

	// 2. Unregistered StoryContentId in save data must be skipped safely
	FShadowSlaveStorySaveData ValidSave;
	ValidSave.bIsValid = true;
	ValidSave.StorySubsystemVersion = 1;

	FShadowSlaveStoryContentRecordSaveData UnknownRecord;
	UnknownRecord.StoryContentId = FName("Arc_Nonexistent_Story_Id");
	UnknownRecord.ContentVersion = 1;
	UnknownRecord.State = EShadowSlaveStoryContentState::Active;
	ValidSave.StoryContents.Add(UnknownRecord);

	TestTrue(TEXT("ImportSaveData skips missing definition safely and returns true"), StorySub->ImportSaveData(ValidSave));
	TestEqual(TEXT("Unregistered story content must remain in Unknown state"),
		StorySub->GetStoryContentState(FName("Arc_Nonexistent_Story_Id")), EShadowSlaveStoryContentState::Unknown);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveStoryRestoreFailedEntryReconciliationTest,
	"ShadowSlave.Story.RestoreReconcilesActiveParentWithFailedEntry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveStoryRestoreFailedEntryReconciliationTest::RunTest(const FString& Parameters)
{
	UShadowSlaveStorySubsystem* StorySub = NewObject<UShadowSlaveStorySubsystem>();
	TestNotNull(TEXT("StorySubsystem must instantiate"), StorySub);
	if (!StorySub)
	{
		return false;
	}

	const FName ArcId = FName("Arc_Test_FailedReconcile");
	const FName EntryReq1 = FName("Entry_Mandatory_1");
	const FName EntryReq2 = FName("Entry_Mandatory_2");

	UShadowSlaveStoryContentDefinition* ContentDef = NewObject<UShadowSlaveStoryContentDefinition>();
	ContentDef->SetStoryContentId(ArcId);
	ContentDef->Version = 1;

	FShadowSlaveStoryContentEntry EntryDef1;
	EntryDef1.ContentId = EntryReq1;
	EntryDef1.bIsOptional = false;

	FShadowSlaveStoryContentEntry EntryDef2;
	EntryDef2.ContentId = EntryReq2;
	EntryDef2.bIsOptional = false;

	ContentDef->ContentEntries.Add(EntryDef1);
	ContentDef->ContentEntries.Add(EntryDef2);
	StorySub->RegisterStoryContentDefinition(ContentDef);

	// Craft save data where parent is Active, but mandatory Entry1 is Failed
	FShadowSlaveStorySaveData SaveData;
	SaveData.bIsValid = true;
	SaveData.StorySubsystemVersion = 1;

	FShadowSlaveStoryContentRecordSaveData SavedRecord;
	SavedRecord.StoryContentId = ArcId;
	SavedRecord.ContentVersion = 1;
	SavedRecord.State = EShadowSlaveStoryContentState::Active;
	SavedRecord.CurrentActiveEntryId = EntryReq2;
	SavedRecord.EntryStates.Add(EntryReq1, EShadowSlaveStoryContentState::Failed);
	SavedRecord.EntryStates.Add(EntryReq2, EShadowSlaveStoryContentState::Active);
	SaveData.StoryContents.Add(SavedRecord);

	TestTrue(TEXT("ImportSaveData succeeds"), StorySub->ImportSaveData(SaveData));

	// Reconciled parent must normalize to Failed because mandatory Entry1 is Failed
	TestEqual(TEXT("Active parent with failed mandatory entry must reconcile to Failed"),
		StorySub->GetStoryContentState(ArcId), EShadowSlaveStoryContentState::Failed);

	// Failed parent must not have an active entry
	TestEqual(TEXT("CurrentActiveEntryId must be NAME_None under Failed parent"),
		StorySub->GetCurrentActiveStoryContentEntry(ArcId), NAME_None);

	// Child entries under Failed parent must not remain Active
	TestFalse(TEXT("Child entry must not remain Active under Failed parent"),
		StorySub->GetStoryContentEntryState(ArcId, EntryReq2) == EShadowSlaveStoryContentState::Active);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveStoryRestoreCompletedParentReconciliationTest,
	"ShadowSlave.Story.RestoreReconcilesActiveParentWithAllEntriesCompleted",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveStoryRestoreCompletedParentReconciliationTest::RunTest(const FString& Parameters)
{
	UShadowSlaveStorySubsystem* StorySub = NewObject<UShadowSlaveStorySubsystem>();
	TestNotNull(TEXT("StorySubsystem must instantiate"), StorySub);
	if (!StorySub)
	{
		return false;
	}

	const FName ArcId = FName("Arc_Test_CompleteReconcile");
	const FName EntryReq1 = FName("Entry_Done_1");
	const FName EntryReq2 = FName("Entry_Done_2");

	UShadowSlaveStoryContentDefinition* ContentDef = NewObject<UShadowSlaveStoryContentDefinition>();
	ContentDef->SetStoryContentId(ArcId);
	ContentDef->Version = 1;

	FShadowSlaveStoryContentEntry EntryDef1;
	EntryDef1.ContentId = EntryReq1;
	EntryDef1.bIsOptional = false;

	FShadowSlaveStoryContentEntry EntryDef2;
	EntryDef2.ContentId = EntryReq2;
	EntryDef2.bIsOptional = false;

	ContentDef->ContentEntries.Add(EntryDef1);
	ContentDef->ContentEntries.Add(EntryDef2);
	StorySub->RegisterStoryContentDefinition(ContentDef);

	// Craft save data where parent is Active, but all mandatory entries are Completed
	FShadowSlaveStorySaveData SaveData;
	SaveData.bIsValid = true;
	SaveData.StorySubsystemVersion = 1;

	FShadowSlaveStoryContentRecordSaveData SavedRecord;
	SavedRecord.StoryContentId = ArcId;
	SavedRecord.ContentVersion = 1;
	SavedRecord.State = EShadowSlaveStoryContentState::Active;
	SavedRecord.CurrentActiveEntryId = EntryReq2;
	SavedRecord.EntryStates.Add(EntryReq1, EShadowSlaveStoryContentState::Completed);
	SavedRecord.EntryStates.Add(EntryReq2, EShadowSlaveStoryContentState::Completed);
	SaveData.StoryContents.Add(SavedRecord);

	TestTrue(TEXT("ImportSaveData succeeds"), StorySub->ImportSaveData(SaveData));

	// Reconciled parent must normalize to Completed
	TestEqual(TEXT("Active parent with all entries completed must reconcile to Completed"),
		StorySub->GetStoryContentState(ArcId), EShadowSlaveStoryContentState::Completed);

	// Completed parent must have NAME_None as CurrentActiveEntryId
	TestEqual(TEXT("CurrentActiveEntryId must be NAME_None under Completed parent"),
		StorySub->GetCurrentActiveStoryContentEntry(ArcId), NAME_None);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveStoryRestoreEnforcesNonActiveChildTest,
	"ShadowSlave.Story.RestoreEnforcesNonActiveChildForLockedAndAvailable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveStoryRestoreEnforcesNonActiveChildTest::RunTest(const FString& Parameters)
{
	UShadowSlaveStorySubsystem* StorySub = NewObject<UShadowSlaveStorySubsystem>();
	TestNotNull(TEXT("StorySubsystem must instantiate"), StorySub);
	if (!StorySub)
	{
		return false;
	}

	const FName ArcId = FName("Arc_Test_LockedParent");
	const FName Entry1 = FName("Entry_Child_1");

	UShadowSlaveStoryContentDefinition* ContentDef = NewObject<UShadowSlaveStoryContentDefinition>();
	ContentDef->SetStoryContentId(ArcId);
	ContentDef->Version = 1;

	FShadowSlaveStoryContentEntry EntryDef1;
	EntryDef1.ContentId = Entry1;
	ContentDef->ContentEntries.Add(EntryDef1);
	StorySub->RegisterStoryContentDefinition(ContentDef);

	// Craft corrupt save where parent is Locked, but child was marked Active
	FShadowSlaveStorySaveData SaveData;
	SaveData.bIsValid = true;
	SaveData.StorySubsystemVersion = 1;

	FShadowSlaveStoryContentRecordSaveData SavedRecord;
	SavedRecord.StoryContentId = ArcId;
	SavedRecord.ContentVersion = 1;
	SavedRecord.State = EShadowSlaveStoryContentState::Locked;
	SavedRecord.CurrentActiveEntryId = Entry1;
	SavedRecord.EntryStates.Add(Entry1, EShadowSlaveStoryContentState::Active);
	SaveData.StoryContents.Add(SavedRecord);

	TestTrue(TEXT("ImportSaveData succeeds"), StorySub->ImportSaveData(SaveData));

	TestEqual(TEXT("Parent must remain Locked"), StorySub->GetStoryContentState(ArcId), EShadowSlaveStoryContentState::Locked);
	TestEqual(TEXT("CurrentActiveEntryId must be NAME_None"), StorySub->GetCurrentActiveStoryContentEntry(ArcId), NAME_None);

	// Child entry must NOT be restored as Active
	TestFalse(TEXT("Child entry cannot remain Active under Locked parent"),
		StorySub->GetStoryContentEntryState(ArcId, Entry1) == EShadowSlaveStoryContentState::Active);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveStoryRestoreResolvesValidActiveEntryTest,
	"ShadowSlave.Story.RestoreEnforcesValidCurrentActiveEntry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveStoryRestoreResolvesValidActiveEntryTest::RunTest(const FString& Parameters)
{
	UShadowSlaveStorySubsystem* StorySub = NewObject<UShadowSlaveStorySubsystem>();
	TestNotNull(TEXT("StorySubsystem must instantiate"), StorySub);
	if (!StorySub)
	{
		return false;
	}

	const FName ArcId = FName("Arc_Test_ActiveResolution");
	const FName Entry1 = FName("Entry_Valid_1");
	const FName Entry2 = FName("Entry_Valid_2");

	UShadowSlaveStoryContentDefinition* ContentDef = NewObject<UShadowSlaveStoryContentDefinition>();
	ContentDef->SetStoryContentId(ArcId);
	ContentDef->Version = 1;

	FShadowSlaveStoryContentEntry EntryDef1;
	EntryDef1.ContentId = Entry1;

	FShadowSlaveStoryContentEntry EntryDef2;
	EntryDef2.ContentId = Entry2;
	EntryDef2.PrerequisiteContentIds.Add(Entry1);

	ContentDef->ContentEntries.Add(EntryDef1);
	ContentDef->ContentEntries.Add(EntryDef2);
	StorySub->RegisterStoryContentDefinition(ContentDef);

	// Save data has parent = Active, but CurrentActiveEntryId is an invalid nonexistent ID
	FShadowSlaveStorySaveData SaveData;
	SaveData.bIsValid = true;
	SaveData.StorySubsystemVersion = 1;

	FShadowSlaveStoryContentRecordSaveData SavedRecord;
	SavedRecord.StoryContentId = ArcId;
	SavedRecord.ContentVersion = 1;
	SavedRecord.State = EShadowSlaveStoryContentState::Active;
	SavedRecord.CurrentActiveEntryId = FName("Corrupt_Nonexistent_Entry");
	SavedRecord.EntryStates.Add(Entry1, EShadowSlaveStoryContentState::Locked);
	SavedRecord.EntryStates.Add(Entry2, EShadowSlaveStoryContentState::Locked);
	SaveData.StoryContents.Add(SavedRecord);

	TestTrue(TEXT("ImportSaveData succeeds"), StorySub->ImportSaveData(SaveData));

	TestEqual(TEXT("Parent is Active"), StorySub->GetStoryContentState(ArcId), EShadowSlaveStoryContentState::Active);

	// Deterministic selection must resolve to Entry1 (first valid entry with satisfied prerequisites)
	const FName ResolvedEntryId = StorySub->GetCurrentActiveStoryContentEntry(ArcId);
	TestEqual(TEXT("CurrentActiveEntryId must resolve deterministically to Entry1"), ResolvedEntryId, Entry1);
	TestEqual(TEXT("Resolved entry state must be Active"),
		StorySub->GetStoryContentEntryState(ArcId, Entry1), EShadowSlaveStoryContentState::Active);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveStoryPrerequisitesRespectedDuringRestoreTest,
	"ShadowSlave.Story.PrerequisitesRespectedDuringRestore",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveStoryPrerequisitesRespectedDuringRestoreTest::RunTest(const FString& Parameters)
{
	UShadowSlaveStorySubsystem* StorySub = NewObject<UShadowSlaveStorySubsystem>();
	TestNotNull(TEXT("StorySubsystem must instantiate"), StorySub);
	if (!StorySub)
	{
		return false;
	}

	const FName ArcA = FName("Arc_Prereq_A");
	const FName ArcB = FName("Arc_Prereq_B");

	UShadowSlaveStoryContentDefinition* DefA = NewObject<UShadowSlaveStoryContentDefinition>();
	DefA->SetStoryContentId(ArcA);
	DefA->Version = 1;

	UShadowSlaveStoryContentDefinition* DefB = NewObject<UShadowSlaveStoryContentDefinition>();
	DefB->SetStoryContentId(ArcB);
	DefB->Version = 1;
	DefB->PrerequisiteStoryContentIds.Add(ArcA);

	StorySub->RegisterStoryContentDefinition(DefA);
	StorySub->RegisterStoryContentDefinition(DefB);

	// Save data has ArcB as Available, but ArcA is only Locked (not Completed)
	FShadowSlaveStorySaveData SaveData;
	SaveData.bIsValid = true;
	SaveData.StorySubsystemVersion = 1;

	FShadowSlaveStoryContentRecordSaveData RecordA;
	RecordA.StoryContentId = ArcA;
	RecordA.ContentVersion = 1;
	RecordA.State = EShadowSlaveStoryContentState::Locked;

	FShadowSlaveStoryContentRecordSaveData RecordB;
	RecordB.StoryContentId = ArcB;
	RecordB.ContentVersion = 1;
	RecordB.State = EShadowSlaveStoryContentState::Available;

	SaveData.StoryContents.Add(RecordA);
	SaveData.StoryContents.Add(RecordB);

	TestTrue(TEXT("ImportSaveData succeeds"), StorySub->ImportSaveData(SaveData));

	// ArcB must fall back to Locked because ArcA is not Completed!
	TestEqual(TEXT("ArcB must fall back to Locked when prerequisite ArcA is not Completed"),
		StorySub->GetStoryContentState(ArcB), EShadowSlaveStoryContentState::Locked);

	return true;
}

// ------------------------------------------------------------------------------------------------
// Step 41 — Story Content Definition Content Pipeline Integration Tests
// ------------------------------------------------------------------------------------------------

// 1. DefinitionUsesGenericContentBase Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveStoryContentDefinitionUsesGenericContentBaseTest,
	"ShadowSlave.StoryContent.DefinitionUsesGenericContentBase",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveStoryContentDefinitionUsesGenericContentBaseTest::RunTest(const FString& Parameters)
{
	UShadowSlaveStoryContentDefinition* StoryDef = NewObject<UShadowSlaveStoryContentDefinition>();
	TestNotNull(TEXT("StoryDef must be instantiable"), StoryDef);

	// C++ type hierarchy verification
	UShadowSlaveContentDefinition* ContentDef = Cast<UShadowSlaveContentDefinition>(StoryDef);
	TestNotNull(TEXT("StoryContent definition must cast to UShadowSlaveContentDefinition"), ContentDef);

	UPrimaryDataAsset* PrimaryDataAsset = Cast<UPrimaryDataAsset>(StoryDef);
	TestNotNull(TEXT("StoryContent definition must cast to UPrimaryDataAsset"), PrimaryDataAsset);

	// Unreal reflection hierarchy verification
	TestTrue(TEXT("StaticClass must be child of UShadowSlaveContentDefinition"),
		UShadowSlaveStoryContentDefinition::StaticClass()->IsChildOf(UShadowSlaveContentDefinition::StaticClass()));
	TestTrue(TEXT("StaticClass must be child of UPrimaryDataAsset"),
		UShadowSlaveStoryContentDefinition::StaticClass()->IsChildOf(UPrimaryDataAsset::StaticClass()));

	return true;
}

// 2. DefinitionUsesStoryContentType Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveStoryContentDefinitionUsesStoryContentTypeTest,
	"ShadowSlave.StoryContent.DefinitionUsesStoryContentType",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveStoryContentDefinitionUsesStoryContentTypeTest::RunTest(const FString& Parameters)
{
	UShadowSlaveStoryContentDefinition* StoryDef = NewObject<UShadowSlaveStoryContentDefinition>();
	TestNotNull(TEXT("StoryDef must be instantiable"), StoryDef);

	// Generic ContentType must be Story
	TestEqual(TEXT("Default ContentType must be Story"), StoryDef->ContentType, EShadowSlaveContentType::Story);

	// Must NOT report other generic categories
	TestFalse(TEXT("Must not be None"), StoryDef->ContentType == EShadowSlaveContentType::None);
	TestFalse(TEXT("Must not be Memory"), StoryDef->ContentType == EShadowSlaveContentType::Memory);
	TestFalse(TEXT("Must not be Echo"), StoryDef->ContentType == EShadowSlaveContentType::Echo);
	TestFalse(TEXT("Must not be Item"), StoryDef->ContentType == EShadowSlaveContentType::Item);
	TestFalse(TEXT("Must not be Quest"), StoryDef->ContentType == EShadowSlaveContentType::Quest);
	TestFalse(TEXT("Must not be Custom"), StoryDef->ContentType == EShadowSlaveContentType::Custom);

	// Verify Story-specific content taxonomy remains intact and separate for child entries
	FShadowSlaveStoryContentEntry EntryQuest;
	EntryQuest.ContentId = FName(TEXT("Entry_Test_Quest"));
	EntryQuest.ContentType = EShadowSlaveStoryContentType::Quest;
	EntryQuest.TargetId = FName(TEXT("Quest_Target_01"));
	TestEqual(TEXT("Entry ContentType is Quest"), EntryQuest.ContentType, EShadowSlaveStoryContentType::Quest);

	FShadowSlaveStoryContentEntry EntryDialogue;
	EntryDialogue.ContentId = FName(TEXT("Entry_Test_Dialogue"));
	EntryDialogue.ContentType = EShadowSlaveStoryContentType::Dialogue;
	EntryDialogue.TargetId = FName(TEXT("Dialogue_Target_01"));
	TestEqual(TEXT("Entry ContentType is Dialogue"), EntryDialogue.ContentType, EShadowSlaveStoryContentType::Dialogue);

	FShadowSlaveStoryContentEntry EntryNightmare;
	EntryNightmare.ContentId = FName(TEXT("Entry_Test_Nightmare"));
	EntryNightmare.ContentType = EShadowSlaveStoryContentType::Nightmare;
	EntryNightmare.TargetId = FName(TEXT("Nightmare_Scenario_01"));
	TestEqual(TEXT("Entry ContentType is Nightmare"), EntryNightmare.ContentType, EShadowSlaveStoryContentType::Nightmare);

	FShadowSlaveStoryContentEntry EntryWorldState;
	EntryWorldState.ContentId = FName(TEXT("Entry_Test_WorldState"));
	EntryWorldState.ContentType = EShadowSlaveStoryContentType::WorldState;
	EntryWorldState.TargetId = FName(TEXT("WS_Key_01"));
	TestEqual(TEXT("Entry ContentType is WorldState"), EntryWorldState.ContentType, EShadowSlaveStoryContentType::WorldState);

	return true;
}

// 3. DefinitionHasSingleAuthoritativeId Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveStoryContentDefinitionHasSingleAuthoritativeIdTest,
	"ShadowSlave.StoryContent.DefinitionHasSingleAuthoritativeId",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveStoryContentDefinitionHasSingleAuthoritativeIdTest::RunTest(const FString& Parameters)
{
	UShadowSlaveStoryContentDefinition* StoryDef = NewObject<UShadowSlaveStoryContentDefinition>();
	TestNotNull(TEXT("StoryDef must be instantiable"), StoryDef);

	// Verify ContentId is the sole stored identifier property
	FProperty* ContentIdProp = UShadowSlaveStoryContentDefinition::StaticClass()->FindPropertyByName(TEXT("ContentId"));
	TestNotNull(TEXT("ContentId property must exist in reflection"), ContentIdProp);

	// Verify NO second stored StoryContentId property exists in reflection
	FProperty* StoryContentIdProp = UShadowSlaveStoryContentDefinition::StaticClass()->FindPropertyByName(TEXT("StoryContentId"));
	TestNull(TEXT("StoryContentId property must NOT exist in reflection (no duplicate stored identifier)"), StoryContentIdProp);

	// Verify compatibility accessors read and write ContentId
	const FName TestId(TEXT("Test_Authoritative_Story"));
	StoryDef->SetStoryContentId(TestId);
	TestEqual(TEXT("GetStoryContentId() must return the value set via SetStoryContentId()"), StoryDef->GetStoryContentId(), TestId);
	TestEqual(TEXT("ContentId must match the value set via SetStoryContentId()"), StoryDef->ContentId, TestId);

	const FName DirectId(TEXT("Test_Direct_StoryContentId"));
	StoryDef->ContentId = DirectId;
	TestEqual(TEXT("GetStoryContentId() must return value written directly to ContentId"), StoryDef->GetStoryContentId(), DirectId);

	return true;
}

// 4. DefinitionValidation Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveStoryContentDefinitionValidationTest,
	"ShadowSlave.StoryContent.DefinitionValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveStoryContentDefinitionValidationTest::RunTest(const FString& Parameters)
{
	// 1. Valid definition passes validation
	UShadowSlaveStoryContentDefinition* ValidDef = NewObject<UShadowSlaveStoryContentDefinition>();
	ValidDef->ContentId = FName(TEXT("Arc_Test_ValidStory"));
	ValidDef->Version = 1;

	FShadowSlaveStoryContentEntry ValidEntry;
	ValidEntry.ContentId = FName(TEXT("Entry_Test_01"));
	ValidEntry.ContentType = EShadowSlaveStoryContentType::Quest;
	ValidEntry.TargetId = FName(TEXT("Quest_01"));
	ValidDef->ContentEntries.Add(ValidEntry);

	FString ErrorMsg;
	TestTrue(TEXT("Valid story content passes IsValidDefinition"), ValidDef->IsValidDefinition(&ErrorMsg));
	TArray<FText> Errors;
	TestTrue(TEXT("Valid story content passes ValidateDefinition"), ValidDef->ValidateDefinition(Errors));
	TestEqual(TEXT("Errors array must be empty for valid definition"), Errors.Num(), 0);

	// 2. NAME_None ContentId fails validation
	ValidDef->ContentId = NAME_None;
	TestFalse(TEXT("NAME_None ContentId must fail IsValidDefinition"), ValidDef->IsValidDefinition(&ErrorMsg));
	TestFalse(TEXT("Error message must be populated for None ContentId"), ErrorMsg.IsEmpty());
	TestFalse(TEXT("NAME_None ContentId must fail ValidateDefinition"), ValidDef->ValidateDefinition(Errors));

	// 3. Version < 1 fails validation
	ValidDef->ContentId = FName(TEXT("Arc_Test_ValidStory"));
	ValidDef->Version = 0;
	TestFalse(TEXT("Version 0 must fail IsValidDefinition"), ValidDef->IsValidDefinition(&ErrorMsg));
	TestFalse(TEXT("Version 0 must fail ValidateDefinition"), ValidDef->ValidateDefinition(Errors));

	// 4. Incorrect generic ContentType fails validation
	ValidDef->Version = 1;
	ValidDef->ContentType = EShadowSlaveContentType::Memory;
	TestFalse(TEXT("ContentType != Story must fail IsValidDefinition"), ValidDef->IsValidDefinition(&ErrorMsg));
	TestFalse(TEXT("ContentType != Story must fail ValidateDefinition"), ValidDef->ValidateDefinition(Errors));

	// 5. Existing story-specific validation: Self-prerequisite fails
	ValidDef->ContentType = EShadowSlaveContentType::Story;
	ValidDef->PrerequisiteStoryContentIds.Add(ValidDef->ContentId);
	TestFalse(TEXT("Self-prerequisite must fail IsValidDefinition"), ValidDef->IsValidDefinition(&ErrorMsg));
	TestFalse(TEXT("Self-prerequisite must fail ValidateDefinition"), ValidDef->ValidateDefinition(Errors));
	ValidDef->PrerequisiteStoryContentIds.Empty();

	// 6. Existing story-specific validation: None TargetId for Quest entry fails
	ValidDef->ContentEntries[0].TargetId = NAME_None;
	TestFalse(TEXT("Missing TargetId for Quest must fail IsValidDefinition"), ValidDef->IsValidDefinition(&ErrorMsg));
	TestFalse(TEXT("Missing TargetId for Quest must fail ValidateDefinition"), ValidDef->ValidateDefinition(Errors));

	// Restored definition passes again
	ValidDef->ContentEntries[0].TargetId = FName(TEXT("Quest_01"));
	TestTrue(TEXT("Restored definition passes IsValidDefinition"), ValidDef->IsValidDefinition());
	TestTrue(TEXT("Restored definition passes ValidateDefinition"), ValidDef->ValidateDefinition(Errors));

	return true;
}

// 5. DefinitionRegistryIntegration Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveStoryContentDefinitionRegistryIntegrationTest,
	"ShadowSlave.StoryContent.DefinitionRegistryIntegration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveStoryContentDefinitionRegistryIntegrationTest::RunTest(const FString& Parameters)
{
	UShadowSlaveContentRegistrySubsystem* Registry = NewObject<UShadowSlaveContentRegistrySubsystem>();
	TestNotNull(TEXT("Registry subsystem must be created"), Registry);

	UShadowSlaveStoryContentDefinition* StoryDef = NewObject<UShadowSlaveStoryContentDefinition>();
	StoryDef->ContentId = FName(TEXT("Arc_Registry_Test"));
	StoryDef->DisplayName = FText::FromString(TEXT("Test Story Arc"));
	StoryDef->Version = 1;

	FShadowSlaveStoryContentEntry Entry1;
	Entry1.ContentId = FName(TEXT("Entry_Reg_01"));
	Entry1.ContentType = EShadowSlaveStoryContentType::Custom;
	StoryDef->ContentEntries.Add(Entry1);

	// Register with generic registry subsystem
	const bool bRegistered = Registry->RegisterDefinition(StoryDef);
	TestTrue(TEXT("RegisterDefinition must succeed for UShadowSlaveStoryContentDefinition"), bRegistered);

	// Query existence
	TestTrue(TEXT("HasContent must return true for registered StoryDef"), Registry->HasContent(StoryDef->ContentId));
	TestEqual(TEXT("Total registered count must be 1"), Registry->GetRegisteredContentCount(), 1);
	TestEqual(TEXT("Story count by type must be 1"), Registry->GetRegisteredContentCountByType(EShadowSlaveContentType::Story), 1);
	TestEqual(TEXT("Memory count by type must be 0"), Registry->GetRegisteredContentCountByType(EShadowSlaveContentType::Memory), 0);
	TestEqual(TEXT("Echo count by type must be 0"), Registry->GetRegisteredContentCountByType(EShadowSlaveContentType::Echo), 0);
	TestEqual(TEXT("Custom count by type must be 0"), Registry->GetRegisteredContentCountByType(EShadowSlaveContentType::Custom), 0);

	// Generic resolution
	UShadowSlaveContentDefinition* ResolvedGeneric = Registry->ResolveContentDefinition(StoryDef->ContentId);
	TestNotNull(TEXT("Resolved generic definition must not be null"), ResolvedGeneric);
	TestEqual(TEXT("Resolved generic definition must match StoryDef"), ResolvedGeneric, Cast<UShadowSlaveContentDefinition>(StoryDef));

	// Typed resolution
	UShadowSlaveStoryContentDefinition* ResolvedStory = Registry->ResolveContentDefinition<UShadowSlaveStoryContentDefinition>(StoryDef->ContentId);
	TestNotNull(TEXT("Resolved typed story definition must not be null"), ResolvedStory);
	TestEqual(TEXT("Resolved typed story must match original StoryDef"), ResolvedStory, StoryDef);
	TestEqual(TEXT("Resolved ContentEntries count matches"), ResolvedStory->ContentEntries.Num(), 1);

	// PrimaryAssetId verification
	const FPrimaryAssetId ExpectedAssetId(TEXT("Story"), StoryDef->ContentId);
	TestEqual(TEXT("GetPrimaryAssetId must match expected PrimaryAssetId"), StoryDef->GetPrimaryAssetId(), ExpectedAssetId);

	return true;
}

// 6. ExistingRuntimeCompatibility Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveStoryContentExistingRuntimeCompatibilityTest,
	"ShadowSlave.StoryContent.ExistingRuntimeCompatibility",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveStoryContentExistingRuntimeCompatibilityTest::RunTest(const FString& Parameters)
{
	UShadowSlaveStorySubsystem* StorySub = NewObject<UShadowSlaveStorySubsystem>();
	TestNotNull(TEXT("StorySubsystem must instantiate"), StorySub);

	const FName ArcId = FName("Arc_Runtime_Compat");
	const FName Entry1 = FName("Entry_Compat_1");
	const FName Entry2 = FName("Entry_Compat_2");

	UShadowSlaveStoryContentDefinition* ContentDef = NewObject<UShadowSlaveStoryContentDefinition>();
	ContentDef->SetStoryContentId(ArcId);
	ContentDef->Version = 1;

	FShadowSlaveStoryContentEntry EntryDef1;
	EntryDef1.ContentId = Entry1;
	EntryDef1.ContentType = EShadowSlaveStoryContentType::Quest;
	EntryDef1.TargetId = FName("Quest_Compat_1");
	EntryDef1.bIsOptional = false;

	FShadowSlaveStoryContentEntry EntryDef2;
	EntryDef2.ContentId = Entry2;
	EntryDef2.ContentType = EShadowSlaveStoryContentType::Dialogue;
	EntryDef2.TargetId = FName("Dialogue_Compat_2");
	EntryDef2.bIsOptional = false;
	EntryDef2.PrerequisiteContentIds.Add(Entry1);

	ContentDef->ContentEntries.Add(EntryDef1);
	ContentDef->ContentEntries.Add(EntryDef2);

	// 1. Registration
	TestTrue(TEXT("RegisterStoryContentDefinition succeeds"), StorySub->RegisterStoryContentDefinition(ContentDef));
	TestTrue(TEXT("HasStoryContentDefinition succeeds"), StorySub->HasStoryContentDefinition(ArcId));
	TestEqual(TEXT("Initial state is Available (no prerequisites)"),
		StorySub->GetStoryContentState(ArcId), EShadowSlaveStoryContentState::Available);

	// 2. Activation
	TestTrue(TEXT("ActivateStoryContent succeeds"), StorySub->ActivateStoryContent(ArcId));
	TestTrue(TEXT("IsStoryContentActive returns true"), StorySub->IsStoryContentActive(ArcId));
	TestEqual(TEXT("CurrentActiveEntryId is Entry1"),
		StorySub->GetCurrentActiveStoryContentEntry(ArcId), Entry1);

	// 3. Step transition: Complete Entry1
	TestTrue(TEXT("CompleteStoryContentEntry succeeds"), StorySub->CompleteStoryContentEntry(ArcId, Entry1));
	TestEqual(TEXT("Entry1 is Completed"),
		StorySub->GetStoryContentEntryState(ArcId, Entry1), EShadowSlaveStoryContentState::Completed);

	// Progression advances to Entry2
	StorySub->AdvanceStoryContentProgression(ArcId);
	TestEqual(TEXT("CurrentActiveEntryId advances to Entry2"),
		StorySub->GetCurrentActiveStoryContentEntry(ArcId), Entry2);

	// 4. Complete Entry2 and Arc
	TestTrue(TEXT("CompleteStoryContentEntry for Entry2 succeeds"), StorySub->CompleteStoryContentEntry(ArcId, Entry2));
	TestTrue(TEXT("CompleteStoryContent succeeds"), StorySub->CompleteStoryContent(ArcId));
	TestTrue(TEXT("IsStoryContentCompleted returns true"), StorySub->IsStoryContentCompleted(ArcId));

	// 5. Save/Load export and restore
	FShadowSlaveStorySaveData SaveData;
	StorySub->ExportSaveData(SaveData);
	TestTrue(TEXT("ExportSaveData produces valid save"), SaveData.bIsValid);

	UShadowSlaveStorySubsystem* RestoredSub = NewObject<UShadowSlaveStorySubsystem>();
	RestoredSub->RegisterStoryContentDefinition(ContentDef);
	TestTrue(TEXT("ImportSaveData succeeds on restored subsystem"), RestoredSub->ImportSaveData(SaveData));
	TestTrue(TEXT("Restored subsystem preserves Completed state"), RestoredSub->IsStoryContentCompleted(ArcId));
	TestEqual(TEXT("Restored subsystem preserves Entry1 Completed"),
		RestoredSub->GetStoryContentEntryState(ArcId, Entry1), EShadowSlaveStoryContentState::Completed);
	TestEqual(TEXT("Restored subsystem preserves Entry2 Completed"),
		RestoredSub->GetStoryContentEntryState(ArcId, Entry2), EShadowSlaveStoryContentState::Completed);

	return true;
}

#endif // WITH_AUTOMATION_TESTS
