// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_AUTOMATION_TESTS

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Echoes/ShadowSlaveEchoComponent.h"
#include "Echoes/ShadowSlaveEchoDefinition.h"
#include "Echoes/ShadowSlaveEchoTypes.h"
#include "Attributes/ShadowSlaveAttributeComponent.h"
#include "Characters/ShadowSlavePlayerCharacter.h"
#include "Save/ShadowSlaveSaveSubsystem.h"
#include "Save/ShadowSlaveSaveTypes.h"

namespace
{
	struct FShadowSlaveEchoTestFixture
	{
		AShadowSlavePlayerCharacter* Player = nullptr;
		UShadowSlaveEchoComponent* EchoComp = nullptr;
		UShadowSlaveAttributeComponent* Attributes = nullptr;
		UShadowSlaveEchoDefinition* EchoDef = nullptr;

		bool Initialize(float SummonCost = 0.0f, EShadowSlaveEchoRank Rank = EShadowSlaveEchoRank::Awakened, EShadowSlaveEchoClass Class = EShadowSlaveEchoClass::Monster)
		{
			Player = NewObject<AShadowSlavePlayerCharacter>();
			if (!Player)
			{
				return false;
			}

			EchoComp = Player->GetEchoComponent();
			Attributes = Player->GetAttributeComponent();
			EchoDef = NewObject<UShadowSlaveEchoDefinition>(Player);
			if (!EchoComp || !Attributes || !EchoDef)
			{
				return false;
			}

			EchoDef->EchoId = FName(TEXT("Test_Echo_Scout"));
			EchoDef->DisplayName = FText::FromString(TEXT("Test Scout"));
			EchoDef->Rank = Rank;
			EchoDef->Class = Class;
			EchoDef->SummonEssenceCost = SummonCost;

			return true;
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveEchoAcquisitionAndQueriesTest,
	"ShadowSlave.Echoes.AcquisitionAndQueries",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveEchoAcquisitionAndQueriesTest::RunTest(const FString& Parameters)
{
	FShadowSlaveEchoTestFixture Fixture;
	TestTrue(TEXT("Fixture must initialize"), Fixture.Initialize(0.0f, EShadowSlaveEchoRank::Awakened, EShadowSlaveEchoClass::Monster));
	if (!Fixture.EchoComp || !Fixture.EchoDef)
	{
		return false;
	}

	// Initial state
	TestEqual(TEXT("Initial echo count must be 0"), Fixture.EchoComp->GetEchoCount(), 0);
	TestFalse(TEXT("HasEcho must return false before acquisition"), Fixture.EchoComp->HasEcho(Fixture.EchoDef));

	// Acquisition
	FShadowSlaveEchoInstance AcquiredInstance;
	TestTrue(TEXT("AcquireEcho must succeed"), Fixture.EchoComp->AcquireEcho(Fixture.EchoDef, AcquiredInstance));
	TestTrue(TEXT("Acquired instance must be valid"), AcquiredInstance.IsValid());
	TestTrue(TEXT("Acquired instance must have valid GUID"), AcquiredInstance.InstanceId.IsValid());
	TestEqual(TEXT("Acquired instance must start in Dormant state"), AcquiredInstance.State, EShadowSlaveEchoState::Dormant);
	TestFalse(TEXT("Acquired instance must not start summoned"), AcquiredInstance.bIsSummoned);
	TestEqual(TEXT("Echo count must now be 1"), Fixture.EchoComp->GetEchoCount(), 1);

	// Queries
	TestTrue(TEXT("HasEcho must return true for definition"), Fixture.EchoComp->HasEcho(Fixture.EchoDef));
	TestTrue(TEXT("HasEchoByInstanceId must return true for GUID"), Fixture.EchoComp->HasEchoByInstanceId(AcquiredInstance.InstanceId));

	FShadowSlaveEchoInstance FoundInstance;
	TestTrue(TEXT("FindEcho must find by GUID"), Fixture.EchoComp->FindEcho(AcquiredInstance.InstanceId, FoundInstance));
	TestEqual(TEXT("Found GUID must match"), FoundInstance.InstanceId, AcquiredInstance.InstanceId);

	TArray<FShadowSlaveEchoInstance> ByRank = Fixture.EchoComp->GetEchoesByRank(EShadowSlaveEchoRank::Awakened);
	TestEqual(TEXT("GetEchoesByRank must return 1 match"), ByRank.Num(), 1);

	TArray<FShadowSlaveEchoInstance> ByClass = Fixture.EchoComp->GetEchoesByClass(EShadowSlaveEchoClass::Monster);
	TestEqual(TEXT("GetEchoesByClass must return 1 match"), ByClass.Num(), 1);

	// Dynamic properties
	TestTrue(TEXT("SetEchoDynamicProperty must succeed"), Fixture.EchoComp->SetEchoDynamicProperty(AcquiredInstance.InstanceId, FName(TEXT("Nickname")), TEXT("Scouty")));
	FString PropValue;
	TestTrue(TEXT("GetEchoDynamicProperty must retrieve value"), Fixture.EchoComp->GetEchoDynamicProperty(AcquiredInstance.InstanceId, FName(TEXT("Nickname")), PropValue));
	TestEqual(TEXT("Dynamic property value must match"), PropValue, TEXT("Scouty"));
	TestTrue(TEXT("RemoveEchoDynamicProperty must succeed"), Fixture.EchoComp->RemoveEchoDynamicProperty(AcquiredInstance.InstanceId, FName(TEXT("Nickname"))));
	TestFalse(TEXT("Dynamic property must be removed"), Fixture.EchoComp->GetEchoDynamicProperty(AcquiredInstance.InstanceId, FName(TEXT("Nickname")), PropValue));

	// Duplicate GUID rejection
	TestFalse(TEXT("AddEchoInstance with existing GUID must be rejected"), Fixture.EchoComp->AddEchoInstance(AcquiredInstance));
	TestEqual(TEXT("Echo count must remain 1 after duplicate rejection"), Fixture.EchoComp->GetEchoCount(), 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveEchoSummonLifecycleTest,
	"ShadowSlave.Echoes.SummonAndDismissLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveEchoSummonLifecycleTest::RunTest(const FString& Parameters)
{
	FShadowSlaveEchoTestFixture Fixture;
	TestTrue(TEXT("Fixture must initialize"), Fixture.Initialize(0.0f));
	if (!Fixture.EchoComp || !Fixture.EchoDef)
	{
		return false;
	}

	FShadowSlaveEchoInstance Echo;
	TestTrue(TEXT("AcquireEcho must succeed"), Fixture.EchoComp->AcquireEcho(Fixture.EchoDef, Echo));

	// Summoning
	TestTrue(TEXT("SummonEcho must succeed"), Fixture.EchoComp->SummonEcho(Echo.InstanceId));
	TestTrue(TEXT("IsEchoSummoned must return true"), Fixture.EchoComp->IsEchoSummoned(Echo.InstanceId));
	EShadowSlaveEchoState State = EShadowSlaveEchoState::Dormant;
	TestTrue(TEXT("GetEchoState must succeed"), Fixture.EchoComp->GetEchoState(Echo.InstanceId, State));
	TestEqual(TEXT("Echo state must be Summoned"), State, EShadowSlaveEchoState::Summoned);
	TestEqual(TEXT("GetSummonedEchoes must contain 1 echo"), Fixture.EchoComp->GetSummonedEchoes().Num(), 1);

	// Idempotent summoning
	TestTrue(TEXT("Repeated SummonEcho must return true"), Fixture.EchoComp->SummonEcho(Echo.InstanceId));
	TestTrue(TEXT("Echo must remain summoned"), Fixture.EchoComp->IsEchoSummoned(Echo.InstanceId));

	// Dismissal
	TestTrue(TEXT("DismissEcho must succeed"), Fixture.EchoComp->DismissEcho(Echo.InstanceId));
	TestFalse(TEXT("IsEchoSummoned must return false after dismissal"), Fixture.EchoComp->IsEchoSummoned(Echo.InstanceId));
	TestTrue(TEXT("GetEchoState must succeed"), Fixture.EchoComp->GetEchoState(Echo.InstanceId, State));
	TestEqual(TEXT("Echo state must be Dormant"), State, EShadowSlaveEchoState::Dormant);
	TestEqual(TEXT("GetSummonedEchoes must be empty"), Fixture.EchoComp->GetSummonedEchoes().Num(), 0);

	// Idempotent dismissal
	TestTrue(TEXT("Repeated DismissEcho must return true"), Fixture.EchoComp->DismissEcho(Echo.InstanceId));
	TestFalse(TEXT("Echo must remain dormant"), Fixture.EchoComp->IsEchoSummoned(Echo.InstanceId));

	// DismissAllEchoes
	TestTrue(TEXT("SummonEcho again"), Fixture.EchoComp->SummonEcho(Echo.InstanceId));
	TestTrue(TEXT("DismissAllEchoes must succeed"), Fixture.EchoComp->DismissAllEchoes());
	TestFalse(TEXT("Echo must be dormant after DismissAllEchoes"), Fixture.EchoComp->IsEchoSummoned(Echo.InstanceId));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveEchoEssenceCostTest,
	"ShadowSlave.Echoes.EssenceCostValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveEchoEssenceCostTest::RunTest(const FString& Parameters)
{
	FShadowSlaveEchoTestFixture Fixture;
	TestTrue(TEXT("Fixture must initialize with 35 essence cost"), Fixture.Initialize(35.0f));
	if (!Fixture.EchoComp || !Fixture.EchoDef || !Fixture.Attributes)
	{
		return false;
	}

	FShadowSlaveEchoInstance Echo;
	TestTrue(TEXT("AcquireEcho must succeed"), Fixture.EchoComp->AcquireEcho(Fixture.EchoDef, Echo));

	// Insufficient essence
	Fixture.Attributes->SetEssence(20.0f);
	TestFalse(TEXT("SummonEcho must fail with insufficient essence"), Fixture.EchoComp->SummonEcho(Echo.InstanceId));
	TestFalse(TEXT("Echo must remain dormant"), Fixture.EchoComp->IsEchoSummoned(Echo.InstanceId));
	TestNearlyEqual(TEXT("Essence must not be consumed on failed summon"), Fixture.Attributes->GetCurrentEssence(), 20.0f, 0.001f);

	// Sufficient essence
	Fixture.Attributes->SetEssence(60.0f);
	TestTrue(TEXT("SummonEcho must succeed with sufficient essence"), Fixture.EchoComp->SummonEcho(Echo.InstanceId));
	TestTrue(TEXT("Echo must now be summoned"), Fixture.EchoComp->IsEchoSummoned(Echo.InstanceId));
	TestNearlyEqual(TEXT("Essence must be consumed by summon cost (60 - 35 = 25)"), Fixture.Attributes->GetCurrentEssence(), 25.0f, 0.001f);

	// Idempotent repeated summon must not consume essence again
	TestTrue(TEXT("Repeated summon must succeed"), Fixture.EchoComp->SummonEcho(Echo.InstanceId));
	TestNearlyEqual(TEXT("Repeated summon must not consume essence again"), Fixture.Attributes->GetCurrentEssence(), 25.0f, 0.001f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveEchoDestructionVsRemovalTest,
	"ShadowSlave.Echoes.DestructionVsRemoval",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveEchoDestructionVsRemovalTest::RunTest(const FString& Parameters)
{
	FShadowSlaveEchoTestFixture Fixture;
	TestTrue(TEXT("Fixture must initialize"), Fixture.Initialize(0.0f));
	if (!Fixture.EchoComp || !Fixture.EchoDef)
	{
		return false;
	}

	FShadowSlaveEchoInstance Echo1;
	FShadowSlaveEchoInstance Echo2;
	TestTrue(TEXT("Acquire Echo1"), Fixture.EchoComp->AcquireEcho(Fixture.EchoDef, Echo1));
	TestTrue(TEXT("Acquire Echo2"), Fixture.EchoComp->AcquireEcho(Fixture.EchoDef, Echo2));
	TestEqual(TEXT("Echo count must be 2"), Fixture.EchoComp->GetEchoCount(), 2);

	// Summon Echo1 and remove it
	TestTrue(TEXT("Summon Echo1"), Fixture.EchoComp->SummonEcho(Echo1.InstanceId));
	TestTrue(TEXT("RemoveEcho must succeed and auto-dismiss"), Fixture.EchoComp->RemoveEcho(Echo1.InstanceId));
	TestEqual(TEXT("Echo count must now be 1"), Fixture.EchoComp->GetEchoCount(), 1);
	TestFalse(TEXT("Echo1 must not be owned"), Fixture.EchoComp->HasEchoByInstanceId(Echo1.InstanceId));

	// Summon Echo2 and destroy it
	TestTrue(TEXT("Summon Echo2"), Fixture.EchoComp->SummonEcho(Echo2.InstanceId));
	TestTrue(TEXT("DestroyEcho must succeed and auto-dismiss"), Fixture.EchoComp->DestroyEcho(Echo2.InstanceId));
	TestEqual(TEXT("Echo count must now be 0"), Fixture.EchoComp->GetEchoCount(), 0);
	TestFalse(TEXT("Echo2 must not be owned"), Fixture.EchoComp->HasEchoByInstanceId(Echo2.InstanceId));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveEchoSaveLoadRoundtripTest,
	"ShadowSlave.Echoes.SaveLoadSnapshotRestoration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveEchoSaveLoadRoundtripTest::RunTest(const FString& Parameters)
{
	FShadowSlaveEchoTestFixture Fixture;
	TestTrue(TEXT("Fixture must initialize"), Fixture.Initialize(0.0f));
	if (!Fixture.EchoComp || !Fixture.EchoDef)
	{
		return false;
	}

	// Acquire and configure an echo
	FShadowSlaveEchoInstance OriginalEcho;
	TestTrue(TEXT("AcquireEcho"), Fixture.EchoComp->AcquireEcho(Fixture.EchoDef, OriginalEcho));
	TestTrue(TEXT("Set dynamic property"), Fixture.EchoComp->SetEchoDynamicProperty(OriginalEcho.InstanceId, FName(TEXT("TestKey")), TEXT("TestVal")));
	TestTrue(TEXT("Summon echo"), Fixture.EchoComp->SummonEcho(OriginalEcho.InstanceId));

	// Create save subsystem helper
	UShadowSlaveSaveSubsystem* SaveSubsystem = NewObject<UShadowSlaveSaveSubsystem>();
	TestNotNull(TEXT("SaveSubsystem must be valid"), SaveSubsystem);
	if (!SaveSubsystem)
	{
		return false;
	}

	// Capture snapshot
	FShadowSlaveEchoCollectionSaveData SaveData;
	SaveSubsystem->CaptureEchoes(Fixture.EchoComp, SaveData);
	TestTrue(TEXT("SaveData must be marked valid"), SaveData.bIsValid);
	TestEqual(TEXT("SaveData must contain 1 echo"), SaveData.Echoes.Num(), 1);

	const FShadowSlaveEchoSaveData& SavedEcho = SaveData.Echoes[0];
	TestEqual(TEXT("Saved GUID must match"), SavedEcho.InstanceId, OriginalEcho.InstanceId);
	TestEqual(TEXT("Saved EchoId must match"), SavedEcho.EchoId, Fixture.EchoDef->EchoId);
	TestTrue(TEXT("Saved bIsSummoned must be true"), SavedEcho.bIsSummoned);
	TestEqual(TEXT("Saved State must be Summoned"), SavedEcho.State, EShadowSlaveEchoState::Summoned);
	TestTrue(TEXT("Saved dynamic properties must contain TestKey"), SavedEcho.DynamicProperties.Contains(FName(TEXT("TestKey"))));
	TestEqual(TEXT("Saved dynamic property value must match"), SavedEcho.DynamicProperties[FName(TEXT("TestKey"))], TEXT("TestVal"));

	// Clear component
	Fixture.EchoComp->ClearEchoes();
	TestEqual(TEXT("Echo count must be 0 after clear"), Fixture.EchoComp->GetEchoCount(), 0);

	// Restore snapshot
	SaveSubsystem->RestoreEchoes(Fixture.EchoComp, SaveData);
	TestEqual(TEXT("Echo count must be 1 after restore"), Fixture.EchoComp->GetEchoCount(), 1);

	FShadowSlaveEchoInstance RestoredEcho;
	TestTrue(TEXT("Restored echo must be found by original GUID"), Fixture.EchoComp->FindEcho(OriginalEcho.InstanceId, RestoredEcho));
	TestEqual(TEXT("Restored GUID must match original"), RestoredEcho.InstanceId, OriginalEcho.InstanceId);
	TestTrue(TEXT("Restored bIsSummoned must match"), RestoredEcho.bIsSummoned);
	TestEqual(TEXT("Restored State must match"), RestoredEcho.State, EShadowSlaveEchoState::Summoned);
	FString RestoredProp;
	TestTrue(TEXT("Restored dynamic property must be present"), Fixture.EchoComp->GetEchoDynamicProperty(RestoredEcho.InstanceId, FName(TEXT("TestKey")), RestoredProp));
	TestEqual(TEXT("Restored dynamic property value must match"), RestoredProp, TEXT("TestVal"));

	return true;
}

#endif // WITH_AUTOMATION_TESTS
