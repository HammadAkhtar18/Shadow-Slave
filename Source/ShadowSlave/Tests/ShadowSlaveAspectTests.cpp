// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_AUTOMATION_TESTS

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Aspects/ShadowSlaveAspectComponent.h"
#include "Aspects/ShadowSlaveAspectDefinition.h"
#include "Aspects/ShadowSlaveAspectAbilityDefinition.h"
#include "Attributes/ShadowSlaveAttributeComponent.h"
#include "Characters/ShadowSlavePlayerCharacter.h"
#include "Progression/ShadowSlaveProgressionComponent.h"

namespace
{
	struct FShadowSlaveAspectActivationFixture
	{
		AShadowSlavePlayerCharacter* Player = nullptr;
		UShadowSlaveAspectComponent* Aspect = nullptr;
		UShadowSlaveAttributeComponent* Attributes = nullptr;
		UShadowSlaveProgressionComponent* Progression = nullptr;
		UShadowSlaveAspectAbilityDefinition* Ability = nullptr;

		bool Initialize(float EssenceCost, EShadowSlaveCharacterRank RequiredRank = EShadowSlaveCharacterRank::Unknown)
		{
			Player = NewObject<AShadowSlavePlayerCharacter>();
			if (!Player)
			{
				return false;
			}

			Aspect = Player->GetAspectComponent();
			Attributes = Player->GetAttributeComponent();
			Progression = Player->GetProgressionComponent();
			Ability = NewObject<UShadowSlaveAspectAbilityDefinition>(Player);
			UShadowSlaveAspectDefinition* AspectDefinition = NewObject<UShadowSlaveAspectDefinition>(Player);
			if (!Aspect || !Attributes || !Progression || !Ability || !AspectDefinition)
			{
				return false;
			}

			Ability->AbilityId = FName(TEXT("Test_GenericAbility"));
			Ability->BaseEssenceCost = EssenceCost;
			Ability->RequiredCharacterRank = RequiredRank;
			AspectDefinition->AspectId = FName(TEXT("Test_GenericAspect"));
			AspectDefinition->AbilityDefinitions.Add(Ability);

			return Aspect->SetAspectDefinition(AspectDefinition);
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveAspectAbilityActivationTest,
	"ShadowSlave.Aspects.Ability.ActivationConsumesConfiguredEssenceOnce",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveAspectAbilityActivationTest::RunTest(const FString& Parameters)
{
	FShadowSlaveAspectActivationFixture Fixture;
	TestTrue(TEXT("Ability activation fixture must initialize"), Fixture.Initialize(25.0f));
	if (!Fixture.Aspect || !Fixture.Attributes || !Fixture.Ability)
	{
		return false;
	}

	Fixture.Attributes->SetEssence(60.0f);
	TestTrue(TEXT("Ability must unlock before activation"), Fixture.Aspect->UnlockAbility(Fixture.Ability->AbilityId));
	TestTrue(TEXT("Unlocked ability with sufficient Essence must activate"), Fixture.Aspect->ActivateAbility(Fixture.Ability->AbilityId));
	TestTrue(TEXT("Successful activation must set transient active state"), Fixture.Aspect->IsAbilityActive(Fixture.Ability->AbilityId));
	TestNearlyEqual(TEXT("Successful activation must consume the configured Essence cost"), Fixture.Attributes->GetCurrentEssence(), 35.0f, 0.001f);
	TestTrue(TEXT("Repeated activation must be idempotent"), Fixture.Aspect->ActivateAbility(Fixture.Ability->AbilityId));
	TestNearlyEqual(TEXT("Repeated activation must not consume Essence again"), Fixture.Attributes->GetCurrentEssence(), 35.0f, 0.001f);
	TestTrue(TEXT("Active ability must deactivate"), Fixture.Aspect->DeactivateAbility(Fixture.Ability->AbilityId));
	TestFalse(TEXT("Deactivation must clear transient active state"), Fixture.Aspect->IsAbilityActive(Fixture.Ability->AbilityId));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveAspectAbilityActivationFailureTest,
	"ShadowSlave.Aspects.Ability.InvalidActivationDoesNotConsumeEssence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveAspectAbilityActivationFailureTest::RunTest(const FString& Parameters)
{
	FShadowSlaveAspectActivationFixture Fixture;
	TestTrue(TEXT("Ability activation fixture must initialize"), Fixture.Initialize(20.0f, EShadowSlaveCharacterRank::Awakened));
	if (!Fixture.Aspect || !Fixture.Attributes || !Fixture.Progression || !Fixture.Ability)
	{
		return false;
	}

	Fixture.Attributes->SetEssence(50.0f);
	TestFalse(TEXT("Locked ability must not activate"), Fixture.Aspect->ActivateAbility(Fixture.Ability->AbilityId));
	TestNearlyEqual(TEXT("Locked activation must not consume Essence"), Fixture.Attributes->GetCurrentEssence(), 50.0f, 0.001f);

	TestTrue(TEXT("Ability must unlock for subsequent validation"), Fixture.Aspect->UnlockAbility(Fixture.Ability->AbilityId));
	TestFalse(TEXT("Unknown character rank must fail a configured rank requirement"), Fixture.Aspect->ActivateAbility(Fixture.Ability->AbilityId));
	TestNearlyEqual(TEXT("Rank validation failure must not consume Essence"), Fixture.Attributes->GetCurrentEssence(), 50.0f, 0.001f);

	TestTrue(TEXT("Required character rank must be assignable for activation"), Fixture.Progression->SetCharacterRank(EShadowSlaveCharacterRank::Awakened));
	Fixture.Attributes->SetEssence(10.0f);
	TestFalse(TEXT("Insufficient Essence must reject activation"), Fixture.Aspect->ActivateAbility(Fixture.Ability->AbilityId));
	TestFalse(TEXT("Failed activation must leave ability inactive"), Fixture.Aspect->IsAbilityActive(Fixture.Ability->AbilityId));
	TestNearlyEqual(TEXT("Insufficient Essence failure must not consume any resource"), Fixture.Attributes->GetCurrentEssence(), 10.0f, 0.001f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveAspectAbilityLifecycleTest,
	"ShadowSlave.Aspects.Ability.AspectReplacementClearsTransientActivation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveAspectAbilityLifecycleTest::RunTest(const FString& Parameters)
{
	FShadowSlaveAspectActivationFixture Fixture;
	TestTrue(TEXT("Ability lifecycle fixture must initialize"), Fixture.Initialize(0.0f));
	if (!Fixture.Aspect || !Fixture.Ability)
	{
		return false;
	}

	TestTrue(TEXT("Ability must unlock before lifecycle activation"), Fixture.Aspect->UnlockAbility(Fixture.Ability->AbilityId));
	TestTrue(TEXT("Zero-cost ability must activate"), Fixture.Aspect->ActivateAbility(Fixture.Ability->AbilityId));
	TestTrue(TEXT("Ability must be active before Aspect replacement"), Fixture.Aspect->IsAbilityActive(Fixture.Ability->AbilityId));
	TestTrue(TEXT("Removing an Aspect must succeed"), Fixture.Aspect->SetAspectDefinition(nullptr));
	TestFalse(TEXT("Aspect replacement must clear transient active ability state"), Fixture.Aspect->IsAbilityActive(Fixture.Ability->AbilityId));
	TestFalse(TEXT("Removed Aspect must no longer permit activation"), Fixture.Aspect->CanActivateAbility(Fixture.Ability->AbilityId));

	return true;
}

#endif // WITH_AUTOMATION_TESTS
