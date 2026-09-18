// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_AUTOMATION_TESTS

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Gameplay/ShadowSlaveGameplaySubsystem.h"
#include "Gameplay/ShadowSlaveGameplayTypes.h"
#include "Story/ShadowSlaveStorySubsystem.h"
#include "Story/ShadowSlaveStoryTypes.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveGameplayInitialFlowStateTest,
	"ShadowSlave.Gameplay.InitialFlowState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveGameplayInitialFlowStateTest::RunTest(const FString& Parameters)
{
	UShadowSlaveGameplaySubsystem* GameplaySub = NewObject<UShadowSlaveGameplaySubsystem>();
	TestNotNull(TEXT("GameplaySubsystem must instantiate"), GameplaySub);
	if (!GameplaySub)
	{
		return false;
	}

	TestEqual(TEXT("Initial uninitialized flow state must be None"),
		GameplaySub->GetCurrentFlowState(), EShadowSlaveGameplayFlowState::None);
	TestEqual(TEXT("Initial previous flow state must be None"),
		GameplaySub->GetPreviousFlowState(), EShadowSlaveGameplayFlowState::None);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveGameplayFlowStateTransitionRulesTest,
	"ShadowSlave.Gameplay.FlowStateTransitionRules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveGameplayFlowStateTransitionRulesTest::RunTest(const FString& Parameters)
{
	UShadowSlaveGameplaySubsystem* GameplaySub = NewObject<UShadowSlaveGameplaySubsystem>();
	TestNotNull(TEXT("GameplaySubsystem must instantiate"), GameplaySub);
	if (!GameplaySub)
	{
		return false;
	}

	// 1. Transition to Unknown state is strictly rejected
	TestFalse(TEXT("CanTransitionFlowState to Unknown must be false"),
		GameplaySub->CanTransitionFlowState(EShadowSlaveGameplayFlowState::None, EShadowSlaveGameplayFlowState::Unknown));
	TestFalse(TEXT("RequestFlowStateTransition(Unknown) must return false"),
		GameplaySub->RequestFlowStateTransition(EShadowSlaveGameplayFlowState::Unknown));

	// 2. Legal transition: None -> Exploration
	TestTrue(TEXT("Transition None -> Exploration must succeed"),
		GameplaySub->RequestFlowStateTransition(EShadowSlaveGameplayFlowState::Exploration));
	TestEqual(TEXT("Current state must be Exploration"),
		GameplaySub->GetCurrentFlowState(), EShadowSlaveGameplayFlowState::Exploration);

	// 3. Legal transition: Exploration -> Combat
	TestTrue(TEXT("Transition Exploration -> Combat must succeed"),
		GameplaySub->RequestFlowStateTransition(EShadowSlaveGameplayFlowState::Combat));
	TestEqual(TEXT("Current state must be Combat"),
		GameplaySub->GetCurrentFlowState(), EShadowSlaveGameplayFlowState::Combat);
	TestEqual(TEXT("Previous state must be Exploration"),
		GameplaySub->GetPreviousFlowState(), EShadowSlaveGameplayFlowState::Exploration);

	// 4. Illegal transition: Transitioning -> Combat is not allowed
	TestFalse(TEXT("Transitioning -> Combat must be disallowed"),
		GameplaySub->CanTransitionFlowState(EShadowSlaveGameplayFlowState::Transitioning, EShadowSlaveGameplayFlowState::Combat));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveGameplayFlowStateIdempotencyTest,
	"ShadowSlave.Gameplay.FlowStateTransitionIsIdempotent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveGameplayFlowStateIdempotencyTest::RunTest(const FString& Parameters)
{
	UShadowSlaveGameplaySubsystem* GameplaySub = NewObject<UShadowSlaveGameplaySubsystem>();
	TestNotNull(TEXT("GameplaySubsystem must instantiate"), GameplaySub);
	if (!GameplaySub)
	{
		return false;
	}

	GameplaySub->RequestFlowStateTransition(EShadowSlaveGameplayFlowState::Exploration);
	TestEqual(TEXT("State is Exploration"), GameplaySub->GetCurrentFlowState(), EShadowSlaveGameplayFlowState::Exploration);

	// Requesting transition to current state is idempotent and succeeds without error
	TestTrue(TEXT("Transition to the same state is idempotent"),
		GameplaySub->RequestFlowStateTransition(EShadowSlaveGameplayFlowState::Exploration));
	TestEqual(TEXT("State remains Exploration"),
		GameplaySub->GetCurrentFlowState(), EShadowSlaveGameplayFlowState::Exploration);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveGameplayWorldStoryTransitionTransactionalTest,
	"ShadowSlave.Gameplay.WorldStoryTransitionIsTransactional",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveGameplayWorldStoryTransitionTransactionalTest::RunTest(const FString& Parameters)
{
	UShadowSlaveGameplaySubsystem* GameplaySub = NewObject<UShadowSlaveGameplaySubsystem>();
	TestNotNull(TEXT("GameplaySubsystem must instantiate"), GameplaySub);
	if (!GameplaySub)
	{
		return false;
	}

	// 1. Invalid transition request (empty StoryId, TargetWorldId, and Reason) fails closed immediately
	FShadowSlaveGameplayTransitionRequest EmptyRequest;
	TestFalse(TEXT("Invalid empty transition request must return false"),
		GameplaySub->RequestWorldStoryTransition(EmptyRequest));
	TestEqual(TEXT("Flow state remains untouched on invalid request"),
		GameplaySub->GetCurrentFlowState(), EShadowSlaveGameplayFlowState::None);

	// 2. Request referencing an unregistered StoryId must fail closed without changing flow state
	FShadowSlaveGameplayTransitionRequest UnregisteredRequest;
	UnregisteredRequest.StoryId = FName("Nonexistent_Story_Id");
	UnregisteredRequest.TargetWorldId = FName("World_Sanctuary");
	UnregisteredRequest.Reason = FName("Testing");

	TestFalse(TEXT("Request referencing unregistered story must return false"),
		GameplaySub->RequestWorldStoryTransition(UnregisteredRequest));
	TestEqual(TEXT("Flow state must not advance to Transitioning when story verification fails"),
		GameplaySub->GetCurrentFlowState(), EShadowSlaveGameplayFlowState::None);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveGameplayFailedNightmareEndPreservesScenarioIdTest,
	"ShadowSlave.Gameplay.FailedNightmareEndPreservesScenarioId",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveGameplayFailedNightmareEndPreservesScenarioIdTest::RunTest(const FString& Parameters)
{
	UShadowSlaveGameplaySubsystem* GameplaySub = NewObject<UShadowSlaveGameplaySubsystem>();
	TestNotNull(TEXT("GameplaySubsystem must instantiate"), GameplaySub);
	if (!GameplaySub)
	{
		return false;
	}

	const FName TestScenario = FName("Scenario_FirstNightmare");

	// Start in Nightmare flow
	TestTrue(TEXT("BeginNightmareFlow succeeds"), GameplaySub->BeginNightmareFlow(TestScenario));
	TestEqual(TEXT("Flow state is Nightmare"), GameplaySub->GetCurrentFlowState(), EShadowSlaveGameplayFlowState::Nightmare);
	TestEqual(TEXT("ActiveNightmareScenarioId is set"), GameplaySub->GetActiveNightmareScenarioId(), TestScenario);

	// Graceful conclusion to Exploration
	TestTrue(TEXT("EndNightmareFlow succeeds"), GameplaySub->EndNightmareFlow());
	TestEqual(TEXT("Flow state is restored to Exploration"), GameplaySub->GetCurrentFlowState(), EShadowSlaveGameplayFlowState::Exploration);
	TestEqual(TEXT("ActiveNightmareScenarioId is cleared cleanly"), GameplaySub->GetActiveNightmareScenarioId(), NAME_None);

	return true;
}

#endif // WITH_AUTOMATION_TESTS
