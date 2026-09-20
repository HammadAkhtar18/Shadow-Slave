// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_AUTOMATION_TESTS

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Aspects/ShadowSlaveAspectComponent.h"
#include "Aspects/ShadowSlaveAspectDefinition.h"
#include "Aspects/ShadowSlaveAspectAbilityDefinition.h"
#include "Aspects/ShadowSlaveFlawDefinition.h"
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
			AspectDefinition->SetAspectId(FName(TEXT("Test_GenericAspect")));
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
	AspectDef->SetAspectId(FName(TEXT("Compat_Runtime_Aspect")));
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

// =============================================================================
// Step 44: Flaw Definition Content Pipeline Integration Tests
// =============================================================================

// 1. DefinitionUsesGenericContentBase Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveFlawDefinitionUsesGenericContentBaseTest,
	"ShadowSlave.FlawDefinition.DefinitionUsesGenericContentBase",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveFlawDefinitionUsesGenericContentBaseTest::RunTest(const FString& Parameters)
{
	UShadowSlaveFlawDefinition* FlawDef = NewObject<UShadowSlaveFlawDefinition>();
	TestNotNull(TEXT("Flaw definition must instantiate"), FlawDef);

	// 1. Inheritance verification
	TestTrue(TEXT("Flaw definition must be a UShadowSlaveContentDefinition"),
		FlawDef->IsA(UShadowSlaveContentDefinition::StaticClass()));

	UShadowSlaveContentDefinition* BaseDef = Cast<UShadowSlaveContentDefinition>(FlawDef);
	TestNotNull(TEXT("Cast to UShadowSlaveContentDefinition must succeed"), BaseDef);

	// 2. Generic content properties inherited and accessible
	FlawDef->ContentId = FName(TEXT("Test_Generic_Flaw"));
	FlawDef->DisplayName = FText::FromString(TEXT("Generic Test Flaw"));
	FlawDef->Description = FText::FromString(TEXT("Test description for flaw content pipeline."));
	FlawDef->Version = 2;
	FlawDef->ProvenanceNote = TEXT("Test Provenance Note");

	TestEqual(TEXT("Base ContentId matches"), BaseDef->ContentId, FName(TEXT("Test_Generic_Flaw")));
	TestEqual(TEXT("Base DisplayName matches"), BaseDef->DisplayName.ToString(), TEXT("Generic Test Flaw"));
	TestEqual(TEXT("Base Description matches"), BaseDef->Description.ToString(), TEXT("Test description for flaw content pipeline."));
	TestEqual(TEXT("Base Version matches"), BaseDef->Version, 2);
	TestEqual(TEXT("Base ProvenanceNote matches"), BaseDef->ProvenanceNote, TEXT("Test Provenance Note"));
	TestEqual(TEXT("Base ContentType is Custom"), BaseDef->ContentType, EShadowSlaveContentType::Custom);

	return true;
}

// 2. DefinitionUsesCustomContentType Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveFlawDefinitionUsesCustomContentTypeTest,
	"ShadowSlave.FlawDefinition.DefinitionUsesCustomContentType",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveFlawDefinitionUsesCustomContentTypeTest::RunTest(const FString& Parameters)
{
	UShadowSlaveFlawDefinition* FlawDef = NewObject<UShadowSlaveFlawDefinition>();
	TestNotNull(TEXT("Flaw definition must instantiate"), FlawDef);

	// 1. Top-level generic content type must default to Custom
	TestEqual(TEXT("Default ContentType must be EShadowSlaveContentType::Custom"),
		FlawDef->ContentType, EShadowSlaveContentType::Custom);

	// 2. Verify no dedicated Flaw generic enum value exists in EShadowSlaveContentType
	TestTrue(TEXT("ContentType is not Ability"), FlawDef->ContentType != EShadowSlaveContentType::Ability);
	TestTrue(TEXT("ContentType is not Item"), FlawDef->ContentType != EShadowSlaveContentType::Item);
	TestTrue(TEXT("ContentType is not Memory"), FlawDef->ContentType != EShadowSlaveContentType::Memory);
	TestTrue(TEXT("ContentType is not Echo"), FlawDef->ContentType != EShadowSlaveContentType::Echo);
	TestTrue(TEXT("ContentType is not Story"), FlawDef->ContentType != EShadowSlaveContentType::Story);

	// 3. Verify Flaw-specific fields exist and are separate
	FlawDef->Metadata.Add(FName(TEXT("PenaltyType")), TEXT("Metaphysical"));
	FlawDef->CanonProvenance = TEXT("Test Canon Provenance");
	TestTrue(TEXT("Metadata contains PenaltyType"), FlawDef->Metadata.Contains(FName(TEXT("PenaltyType"))));
	TestEqual(TEXT("CanonProvenance matches"), FlawDef->CanonProvenance, TEXT("Test Canon Provenance"));

	return true;
}

// 3. DefinitionHasSingleAuthoritativeId Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveFlawDefinitionHasSingleAuthoritativeIdTest,
	"ShadowSlave.FlawDefinition.DefinitionHasSingleAuthoritativeId",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveFlawDefinitionHasSingleAuthoritativeIdTest::RunTest(const FString& Parameters)
{
	UShadowSlaveFlawDefinition* FlawDef = NewObject<UShadowSlaveFlawDefinition>();
	TestNotNull(TEXT("Flaw definition must instantiate"), FlawDef);

	// 1. Initial state: ContentId is NAME_None, GetFlawId() returns NAME_None
	TestEqual(TEXT("Initial ContentId is NAME_None"), FlawDef->ContentId, NAME_None);
	TestEqual(TEXT("Initial GetFlawId() returns NAME_None"), FlawDef->GetFlawId(), NAME_None);

	// 2. Set ContentId directly -> GetFlawId() must return the same ID
	const FName Id1(TEXT("Flaw_Authoritative_01"));
	FlawDef->ContentId = Id1;
	TestEqual(TEXT("GetFlawId() returns authoritative ContentId"), FlawDef->GetFlawId(), Id1);

	// 3. Set via SetFlawId() -> ContentId must be updated
	const FName Id2(TEXT("Flaw_Authoritative_02"));
	FlawDef->SetFlawId(Id2);
	TestEqual(TEXT("ContentId reflects SetFlawId()"), FlawDef->ContentId, Id2);
	TestEqual(TEXT("GetFlawId() reflects SetFlawId()"), FlawDef->GetFlawId(), Id2);

	// 4. Verification that GetPrimaryAssetId() uses authoritative ContentId and specialized "Flaw" type
	const FPrimaryAssetId ExpectedAssetId(TEXT("Flaw"), Id2);
	TestEqual(TEXT("GetPrimaryAssetId() uses ContentId and Flaw type"), FlawDef->GetPrimaryAssetId(), ExpectedAssetId);

	return true;
}

// 4. DefinitionValidation Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveFlawDefinitionValidationTest,
	"ShadowSlave.FlawDefinition.DefinitionValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveFlawDefinitionValidationTest::RunTest(const FString& Parameters)
{
	UShadowSlaveFlawDefinition* ValidDef = NewObject<UShadowSlaveFlawDefinition>();
	ValidDef->ContentId = FName(TEXT("Test_Valid_Flaw"));
	ValidDef->DisplayName = FText::FromString(TEXT("Valid Test Flaw"));
	ValidDef->Version = 1;
	ValidDef->ContentType = EShadowSlaveContentType::Custom;

	FString ErrorMsg;

	// 1. Valid definition passes
	TestTrue(TEXT("Valid flaw definition must pass IsValidDefinition"), ValidDef->IsValidDefinition(&ErrorMsg));
	TestTrue(TEXT("Valid flaw definition must pass ValidateDefinition"), ValidDef->ValidateDefinition(ErrorMsg));

	// 2. Generic validation: ContentId None fails
	ValidDef->ContentId = NAME_None;
	TestFalse(TEXT("None ContentId must fail IsValidDefinition"), ValidDef->IsValidDefinition(&ErrorMsg));
	TestFalse(TEXT("Error message must not be empty on failure"), ErrorMsg.IsEmpty());
	ValidDef->ContentId = FName(TEXT("Test_Valid_Flaw"));

	// 3. Generic validation: Empty DisplayName fails
	ValidDef->DisplayName = FText::GetEmpty();
	TestFalse(TEXT("Empty DisplayName must fail IsValidDefinition"), ValidDef->IsValidDefinition(&ErrorMsg));
	ValidDef->DisplayName = FText::FromString(TEXT("Valid Test Flaw"));

	// 4. Generic validation: Version < 1 fails
	ValidDef->Version = 0;
	TestFalse(TEXT("Version 0 must fail IsValidDefinition"), ValidDef->IsValidDefinition(&ErrorMsg));
	ValidDef->Version = 1;

	// 5. Generic validation: Wrong ContentType fails
	ValidDef->ContentType = EShadowSlaveContentType::Ability;
	TestFalse(TEXT("Wrong ContentType must fail IsValidDefinition"), ValidDef->IsValidDefinition(&ErrorMsg));
	ValidDef->ContentType = EShadowSlaveContentType::Custom;

	// Restored definition passes again
	TestTrue(TEXT("Restored definition passes IsValidDefinition"), ValidDef->IsValidDefinition(&ErrorMsg));

	return true;
}

// 5. DefinitionRegistryIntegration Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveFlawDefinitionRegistryIntegrationTest,
	"ShadowSlave.FlawDefinition.DefinitionRegistryIntegration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveFlawDefinitionRegistryIntegrationTest::RunTest(const FString& Parameters)
{
	UShadowSlaveContentRegistrySubsystem* Registry = NewObject<UShadowSlaveContentRegistrySubsystem>();
	TestNotNull(TEXT("Registry subsystem must be created"), Registry);

	UShadowSlaveFlawDefinition* FlawDef = NewObject<UShadowSlaveFlawDefinition>();
	FlawDef->ContentId = FName(TEXT("Flaw_Registry_Test"));
	FlawDef->DisplayName = FText::FromString(TEXT("Registry Test Flaw"));
	FlawDef->Version = 1;

	// 1. Register with generic registry subsystem
	const bool bRegistered = Registry->RegisterDefinition(FlawDef);
	TestTrue(TEXT("RegisterDefinition must succeed for UShadowSlaveFlawDefinition"), bRegistered);

	// 2. Query existence & count
	TestTrue(TEXT("HasContent must return true for registered FlawDef"), Registry->HasContent(FlawDef->ContentId));
	TestEqual(TEXT("Total registered count must be 1"), Registry->GetRegisteredContentCount(), 1);
	TestEqual(TEXT("Custom count by type must be 1"), Registry->GetRegisteredContentCountByType(EShadowSlaveContentType::Custom), 1);
	TestEqual(TEXT("Ability count by type must be 0"), Registry->GetRegisteredContentCountByType(EShadowSlaveContentType::Ability), 0);
	TestEqual(TEXT("Memory count by type must be 0"), Registry->GetRegisteredContentCountByType(EShadowSlaveContentType::Memory), 0);
	TestEqual(TEXT("Echo count by type must be 0"), Registry->GetRegisteredContentCountByType(EShadowSlaveContentType::Echo), 0);
	TestEqual(TEXT("Story count by type must be 0"), Registry->GetRegisteredContentCountByType(EShadowSlaveContentType::Story), 0);
	TestEqual(TEXT("Item count by type must be 0"), Registry->GetRegisteredContentCountByType(EShadowSlaveContentType::Item), 0);

	// 3. Generic resolution
	UShadowSlaveContentDefinition* ResolvedGeneric = Registry->ResolveContentDefinition(FlawDef->ContentId);
	TestNotNull(TEXT("Resolved generic definition must not be null"), ResolvedGeneric);
	TestEqual(TEXT("Resolved generic definition must match FlawDef"), ResolvedGeneric, Cast<UShadowSlaveContentDefinition>(FlawDef));

	// 4. Typed resolution
	UShadowSlaveFlawDefinition* ResolvedFlaw = Registry->ResolveContentDefinition<UShadowSlaveFlawDefinition>(FlawDef->ContentId);
	TestNotNull(TEXT("Resolved typed flaw definition must not be null"), ResolvedFlaw);
	TestEqual(TEXT("Resolved typed flaw must match original FlawDef"), ResolvedFlaw, FlawDef);

	// 5. PrimaryAssetId verification: uses specialized "Flaw" type
	const FPrimaryAssetId ExpectedAssetId(TEXT("Flaw"), FlawDef->ContentId);
	TestEqual(TEXT("GetPrimaryAssetId must match expected PrimaryAssetId with Flaw type"), FlawDef->GetPrimaryAssetId(), ExpectedAssetId);

	return true;
}

// 6. ExistingRuntimeCompatibility Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveFlawDefinitionExistingRuntimeCompatibilityTest,
	"ShadowSlave.FlawDefinition.ExistingRuntimeCompatibility",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveFlawDefinitionExistingRuntimeCompatibilityTest::RunTest(const FString& Parameters)
{
	AShadowSlavePlayerCharacter* Player = NewObject<AShadowSlavePlayerCharacter>();
	TestNotNull(TEXT("Player must instantiate"), Player);
	if (!Player)
	{
		return false;
	}

	UShadowSlaveAspectComponent* Aspect = Player->GetAspectComponent();
	TestNotNull(TEXT("Aspect component must exist"), Aspect);
	if (!Aspect)
	{
		return false;
	}

	const FName FlawId(TEXT("Compat_Runtime_Flaw"));
	UShadowSlaveFlawDefinition* FlawDef = NewObject<UShadowSlaveFlawDefinition>(Player);
	FlawDef->SetFlawId(FlawId);
	FlawDef->DisplayName = FText::FromString(TEXT("Compat Runtime Flaw"));
	FlawDef->Description = FText::FromString(TEXT("A metaphysical limitation for testing runtime compatibility."));
	FlawDef->Version = 1;

	UShadowSlaveAspectDefinition* AspectDef = NewObject<UShadowSlaveAspectDefinition>(Player);
	AspectDef->SetAspectId(FName(TEXT("Compat_Flaw_Aspect")));
	AspectDef->FlawDefinition = FlawDef;

	// 1. Aspect assignment binds the Flaw
	TestTrue(TEXT("SetAspectDefinition must succeed"), Aspect->SetAspectDefinition(AspectDef));
	TestEqual(TEXT("Active Flaw matches AspectDef FlawDefinition"), Aspect->GetFlawDefinition(), FlawDef);

	// 2. Direct Flaw override/assignment
	UShadowSlaveFlawDefinition* OverrideFlaw = NewObject<UShadowSlaveFlawDefinition>(Player);
	OverrideFlaw->SetFlawId(FName(TEXT("Compat_Override_Flaw")));
	OverrideFlaw->DisplayName = FText::FromString(TEXT("Override Flaw"));
	OverrideFlaw->Version = 1;

	TestTrue(TEXT("SetFlawDefinition succeeds"), Aspect->SetFlawDefinition(OverrideFlaw));
	TestEqual(TEXT("Active Flaw matches OverrideFlaw"), Aspect->GetFlawDefinition(), OverrideFlaw);

	// 3. Save/Load capture and restore
	UShadowSlaveSaveSubsystem* SaveSub = NewObject<UShadowSlaveSaveSubsystem>();
	TestNotNull(TEXT("SaveSubsystem must instantiate"), SaveSub);

	FShadowSlaveAspectSaveData SaveData;
	SaveSub->CaptureAspect(Aspect, SaveData);
	TestTrue(TEXT("CaptureAspect produces valid save data"), SaveData.bIsValid);
	TestEqual(TEXT("Saved FlawId matches OverrideFlaw ID"), SaveData.FlawId, OverrideFlaw->GetFlawId());
	TestEqual(TEXT("Saved FlawAssetId matches OverrideFlaw PrimaryAssetId"), SaveData.FlawAssetId, OverrideFlaw->GetPrimaryAssetId());

	// 4. Aspect replacement clears or replaces Flaw
	TestTrue(TEXT("Removing Aspect clears Flaw"), Aspect->SetAspectDefinition(nullptr));
	TestNull(TEXT("Active Flaw is null after Aspect removal"), Aspect->GetFlawDefinition());

	return true;
}

// =============================================================================
// Step 50: Aspect Definition Content Pipeline Integration Tests
// =============================================================================

// 1. DefinitionUsesGenericContentBase Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveAspectDefinitionUsesGenericContentBaseTest,
	"ShadowSlave.AspectDefinition.DefinitionUsesGenericContentBase",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveAspectDefinitionUsesGenericContentBaseTest::RunTest(const FString& Parameters)
{
	UShadowSlaveAspectDefinition* AspectDef = NewObject<UShadowSlaveAspectDefinition>();
	TestNotNull(TEXT("Aspect definition must instantiate"), AspectDef);

	// 1. Inheritance verification
	TestTrue(TEXT("Aspect definition must be a UShadowSlaveContentDefinition"),
		AspectDef->IsA(UShadowSlaveContentDefinition::StaticClass()));

	UShadowSlaveContentDefinition* BaseDef = Cast<UShadowSlaveContentDefinition>(AspectDef);
	TestNotNull(TEXT("Cast to UShadowSlaveContentDefinition must succeed"), BaseDef);

	// 2. Generic content properties inherited and accessible
	AspectDef->ContentId = FName(TEXT("Test_Generic_Aspect"));
	AspectDef->DisplayName = FText::FromString(TEXT("Generic Test Aspect"));
	AspectDef->Description = FText::FromString(TEXT("Test description for aspect content pipeline."));
	AspectDef->Version = 2;
	AspectDef->ProvenanceNote = TEXT("Test Provenance Note");

	TestEqual(TEXT("Base ContentId matches"), BaseDef->ContentId, FName(TEXT("Test_Generic_Aspect")));
	TestEqual(TEXT("Base DisplayName matches"), BaseDef->DisplayName.ToString(), TEXT("Generic Test Aspect"));
	TestEqual(TEXT("Base Description matches"), BaseDef->Description.ToString(), TEXT("Test description for aspect content pipeline."));
	TestEqual(TEXT("Base Version matches"), BaseDef->Version, 2);
	TestEqual(TEXT("Base ProvenanceNote matches"), BaseDef->ProvenanceNote, TEXT("Test Provenance Note"));
	TestEqual(TEXT("Base ContentType is Custom"), BaseDef->ContentType, EShadowSlaveContentType::Custom);

	return true;
}

// 2. DefinitionUsesCustomContentType Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveAspectDefinitionUsesCustomContentTypeTest,
	"ShadowSlave.AspectDefinition.DefinitionUsesCustomContentType",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveAspectDefinitionUsesCustomContentTypeTest::RunTest(const FString& Parameters)
{
	UShadowSlaveAspectDefinition* AspectDef = NewObject<UShadowSlaveAspectDefinition>();
	TestNotNull(TEXT("Aspect definition must instantiate"), AspectDef);

	// 1. Top-level generic content type must default to Custom
	TestEqual(TEXT("Default ContentType must be EShadowSlaveContentType::Custom"),
		AspectDef->ContentType, EShadowSlaveContentType::Custom);

	// 2. Verify no dedicated Aspect generic enum value exists in EShadowSlaveContentType
	TestTrue(TEXT("ContentType is not Ability"), AspectDef->ContentType != EShadowSlaveContentType::Ability);
	TestTrue(TEXT("ContentType is not Item"), AspectDef->ContentType != EShadowSlaveContentType::Item);
	TestTrue(TEXT("ContentType is not Memory"), AspectDef->ContentType != EShadowSlaveContentType::Memory);
	TestTrue(TEXT("ContentType is not Echo"), AspectDef->ContentType != EShadowSlaveContentType::Echo);
	TestTrue(TEXT("ContentType is not Story"), AspectDef->ContentType != EShadowSlaveContentType::Story);

	// 3. Verify Aspect-specific fields exist and are separate from generic ContentType
	AspectDef->AspectRank = EShadowSlaveAspectRank::Divine;
	AspectDef->Metadata.Add(FName(TEXT("Domain")), TEXT("Shadow"));
	AspectDef->CanonProvenance = TEXT("Test Canon Provenance");
	TestEqual(TEXT("AspectRank is Divine"), AspectDef->AspectRank, EShadowSlaveAspectRank::Divine);
	TestTrue(TEXT("HasKnownAspectRank is true"), AspectDef->HasKnownAspectRank());
	TestTrue(TEXT("Metadata contains Domain"), AspectDef->Metadata.Contains(FName(TEXT("Domain"))));
	TestEqual(TEXT("CanonProvenance matches"), AspectDef->CanonProvenance, TEXT("Test Canon Provenance"));
	TestEqual(TEXT("ContentType remains Custom after mutating Aspect-specific fields"),
		AspectDef->ContentType, EShadowSlaveContentType::Custom);

	return true;
}

// 3. DefinitionHasSingleAuthoritativeId Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveAspectDefinitionHasSingleAuthoritativeIdTest,
	"ShadowSlave.AspectDefinition.DefinitionHasSingleAuthoritativeId",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveAspectDefinitionHasSingleAuthoritativeIdTest::RunTest(const FString& Parameters)
{
	UShadowSlaveAspectDefinition* AspectDef = NewObject<UShadowSlaveAspectDefinition>();
	TestNotNull(TEXT("Aspect definition must instantiate"), AspectDef);

	// 1. Initial state: ContentId is NAME_None, GetAspectId() returns NAME_None
	TestEqual(TEXT("Initial ContentId is NAME_None"), AspectDef->ContentId, NAME_None);
	TestEqual(TEXT("Initial GetAspectId() returns NAME_None"), AspectDef->GetAspectId(), NAME_None);

	// 2. Set ContentId directly -> GetAspectId() must return the same ID
	const FName Id1(TEXT("Aspect_Authoritative_01"));
	AspectDef->ContentId = Id1;
	TestEqual(TEXT("GetAspectId() returns authoritative ContentId"), AspectDef->GetAspectId(), Id1);

	// 3. Set via SetAspectId() -> ContentId must be updated
	const FName Id2(TEXT("Aspect_Authoritative_02"));
	AspectDef->SetAspectId(Id2);
	TestEqual(TEXT("ContentId reflects SetAspectId()"), AspectDef->ContentId, Id2);
	TestEqual(TEXT("GetAspectId() reflects SetAspectId()"), AspectDef->GetAspectId(), Id2);

	// 4. Verification that GetPrimaryAssetId() uses authoritative ContentId and specialized "Aspect" type
	const FPrimaryAssetId ExpectedAssetId(TEXT("Aspect"), Id2);
	TestEqual(TEXT("GetPrimaryAssetId() uses ContentId and Aspect type"), AspectDef->GetPrimaryAssetId(), ExpectedAssetId);

	return true;
}

// 4. DefinitionValidation Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveAspectDefinitionValidationTest,
	"ShadowSlave.AspectDefinition.DefinitionValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveAspectDefinitionValidationTest::RunTest(const FString& Parameters)
{
	UShadowSlaveAspectDefinition* ValidDef = NewObject<UShadowSlaveAspectDefinition>();
	ValidDef->ContentId = FName(TEXT("Test_Valid_Aspect"));
	ValidDef->DisplayName = FText::FromString(TEXT("Valid Test Aspect"));
	ValidDef->Version = 1;
	ValidDef->ContentType = EShadowSlaveContentType::Custom;

	FString ErrorMsg;

	// 1. Valid definition passes
	TestTrue(TEXT("Valid aspect definition must pass IsValidDefinition"), ValidDef->IsValidDefinition(&ErrorMsg));
	TestTrue(TEXT("Valid aspect definition must pass ValidateDefinition"), ValidDef->ValidateDefinition(ErrorMsg));

	// 2. Generic validation: ContentId None fails
	ValidDef->ContentId = NAME_None;
	TestFalse(TEXT("None ContentId must fail IsValidDefinition"), ValidDef->IsValidDefinition(&ErrorMsg));
	TestFalse(TEXT("Error message must not be empty on failure"), ErrorMsg.IsEmpty());
	ValidDef->ContentId = FName(TEXT("Test_Valid_Aspect"));

	// 3. Generic validation: Empty DisplayName fails
	ValidDef->DisplayName = FText::GetEmpty();
	TestFalse(TEXT("Empty DisplayName must fail IsValidDefinition"), ValidDef->IsValidDefinition(&ErrorMsg));
	ValidDef->DisplayName = FText::FromString(TEXT("Valid Test Aspect"));

	// 4. Generic validation: Version < 1 fails
	ValidDef->Version = 0;
	TestFalse(TEXT("Version 0 must fail IsValidDefinition"), ValidDef->IsValidDefinition(&ErrorMsg));
	ValidDef->Version = 1;

	// 5. Generic validation: Wrong ContentType fails
	ValidDef->ContentType = EShadowSlaveContentType::Ability;
	TestFalse(TEXT("Wrong ContentType must fail IsValidDefinition"), ValidDef->IsValidDefinition(&ErrorMsg));
	ValidDef->ContentType = EShadowSlaveContentType::Custom;

	// Restored definition passes again
	TestTrue(TEXT("Restored definition passes IsValidDefinition"), ValidDef->IsValidDefinition(&ErrorMsg));

	return true;
}

// 5. DefinitionPrimaryAssetId Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveAspectDefinitionPrimaryAssetIdTest,
	"ShadowSlave.AspectDefinition.DefinitionPrimaryAssetId",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveAspectDefinitionPrimaryAssetIdTest::RunTest(const FString& Parameters)
{
	UShadowSlaveAspectDefinition* AspectDef = NewObject<UShadowSlaveAspectDefinition>();
	TestNotNull(TEXT("Aspect definition must instantiate"), AspectDef);

	const FName AspectId(TEXT("Aspect_PrimaryAsset_01"));
	AspectDef->SetAspectId(AspectId);
	AspectDef->DisplayName = FText::FromString(TEXT("Primary Asset Aspect"));
	AspectDef->Version = 1;

	const FPrimaryAssetId AssetId = AspectDef->GetPrimaryAssetId();
	TestEqual(TEXT("GetPrimaryAssetId must equal FPrimaryAssetId(Aspect, ContentId)"),
		AssetId, FPrimaryAssetId(TEXT("Aspect"), AspectId));
	TestTrue(TEXT("PrimaryAssetType must be Aspect"), AssetId.PrimaryAssetType == FPrimaryAssetType(TEXT("Aspect")));
	TestEqual(TEXT("PrimaryAssetName must be ContentId"), AssetId.PrimaryAssetName, AspectId);

	// Fallback: none ContentId uses UObject name, still Aspect type via Custom hook
	AspectDef->ContentId = NAME_None;
	const FPrimaryAssetId FallbackId = AspectDef->GetPrimaryAssetId();
	TestTrue(TEXT("Fallback PrimaryAssetType remains Aspect"), FallbackId.PrimaryAssetType == FPrimaryAssetType(TEXT("Aspect")));
	TestEqual(TEXT("Fallback PrimaryAssetName uses GetFName()"), FallbackId.PrimaryAssetName, AspectDef->GetFName());

	return true;
}

// 6. DefinitionRegistryIntegration Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveAspectDefinitionRegistryIntegrationTest,
	"ShadowSlave.AspectDefinition.DefinitionRegistryIntegration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveAspectDefinitionRegistryIntegrationTest::RunTest(const FString& Parameters)
{
	UShadowSlaveContentRegistrySubsystem* Registry = NewObject<UShadowSlaveContentRegistrySubsystem>();
	TestNotNull(TEXT("Registry subsystem must be created"), Registry);

	UShadowSlaveAspectDefinition* AspectDef = NewObject<UShadowSlaveAspectDefinition>();
	AspectDef->ContentId = FName(TEXT("Aspect_Registry_Test"));
	AspectDef->DisplayName = FText::FromString(TEXT("Registry Test Aspect"));
	AspectDef->Version = 1;
	AspectDef->AspectRank = EShadowSlaveAspectRank::Awakened;

	// 1. Register with generic registry subsystem
	const bool bRegistered = Registry->RegisterDefinition(AspectDef);
	TestTrue(TEXT("RegisterDefinition must succeed for UShadowSlaveAspectDefinition"), bRegistered);

	// 2. Query existence & count
	TestTrue(TEXT("HasContent must return true for registered AspectDef"), Registry->HasContent(AspectDef->ContentId));
	TestEqual(TEXT("Total registered count must be 1"), Registry->GetRegisteredContentCount(), 1);
	TestEqual(TEXT("Custom count by type must be 1"), Registry->GetRegisteredContentCountByType(EShadowSlaveContentType::Custom), 1);
	TestEqual(TEXT("Ability count by type must be 0"), Registry->GetRegisteredContentCountByType(EShadowSlaveContentType::Ability), 0);
	TestEqual(TEXT("Memory count by type must be 0"), Registry->GetRegisteredContentCountByType(EShadowSlaveContentType::Memory), 0);
	TestEqual(TEXT("Echo count by type must be 0"), Registry->GetRegisteredContentCountByType(EShadowSlaveContentType::Echo), 0);
	TestEqual(TEXT("Story count by type must be 0"), Registry->GetRegisteredContentCountByType(EShadowSlaveContentType::Story), 0);
	TestEqual(TEXT("Item count by type must be 0"), Registry->GetRegisteredContentCountByType(EShadowSlaveContentType::Item), 0);

	// 3. Generic resolution
	UShadowSlaveContentDefinition* ResolvedGeneric = Registry->ResolveContentDefinition(AspectDef->ContentId);
	TestNotNull(TEXT("Resolved generic definition must not be null"), ResolvedGeneric);
	TestEqual(TEXT("Resolved generic definition must match AspectDef"), ResolvedGeneric, Cast<UShadowSlaveContentDefinition>(AspectDef));

	// 4. Typed resolution
	UShadowSlaveAspectDefinition* ResolvedAspect = Registry->ResolveContentDefinition<UShadowSlaveAspectDefinition>(AspectDef->ContentId);
	TestNotNull(TEXT("Resolved typed aspect definition must not be null"), ResolvedAspect);
	TestEqual(TEXT("Resolved typed aspect must match original AspectDef"), ResolvedAspect, AspectDef);
	TestEqual(TEXT("Resolved AspectRank matches"), ResolvedAspect->AspectRank, EShadowSlaveAspectRank::Awakened);

	// 5. PrimaryAssetId verification: uses specialized "Aspect" type
	const FPrimaryAssetId ExpectedAssetId(TEXT("Aspect"), AspectDef->ContentId);
	TestEqual(TEXT("GetPrimaryAssetId must match expected PrimaryAssetId with Aspect type"), AspectDef->GetPrimaryAssetId(), ExpectedAssetId);

	return true;
}

// 7. ExistingRuntimeCompatibility Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveAspectDefinitionExistingRuntimeCompatibilityTest,
	"ShadowSlave.AspectDefinition.ExistingRuntimeCompatibility",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveAspectDefinitionExistingRuntimeCompatibilityTest::RunTest(const FString& Parameters)
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

	const FName AspectId(TEXT("Compat_Pipeline_Aspect"));
	const FName AbilityId(TEXT("Compat_Pipeline_Ability"));

	UShadowSlaveAspectAbilityDefinition* AbilityDef = NewObject<UShadowSlaveAspectAbilityDefinition>(Player);
	AbilityDef->SetAbilityId(AbilityId);
	AbilityDef->DisplayName = FText::FromString(TEXT("Compat Pipeline Ability"));
	AbilityDef->Version = 1;
	AbilityDef->BaseEssenceCost = 10.0f;
	AbilityDef->RequiredCharacterRank = EShadowSlaveCharacterRank::Unknown;

	UShadowSlaveFlawDefinition* FlawDef = NewObject<UShadowSlaveFlawDefinition>(Player);
	FlawDef->SetFlawId(FName(TEXT("Compat_Pipeline_Flaw")));
	FlawDef->DisplayName = FText::FromString(TEXT("Compat Pipeline Flaw"));
	FlawDef->Version = 1;

	UShadowSlaveAspectDefinition* AspectDef = NewObject<UShadowSlaveAspectDefinition>(Player);
	AspectDef->SetAspectId(AspectId);
	AspectDef->DisplayName = FText::FromString(TEXT("Compat Pipeline Aspect"));
	AspectDef->Version = 1;
	AspectDef->AspectRank = EShadowSlaveAspectRank::Ascended;
	AspectDef->AbilityDefinitions.Add(AbilityDef);
	AspectDef->FlawDefinition = FlawDef;

	TestTrue(TEXT("SetAspectDefinition must succeed"), Aspect->SetAspectDefinition(AspectDef));
	TestEqual(TEXT("Bound Aspect definition matches"), Aspect->GetAspectDefinition(), AspectDef);
	TestEqual(TEXT("GetAspectId remains ContentId"), AspectDef->GetAspectId(), AspectId);
	TestEqual(TEXT("Aspect rank is read from definition"), Aspect->GetAspectRank(), EShadowSlaveAspectRank::Ascended);
	TestEqual(TEXT("Bound Flaw matches definition"), Aspect->GetFlawDefinition(), FlawDef);
	TestEqual(TEXT("FindAbilityById returns the granted ability"), AspectDef->FindAbilityById(AbilityId), AbilityDef);

	Attributes->SetEssence(40.0f);
	TestTrue(TEXT("UnlockAbility succeeds"), Aspect->UnlockAbility(AbilityId));
	TestTrue(TEXT("CanActivateAbility succeeds with sufficient Essence"), Aspect->CanActivateAbility(AbilityId));
	TestTrue(TEXT("ActivateAbility succeeds"), Aspect->ActivateAbility(AbilityId));
	TestTrue(TEXT("Ability is transiently active"), Aspect->IsAbilityActive(AbilityId));
	TestNearlyEqual(TEXT("Essence consumed on activation"), Attributes->GetCurrentEssence(), 30.0f, 0.001f);

	UShadowSlaveSaveSubsystem* SaveSub = NewObject<UShadowSlaveSaveSubsystem>();
	TestNotNull(TEXT("SaveSubsystem must instantiate"), SaveSub);

	FShadowSlaveAspectSaveData SaveData;
	SaveSub->CaptureAspect(Aspect, SaveData);
	TestTrue(TEXT("CaptureAspect produces valid save data"), SaveData.bIsValid);
	TestEqual(TEXT("Saved AspectId matches GetAspectId/ContentId"), SaveData.AspectId, AspectDef->GetAspectId());
	TestEqual(TEXT("Saved AspectAssetId matches GetPrimaryAssetId"), SaveData.AspectAssetId, AspectDef->GetPrimaryAssetId());
	TestTrue(TEXT("UnlockedAbilityIds contains AbilityId"), SaveData.UnlockedAbilityIds.Contains(AbilityId));
	TestEqual(TEXT("Saved FlawId matches bound Flaw"), SaveData.FlawId, FlawDef->GetFlawId());

	TestTrue(TEXT("Aspect replacement succeeds"), Aspect->SetAspectDefinition(nullptr));
	TestFalse(TEXT("HasAspect is false after replacement"), Aspect->HasAspect());
	TestFalse(TEXT("Transient active ability state is cleared"), Aspect->IsAbilityActive(AbilityId));
	TestNull(TEXT("Active Flaw is null after Aspect removal"), Aspect->GetFlawDefinition());

	return true;
}

#endif // WITH_AUTOMATION_TESTS
