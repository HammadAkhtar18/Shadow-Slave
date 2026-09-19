// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_AUTOMATION_TESTS

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "StatusEffects/ShadowSlaveStatusEffectComponent.h"
#include "StatusEffects/ShadowSlaveStatusEffectDefinition.h"
#include "StatusEffects/ShadowSlaveStatusEffectTypes.h"
#include "Characters/ShadowSlaveCharacterBase.h"
#include "Characters/ShadowSlavePlayerCharacter.h"
#include "Attributes/ShadowSlaveAttributeComponent.h"
#include "Save/ShadowSlaveSaveSubsystem.h"
#include "Save/ShadowSlaveSaveTypes.h"

namespace
{
	struct FShadowSlaveStatusEffectTestFixture
	{
		AShadowSlavePlayerCharacter* Player = nullptr;
		UShadowSlaveStatusEffectComponent* StatusComp = nullptr;
		UShadowSlaveAttributeComponent* Attributes = nullptr;
		UShadowSlaveStatusEffectDefinition* EffectDef = nullptr;

		bool Initialize(
			FName InEffectId = FName(TEXT("Test_Status_Buff")),
			EStatusEffectDurationPolicy DurationPolicy = EStatusEffectDurationPolicy::Timed,
			float Duration = 5.0f,
			EStatusEffectStackingPolicy StackingPolicy = EStatusEffectStackingPolicy::RefreshDuration,
			int32 MaxStacks = 1,
			EStatusEffectPolarity Polarity = EStatusEffectPolarity::Neutral,
			bool bPersist = false)
		{
			Player = NewObject<AShadowSlavePlayerCharacter>();
			if (!Player)
			{
				return false;
			}

			StatusComp = Player->GetStatusEffectComponent();
			Attributes = Player->GetAttributeComponent();
			EffectDef = NewObject<UShadowSlaveStatusEffectDefinition>(Player);
			if (!StatusComp || !Attributes || !EffectDef)
			{
				return false;
			}

			EffectDef->EffectId = InEffectId;
			EffectDef->DisplayName = FText::FromString(TEXT("Test Status"));
			EffectDef->Description = FText::FromString(TEXT("A test status effect for automation."));
			EffectDef->DurationPolicy = DurationPolicy;
			EffectDef->Duration = Duration;
			EffectDef->StackingPolicy = StackingPolicy;
			EffectDef->MaxStacks = MaxStacks;
			EffectDef->Polarity = Polarity;
			EffectDef->bPersistAcrossSaveLoad = bPersist;

			return true;
		}
	};
}

// 1. Application and Queries Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveStatusEffectApplicationAndQueriesTest,
	"ShadowSlave.StatusEffects.ApplicationAndQueries",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveStatusEffectApplicationAndQueriesTest::RunTest(const FString& Parameters)
{
	FShadowSlaveStatusEffectTestFixture Fixture;
	TestTrue(TEXT("Fixture must initialize"), Fixture.Initialize());
	if (!Fixture.StatusComp || !Fixture.EffectDef)
	{
		return false;
	}

	// Initial state verification
	TestEqual(TEXT("Initial effect count must be 0"), Fixture.StatusComp->GetEffectCount(), 0);
	TestFalse(TEXT("HasEffect must return false before application"), Fixture.StatusComp->HasEffect(Fixture.EffectDef));
	TestFalse(TEXT("HasEffectById must return false before application"), Fixture.StatusComp->HasEffectById(Fixture.EffectDef->EffectId));

	// Application with source attribution and dynamic properties
	const FGuid SourceGuid = FGuid::NewGuid();
	const FShadowSlaveStatusEffectSource Source(SourceGuid, FName(TEXT("TestSource")), Fixture.Player);
	TMap<FName, FString> DynamicProps;
	DynamicProps.Add(FName(TEXT("Intensity")), TEXT("High"));

	const FGuid AppliedId = Fixture.StatusComp->ApplyEffect(Fixture.EffectDef, Source, DynamicProps);
	TestTrue(TEXT("ApplyEffect must return a valid GUID"), AppliedId.IsValid());
	TestEqual(TEXT("Effect count must be 1"), Fixture.StatusComp->GetEffectCount(), 1);

	// Query verification
	TestTrue(TEXT("HasEffect must return true"), Fixture.StatusComp->HasEffect(Fixture.EffectDef));
	TestTrue(TEXT("HasEffectById must return true"), Fixture.StatusComp->HasEffectById(Fixture.EffectDef->EffectId));
	TestTrue(TEXT("HasEffectByInstanceId must return true"), Fixture.StatusComp->HasEffectByInstanceId(AppliedId));

	FShadowSlaveStatusEffectInstance FoundInstance;
	TestTrue(TEXT("FindEffect must find by definition"), Fixture.StatusComp->FindEffect(Fixture.EffectDef, FoundInstance));
	TestEqual(TEXT("Found GUID must match applied GUID"), FoundInstance.InstanceId, AppliedId);
	TestEqual(TEXT("Found stacks must be 1"), FoundInstance.CurrentStacks, 1);
	TestEqual(TEXT("Found source GUID must match"), FoundInstance.Source.SourceId, SourceGuid);
	TestEqual(TEXT("Found source Name must match"), FoundInstance.Source.SourceName, FName(TEXT("TestSource")));

	const FString* FoundProp = FoundInstance.DynamicProperties.Find(FName(TEXT("Intensity")));
	TestNotNull(TEXT("Dynamic property must exist"), FoundProp);
	if (FoundProp)
	{
		TestEqual(TEXT("Dynamic property value must match"), *FoundProp, TEXT("High"));
	}

	TestEqual(TEXT("GetStackCount must return 1"), Fixture.StatusComp->GetStackCount(Fixture.EffectDef), 1);

	return true;
}

// 2. Stacking Policies Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveStatusEffectStackingPoliciesTest,
	"ShadowSlave.StatusEffects.StackingPolicies",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveStatusEffectStackingPoliciesTest::RunTest(const FString& Parameters)
{
	// Policy A: IgnoreNew
	{
		FShadowSlaveStatusEffectTestFixture Fixture;
		TestTrue(TEXT("Fixture must initialize"), Fixture.Initialize(
			FName(TEXT("Test_IgnoreNew")),
			EStatusEffectDurationPolicy::Timed,
			5.0f,
			EStatusEffectStackingPolicy::IgnoreNew,
			1
		));

		const FGuid FirstId = Fixture.StatusComp->ApplyEffectSimple(Fixture.EffectDef);
		TestTrue(TEXT("First application must succeed"), FirstId.IsValid());
		TestEqual(TEXT("Effect count must be 1"), Fixture.StatusComp->GetEffectCount(), 1);

		const FGuid SecondId = Fixture.StatusComp->ApplyEffectSimple(Fixture.EffectDef);
		TestEqual(TEXT("IgnoreNew must return existing instance GUID"), SecondId, FirstId);
		TestEqual(TEXT("Effect count must remain 1"), Fixture.StatusComp->GetEffectCount(), 1);
		TestEqual(TEXT("Stack count must remain 1"), Fixture.StatusComp->GetStackCount(Fixture.EffectDef), 1);
	}

	// Policy B: AddStacks up to MaxStacks
	{
		FShadowSlaveStatusEffectTestFixture Fixture;
		TestTrue(TEXT("Fixture must initialize"), Fixture.Initialize(
			FName(TEXT("Test_AddStacks")),
			EStatusEffectDurationPolicy::Timed,
			5.0f,
			EStatusEffectStackingPolicy::AddStacks,
			3
		));

		const FGuid Id1 = Fixture.StatusComp->ApplyEffectSimple(Fixture.EffectDef);
		TestTrue(TEXT("Id1 must be valid"), Id1.IsValid());
		TestEqual(TEXT("Stack count must be 1"), Fixture.StatusComp->GetStackCount(Fixture.EffectDef), 1);

		const FGuid Id2 = Fixture.StatusComp->ApplyEffectSimple(Fixture.EffectDef);
		TestEqual(TEXT("Id2 must equal Id1"), Id2, Id1);
		TestEqual(TEXT("Stack count must be 2"), Fixture.StatusComp->GetStackCount(Fixture.EffectDef), 2);

		const FGuid Id3 = Fixture.StatusComp->ApplyEffectSimple(Fixture.EffectDef);
		TestEqual(TEXT("Stack count must be 3"), Fixture.StatusComp->GetStackCount(Fixture.EffectDef), 3);

		// Exceeding MaxStacks clamps to MaxStacks
		const FGuid Id4 = Fixture.StatusComp->ApplyEffectSimple(Fixture.EffectDef);
		TestEqual(TEXT("Stack count must remain clamped at 3"), Fixture.StatusComp->GetStackCount(Fixture.EffectDef), 3);
		TestEqual(TEXT("Effect count must still be 1"), Fixture.StatusComp->GetEffectCount(), 1);
	}

	// Policy C: Replace
	{
		FShadowSlaveStatusEffectTestFixture Fixture;
		TestTrue(TEXT("Fixture must initialize"), Fixture.Initialize(
			FName(TEXT("Test_Replace")),
			EStatusEffectDurationPolicy::Timed,
			5.0f,
			EStatusEffectStackingPolicy::Replace,
			1
		));

		const FGuid FirstId = Fixture.StatusComp->ApplyEffectSimple(Fixture.EffectDef);
		TestTrue(TEXT("First application must succeed"), FirstId.IsValid());

		const FGuid SecondId = Fixture.StatusComp->ApplyEffectSimple(Fixture.EffectDef);
		TestTrue(TEXT("Second application must succeed"), SecondId.IsValid());
		TestNotEqual(TEXT("Replace must generate a new instance GUID"), SecondId, FirstId);
		TestEqual(TEXT("Effect count must remain 1"), Fixture.StatusComp->GetEffectCount(), 1);
		TestFalse(TEXT("Old GUID must no longer exist"), Fixture.StatusComp->HasEffectByInstanceId(FirstId));
		TestTrue(TEXT("New GUID must exist"), Fixture.StatusComp->HasEffectByInstanceId(SecondId));
	}

	return true;
}

// 3. Duration Policies Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveStatusEffectDurationPoliciesTest,
	"ShadowSlave.StatusEffects.DurationPolicies",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveStatusEffectDurationPoliciesTest::RunTest(const FString& Parameters)
{
	// Policy A: Instant
	{
		FShadowSlaveStatusEffectTestFixture Fixture;
		TestTrue(TEXT("Fixture must initialize"), Fixture.Initialize(
			FName(TEXT("Test_Instant")),
			EStatusEffectDurationPolicy::Instant
		));

		const FGuid InstantId = Fixture.StatusComp->ApplyEffectSimple(Fixture.EffectDef);
		TestTrue(TEXT("Instant effect must return a valid GUID upon execution"), InstantId.IsValid());
		TestEqual(TEXT("Instant effect must NOT be retained in ActiveEffects"), Fixture.StatusComp->GetEffectCount(), 0);
		TestFalse(TEXT("HasEffect must be false for instant effect"), Fixture.StatusComp->HasEffect(Fixture.EffectDef));
	}

	// Policy B: Persistent
	{
		FShadowSlaveStatusEffectTestFixture Fixture;
		TestTrue(TEXT("Fixture must initialize"), Fixture.Initialize(
			FName(TEXT("Test_Persistent")),
			EStatusEffectDurationPolicy::Persistent
		));

		const FGuid PersistId = Fixture.StatusComp->ApplyEffectSimple(Fixture.EffectDef);
		TestTrue(TEXT("Persistent effect must return valid GUID"), PersistId.IsValid());
		TestEqual(TEXT("Effect count must be 1"), Fixture.StatusComp->GetEffectCount(), 1);
		TestEqual(TEXT("Persistent effect remaining duration must return -1.0f"), Fixture.StatusComp->GetRemainingDuration(PersistId), -1.0f);
	}

	// Policy C: Timed
	{
		FShadowSlaveStatusEffectTestFixture Fixture;
		TestTrue(TEXT("Fixture must initialize"), Fixture.Initialize(
			FName(TEXT("Test_Timed")),
			EStatusEffectDurationPolicy::Timed,
			10.0f
		));

		const FGuid TimedId = Fixture.StatusComp->ApplyEffectSimple(Fixture.EffectDef);
		TestTrue(TEXT("Timed effect must return valid GUID"), TimedId.IsValid());
		TestEqual(TEXT("Effect count must be 1"), Fixture.StatusComp->GetEffectCount(), 1);

		FShadowSlaveStatusEffectInstance Inst;
		TestTrue(TEXT("FindEffect must succeed"), Fixture.StatusComp->FindEffect(Fixture.EffectDef, Inst));
		TestEqual(TEXT("TotalDuration must match configured duration"), Inst.TotalDuration, 10.0f);
		TestTrue(TEXT("Remaining duration must be positive"), Fixture.StatusComp->GetRemainingDuration(TimedId) > 0.0f);
		TestTrue(TEXT("Remaining duration must be <= TotalDuration"), Fixture.StatusComp->GetRemainingDuration(TimedId) <= 10.0f);
	}

	return true;
}

// 4. Removal and Cleanup Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveStatusEffectRemovalAndCleanupTest,
	"ShadowSlave.StatusEffects.RemovalAndCleanup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveStatusEffectRemovalAndCleanupTest::RunTest(const FString& Parameters)
{
	FShadowSlaveStatusEffectTestFixture Fixture;
	TestTrue(TEXT("Fixture must initialize"), Fixture.Initialize(FName(TEXT("Test_DefA"))));

	UShadowSlaveStatusEffectDefinition* DefB = NewObject<UShadowSlaveStatusEffectDefinition>(Fixture.Player);
	DefB->EffectId = FName(TEXT("Test_DefB"));
	DefB->DurationPolicy = EStatusEffectDurationPolicy::Persistent;

	const FGuid IdA = Fixture.StatusComp->ApplyEffectSimple(Fixture.EffectDef);
	const FGuid IdB = Fixture.StatusComp->ApplyEffectSimple(DefB);

	TestEqual(TEXT("Two effects must be active"), Fixture.StatusComp->GetEffectCount(), 2);

	// Remove single effect by GUID
	TestTrue(TEXT("RemoveEffect by GUID must return true"), Fixture.StatusComp->RemoveEffect(IdA));
	TestEqual(TEXT("One effect must remain"), Fixture.StatusComp->GetEffectCount(), 1);
	TestFalse(TEXT("DefA must no longer be present"), Fixture.StatusComp->HasEffect(Fixture.EffectDef));
	TestTrue(TEXT("DefB must still be present"), Fixture.StatusComp->HasEffect(DefB));

	// Reapply DefA and remove by definition
	Fixture.StatusComp->ApplyEffectSimple(Fixture.EffectDef);
	TestEqual(TEXT("Two effects must be active again"), Fixture.StatusComp->GetEffectCount(), 2);
	TestTrue(TEXT("RemoveEffectByDefinition must succeed"), Fixture.StatusComp->RemoveEffectByDefinition(Fixture.EffectDef));
	TestEqual(TEXT("One effect must remain"), Fixture.StatusComp->GetEffectCount(), 1);
	TestFalse(TEXT("DefA must no longer be present"), Fixture.StatusComp->HasEffect(Fixture.EffectDef));

	// Clear all effects
	Fixture.StatusComp->ApplyEffectSimple(Fixture.EffectDef);
	TestEqual(TEXT("Two effects must be active before clear"), Fixture.StatusComp->GetEffectCount(), 2);
	const int32 ClearedCount = Fixture.StatusComp->RemoveAllEffects();
	TestEqual(TEXT("RemoveAllEffects must report 2 removed"), ClearedCount, 2);
	TestEqual(TEXT("Active effects must be empty"), Fixture.StatusComp->GetEffectCount(), 0);

	return true;
}

// 5. Validation and Rejection Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveStatusEffectValidationAndRejectionTest,
	"ShadowSlave.StatusEffects.ValidationAndRejection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveStatusEffectValidationAndRejectionTest::RunTest(const FString& Parameters)
{
	FShadowSlaveStatusEffectTestFixture Fixture;
	TestTrue(TEXT("Fixture must initialize"), Fixture.Initialize());

	// 1. Null definition rejected
	const FGuid NullResult = Fixture.StatusComp->ApplyEffectSimple(nullptr);
	TestFalse(TEXT("Null definition must be rejected"), NullResult.IsValid());
	TestEqual(TEXT("Count must remain 0"), Fixture.StatusComp->GetEffectCount(), 0);

	// 2. Empty EffectId rejected
	Fixture.EffectDef->EffectId = NAME_None;
	TestFalse(TEXT("Effect with NAME_None ID must fail IsValidDefinition"), Fixture.EffectDef->IsValidDefinition());
	const FGuid EmptyIdResult = Fixture.StatusComp->ApplyEffectSimple(Fixture.EffectDef);
	TestFalse(TEXT("Empty EffectId must be rejected"), EmptyIdResult.IsValid());

	// 3. Timed effect with non-positive duration rejected
	Fixture.EffectDef->EffectId = FName(TEXT("Test_InvalidDuration"));
	Fixture.EffectDef->DurationPolicy = EStatusEffectDurationPolicy::Timed;
	Fixture.EffectDef->Duration = 0.0f;
	TestFalse(TEXT("Zero duration must fail IsValidDefinition"), Fixture.EffectDef->IsValidDefinition());
	Fixture.EffectDef->Duration = -5.0f;
	TestFalse(TEXT("Negative duration must fail IsValidDefinition"), Fixture.EffectDef->IsValidDefinition());
	const FGuid InvalidDurationResult = Fixture.StatusComp->ApplyEffectSimple(Fixture.EffectDef);
	TestFalse(TEXT("Invalid duration must be rejected"), InvalidDurationResult.IsValid());

	// 4. MaxStacks < 1 rejected
	Fixture.EffectDef->Duration = 5.0f;
	Fixture.EffectDef->MaxStacks = 0;
	TestFalse(TEXT("Zero MaxStacks must fail IsValidDefinition"), Fixture.EffectDef->IsValidDefinition());
	const FGuid InvalidStacksResult = Fixture.StatusComp->ApplyEffectSimple(Fixture.EffectDef);
	TestFalse(TEXT("Invalid MaxStacks must be rejected"), InvalidStacksResult.IsValid());

	return true;
}

// 6. Save / Load Boundary Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveStatusEffectSaveLoadBoundaryTest,
	"ShadowSlave.StatusEffects.SaveLoadBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveStatusEffectSaveLoadBoundaryTest::RunTest(const FString& Parameters)
{
	FShadowSlaveStatusEffectTestFixture Fixture;
	TestTrue(TEXT("Fixture must initialize"), Fixture.Initialize(
		FName(TEXT("Test_PersistentStatus")),
		EStatusEffectDurationPolicy::Persistent,
		0.0f,
		EStatusEffectStackingPolicy::RefreshDuration,
		1,
		EStatusEffectPolarity::Neutral,
		true // bPersistAcrossSaveLoad = true
	));

	// Create non-persistent definition
	UShadowSlaveStatusEffectDefinition* TransientDef = NewObject<UShadowSlaveStatusEffectDefinition>(Fixture.Player);
	TransientDef->EffectId = FName(TEXT("Test_TransientStatus"));
	TransientDef->DurationPolicy = EStatusEffectDurationPolicy::Timed;
	TransientDef->Duration = 5.0f;
	TransientDef->bPersistAcrossSaveLoad = false;

	// Apply persistent effect with dynamic property
	TMap<FName, FString> DynamicProps;
	DynamicProps.Add(FName(TEXT("SaveKey")), TEXT("SaveValue"));
	const FGuid PersistId = Fixture.StatusComp->ApplyEffect(Fixture.EffectDef, FShadowSlaveStatusEffectSource(), DynamicProps);

	// Apply transient effect
	const FGuid TransientId = Fixture.StatusComp->ApplyEffectSimple(TransientDef);

	TestEqual(TEXT("Total active effects before save must be 2"), Fixture.StatusComp->GetEffectCount(), 2);

	// Capture via SaveSubsystem
	UShadowSlaveSaveSubsystem* SaveSubsystem = NewObject<UShadowSlaveSaveSubsystem>();
	FShadowSlaveStatusEffectCollectionSaveData SaveData;
	SaveSubsystem->CaptureStatusEffects(Fixture.StatusComp, SaveData);

	TestTrue(TEXT("SaveData must be marked valid"), SaveData.bIsValid);
	TestEqual(TEXT("Only persistent effects must be captured (1 of 2)"), SaveData.Effects.Num(), 1);

	if (SaveData.Effects.Num() == 1)
	{
		const FShadowSlaveStatusEffectSaveData& Saved = SaveData.Effects[0];
		TestEqual(TEXT("Saved EffectId must match persistent effect"), Saved.EffectId, Fixture.EffectDef->EffectId);
		TestEqual(TEXT("Saved InstanceId must match original GUID"), Saved.InstanceId, PersistId);
		TestEqual(TEXT("Saved stacks must match"), Saved.CurrentStacks, 1);

		const FString* SavedVal = Saved.DynamicProperties.Find(FName(TEXT("SaveKey")));
		TestNotNull(TEXT("Dynamic property must be saved"), SavedVal);
		if (SavedVal)
		{
			TestEqual(TEXT("Dynamic property value must match"), *SavedVal, TEXT("SaveValue"));
		}
	}

	// Restore into a fresh component
	UShadowSlaveStatusEffectComponent* RestoredComp = NewObject<UShadowSlaveStatusEffectComponent>(Fixture.Player);
	TArray<FShadowSlaveStatusEffectInstance> RestoredInstances;
	for (const FShadowSlaveStatusEffectSaveData& Saved : SaveData.Effects)
	{
		FShadowSlaveStatusEffectInstance RestoredInst;
		RestoredInst.InstanceId = Saved.InstanceId;
		RestoredInst.EffectDefinition = Fixture.EffectDef;
		RestoredInst.CurrentStacks = Saved.CurrentStacks;
		RestoredInst.DynamicProperties = Saved.DynamicProperties;
		RestoredInstances.Add(RestoredInst);
	}

	RestoredComp->RestoreEffects(RestoredInstances);

	TestEqual(TEXT("Restored component must have 1 effect"), RestoredComp->GetEffectCount(), 1);
	TestTrue(TEXT("Restored component must have persistent definition"), RestoredComp->HasEffect(Fixture.EffectDef));
	TestTrue(TEXT("Restored component must have original GUID"), RestoredComp->HasEffectByInstanceId(PersistId));

	return true;
}

// 7. Attribute Authority Boundary Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveStatusEffectAttributeAuthorityBoundaryTest,
	"ShadowSlave.StatusEffects.AttributeAuthorityBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveStatusEffectAttributeAuthorityBoundaryTest::RunTest(const FString& Parameters)
{
	FShadowSlaveStatusEffectTestFixture Fixture;
	TestTrue(TEXT("Fixture must initialize"), Fixture.Initialize());

	const float InitialHealth = Fixture.Attributes->GetCurrentHealth();
	const float InitialStamina = Fixture.Attributes->GetCurrentStamina();
	const float InitialEssence = Fixture.Attributes->GetCurrentEssence();

	// Apply status effect
	const FGuid Id = Fixture.StatusComp->ApplyEffectSimple(Fixture.EffectDef);
	TestTrue(TEXT("Status effect applied"), Id.IsValid());

	// Attributes must remain untouched by StatusEffectComponent
	TestEqual(TEXT("Health must be unchanged after effect application"), Fixture.Attributes->GetCurrentHealth(), InitialHealth);
	TestEqual(TEXT("Stamina must be unchanged after effect application"), Fixture.Attributes->GetCurrentStamina(), InitialStamina);
	TestEqual(TEXT("Essence must be unchanged after effect application"), Fixture.Attributes->GetCurrentEssence(), InitialEssence);

	// Remove status effect
	Fixture.StatusComp->RemoveEffect(Id);

	// Attributes must remain untouched after effect removal
	TestEqual(TEXT("Health must be unchanged after effect removal"), Fixture.Attributes->GetCurrentHealth(), InitialHealth);
	TestEqual(TEXT("Stamina must be unchanged after effect removal"), Fixture.Attributes->GetCurrentStamina(), InitialStamina);
	TestEqual(TEXT("Essence must be unchanged after effect removal"), Fixture.Attributes->GetCurrentEssence(), InitialEssence);

	return true;
}

#endif // WITH_AUTOMATION_TESTS
