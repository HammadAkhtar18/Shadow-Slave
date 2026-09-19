// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_AUTOMATION_TESTS

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "GameplayTagContainer.h"
#include "Core/ShadowSlaveGameplayTagTypes.h"

// 1. EmptyContainer Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveGameplayTagsEmptyContainerTest,
	"ShadowSlave.GameplayTags.EmptyContainer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveGameplayTagsEmptyContainerTest::RunTest(const FString& Parameters)
{
	FGameplayTagContainer Container;

	TestTrue(TEXT("New container must be empty"), Container.IsEmpty());
	TestEqual(TEXT("New container tag count must be 0"), Container.Num(), 0);
	TestFalse(TEXT("Empty container must not have State tag"), Container.HasTag(ShadowSlaveGameplayTags::State));
	TestFalse(TEXT("Empty container must not have State_Active tag"), Container.HasTag(ShadowSlaveGameplayTags::State_Active));
	TestFalse(TEXT("Empty container must not have State_Active exact tag"), Container.HasTagExact(ShadowSlaveGameplayTags::State_Active));

	return true;
}

// 2. AddRemove Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveGameplayTagsAddRemoveTest,
	"ShadowSlave.GameplayTags.AddRemove",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveGameplayTagsAddRemoveTest::RunTest(const FString& Parameters)
{
	FGameplayTagContainer Container;

	// AddTag -> HasTag == true
	Container.AddTag(ShadowSlaveGameplayTags::State_Active);
	TestEqual(TEXT("Container must have 1 tag"), Container.Num(), 1);
	TestTrue(TEXT("HasTagExact must return true after AddTag"), Container.HasTagExact(ShadowSlaveGameplayTags::State_Active));
	TestTrue(TEXT("HasTag must return true after AddTag"), Container.HasTag(ShadowSlaveGameplayTags::State_Active));

	// Duplicate AddTag does not duplicate tag in native FGameplayTagContainer
	Container.AddTag(ShadowSlaveGameplayTags::State_Active);
	TestEqual(TEXT("Container count must remain 1 after duplicate AddTag"), Container.Num(), 1);

	// RemoveTag -> HasTag == false
	const bool bRemoved = Container.RemoveTag(ShadowSlaveGameplayTags::State_Active);
	TestTrue(TEXT("RemoveTag must succeed for existing tag"), bRemoved);
	TestEqual(TEXT("Container must be empty after RemoveTag"), Container.Num(), 0);
	TestFalse(TEXT("HasTagExact must return false after RemoveTag"), Container.HasTagExact(ShadowSlaveGameplayTags::State_Active));
	TestFalse(TEXT("HasTag must return false after RemoveTag"), Container.HasTag(ShadowSlaveGameplayTags::State_Active));

	// Repeated RemoveTag returns false
	const bool bRepeatedRemoved = Container.RemoveTag(ShadowSlaveGameplayTags::State_Active);
	TestFalse(TEXT("Repeated RemoveTag must return false"), bRepeatedRemoved);

	return true;
}

// 3. HasAny Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveGameplayTagsHasAnyTest,
	"ShadowSlave.GameplayTags.HasAny",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveGameplayTagsHasAnyTest::RunTest(const FString& Parameters)
{
	FGameplayTagContainer Container;
	Container.AddTag(ShadowSlaveGameplayTags::State_Active);
	Container.AddTag(ShadowSlaveGameplayTags::Combat_Engaged);

	// Query containing 1 overlapping tag
	FGameplayTagContainer OverlappingQuery;
	OverlappingQuery.AddTag(ShadowSlaveGameplayTags::State_Active);
	OverlappingQuery.AddTag(ShadowSlaveGameplayTags::Ability_Action);
	TestTrue(TEXT("HasAny must return true when at least one tag overlaps"), Container.HasAny(OverlappingQuery));

	// Query containing disjoint tags
	FGameplayTagContainer DisjointQuery;
	DisjointQuery.AddTag(ShadowSlaveGameplayTags::Ability_Action);
	DisjointQuery.AddTag(ShadowSlaveGameplayTags::State_Disabled);
	TestFalse(TEXT("HasAny must return false when no tags overlap"), Container.HasAny(DisjointQuery));

	// Empty query
	FGameplayTagContainer EmptyQuery;
	TestFalse(TEXT("HasAny against empty query must return false"), Container.HasAny(EmptyQuery));

	return true;
}

// 4. HasAll Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveGameplayTagsHasAllTest,
	"ShadowSlave.GameplayTags.HasAll",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveGameplayTagsHasAllTest::RunTest(const FString& Parameters)
{
	FGameplayTagContainer Container;
	Container.AddTag(ShadowSlaveGameplayTags::State_Active);
	Container.AddTag(ShadowSlaveGameplayTags::Combat_Engaged);

	// Query with exact subset
	FGameplayTagContainer ExactSubset;
	ExactSubset.AddTag(ShadowSlaveGameplayTags::State_Active);
	ExactSubset.AddTag(ShadowSlaveGameplayTags::Combat_Engaged);
	TestTrue(TEXT("HasAll must return true for exact subset"), Container.HasAll(ExactSubset));

	// Query with partial overlap but missing 1 tag
	FGameplayTagContainer SupersetQuery;
	SupersetQuery.AddTag(ShadowSlaveGameplayTags::State_Active);
	SupersetQuery.AddTag(ShadowSlaveGameplayTags::Combat_Engaged);
	SupersetQuery.AddTag(ShadowSlaveGameplayTags::Ability_Action);
	TestFalse(TEXT("HasAll must return false when container is missing a tag from query"), Container.HasAll(SupersetQuery));

	// Empty query
	FGameplayTagContainer EmptyQuery;
	TestTrue(TEXT("HasAll against empty query must return true (all 0 tags present)"), Container.HasAll(EmptyQuery));

	return true;
}

// 5. HierarchicalMatching Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveGameplayTagsHierarchicalMatchingTest,
	"ShadowSlave.GameplayTags.HierarchicalMatching",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveGameplayTagsHierarchicalMatchingTest::RunTest(const FString& Parameters)
{
	FGameplayTagContainer Container;
	Container.AddTag(ShadowSlaveGameplayTags::State_Active);

	// Child tag matches parent query in hierarchical matching
	TestTrue(TEXT("State.Active must match parent State query"), Container.HasTag(ShadowSlaveGameplayTags::State));

	// Exact match requires exact equality
	TestFalse(TEXT("Native HasTagExact on parent State must return false when only State.Active is present"), Container.HasTagExact(ShadowSlaveGameplayTags::State));

	// Direct tag hierarchical matches
	TestTrue(TEXT("State.Active MatchesTag State"), ShadowSlaveGameplayTags::State_Active.MatchesTag(ShadowSlaveGameplayTags::State));
	TestFalse(TEXT("State.Active MatchesTagExact State"), ShadowSlaveGameplayTags::State_Active.MatchesTagExact(ShadowSlaveGameplayTags::State));
	TestFalse(TEXT("Parent State does NOT match child State.Active query"), ShadowSlaveGameplayTags::State.MatchesTag(ShadowSlaveGameplayTags::State_Active));

	return true;
}

// 6. NoGameplaySystemCoupling Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveGameplayTagsNoGameplaySystemCouplingTest,
	"ShadowSlave.GameplayTags.NoGameplaySystemCoupling",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveGameplayTagsNoGameplaySystemCouplingTest::RunTest(const FString& Parameters)
{
	// Architecture requirement: Gameplay Tag foundation must operate purely on native data/types
	// without coupling to, or requiring instantiation of:
	// - CombatComponent
	// - StatusEffectComponent
	// - EchoComponent
	// - MemoryComponent
	// - QuestSubsystem / StorySubsystem
	// - GAS AbilitySystemComponent

	FGameplayTagContainer StandaloneContainer;
	TestTrue(TEXT("Standalone container can be instantiated without world or actors"), StandaloneContainer.IsEmpty());

	StandaloneContainer.AddTag(ShadowSlaveGameplayTags::Interaction_Interactable);
	TestTrue(TEXT("Tag operations work completely independently of any gameplay subsystem"), StandaloneContainer.HasTagExact(ShadowSlaveGameplayTags::Interaction_Interactable));

	// FGameplayTagQuery matching also works standalone
	const FGameplayTagQuery Query = FGameplayTagQuery::MakeQuery_MatchTag(ShadowSlaveGameplayTags::Interaction_Interactable);
	TestTrue(TEXT("Query.Matches works standalone without gameplay systems"), Query.Matches(StandaloneContainer));

	return true;
}

#endif // WITH_AUTOMATION_TESTS
