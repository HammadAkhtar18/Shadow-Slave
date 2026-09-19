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

	// 1. Initial state verification
	TestEqual(TEXT("Initial echo count must be 0"), Fixture.EchoComp->GetEchoCount(), 0);
	TestFalse(TEXT("HasEcho must return false before acquisition"), Fixture.EchoComp->HasEcho(Fixture.EchoDef));

	// 2. Acquisition creates valid owned Echo
	FShadowSlaveEchoInstance AcquiredInstance;
	TestTrue(TEXT("AcquireEcho must succeed"), Fixture.EchoComp->AcquireEcho(Fixture.EchoDef, AcquiredInstance));
	TestTrue(TEXT("Acquired instance must be valid"), AcquiredInstance.IsValid());
	TestTrue(TEXT("Acquired instance must have valid non-zero GUID"), AcquiredInstance.InstanceId.IsValid());
	TestEqual(TEXT("Acquired instance must start in Dormant state"), AcquiredInstance.State, EShadowSlaveEchoState::Dormant);
	TestFalse(TEXT("Acquired instance must not start summoned"), AcquiredInstance.bIsSummoned);
	TestEqual(TEXT("Echo count must now be 1"), Fixture.EchoComp->GetEchoCount(), 1);

	// 3. Ownership Queries
	TestTrue(TEXT("HasEcho must return true for definition"), Fixture.EchoComp->HasEcho(Fixture.EchoDef));
	TestTrue(TEXT("HasEchoByInstanceId must return true for GUID"), Fixture.EchoComp->HasEchoByInstanceId(AcquiredInstance.InstanceId));

	FShadowSlaveEchoInstance FoundInstance;
	TestTrue(TEXT("FindEcho must find by GUID"), Fixture.EchoComp->FindEcho(AcquiredInstance.InstanceId, FoundInstance));
	TestEqual(TEXT("Found GUID must match"), FoundInstance.InstanceId, AcquiredInstance.InstanceId);

	TArray<FShadowSlaveEchoInstance> ByRank = Fixture.EchoComp->GetEchoesByRank(EShadowSlaveEchoRank::Awakened);
	TestEqual(TEXT("GetEchoesByRank must return 1 match"), ByRank.Num(), 1);

	TArray<FShadowSlaveEchoInstance> ByClass = Fixture.EchoComp->GetEchoesByClass(EShadowSlaveEchoClass::Monster);
	TestEqual(TEXT("GetEchoesByClass must return 1 match"), ByClass.Num(), 1);

	// 4. Dynamic properties
	TestTrue(TEXT("SetEchoDynamicProperty must succeed"), Fixture.EchoComp->SetEchoDynamicProperty(AcquiredInstance.InstanceId, FName(TEXT("Nickname")), TEXT("Scouty")));
	FString PropValue;
	TestTrue(TEXT("GetEchoDynamicProperty must retrieve value"), Fixture.EchoComp->GetEchoDynamicProperty(AcquiredInstance.InstanceId, FName(TEXT("Nickname")), PropValue));
	TestEqual(TEXT("Dynamic property value must match"), PropValue, TEXT("Scouty"));
	TestTrue(TEXT("RemoveEchoDynamicProperty must succeed"), Fixture.EchoComp->RemoveEchoDynamicProperty(AcquiredInstance.InstanceId, FName(TEXT("Nickname"))));
	TestFalse(TEXT("Dynamic property must be removed"), Fixture.EchoComp->GetEchoDynamicProperty(AcquiredInstance.InstanceId, FName(TEXT("Nickname")), PropValue));

	// 5. Duplicate GUID rejection
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

	// Idempotent summoning (repeated summon does not error or re-transition)
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

	// Insufficient essence: must fail and consume zero essence
	Fixture.Attributes->SetEssence(20.0f);
	TestFalse(TEXT("SummonEcho must fail with insufficient essence"), Fixture.EchoComp->SummonEcho(Echo.InstanceId));
	TestFalse(TEXT("Echo must remain dormant"), Fixture.EchoComp->IsEchoSummoned(Echo.InstanceId));
	TestNearlyEqual(TEXT("Essence must not be consumed on failed summon"), Fixture.Attributes->GetCurrentEssence(), 20.0f, 0.001f);

	// Sufficient essence: must succeed and consume configured essence cost
	Fixture.Attributes->SetEssence(60.0f);
	TestTrue(TEXT("SummonEcho must succeed with sufficient essence"), Fixture.EchoComp->SummonEcho(Echo.InstanceId));
	TestTrue(TEXT("Echo must now be summoned"), Fixture.EchoComp->IsEchoSummoned(Echo.InstanceId));
	TestNearlyEqual(TEXT("Essence must be consumed by summon cost (60 - 35 = 25)"), Fixture.Attributes->GetCurrentEssence(), 25.0f, 0.001f);

	// Repeated summon is idempotent and must NOT double-charge essence
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

	// 1. Destruction: marks Destroyed (durable terminal state) and auto-dismisses
	TestTrue(TEXT("Summon Echo1"), Fixture.EchoComp->SummonEcho(Echo1.InstanceId));
	TestTrue(TEXT("DestroyEcho must succeed"), Fixture.EchoComp->DestroyEcho(Echo1.InstanceId));
	TestFalse(TEXT("Destroyed Echo must not remain summoned"), Fixture.EchoComp->IsEchoSummoned(Echo1.InstanceId));
	EShadowSlaveEchoState State1 = EShadowSlaveEchoState::Dormant;
	TestTrue(TEXT("GetEchoState must succeed on destroyed Echo"), Fixture.EchoComp->GetEchoState(Echo1.InstanceId, State1));
	TestEqual(TEXT("State must be Destroyed"), State1, EShadowSlaveEchoState::Destroyed);
	TestFalse(TEXT("Destroyed Echo can NEVER be summoned"), Fixture.EchoComp->SummonEcho(Echo1.InstanceId));

	// 2. Removal: completely removes Echo from owned collection
	TestTrue(TEXT("Summon Echo2"), Fixture.EchoComp->SummonEcho(Echo2.InstanceId));
	TestTrue(TEXT("RemoveEcho must succeed and auto-dismiss"), Fixture.EchoComp->RemoveEcho(Echo2.InstanceId));
	TestFalse(TEXT("Removed Echo cannot be queried as owned"), Fixture.EchoComp->HasEchoByInstanceId(Echo2.InstanceId));
	FShadowSlaveEchoInstance RemovedFound;
	TestFalse(TEXT("FindEcho must return false for removed Echo"), Fixture.EchoComp->FindEcho(Echo2.InstanceId, RemovedFound));

	// Remove destroyed Echo1 to verify purging
	TestTrue(TEXT("RemoveEcho on destroyed Echo must succeed"), Fixture.EchoComp->RemoveEcho(Echo1.InstanceId));
	TestFalse(TEXT("Purged Echo1 cannot be queried as owned"), Fixture.EchoComp->HasEchoByInstanceId(Echo1.InstanceId));
	TestEqual(TEXT("Echo count must now be 0"), Fixture.EchoComp->GetEchoCount(), 0);

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

	// Acquire Echo1 (active/summoned) and Echo2 (destroyed)
	FShadowSlaveEchoInstance OriginalEcho1;
	FShadowSlaveEchoInstance OriginalEcho2;
	TestTrue(TEXT("Acquire Echo1"), Fixture.EchoComp->AcquireEcho(Fixture.EchoDef, OriginalEcho1));
	TestTrue(TEXT("Acquire Echo2"), Fixture.EchoComp->AcquireEcho(Fixture.EchoDef, OriginalEcho2));
	TestTrue(TEXT("Set dynamic property on Echo1"), Fixture.EchoComp->SetEchoDynamicProperty(OriginalEcho1.InstanceId, FName(TEXT("TestKey")), TEXT("TestVal")));
	TestTrue(TEXT("Summon Echo1"), Fixture.EchoComp->SummonEcho(OriginalEcho1.InstanceId));
	TestTrue(TEXT("Destroy Echo2"), Fixture.EchoComp->DestroyEcho(OriginalEcho2.InstanceId));

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
	TestEqual(TEXT("SaveData must contain 2 echoes"), SaveData.Echoes.Num(), 2);

	// Verify Echo1 save representation: transient summon state is NOT persisted as world truth
	const FShadowSlaveEchoSaveData* SavedEcho1 = SaveData.Echoes.FindByPredicate([&](const FShadowSlaveEchoSaveData& E) { return E.InstanceId == OriginalEcho1.InstanceId; });
	TestNotNull(TEXT("SavedEcho1 must exist"), SavedEcho1);
	if (SavedEcho1)
	{
		TestEqual(TEXT("Saved GUID must match"), SavedEcho1->InstanceId, OriginalEcho1.InstanceId);
		TestEqual(TEXT("Saved EchoId must match"), SavedEcho1->EchoId, Fixture.EchoDef->EchoId);
		TestEqual(TEXT("Saved State must be Dormant (summoned state is transient and never saved)"), SavedEcho1->State, EShadowSlaveEchoState::Dormant);
		TestTrue(TEXT("Saved dynamic properties must contain TestKey"), SavedEcho1->DynamicProperties.Contains(FName(TEXT("TestKey"))));
		TestEqual(TEXT("Saved dynamic property value must match"), SavedEcho1->DynamicProperties[FName(TEXT("TestKey"))], TEXT("TestVal"));
	}

	// Verify Echo2 save representation: durable Destroyed state IS preserved
	const FShadowSlaveEchoSaveData* SavedEcho2 = SaveData.Echoes.FindByPredicate([&](const FShadowSlaveEchoSaveData& E) { return E.InstanceId == OriginalEcho2.InstanceId; });
	TestNotNull(TEXT("SavedEcho2 must exist"), SavedEcho2);
	if (SavedEcho2)
	{
		TestEqual(TEXT("Saved Echo2 state must be Destroyed"), SavedEcho2->State, EShadowSlaveEchoState::Destroyed);
	}

	// Clear component
	Fixture.EchoComp->ClearEchoes();
	TestEqual(TEXT("Echo count must be 0 after clear"), Fixture.EchoComp->GetEchoCount(), 0);

	// Restore snapshot
	SaveSubsystem->RestoreEchoes(Fixture.EchoComp, SaveData);
	TestEqual(TEXT("Echo count must be 2 after restore"), Fixture.EchoComp->GetEchoCount(), 2);

	// Invariant: Save/load does NOT restore a fake active world summon
	FShadowSlaveEchoInstance RestoredEcho1;
	TestTrue(TEXT("Restored Echo1 must be found"), Fixture.EchoComp->FindEcho(OriginalEcho1.InstanceId, RestoredEcho1));
	TestEqual(TEXT("Restored GUID must match original"), RestoredEcho1.InstanceId, OriginalEcho1.InstanceId);
	TestFalse(TEXT("Restored Echo1 must NOT be summoned (never restore fake world summon)"), RestoredEcho1.bIsSummoned);
	TestEqual(TEXT("Restored Echo1 state must be Dormant"), RestoredEcho1.State, EShadowSlaveEchoState::Dormant);
	FString RestoredProp;
	TestTrue(TEXT("Restored dynamic property must survive save/load"), Fixture.EchoComp->GetEchoDynamicProperty(RestoredEcho1.InstanceId, FName(TEXT("TestKey")), RestoredProp));
	TestEqual(TEXT("Restored dynamic property value must match"), RestoredProp, TEXT("TestVal"));

	// Invariant: Restored destroyed Echo remains in Destroyed state and cannot be summoned
	FShadowSlaveEchoInstance RestoredEcho2;
	TestTrue(TEXT("Restored Echo2 must be found"), Fixture.EchoComp->FindEcho(OriginalEcho2.InstanceId, RestoredEcho2));
	TestEqual(TEXT("Restored Echo2 state must remain Destroyed"), RestoredEcho2.State, EShadowSlaveEchoState::Destroyed);
	TestFalse(TEXT("Restored destroyed Echo CANNOT be summoned"), Fixture.EchoComp->SummonEcho(OriginalEcho2.InstanceId));

	return true;
}

#endif // WITH_AUTOMATION_TESTS
