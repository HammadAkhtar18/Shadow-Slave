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
#include "Content/ShadowSlaveContentTypes.h"
#include "Content/ShadowSlaveContentDefinition.h"
#include "Content/ShadowSlaveContentRegistrySubsystem.h"
#include "Save/ShadowSlaveSaveSubsystem.h"
#include "Save/ShadowSlaveSaveTypes.h"

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

			Ability->SetAbilityId(FName(TEXT("Test_GenericAbility")));
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
	TestTrue(TEXT("Ability must unlock before activation"), Fixture.Aspect->UnlockAbility(Fixture.Ability->GetAbilityId()));
	TestTrue(TEXT("Unlocked ability with sufficient Essence must activate"), Fixture.Aspect->ActivateAbility(Fixture.Ability->GetAbilityId()));
	TestTrue(TEXT("Successful activation must set transient active state"), Fixture.Aspect->IsAbilityActive(Fixture.Ability->GetAbilityId()));
	TestNearlyEqual(TEXT("Successful activation must consume the configured Essence cost"), Fixture.Attributes->GetCurrentEssence(), 35.0f, 0.001f);
	TestTrue(TEXT("Repeated activation must be idempotent"), Fixture.Aspect->ActivateAbility(Fixture.Ability->GetAbilityId()));
	TestNearlyEqual(TEXT("Repeated activation must not consume Essence again"), Fixture.Attributes->GetCurrentEssence(), 35.0f, 0.001f);
	TestTrue(TEXT("Active ability must deactivate"), Fixture.Aspect->DeactivateAbility(Fixture.Ability->GetAbilityId()));
	TestFalse(TEXT("Deactivation must clear transient active state"), Fixture.Aspect->IsAbilityActive(Fixture.Ability->GetAbilityId()));

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
	TestFalse(TEXT("Locked ability must not activate"), Fixture.Aspect->ActivateAbility(Fixture.Ability->GetAbilityId()));
	TestNearlyEqual(TEXT("Locked activation must not consume Essence"), Fixture.Attributes->GetCurrentEssence(), 50.0f, 0.001f);

	TestTrue(TEXT("Ability must unlock for subsequent validation"), Fixture.Aspect->UnlockAbility(Fixture.Ability->GetAbilityId()));
	TestFalse(TEXT("Unknown character rank must fail a configured rank requirement"), Fixture.Aspect->ActivateAbility(Fixture.Ability->GetAbilityId()));
	TestNearlyEqual(TEXT("Rank validation failure must not consume Essence"), Fixture.Attributes->GetCurrentEssence(), 50.0f, 0.001f);

	TestTrue(TEXT("Required character rank must be assignable for activation"), Fixture.Progression->SetCharacterRank(EShadowSlaveCharacterRank::Awakened));
	Fixture.Attributes->SetEssence(10.0f);
	TestFalse(TEXT("Insufficient Essence must reject activation"), Fixture.Aspect->ActivateAbility(Fixture.Ability->GetAbilityId()));
	TestFalse(TEXT("Failed activation must leave ability inactive"), Fixture.Aspect->IsAbilityActive(Fixture.Ability->GetAbilityId()));
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

	TestTrue(TEXT("Ability must unlock before lifecycle activation"), Fixture.Aspect->UnlockAbility(Fixture.Ability->GetAbilityId()));
	TestTrue(TEXT("Zero-cost ability must activate"), Fixture.Aspect->ActivateAbility(Fixture.Ability->GetAbilityId()));
	TestTrue(TEXT("Ability must be active before Aspect replacement"), Fixture.Aspect->IsAbilityActive(Fixture.Ability->GetAbilityId()));
	TestTrue(TEXT("Removing an Aspect must succeed"), Fixture.Aspect->SetAspectDefinition(nullptr));
	TestFalse(TEXT("Aspect replacement must clear transient active ability state"), Fixture.Aspect->IsAbilityActive(Fixture.Ability->GetAbilityId()));
	TestFalse(TEXT("Removed Aspect must no longer permit activation"), Fixture.Aspect->CanActivateAbility(Fixture.Ability->GetAbilityId()));

	return true;
}

// =============================================================================
// Step 43: Ability Definition Content Pipeline Integration Tests
// =============================================================================

// 1. DefinitionUsesGenericContentBase Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveAbilityDefinitionUsesGenericContentBaseTest,
	"ShadowSlave.AbilityDefinition.DefinitionUsesGenericContentBase",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveAbilityDefinitionUsesGenericContentBaseTest::RunTest(const FString& Parameters)
{
	UShadowSlaveAspectAbilityDefinition* AbilityDef = NewObject<UShadowSlaveAspectAbilityDefinition>();
	TestNotNull(TEXT("Ability definition must instantiate"), AbilityDef);

	// 1. Inheritance verification
	TestTrue(TEXT("Ability definition must be a UShadowSlaveContentDefinition"),
		AbilityDef->IsA(UShadowSlaveContentDefinition::StaticClass()));

	UShadowSlaveContentDefinition* BaseDef = Cast<UShadowSlaveContentDefinition>(AbilityDef);
	TestNotNull(TEXT("Cast to UShadowSlaveContentDefinition must succeed"), BaseDef);

	// 2. Generic content properties inherited and accessible
	AbilityDef->ContentId = FName(TEXT("Test_Generic_Ability"));
	AbilityDef->DisplayName = FText::FromString(TEXT("Generic Test Ability"));
	AbilityDef->Description = FText::FromString(TEXT("Test description for ability content pipeline."));
	AbilityDef->Version = 2;
	AbilityDef->ProvenanceNote = TEXT("Test Provenance Note");

	TestEqual(TEXT("Base ContentId matches"), BaseDef->ContentId, FName(TEXT("Test_Generic_Ability")));
	TestEqual(TEXT("Base DisplayName matches"), BaseDef->DisplayName.ToString(), TEXT("Generic Test Ability"));
	TestEqual(TEXT("Base Description matches"), BaseDef->Description.ToString(), TEXT("Test description for ability content pipeline."));
	TestEqual(TEXT("Base Version matches"), BaseDef->Version, 2);
	TestEqual(TEXT("Base ProvenanceNote matches"), BaseDef->ProvenanceNote, TEXT("Test Provenance Note"));
	TestEqual(TEXT("Base ContentType is Ability"), BaseDef->ContentType, EShadowSlaveContentType::Ability);

	return true;
}

// 2. DefinitionUsesAbilityContentType Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveAbilityDefinitionUsesAbilityContentTypeTest,
	"ShadowSlave.AbilityDefinition.DefinitionUsesAbilityContentType",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveAbilityDefinitionUsesAbilityContentTypeTest::RunTest(const FString& Parameters)
{
	UShadowSlaveAspectAbilityDefinition* AbilityDef = NewObject<UShadowSlaveAspectAbilityDefinition>();
	TestNotNull(TEXT("Ability definition must instantiate"), AbilityDef);

	// 1. Top-level generic content type must default to Ability
	TestEqual(TEXT("Default ContentType must be EShadowSlaveContentType::Ability"),
		AbilityDef->ContentType, EShadowSlaveContentType::Ability);

	// 2. Ability-specific taxonomy (RequiredCharacterRank, BaseEssenceCost) remains separate
	TestEqual(TEXT("Default RequiredCharacterRank is Unknown"),
		AbilityDef->RequiredCharacterRank, EShadowSlaveCharacterRank::Unknown);
	TestEqual(TEXT("Default BaseEssenceCost is 0.0f"),
		AbilityDef->BaseEssenceCost, 0.0f);

	// Mutating Ability-specific fields must not affect generic ContentType
	AbilityDef->RequiredCharacterRank = EShadowSlaveCharacterRank::Ascended;
	AbilityDef->BaseEssenceCost = 50.0f;
	TestEqual(TEXT("ContentType remains Ability after mutating rank/cost"),
		AbilityDef->ContentType, EShadowSlaveContentType::Ability);
	TestEqual(TEXT("RequiredCharacterRank is Ascended"),
		AbilityDef->RequiredCharacterRank, EShadowSlaveCharacterRank::Ascended);
	TestEqual(TEXT("BaseEssenceCost is 50.0f"),
		AbilityDef->BaseEssenceCost, 50.0f);

	return true;
}

// 3. DefinitionHasSingleAuthoritativeId Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveAbilityDefinitionHasSingleAuthoritativeIdTest,
	"ShadowSlave.AbilityDefinition.DefinitionHasSingleAuthoritativeId",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveAbilityDefinitionHasSingleAuthoritativeIdTest::RunTest(const FString& Parameters)
{
	UShadowSlaveAspectAbilityDefinition* AbilityDef = NewObject<UShadowSlaveAspectAbilityDefinition>();
	TestNotNull(TEXT("Ability definition must instantiate"), AbilityDef);

	// 1. Initial state: ContentId is NAME_None, GetAbilityId() returns NAME_None
	TestEqual(TEXT("Initial ContentId is NAME_None"), AbilityDef->ContentId, NAME_None);
	TestEqual(TEXT("Initial GetAbilityId() returns NAME_None"), AbilityDef->GetAbilityId(), NAME_None);

	// 2. Set ContentId directly -> GetAbilityId() must return the same ID
	const FName Id1(TEXT("Ability_Authoritative_01"));
	AbilityDef->ContentId = Id1;
	TestEqual(TEXT("GetAbilityId() returns authoritative ContentId"), AbilityDef->GetAbilityId(), Id1);

	// 3. Set via SetAbilityId() -> ContentId must be updated
	const FName Id2(TEXT("Ability_Authoritative_02"));
	AbilityDef->SetAbilityId(Id2);
	TestEqual(TEXT("ContentId reflects SetAbilityId()"), AbilityDef->ContentId, Id2);
	TestEqual(TEXT("GetAbilityId() reflects SetAbilityId()"), AbilityDef->GetAbilityId(), Id2);

	// 4. Verification that GetPrimaryAssetId() uses authoritative ContentId
	const FPrimaryAssetId ExpectedAssetId(TEXT("Ability"), Id2);
	TestEqual(TEXT("GetPrimaryAssetId() uses ContentId"), AbilityDef->GetPrimaryAssetId(), ExpectedAssetId);

	return true;
}

// 4. DefinitionValidation Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveAbilityDefinitionValidationTest,
	"ShadowSlave.AbilityDefinition.DefinitionValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveAbilityDefinitionValidationTest::RunTest(const FString& Parameters)
{
	UShadowSlaveAspectAbilityDefinition* ValidDef = NewObject<UShadowSlaveAspectAbilityDefinition>();
	ValidDef->ContentId = FName(TEXT("Test_Valid_Ability"));
	ValidDef->DisplayName = FText::FromString(TEXT("Valid Test Ability"));
	ValidDef->Version = 1;
	ValidDef->ContentType = EShadowSlaveContentType::Ability;
	ValidDef->RequiredCharacterRank = EShadowSlaveCharacterRank::Awakened;
	ValidDef->BaseEssenceCost = 15.0f;

	FString ErrorMsg;

	// 1. Valid definition passes
	TestTrue(TEXT("Valid ability definition must pass IsValidDefinition"), ValidDef->IsValidDefinition(&ErrorMsg));
	TestTrue(TEXT("Valid ability definition must pass ValidateDefinition"), ValidDef->ValidateDefinition(ErrorMsg));

	// 2. Generic validation: ContentId None fails
	ValidDef->ContentId = NAME_None;
	TestFalse(TEXT("None ContentId must fail IsValidDefinition"), ValidDef->IsValidDefinition(&ErrorMsg));
	TestFalse(TEXT("Error message must not be empty on failure"), ErrorMsg.IsEmpty());
	ValidDef->ContentId = FName(TEXT("Test_Valid_Ability"));

	// 3. Generic validation: Empty DisplayName fails
	ValidDef->DisplayName = FText::GetEmpty();
	TestFalse(TEXT("Empty DisplayName must fail IsValidDefinition"), ValidDef->IsValidDefinition(&ErrorMsg));
	ValidDef->DisplayName = FText::FromString(TEXT("Valid Test Ability"));

	// 4. Generic validation: Version < 1 fails
	ValidDef->Version = 0;
	TestFalse(TEXT("Version 0 must fail IsValidDefinition"), ValidDef->IsValidDefinition(&ErrorMsg));
	ValidDef->Version = 1;

	// 5. Generic validation: Wrong ContentType fails
	ValidDef->ContentType = EShadowSlaveContentType::Memory;
	TestFalse(TEXT("Wrong ContentType must fail IsValidDefinition"), ValidDef->IsValidDefinition(&ErrorMsg));
	ValidDef->ContentType = EShadowSlaveContentType::Ability;

	// 6. Ability-specific validation: Negative BaseEssenceCost fails
	ValidDef->BaseEssenceCost = -5.0f;
	TestFalse(TEXT("Negative BaseEssenceCost must fail IsValidDefinition"), ValidDef->IsValidDefinition(&ErrorMsg));
	ValidDef->BaseEssenceCost = 15.0f;

	// 7. Ability-specific validation: Non-finite BaseEssenceCost fails
	ValidDef->BaseEssenceCost = TNumericLimits<float>::QuietNaN();
	TestFalse(TEXT("NaN BaseEssenceCost must fail IsValidDefinition"), ValidDef->IsValidDefinition(&ErrorMsg));
	ValidDef->BaseEssenceCost = 15.0f;

	// Restored definition passes again
	TestTrue(TEXT("Restored definition passes IsValidDefinition"), ValidDef->IsValidDefinition(&ErrorMsg));

	return true;
}

// 5. DefinitionRegistryIntegration Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveAbilityDefinitionRegistryIntegrationTest,
	"ShadowSlave.AbilityDefinition.DefinitionRegistryIntegration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveAbilityDefinitionRegistryIntegrationTest::RunTest(const FString& Parameters)
{
	UShadowSlaveContentRegistrySubsystem* Registry = NewObject<UShadowSlaveContentRegistrySubsystem>();
	TestNotNull(TEXT("Registry subsystem must be created"), Registry);

	UShadowSlaveAspectAbilityDefinition* AbilityDef = NewObject<UShadowSlaveAspectAbilityDefinition>();
	AbilityDef->ContentId = FName(TEXT("Ability_Registry_Test"));
	AbilityDef->DisplayName = FText::FromString(TEXT("Registry Test Ability"));
	AbilityDef->Version = 1;
	AbilityDef->RequiredCharacterRank = EShadowSlaveCharacterRank::Awakened;
	AbilityDef->BaseEssenceCost = 20.0f;

	// 1. Register with generic registry subsystem
	const bool bRegistered = Registry->RegisterDefinition(AbilityDef);
	TestTrue(TEXT("RegisterDefinition must succeed for UShadowSlaveAspectAbilityDefinition"), bRegistered);

	// 2. Query existence & count
	TestTrue(TEXT("HasContent must return true for registered AbilityDef"), Registry->HasContent(AbilityDef->ContentId));
	TestEqual(TEXT("Total registered count must be 1"), Registry->GetRegisteredContentCount(), 1);
	TestEqual(TEXT("Ability count by type must be 1"), Registry->GetRegisteredContentCountByType(EShadowSlaveContentType::Ability), 1);
	TestEqual(TEXT("Memory count by type must be 0"), Registry->GetRegisteredContentCountByType(EShadowSlaveContentType::Memory), 0);
	TestEqual(TEXT("Echo count by type must be 0"), Registry->GetRegisteredContentCountByType(EShadowSlaveContentType::Echo), 0);
	TestEqual(TEXT("Story count by type must be 0"), Registry->GetRegisteredContentCountByType(EShadowSlaveContentType::Story), 0);
	TestEqual(TEXT("Item count by type must be 0"), Registry->GetRegisteredContentCountByType(EShadowSlaveContentType::Item), 0);
	TestEqual(TEXT("Custom count by type must be 0"), Registry->GetRegisteredContentCountByType(EShadowSlaveContentType::Custom), 0);

	// 3. Generic resolution
	UShadowSlaveContentDefinition* ResolvedGeneric = Registry->ResolveContentDefinition(AbilityDef->ContentId);
	TestNotNull(TEXT("Resolved generic definition must not be null"), ResolvedGeneric);
	TestEqual(TEXT("Resolved generic definition must match AbilityDef"), ResolvedGeneric, Cast<UShadowSlaveContentDefinition>(AbilityDef));

	// 4. Typed resolution
	UShadowSlaveAspectAbilityDefinition* ResolvedAbility = Registry->ResolveContentDefinition<UShadowSlaveAspectAbilityDefinition>(AbilityDef->ContentId);
	TestNotNull(TEXT("Resolved typed ability definition must not be null"), ResolvedAbility);
	TestEqual(TEXT("Resolved typed ability must match original AbilityDef"), ResolvedAbility, AbilityDef);
	TestEqual(TEXT("Resolved RequiredCharacterRank matches"), ResolvedAbility->RequiredCharacterRank, EShadowSlaveCharacterRank::Awakened);
	TestEqual(TEXT("Resolved BaseEssenceCost matches"), ResolvedAbility->BaseEssenceCost, 20.0f);

	// 5. PrimaryAssetId verification
	const FPrimaryAssetId ExpectedAssetId(TEXT("Ability"), AbilityDef->ContentId);
	TestEqual(TEXT("GetPrimaryAssetId must match expected PrimaryAssetId"), AbilityDef->GetPrimaryAssetId(), ExpectedAssetId);

	return true;
}

// 6. ExistingRuntimeCompatibility Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveAbilityDefinitionExistingRuntimeCompatibilityTest,
	"ShadowSlave.AbilityDefinition.ExistingRuntimeCompatibility",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveAbilityDefinitionExistingRuntimeCompatibilityTest::RunTest(const FString& Parameters)
{
	AShadowSlavePlayerCharacter* Player = NewObject<AShadowSlavePlayerCharacter>();
	TestNotNull(TEXT("Player must instantiate"), Player);
	if (!Player)
	{
		return false;
	}

	UShadowSlaveAspectComponent* Aspect = Player->GetAspectComponent();
	UShadowSlaveAttributeComponent* Attributes = Player->GetAttributeComponent();
	UShadowSlaveProgressionComponent* Progression = Player->GetProgressionComponent();
	TestNotNull(TEXT("Aspect component must exist"), Aspect);
	TestNotNull(TEXT("Attribute component must exist"), Attributes);
	TestNotNull(TEXT("Progression component must exist"), Progression);
	if (!Aspect || !Attributes || !Progression)
	{
		return false;
	}

	const FName AbilityId(TEXT("Compat_Runtime_Ability"));
	UShadowSlaveAspectAbilityDefinition* AbilityDef = NewObject<UShadowSlaveAspectAbilityDefinition>(Player);
	AbilityDef->SetAbilityId(AbilityId);
	AbilityDef->DisplayName = FText::FromString(TEXT("Compat Runtime Ability"));
	AbilityDef->Version = 1;
	AbilityDef->BaseEssenceCost = 30.0f;
	AbilityDef->RequiredCharacterRank = EShadowSlaveCharacterRank::Awakened;

	UShadowSlaveAspectDefinition* AspectDef = NewObject<UShadowSlaveAspectDefinition>(Player);
	AspectDef->AspectId = FName(TEXT("Compat_Runtime_Aspect"));
	AspectDef->AbilityDefinitions.Add(AbilityDef);

	TestTrue(TEXT("SetAspectDefinition must succeed"), Aspect->SetAspectDefinition(AspectDef));

	// 1. Locked ability cannot activate
	Attributes->SetEssence(100.0f);
	Progression->SetCharacterRank(EShadowSlaveCharacterRank::Awakened);
	TestFalse(TEXT("Locked ability cannot activate"), Aspect->ActivateAbility(AbilityId));

	// 2. Unlock ability
	TestTrue(TEXT("UnlockAbility succeeds"), Aspect->UnlockAbility(AbilityId));
	TestTrue(TEXT("IsAbilityUnlocked returns true"), Aspect->IsAbilityUnlocked(AbilityId));

	// 3. Rank prerequisite enforcement
	Progression->SetCharacterRank(EShadowSlaveCharacterRank::Dormant);
	TestFalse(TEXT("Dormant rank cannot activate Awakened ability"), Aspect->ActivateAbility(AbilityId));
	TestNearlyEqual(TEXT("Essence unchanged on rank rejection"), Attributes->GetCurrentEssence(), 100.0f, 0.001f);

	// 4. Insufficient Essence enforcement
	Progression->SetCharacterRank(EShadowSlaveCharacterRank::Awakened);
	Attributes->SetEssence(15.0f);
	TestFalse(TEXT("Insufficient essence rejects activation"), Aspect->ActivateAbility(AbilityId));
	TestNearlyEqual(TEXT("Essence unchanged on essence rejection"), Attributes->GetCurrentEssence(), 15.0f, 0.001f);
	TestFalse(TEXT("Ability is not active"), Aspect->IsAbilityActive(AbilityId));

	// 5. Successful activation
	Attributes->SetEssence(100.0f);
	TestTrue(TEXT("ActivateAbility succeeds with valid rank and essence"), Aspect->ActivateAbility(AbilityId));
	TestTrue(TEXT("IsAbilityActive returns true"), Aspect->IsAbilityActive(AbilityId));
	TestNearlyEqual(TEXT("Essence consumed correctly (100 - 30 = 70)"), Attributes->GetCurrentEssence(), 70.0f, 0.001f);

	// 6. Idempotent repeated activation
	TestTrue(TEXT("Repeated activation is idempotent"), Aspect->ActivateAbility(AbilityId));
	TestNearlyEqual(TEXT("Repeated activation does not consume essence again"), Attributes->GetCurrentEssence(), 70.0f, 0.001f);

	// 7. Aspect replacement clears transient active state
	TestTrue(TEXT("Replacing Aspect with nullptr succeeds"), Aspect->SetAspectDefinition(nullptr));
	TestFalse(TEXT("Transient active state cleared on Aspect replacement"), Aspect->IsAbilityActive(AbilityId));

	// 8. Save/Load capture: verify UnlockedAbilityIds captures the ability ID
	Aspect->SetAspectDefinition(AspectDef);
	Aspect->UnlockAbility(AbilityId);

	UShadowSlaveSaveSubsystem* SaveSub = NewObject<UShadowSlaveSaveSubsystem>();
	TestNotNull(TEXT("SaveSubsystem must instantiate"), SaveSub);

	FShadowSlaveAspectSaveData SaveData;
	SaveSub->CaptureAspect(Aspect, SaveData);
	TestTrue(TEXT("CaptureAspect produces valid save data"), SaveData.bIsValid);
	TestTrue(TEXT("UnlockedAbilityIds contains AbilityId"), SaveData.UnlockedAbilityIds.Contains(AbilityId));

	return true;
}

#endif // WITH_AUTOMATION_TESTS
