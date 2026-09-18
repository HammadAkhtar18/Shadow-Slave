// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_AUTOMATION_TESTS

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Combat/ShadowSlaveCombatComponent.h"
#include "Combat/ShadowSlaveCombatTypes.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveCombatInitialStateTest,
	"ShadowSlave.Combat.InitialStateIsNeutral",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveCombatInitialStateTest::RunTest(const FString& Parameters)
{
	UShadowSlaveCombatComponent* CombatComp = NewObject<UShadowSlaveCombatComponent>();
	TestNotNull(TEXT("CombatComponent must instantiate successfully"), CombatComp);
	if (!CombatComp)
	{
		return false;
	}

	TestEqual(TEXT("Initial combat state must be Neutral"), CombatComp->GetCombatState(), ECombatState::Neutral);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveCombatValidTransitionsTest,
	"ShadowSlave.Combat.ValidTransitionsFromNeutral",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveCombatValidTransitionsTest::RunTest(const FString& Parameters)
{
	UShadowSlaveCombatComponent* CombatComp = NewObject<UShadowSlaveCombatComponent>();
	TestNotNull(TEXT("CombatComponent must instantiate successfully"), CombatComp);
	if (!CombatComp)
	{
		return false;
	}

	// From Neutral, the character can legally transition to Attacking, Dodging, Stunned, or Dead
	TestTrue(TEXT("Neutral -> Attacking must be allowed"), CombatComp->CanTransitionToState(ECombatState::Attacking));
	TestTrue(TEXT("Neutral -> Dodging must be allowed"), CombatComp->CanTransitionToState(ECombatState::Dodging));
	TestTrue(TEXT("Neutral -> Stunned must be allowed"), CombatComp->CanTransitionToState(ECombatState::Stunned));
	TestTrue(TEXT("Neutral -> Dead must be allowed"), CombatComp->CanTransitionToState(ECombatState::Dead));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveCombatIllegalTransitionsRejectedTest,
	"ShadowSlave.Combat.IllegalTransitionsRejected",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveCombatIllegalTransitionsRejectedTest::RunTest(const FString& Parameters)
{
	UShadowSlaveCombatComponent* CombatComp = NewObject<UShadowSlaveCombatComponent>();
	TestNotNull(TEXT("CombatComponent must instantiate successfully"), CombatComp);
	if (!CombatComp)
	{
		return false;
	}

	// Neutral cannot directly transition to Recovering (Recovering is only entered from Attacking)
	TestFalse(TEXT("Neutral -> Recovering must be disallowed"), CombatComp->CanTransitionToState(ECombatState::Recovering));

	CombatComp->SetCombatState(ECombatState::Recovering);
	TestEqual(TEXT("State must remain Neutral after rejected transition"), CombatComp->GetCombatState(), ECombatState::Neutral);

	// Transition to Dodging
	CombatComp->SetCombatState(ECombatState::Dodging);
	TestEqual(TEXT("State must be Dodging"), CombatComp->GetCombatState(), ECombatState::Dodging);

	// Dodging can only transition back to Neutral, Dead, or Stunned; not directly to Attacking
	TestFalse(TEXT("Dodging -> Attacking must be disallowed"), CombatComp->CanTransitionToState(ECombatState::Attacking));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveCombatDeadStateIsTerminalTest,
	"ShadowSlave.Combat.DeadStateIsTerminal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveCombatDeadStateIsTerminalTest::RunTest(const FString& Parameters)
{
	UShadowSlaveCombatComponent* CombatComp = NewObject<UShadowSlaveCombatComponent>();
	TestNotNull(TEXT("CombatComponent must instantiate successfully"), CombatComp);
	if (!CombatComp)
	{
		return false;
	}

	CombatComp->SetCombatState(ECombatState::Dead);
	TestEqual(TEXT("State must be Dead"), CombatComp->GetCombatState(), ECombatState::Dead);

	// Dead is terminal: no transition out of Dead is permitted
	TestFalse(TEXT("Dead -> Neutral must be disallowed"), CombatComp->CanTransitionToState(ECombatState::Neutral));
	TestFalse(TEXT("Dead -> Attacking must be disallowed"), CombatComp->CanTransitionToState(ECombatState::Attacking));
	TestFalse(TEXT("Dead -> Dodging must be disallowed"), CombatComp->CanTransitionToState(ECombatState::Dodging));
	TestFalse(TEXT("Dead -> Stunned must be disallowed"), CombatComp->CanTransitionToState(ECombatState::Stunned));

	CombatComp->SetCombatState(ECombatState::Neutral);
	TestEqual(TEXT("State must remain Dead after illegal transition attempt"), CombatComp->GetCombatState(), ECombatState::Dead);

	// ResetToNeutral must also refuse to resurrect a Dead component
	CombatComp->ResetToNeutral();
	TestEqual(TEXT("ResetToNeutral must not change state when Dead"), CombatComp->GetCombatState(), ECombatState::Dead);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveCombatRepeatedTransitionTest,
	"ShadowSlave.Combat.RepeatedStateTransitionIsIdempotent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveCombatRepeatedTransitionTest::RunTest(const FString& Parameters)
{
	UShadowSlaveCombatComponent* CombatComp = NewObject<UShadowSlaveCombatComponent>();
	TestNotNull(TEXT("CombatComponent must instantiate successfully"), CombatComp);
	if (!CombatComp)
	{
		return false;
	}

	// Transitioning from a state to itself is legally allowed and idempotent
	TestTrue(TEXT("Neutral -> Neutral must be permitted"), CombatComp->CanTransitionToState(ECombatState::Neutral));

	CombatComp->SetCombatState(ECombatState::Neutral);
	TestEqual(TEXT("State must still be Neutral"), CombatComp->GetCombatState(), ECombatState::Neutral);

	CombatComp->SetCombatState(ECombatState::Attacking);
	TestEqual(TEXT("State must be Attacking"), CombatComp->GetCombatState(), ECombatState::Attacking);

	TestTrue(TEXT("Attacking -> Attacking must be permitted"), CombatComp->CanTransitionToState(ECombatState::Attacking));
	CombatComp->SetCombatState(ECombatState::Attacking);
	TestEqual(TEXT("State must remain Attacking"), CombatComp->GetCombatState(), ECombatState::Attacking);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveCombatDeadStateRejectsAttackAndDodgeTest,
	"ShadowSlave.Combat.DeadStateRejectsAttackAndDodge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveCombatDeadStateRejectsAttackAndDodgeTest::RunTest(const FString& Parameters)
{
	UShadowSlaveCombatComponent* CombatComp = NewObject<UShadowSlaveCombatComponent>();
	TestNotNull(TEXT("CombatComponent must instantiate successfully"), CombatComp);
	if (!CombatComp)
	{
		return false;
	}

	CombatComp->SetCombatState(ECombatState::Dead);

	// A Dead component cannot execute attacks or dodges
	TestFalse(TEXT("Cannot perform Light attack when Dead"), CombatComp->CanPerformAttack(EAttackType::Light));
	TestFalse(TEXT("Cannot perform Heavy attack when Dead"), CombatComp->CanPerformAttack(EAttackType::Heavy));
	TestFalse(TEXT("Cannot perform Dodge when Dead"), CombatComp->CanPerformDodge());

	return true;
}

#endif // WITH_AUTOMATION_TESTS
