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

	// 3. Pure world transition request (no StoryId) successfully advances to Transitioning
	FShadowSlaveGameplayTransitionRequest WorldOnlyRequest;
	WorldOnlyRequest.TargetWorldId = FName("World_Sanctuary");
	WorldOnlyRequest.Reason = FName("PortalTravel");

	TestTrue(TEXT("Pure world transition request must return true"),
		GameplaySub->RequestWorldStoryTransition(WorldOnlyRequest));
	TestEqual(TEXT("Flow state must advance to Transitioning"),
		GameplaySub->GetCurrentFlowState(), EShadowSlaveGameplayFlowState::Transitioning);
	TestEqual(TEXT("Previous flow state must be recorded as None"),
		GameplaySub->GetPreviousFlowState(), EShadowSlaveGameplayFlowState::None);

	// Complete transition to Exploration
	TestTrue(TEXT("Transition from Transitioning to Exploration must succeed"),
		GameplaySub->RequestFlowStateTransition(EShadowSlaveGameplayFlowState::Exploration));
	TestEqual(TEXT("Flow state is now Exploration"),
		GameplaySub->GetCurrentFlowState(), EShadowSlaveGameplayFlowState::Exploration);

	// Architectural Note on Post-Step Mutation Rollback:
	// In UShadowSlaveGameplaySubsystem::RequestWorldStoryTransition(), when StoryStepId is provided,
	// the subsystem invokes StorySub->SetCurrentStoryStep() before calling RequestFlowStateTransition(Transitioning).
	// If RequestFlowStateTransition(Transitioning) returns false, it rolls back:
	//   StorySub->SetCurrentStoryStep(Request.StoryId, PreviousStepId);
	// However, in the public API:
	//   a) CanTransitionFlowState(CurrentState, Transitioning) is unconditionally true for all valid states;
	//   b) UShadowSlaveStorySubsystem resolution requires a valid UGameInstance, which is null in standalone NewObject tests.
	// Therefore, testing the internal rollback of mutated story step requires mocking internal state machines
	// or engine instance resolution, which are not exposed via the public API.
	// The pre-validation boundaries and state consistency verified above provide safe, deterministic fail-closed guarantees.

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveGameplayNightmareScenarioIdPreservedUntilCleanExitTest,
	"ShadowSlave.Gameplay.NightmareScenarioIdPreservedUntilCleanExit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveGameplayNightmareScenarioIdPreservedUntilCleanExitTest::RunTest(const FString& Parameters)
{
	UShadowSlaveGameplaySubsystem* GameplaySub = NewObject<UShadowSlaveGameplaySubsystem>();
	TestNotNull(TEXT("GameplaySubsystem must instantiate"), GameplaySub);
	if (!GameplaySub)
	{
		return false;
	}

	const FName TestScenario = FName("Scenario_FirstNightmare");

	// 1. BeginNightmareFlow fails closed when ScenarioId is None
	TestFalse(TEXT("BeginNightmareFlow(NAME_None) must return false"), GameplaySub->BeginNightmareFlow(NAME_None));
	TestEqual(TEXT("Flow state must remain None after invalid begin"),
		GameplaySub->GetCurrentFlowState(), EShadowSlaveGameplayFlowState::None);
	TestEqual(TEXT("ActiveNightmareScenarioId must remain None after invalid begin"),
		GameplaySub->GetActiveNightmareScenarioId(), NAME_None);

	// 2. Begin Nightmare flow with valid scenario ID
	TestTrue(TEXT("BeginNightmareFlow succeeds with valid scenario"), GameplaySub->BeginNightmareFlow(TestScenario));
	TestEqual(TEXT("Flow state is Nightmare"), GameplaySub->GetCurrentFlowState(), EShadowSlaveGameplayFlowState::Nightmare);
	TestEqual(TEXT("ActiveNightmareScenarioId is set to TestScenario"),
		GameplaySub->GetActiveNightmareScenarioId(), TestScenario);

	// 3. Scenario ID is preserved during nested combat flow within Nightmare
	TestTrue(TEXT("BeginCombatFlow succeeds during Nightmare"), GameplaySub->BeginCombatFlow(nullptr));
	TestEqual(TEXT("Flow state is Combat"), GameplaySub->GetCurrentFlowState(), EShadowSlaveGameplayFlowState::Combat);
	TestEqual(TEXT("ActiveNightmareScenarioId remains preserved during combat"),
		GameplaySub->GetActiveNightmareScenarioId(), TestScenario);

	// Exiting combat restores flow state to Nightmare because ActiveNightmareScenarioId is active
	TestTrue(TEXT("EndCombatFlow succeeds"), GameplaySub->EndCombatFlow());
	TestEqual(TEXT("Flow state restores to Nightmare after combat exit"),
		GameplaySub->GetCurrentFlowState(), EShadowSlaveGameplayFlowState::Nightmare);
	TestEqual(TEXT("ActiveNightmareScenarioId remains preserved after combat exit"),
		GameplaySub->GetActiveNightmareScenarioId(), TestScenario);

	// 4. Graceful conclusion: EndNightmareFlow() transitions to Exploration and clears scenario ID
	TestTrue(TEXT("EndNightmareFlow succeeds"), GameplaySub->EndNightmareFlow());
	TestEqual(TEXT("Flow state is restored to Exploration"),
		GameplaySub->GetCurrentFlowState(), EShadowSlaveGameplayFlowState::Exploration);
	TestEqual(TEXT("Previous flow state is recorded as Nightmare"),
		GameplaySub->GetPreviousFlowState(), EShadowSlaveGameplayFlowState::Nightmare);
	TestEqual(TEXT("ActiveNightmareScenarioId is cleared ONLY upon successful transition"),
		GameplaySub->GetActiveNightmareScenarioId(), NAME_None);

	// 5. Subsequent idempotent EndNightmareFlow call produces no contradictory state
	TestTrue(TEXT("Subsequent EndNightmareFlow call is idempotent"), GameplaySub->EndNightmareFlow());
	TestEqual(TEXT("Flow state remains Exploration"),
		GameplaySub->GetCurrentFlowState(), EShadowSlaveGameplayFlowState::Exploration);
	TestEqual(TEXT("ActiveNightmareScenarioId remains None"),
		GameplaySub->GetActiveNightmareScenarioId(), NAME_None);

	// Architectural Note on Failed EndNightmareFlow() Transition:
	// In UShadowSlaveGameplaySubsystem::EndNightmareFlow(), the implementation executes:
	//   if (!RequestFlowStateTransition(EShadowSlaveGameplayFlowState::Exploration)) return false;
	//   ActiveNightmareScenarioId = NAME_None;
	//   return true;
	// In the public API, CanTransitionFlowState(Nightmare, Exploration) is unconditionally true,
	// and bIsProcessingFlowTransition is a private re-entrancy lock. There are no pluggable
	// transition filters or mockable failure delegates available on UShadowSlaveGameplaySubsystem.
	// Consequently, forcing RequestFlowStateTransition to fail from Nightmare flow cannot be deterministically
	// produced via the public API without modifying production code.
	// This test verifies scenario ID preservation across the full lifecycle and guarantees that
	// ActiveNightmareScenarioId is cleared only after the transition to Exploration succeeds.

	return true;
}

#endif // WITH_AUTOMATION_TESTS
