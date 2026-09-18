// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_AUTOMATION_TESTS

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Story/ShadowSlaveStorySubsystem.h"
#include "Story/ShadowSlaveStoryContentDefinition.h"
#include "Story/ShadowSlaveStoryContentTypes.h"
#include "Story/ShadowSlaveStoryTypes.h"

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
	ContentDef->StoryContentId = ArcId;
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
	ContentDef->StoryContentId = ArcId;
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
	ContentDef->StoryContentId = ArcId;
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
	ContentDef->StoryContentId = ArcId;
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
	DefA->StoryContentId = ArcA;
	DefA->Version = 1;

	UShadowSlaveStoryContentDefinition* DefB = NewObject<UShadowSlaveStoryContentDefinition>();
	DefB->StoryContentId = ArcB;
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

#endif // WITH_AUTOMATION_TESTS
