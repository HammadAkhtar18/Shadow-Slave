// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_AUTOMATION_TESTS

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Content/ShadowSlaveContentTypes.h"
#include "Content/ShadowSlaveContentDefinition.h"
#include "Content/ShadowSlaveContentRegistrySubsystem.h"
#include "Core/ShadowSlaveGameplayTagTypes.h"

// 1. ValidRegistrationAndLookup Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveContentRegistryValidRegistrationAndLookupTest,
	"ShadowSlave.ContentRegistry.ValidRegistrationAndLookup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveContentRegistryValidRegistrationAndLookupTest::RunTest(const FString& Parameters)
{
	UShadowSlaveContentRegistrySubsystem* Registry = NewObject<UShadowSlaveContentRegistrySubsystem>();
	TestNotNull(TEXT("Registry subsystem must be created"), Registry);

	UShadowSlaveContentDefinition* Def = NewObject<UShadowSlaveContentDefinition>();
	Def->ContentId = FName(TEXT("Test_Item_01"));
	Def->ContentType = EShadowSlaveContentType::Item;
	Def->DisplayName = FText::FromString(TEXT("Test Item"));
	Def->Description = FText::FromString(TEXT("A test item definition for registration verification."));
	Def->Version = 1;
	Def->ProvenanceNote = TEXT("Automated Test Artifact");
	Def->MetadataTags.AddTag(ShadowSlaveGameplayTags::Interaction_Interactable);

	const bool bRegistered = Registry->RegisterDefinition(Def);
	TestTrue(TEXT("RegisterDefinition must return true for valid definition"), bRegistered);

	// Existence check
	TestTrue(TEXT("HasContent must return true for registered ID"), Registry->HasContent(FName(TEXT("Test_Item_01"))));
	TestEqual(TEXT("Total registered count must be 1"), Registry->GetRegisteredContentCount(), 1);
	TestEqual(TEXT("Item count must be 1"), Registry->GetRegisteredContentCountByType(EShadowSlaveContentType::Item), 1);
	TestEqual(TEXT("Memory count must be 0"), Registry->GetRegisteredContentCountByType(EShadowSlaveContentType::Memory), 0);

	// Direct resolution
	UShadowSlaveContentDefinition* Resolved = Registry->ResolveContentDefinition(FName(TEXT("Test_Item_01")));
	TestNotNull(TEXT("Resolved definition must not be null"), Resolved);
	TestEqual(TEXT("Resolved definition must match registered definition"), Resolved, Def);
	TestEqual(TEXT("Resolved definition ContentId must match"), Resolved->ContentId, FName(TEXT("Test_Item_01")));
	TestEqual(TEXT("Resolved definition ContentType must match"), Resolved->ContentType, EShadowSlaveContentType::Item);
	TestTrue(TEXT("Resolved definition has expected metadata tag"), Resolved->MetadataTags.HasTag(ShadowSlaveGameplayTags::Interaction_Interactable));

	// Loaded check
	TestTrue(TEXT("IsDefinitionLoaded must return true for registered in-memory definition"), Registry->IsDefinitionLoaded(FName(TEXT("Test_Item_01"))));

	// Entry lookup
	FShadowSlaveContentRegistryEntry FoundEntry;
	TestTrue(TEXT("FindEntry must succeed for registered ID"), Registry->FindEntry(FName(TEXT("Test_Item_01")), FoundEntry));
	TestEqual(TEXT("FoundEntry ContentId must match"), FoundEntry.ContentId, FName(TEXT("Test_Item_01")));
	TestEqual(TEXT("FoundEntry ContentType must match"), FoundEntry.ContentType, EShadowSlaveContentType::Item);
	TestEqual(TEXT("FoundEntry LoadedDefinition must match"), FoundEntry.LoadedDefinition.Get(), Def);

	return true;
}

// 2. UnknownIdFailsCleanly Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveContentRegistryUnknownIdFailsCleanlyTest,
	"ShadowSlave.ContentRegistry.UnknownIdFailsCleanly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveContentRegistryUnknownIdFailsCleanlyTest::RunTest(const FString& Parameters)
{
	UShadowSlaveContentRegistrySubsystem* Registry = NewObject<UShadowSlaveContentRegistrySubsystem>();
	TestNotNull(TEXT("Registry subsystem must be created"), Registry);

	// None ID lookup
	TestFalse(TEXT("HasContent(NAME_None) must return false"), Registry->HasContent(NAME_None));
	TestNull(TEXT("ResolveContentDefinition(NAME_None) must return nullptr"), Registry->ResolveContentDefinition(NAME_None));
	TestFalse(TEXT("IsDefinitionLoaded(NAME_None) must return false"), Registry->IsDefinitionLoaded(NAME_None));

	FShadowSlaveContentRegistryEntry EntryNone;
	TestFalse(TEXT("FindEntry(NAME_None) must return false"), Registry->FindEntry(NAME_None, EntryNone));

	// Arbitrary unknown ID lookup
	const FName UnknownId(TEXT("NonExistent_Content_9999"));
	TestFalse(TEXT("HasContent for unknown ID must return false"), Registry->HasContent(UnknownId));
	TestNull(TEXT("ResolveContentDefinition for unknown ID must return nullptr"), Registry->ResolveContentDefinition(UnknownId));
	TestFalse(TEXT("IsDefinitionLoaded for unknown ID must return false"), Registry->IsDefinitionLoaded(UnknownId));

	FShadowSlaveContentRegistryEntry EntryUnknown;
	TestFalse(TEXT("FindEntry for unknown ID must return false"), Registry->FindEntry(UnknownId, EntryUnknown));

	return true;
}

// 3. DuplicateIdRejected Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveContentRegistryDuplicateIdRejectedTest,
	"ShadowSlave.ContentRegistry.DuplicateIdRejected",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveContentRegistryDuplicateIdRejectedTest::RunTest(const FString& Parameters)
{
	UShadowSlaveContentRegistrySubsystem* Registry = NewObject<UShadowSlaveContentRegistrySubsystem>();
	TestNotNull(TEXT("Registry subsystem must be created"), Registry);

	const FName DuplicateId(TEXT("Duplicate_Content_ID"));

	UShadowSlaveContentDefinition* DefA = NewObject<UShadowSlaveContentDefinition>();
	DefA->ContentId = DuplicateId;
	DefA->ContentType = EShadowSlaveContentType::Echo;
	DefA->DisplayName = FText::FromString(TEXT("Original Echo"));
	DefA->Version = 1;

	UShadowSlaveContentDefinition* DefB = NewObject<UShadowSlaveContentDefinition>();
	DefB->ContentId = DuplicateId;
	DefB->ContentType = EShadowSlaveContentType::Echo;
	DefB->DisplayName = FText::FromString(TEXT("Duplicate Echo"));
	DefB->Version = 2;

	const bool bFirstRegistered = Registry->RegisterDefinition(DefA);
	TestTrue(TEXT("First registration must succeed"), bFirstRegistered);

	const bool bSecondRegistered = Registry->RegisterDefinition(DefB);
	TestFalse(TEXT("Duplicate registration must be rejected and return false"), bSecondRegistered);

	TestEqual(TEXT("Total registered count must remain 1"), Registry->GetRegisteredContentCount(), 1);

	UShadowSlaveContentDefinition* Resolved = Registry->ResolveContentDefinition(DuplicateId);
	TestEqual(TEXT("Resolved definition must still be the original DefA"), Resolved, DefA);

	return true;
}

// 4. InvalidDefinitionValidation Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveContentRegistryInvalidDefinitionValidationTest,
	"ShadowSlave.ContentRegistry.InvalidDefinitionValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveContentRegistryInvalidDefinitionValidationTest::RunTest(const FString& Parameters)
{
	// 1. Definition with NAME_None ContentId is invalid
	UShadowSlaveContentDefinition* DefNoId = NewObject<UShadowSlaveContentDefinition>();
	DefNoId->ContentId = NAME_None;
	DefNoId->Version = 1;

	FString ErrorMsg;
	TestFalse(TEXT("Definition with None ContentId must fail IsValidDefinition"), DefNoId->IsValidDefinition(&ErrorMsg));
	TestFalse(TEXT("Error message must not be empty"), ErrorMsg.IsEmpty());

	// 2. Definition with Version < 1 is invalid
	UShadowSlaveContentDefinition* DefBadVersion = NewObject<UShadowSlaveContentDefinition>();
	DefBadVersion->ContentId = FName(TEXT("Bad_Version_Content"));
	DefBadVersion->Version = 0;

	TestFalse(TEXT("Definition with Version 0 must fail IsValidDefinition"), DefBadVersion->IsValidDefinition(&ErrorMsg));

	// 3. Registering invalid definitions into the registry must fail
	UShadowSlaveContentRegistrySubsystem* Registry = NewObject<UShadowSlaveContentRegistrySubsystem>();
	TestNotNull(TEXT("Registry subsystem must be created"), Registry);

	TestFalse(TEXT("Registering null definition must return false"), Registry->RegisterDefinition(nullptr));
	TestFalse(TEXT("Registering DefNoId must return false"), Registry->RegisterDefinition(DefNoId));
	TestFalse(TEXT("Registering DefBadVersion must return false"), Registry->RegisterDefinition(DefBadVersion));
	TestEqual(TEXT("Registry must remain empty after invalid registration attempts"), Registry->GetRegisteredContentCount(), 0);

	// 4. Registering empty/malformed FShadowSlaveContentRegistryEntry must fail
	FShadowSlaveContentRegistryEntry EmptyEntry;
	TestFalse(TEXT("Empty entry must not be valid"), EmptyEntry.IsValid());
	TestFalse(TEXT("Registering empty entry must return false"), Registry->RegisterEntry(EmptyEntry));

	return true;
}

// 5. SoftReferenceHandling Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveContentRegistrySoftReferenceHandlingTest,
	"ShadowSlave.ContentRegistry.SoftReferenceHandling",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveContentRegistrySoftReferenceHandlingTest::RunTest(const FString& Parameters)
{
	UShadowSlaveContentRegistrySubsystem* Registry = NewObject<UShadowSlaveContentRegistrySubsystem>();
	TestNotNull(TEXT("Registry subsystem must be created"), Registry);

	// Register entry via soft reference without pre-loading
	const FName SoftContentId(TEXT("Soft_Story_Chapter_01"));
	const FSoftObjectPath DummyPath(TEXT("/Game/ShadowSlave/Content/Stories/Story_Chapter_01.Story_Chapter_01"));

	FShadowSlaveContentRegistryEntry SoftEntry;
	SoftEntry.ContentId = SoftContentId;
	SoftEntry.ContentType = EShadowSlaveContentType::Story;
	SoftEntry.SoftDefinition = TSoftObjectPtr<UShadowSlaveContentDefinition>(DummyPath);

	TestTrue(TEXT("SoftEntry with valid path must be valid"), SoftEntry.IsValid());

	const bool bRegistered = Registry->RegisterEntry(SoftEntry);
	TestTrue(TEXT("Registering soft reference entry must succeed"), bRegistered);

	// HasContent returns true for registered soft reference
	TestTrue(TEXT("HasContent must return true for registered soft reference"), Registry->HasContent(SoftContentId));

	// Definition is not yet loaded in memory
	TestFalse(TEXT("IsDefinitionLoaded must return false prior to loading"), Registry->IsDefinitionLoaded(SoftContentId));

	// Calling Resolve without bAllowSynchronousLoad must safely return nullptr without crashing or loading
	UShadowSlaveContentDefinition* ResolvedUnloaded = Registry->ResolveContentDefinition(SoftContentId, false);
	TestNull(TEXT("Resolve without synchronous load allowed must return nullptr for unloaded soft reference"), ResolvedUnloaded);

	// Calling Resolve with bAllowSynchronousLoad for a non-existent asset path must fail safely and return nullptr
	UShadowSlaveContentDefinition* ResolvedMissing = Registry->ResolveContentDefinition(SoftContentId, true);
	TestNull(TEXT("Synchronous load of non-existent soft path must fail cleanly and return nullptr"), ResolvedMissing);

	return true;
}

// 6. NoGameplaySystemCoupling Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveContentRegistryNoGameplaySystemCouplingTest,
	"ShadowSlave.ContentRegistry.NoGameplaySystemCoupling",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveContentRegistryNoGameplaySystemCouplingTest::RunTest(const FString& Parameters)
{
	// Architecture requirement: Content registry foundation must operate purely on native data
	// without coupling to, or requiring instantiation of:
	// - CombatComponent
	// - StatusEffectComponent
	// - EchoComponent
	// - InventoryComponent
	// - QuestSubsystem / StorySubsystem
	// - GAS AbilitySystemComponent

	UShadowSlaveContentRegistrySubsystem* StandaloneRegistry = NewObject<UShadowSlaveContentRegistrySubsystem>();
	TestNotNull(TEXT("Registry can be instantiated standalone without world or player actors"), StandaloneRegistry);

	// Register definitions across diverse generic categories
	const TArray<EShadowSlaveContentType> Categories = {
		EShadowSlaveContentType::Character,
		EShadowSlaveContentType::Ability,
		EShadowSlaveContentType::Echo,
		EShadowSlaveContentType::Quest,
		EShadowSlaveContentType::Dialogue,
		EShadowSlaveContentType::Story,
		EShadowSlaveContentType::World,
		EShadowSlaveContentType::Custom
	};

	for (int32 i = 0; i < Categories.Num(); ++i)
	{
		UShadowSlaveContentDefinition* Def = NewObject<UShadowSlaveContentDefinition>();
		Def->ContentId = FName(*FString::Printf(TEXT("Standalone_Content_%d"), i));
		Def->ContentType = Categories[i];
		Def->Version = 1;
		TestTrue(TEXT("RegisterDefinition succeeds for generic category"), StandaloneRegistry->RegisterDefinition(Def));
	}

	TestEqual(TEXT("Total registered count must match categories count"), StandaloneRegistry->GetRegisteredContentCount(), Categories.Num());

	// Verify querying by type produces isolated subsets
	TArray<FName> AbilityIds;
	StandaloneRegistry->GetContentIdsByType(EShadowSlaveContentType::Ability, AbilityIds);
	TestEqual(TEXT("Ability query must return exactly 1 entry"), AbilityIds.Num(), 1);
	TestEqual(TEXT("Ability query returned correct ID"), AbilityIds[0], FName(TEXT("Standalone_Content_1")));

	// Verify unregistering cleans up type indices properly
	TestTrue(TEXT("UnregisterEntry succeeds"), StandaloneRegistry->UnregisterEntry(FName(TEXT("Standalone_Content_1"))));
	TestEqual(TEXT("Total count decrements to 7"), StandaloneRegistry->GetRegisteredContentCount(), 7);

	TArray<FName> AbilityIdsAfter;
	StandaloneRegistry->GetContentIdsByType(EShadowSlaveContentType::Ability, AbilityIdsAfter);
	TestEqual(TEXT("Ability query returns 0 after unregister"), AbilityIdsAfter.Num(), 0);
	TestEqual(TEXT("Ability count by type returns 0"), StandaloneRegistry->GetRegisteredContentCountByType(EShadowSlaveContentType::Ability), 0);
	TestFalse(TEXT("HasContent returns false after unregister"), StandaloneRegistry->HasContent(FName(TEXT("Standalone_Content_1"))));

	return true;
}

// 7. UnregisterAndClear Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveContentRegistryUnregisterAndClearTest,
	"ShadowSlave.ContentRegistry.UnregisterAndClear",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveContentRegistryUnregisterAndClearTest::RunTest(const FString& Parameters)
{
	UShadowSlaveContentRegistrySubsystem* Registry = NewObject<UShadowSlaveContentRegistrySubsystem>();
	TestNotNull(TEXT("Registry subsystem must be created"), Registry);

	// Register 3 entries
	for (int32 i = 1; i <= 3; ++i)
	{
		UShadowSlaveContentDefinition* Def = NewObject<UShadowSlaveContentDefinition>();
		Def->ContentId = FName(*FString::Printf(TEXT("Multi_Content_%d"), i));
		Def->ContentType = EShadowSlaveContentType::Item;
		Def->Version = 1;
		Registry->RegisterDefinition(Def);
	}

	TestEqual(TEXT("Count must be 3"), Registry->GetRegisteredContentCount(), 3);

	// Unregister non-existent ID fails
	TestFalse(TEXT("Unregistering unknown ID returns false"), Registry->UnregisterEntry(FName(TEXT("NonExistent_ID"))));
	TestFalse(TEXT("Unregistering NAME_None returns false"), Registry->UnregisterEntry(NAME_None));
	TestEqual(TEXT("Count remains 3"), Registry->GetRegisteredContentCount(), 3);

	// Unregister one entry
	TestTrue(TEXT("Unregistering Multi_Content_2 succeeds"), Registry->UnregisterEntry(FName(TEXT("Multi_Content_2"))));
	TestEqual(TEXT("Count is now 2"), Registry->GetRegisteredContentCount(), 2);
	TestFalse(TEXT("Multi_Content_2 is no longer in registry"), Registry->HasContent(FName(TEXT("Multi_Content_2"))));
	TestTrue(TEXT("Multi_Content_1 still in registry"), Registry->HasContent(FName(TEXT("Multi_Content_1"))));
	TestTrue(TEXT("Multi_Content_3 still in registry"), Registry->HasContent(FName(TEXT("Multi_Content_3"))));

	// Clear registry
	Registry->ClearRegistry();
	TestEqual(TEXT("Count after ClearRegistry must be 0"), Registry->GetRegisteredContentCount(), 0);
	TestFalse(TEXT("Multi_Content_1 is no longer in registry"), Registry->HasContent(FName(TEXT("Multi_Content_1"))));
	TestFalse(TEXT("Multi_Content_3 is no longer in registry"), Registry->HasContent(FName(TEXT("Multi_Content_3"))));

	TArray<FName> AllIds;
	Registry->GetAllRegisteredContentIds(AllIds);
	TestEqual(TEXT("AllIds must be empty after clear"), AllIds.Num(), 0);

	return true;
}

#endif // WITH_AUTOMATION_TESTS
