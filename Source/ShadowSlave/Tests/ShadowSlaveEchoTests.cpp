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
#include "Content/ShadowSlaveContentTypes.h"
#include "Content/ShadowSlaveContentDefinition.h"
#include "Content/ShadowSlaveContentRegistrySubsystem.h"

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

			EchoDef->SetEchoId(FName(TEXT("Test_Echo_Scout")));
			EchoDef->DisplayName = FText::FromString(TEXT("Test Scout"));
			EchoDef->Rank = Rank;
			EchoDef->Class = Class;
			EchoDef->SummonEssenceCost = SummonCost;

			return true;
		}
	};
}

// 1. Acquisition and Queries Test
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

// 2. Summon & Dismiss Lifecycle Baseline Test
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

	// Duplicate summon rejected: the same Echo instance cannot have two simultaneous summoned representations
	TestFalse(TEXT("Duplicate SummonEcho must be rejected"), Fixture.EchoComp->SummonEcho(Echo.InstanceId));
	TestTrue(TEXT("Echo must remain summoned"), Fixture.EchoComp->IsEchoSummoned(Echo.InstanceId));

	// Dismissal
	TestTrue(TEXT("DismissEcho must succeed"), Fixture.EchoComp->DismissEcho(Echo.InstanceId));
	TestFalse(TEXT("IsEchoSummoned must return false after dismissal"), Fixture.EchoComp->IsEchoSummoned(Echo.InstanceId));
	TestTrue(TEXT("GetEchoState must succeed"), Fixture.EchoComp->GetEchoState(Echo.InstanceId, State));
	TestEqual(TEXT("Echo state must be Dormant"), State, EShadowSlaveEchoState::Dormant);
	TestEqual(TEXT("GetSummonedEchoes must be empty"), Fixture.EchoComp->GetSummonedEchoes().Num(), 0);

	// Idempotent/repeated dismissal is harmless
	TestTrue(TEXT("Repeated DismissEcho must return true"), Fixture.EchoComp->DismissEcho(Echo.InstanceId));
	TestFalse(TEXT("Echo must remain dormant"), Fixture.EchoComp->IsEchoSummoned(Echo.InstanceId));

	// DismissAllEchoes
	TestTrue(TEXT("SummonEcho again"), Fixture.EchoComp->SummonEcho(Echo.InstanceId));
	TestTrue(TEXT("DismissAllEchoes must succeed"), Fixture.EchoComp->DismissAllEchoes());
	TestFalse(TEXT("Echo must be dormant after DismissAllEchoes"), Fixture.EchoComp->IsEchoSummoned(Echo.InstanceId));

	return true;
}

// 3. Essence Cost Validation Test
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

	// Repeated duplicate summon is rejected and must NOT double-charge essence
	TestFalse(TEXT("Repeated duplicate summon must be rejected"), Fixture.EchoComp->SummonEcho(Echo.InstanceId));
	TestNearlyEqual(TEXT("Repeated summon must not consume essence again"), Fixture.Attributes->GetCurrentEssence(), 25.0f, 0.001f);

	return true;
}

// 4. Destruction vs Removal Test
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

// 5. Save/Load Roundtrip Test
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
		TestEqual(TEXT("Saved EchoId must match"), SavedEcho1->EchoId, Fixture.EchoDef->GetEchoId());
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

// 6. Required Test: Echo.OwnershipPersistsAcrossDismiss
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveEchoOwnershipPersistsAcrossDismissTest,
	"ShadowSlave.Echoes.OwnershipPersistsAcrossDismiss",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveEchoOwnershipPersistsAcrossDismissTest::RunTest(const FString& Parameters)
{
	FShadowSlaveEchoTestFixture Fixture;
	TestTrue(TEXT("Fixture must initialize"), Fixture.Initialize());

	FShadowSlaveEchoInstance Echo;
	TestTrue(TEXT("AcquireEcho (Owned/Dormant)"), Fixture.EchoComp->AcquireEcho(Fixture.EchoDef, Echo));
	TestEqual(TEXT("Echo count must be 1"), Fixture.EchoComp->GetEchoCount(), 1);
	TestEqual(TEXT("State must start Dormant"), Echo.State, EShadowSlaveEchoState::Dormant);
	TestFalse(TEXT("bIsSummoned must start false"), Echo.bIsSummoned);

	// Summon -> Summoned/Active
	TestTrue(TEXT("SummonEcho must succeed"), Fixture.EchoComp->SummonEcho(Echo.InstanceId));
	TestTrue(TEXT("IsEchoSummoned must be true"), Fixture.EchoComp->IsEchoSummoned(Echo.InstanceId));
	TestTrue(TEXT("HasEchoByInstanceId must be true while summoned"), Fixture.EchoComp->HasEchoByInstanceId(Echo.InstanceId));

	// Dismiss -> Owned/Dormant
	TestTrue(TEXT("DismissEcho must succeed"), Fixture.EchoComp->DismissEcho(Echo.InstanceId));
	TestFalse(TEXT("IsEchoSummoned must be false after dismiss"), Fixture.EchoComp->IsEchoSummoned(Echo.InstanceId));
	EShadowSlaveEchoState State = EShadowSlaveEchoState::Summoned;
	TestTrue(TEXT("GetEchoState must succeed"), Fixture.EchoComp->GetEchoState(Echo.InstanceId, State));
	TestEqual(TEXT("State must return to Dormant"), State, EShadowSlaveEchoState::Dormant);

	// Invariant: Echo is still owned and exists
	TestEqual(TEXT("Echo count must still be 1 (ownership persists)"), Fixture.EchoComp->GetEchoCount(), 1);
	TestTrue(TEXT("HasEcho must still return true"), Fixture.EchoComp->HasEcho(Fixture.EchoDef));
	TestTrue(TEXT("HasEchoByInstanceId must still return true"), Fixture.EchoComp->HasEchoByInstanceId(Echo.InstanceId));

	// Persistent Echo remains available for a future summon
	TestTrue(TEXT("Echo can be summoned again"), Fixture.EchoComp->SummonEcho(Echo.InstanceId));
	TestTrue(TEXT("Echo is summoned again"), Fixture.EchoComp->IsEchoSummoned(Echo.InstanceId));

	return true;
}

// 7. Required Test: Echo.DuplicateSummonRejected
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveEchoDuplicateSummonRejectedTest,
	"ShadowSlave.Echoes.DuplicateSummonRejected",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveEchoDuplicateSummonRejectedTest::RunTest(const FString& Parameters)
{
	FShadowSlaveEchoTestFixture Fixture;
	TestTrue(TEXT("Fixture must initialize"), Fixture.Initialize());

	FShadowSlaveEchoInstance Echo;
	TestTrue(TEXT("AcquireEcho"), Fixture.EchoComp->AcquireEcho(Fixture.EchoDef, Echo));

	// First summon succeeds
	TestTrue(TEXT("First SummonEcho must succeed"), Fixture.EchoComp->SummonEcho(Echo.InstanceId));
	TestTrue(TEXT("Echo must be summoned"), Fixture.EchoComp->IsEchoSummoned(Echo.InstanceId));

	// Second summon must be rejected: cannot have two simultaneous representations
	TestFalse(TEXT("Duplicate SummonEcho must be rejected"), Fixture.EchoComp->SummonEcho(Echo.InstanceId));
	TestTrue(TEXT("Echo must remain summoned"), Fixture.EchoComp->IsEchoSummoned(Echo.InstanceId));
	TestEqual(TEXT("Echo count must remain 1"), Fixture.EchoComp->GetEchoCount(), 1);

	return true;
}

// 8. Required Test: Echo.DismissWithoutActorSafe
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveEchoDismissWithoutActorSafeTest,
	"ShadowSlave.Echoes.DismissWithoutActorSafe",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveEchoDismissWithoutActorSafeTest::RunTest(const FString& Parameters)
{
	FShadowSlaveEchoTestFixture Fixture;
	TestTrue(TEXT("Fixture must initialize"), Fixture.Initialize());

	FShadowSlaveEchoInstance Echo;
	TestTrue(TEXT("AcquireEcho"), Fixture.EchoComp->AcquireEcho(Fixture.EchoDef, Echo));

	// Summoned without transient actor (pure logical summon)
	TestTrue(TEXT("SummonEcho without actor succeeds"), Fixture.EchoComp->SummonEcho(Echo.InstanceId, nullptr));
	TestNull(TEXT("GetSummonedActor must return null"), Fixture.EchoComp->GetSummonedActor(Echo.InstanceId));
	TestTrue(TEXT("Echo is summoned"), Fixture.EchoComp->IsEchoSummoned(Echo.InstanceId));

	// Dismissal without actor must be safe and not cause ownership loss
	TestTrue(TEXT("DismissEcho must succeed even when transient actor is missing"), Fixture.EchoComp->DismissEcho(Echo.InstanceId));
	TestFalse(TEXT("Echo must be dormant after dismissal"), Fixture.EchoComp->IsEchoSummoned(Echo.InstanceId));
	TestEqual(TEXT("Echo count must remain 1 (ownership not lost)"), Fixture.EchoComp->GetEchoCount(), 1);
	TestTrue(TEXT("Echo remains owned by instance ID"), Fixture.EchoComp->HasEchoByInstanceId(Echo.InstanceId));

	// Repeated dismissal is harmless
	TestTrue(TEXT("Repeated DismissEcho must be harmless and return true"), Fixture.EchoComp->DismissEcho(Echo.InstanceId));
	TestEqual(TEXT("Echo count must still remain 1"), Fixture.EchoComp->GetEchoCount(), 1);

	return true;
}

// 9. Required Test: Echo.DestroyedActorDoesNotDestroyOwnership
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveEchoDestroyedActorDoesNotDestroyOwnershipTest,
	"ShadowSlave.Echoes.DestroyedActorDoesNotDestroyOwnership",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveEchoDestroyedActorDoesNotDestroyOwnershipTest::RunTest(const FString& Parameters)
{
	FShadowSlaveEchoTestFixture Fixture;
	TestTrue(TEXT("Fixture must initialize"), Fixture.Initialize());

	FShadowSlaveEchoInstance Echo;
	TestTrue(TEXT("AcquireEcho"), Fixture.EchoComp->AcquireEcho(Fixture.EchoDef, Echo));

	// Create an actor representation using NewObject (safe in headless test fixture)
	AActor* SpawnedPawn = NewObject<AShadowSlavePlayerCharacter>(Fixture.Player);
	TestNotNull(TEXT("Spawned representation must be valid"), SpawnedPawn);

	TestTrue(TEXT("SummonEcho with transient actor"), Fixture.EchoComp->SummonEcho(Echo.InstanceId, SpawnedPawn));
	TestTrue(TEXT("Echo must be summoned"), Fixture.EchoComp->IsEchoSummoned(Echo.InstanceId));
	TestEqual(TEXT("GetSummonedActor must return spawned pawn"), Fixture.EchoComp->GetSummonedActor(Echo.InstanceId), SpawnedPawn);

	// External destruction of the transient representation: invoke HandleSummonedActorDestroyed
	Fixture.EchoComp->HandleSummonedActorDestroyed(SpawnedPawn);

	// The Echo must transition to Dormant, clear the actor pointer, but REMAIN OWNED
	TestFalse(TEXT("Echo must no longer be summoned after actor destruction"), Fixture.EchoComp->IsEchoSummoned(Echo.InstanceId));
	EShadowSlaveEchoState State = EShadowSlaveEchoState::Summoned;
	TestTrue(TEXT("GetEchoState must succeed"), Fixture.EchoComp->GetEchoState(Echo.InstanceId, State));
	TestEqual(TEXT("State must transition safely to Dormant"), State, EShadowSlaveEchoState::Dormant);
	TestNull(TEXT("Transient actor pointer must be cleared (no stale pointer)"), Fixture.EchoComp->GetSummonedActor(Echo.InstanceId));

	// Invariant: Persistent Echo is NOT destroyed and remains owned
	TestEqual(TEXT("Echo count must remain 1"), Fixture.EchoComp->GetEchoCount(), 1);
	TestTrue(TEXT("Echo remains owned by instance ID"), Fixture.EchoComp->HasEchoByInstanceId(Echo.InstanceId));

	// Echo is recoverable and can be summoned again
	TestTrue(TEXT("Echo can be summoned again after actor destruction"), Fixture.EchoComp->SummonEcho(Echo.InstanceId));
	TestTrue(TEXT("Echo is summoned"), Fixture.EchoComp->IsEchoSummoned(Echo.InstanceId));

	return true;
}

// 10. Required Test: Echo.SaveLoadDoesNotRestoreTransientSummon
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveEchoSaveLoadDoesNotRestoreTransientSummonTest,
	"ShadowSlave.Echoes.SaveLoadDoesNotRestoreTransientSummon",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveEchoSaveLoadDoesNotRestoreTransientSummonTest::RunTest(const FString& Parameters)
{
	FShadowSlaveEchoTestFixture Fixture;
	TestTrue(TEXT("Fixture must initialize"), Fixture.Initialize());

	FShadowSlaveEchoInstance Echo;
	TestTrue(TEXT("AcquireEcho"), Fixture.EchoComp->AcquireEcho(Fixture.EchoDef, Echo));

	// Summon Echo
	TestTrue(TEXT("SummonEcho"), Fixture.EchoComp->SummonEcho(Echo.InstanceId));
	TestTrue(TEXT("Echo is summoned"), Fixture.EchoComp->IsEchoSummoned(Echo.InstanceId));

	// Capture save data
	UShadowSlaveSaveSubsystem* SaveSubsystem = NewObject<UShadowSlaveSaveSubsystem>();
	FShadowSlaveEchoCollectionSaveData SaveData;
	SaveSubsystem->CaptureEchoes(Fixture.EchoComp, SaveData);
	TestTrue(TEXT("SaveData must be valid"), SaveData.bIsValid);
	TestEqual(TEXT("One echo captured"), SaveData.Echoes.Num(), 1);

	// Verify save data state is normalized to Dormant (transient summon is NEVER persisted)
	TestEqual(TEXT("Saved state must be Dormant"), SaveData.Echoes[0].State, EShadowSlaveEchoState::Dormant);

	// Restore into a fresh component
	UShadowSlaveEchoComponent* RestoredComp = NewObject<UShadowSlaveEchoComponent>(Fixture.Player);
	SaveSubsystem->RestoreEchoes(RestoredComp, SaveData);

	// Restored Echo must be Dormant, not summoned, and have no transient actor
	TestEqual(TEXT("Restored echo count must be 1"), RestoredComp->GetEchoCount(), 1);
	TestFalse(TEXT("Restored Echo must NOT be summoned (no fake world summon)"), RestoredComp->IsEchoSummoned(Echo.InstanceId));
	EShadowSlaveEchoState RestoredState = EShadowSlaveEchoState::Summoned;
	TestTrue(TEXT("GetEchoState on restored echo"), RestoredComp->GetEchoState(Echo.InstanceId, RestoredState));
	TestEqual(TEXT("Restored state must be Dormant"), RestoredState, EShadowSlaveEchoState::Dormant);
	TestNull(TEXT("Restored transient actor must be null"), RestoredComp->GetSummonedActor(Echo.InstanceId));

	return true;
}

// 11. Required Test: Echo.SaveLoadPreservesPersistentData
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveEchoSaveLoadPreservesPersistentDataTest,
	"ShadowSlave.Echoes.SaveLoadPreservesPersistentData",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveEchoSaveLoadPreservesPersistentDataTest::RunTest(const FString& Parameters)
{
	FShadowSlaveEchoTestFixture Fixture;
	TestTrue(TEXT("Fixture must initialize"), Fixture.Initialize());

	FShadowSlaveEchoInstance Echo;
	TestTrue(TEXT("AcquireEcho"), Fixture.EchoComp->AcquireEcho(Fixture.EchoDef, Echo));
	TestTrue(TEXT("Set DynamicProperty"), Fixture.EchoComp->SetEchoDynamicProperty(Echo.InstanceId, FName(TEXT("CustomName")), TEXT("ShadowHound")));

	UShadowSlaveSaveSubsystem* SaveSubsystem = NewObject<UShadowSlaveSaveSubsystem>();
	FShadowSlaveEchoCollectionSaveData SaveData;
	SaveSubsystem->CaptureEchoes(Fixture.EchoComp, SaveData);

	UShadowSlaveEchoComponent* RestoredComp = NewObject<UShadowSlaveEchoComponent>(Fixture.Player);
	SaveSubsystem->RestoreEchoes(RestoredComp, SaveData);

	// Verify Instance GUID survives
	TestTrue(TEXT("Restored component has echo by original GUID"), RestoredComp->HasEchoByInstanceId(Echo.InstanceId));

	// Verify Definition identity survives
	FShadowSlaveEchoInstance RestoredInstance;
	TestTrue(TEXT("FindEcho on restored component"), RestoredComp->FindEcho(Echo.InstanceId, RestoredInstance));
	TestNotNull(TEXT("Definition must be resolved"), RestoredInstance.EchoDefinition.Get());
	if (RestoredInstance.EchoDefinition)
	{
		TestEqual(TEXT("Definition EchoId must match"), RestoredInstance.EchoDefinition->GetEchoId(), Fixture.EchoDef->GetEchoId());
	}

	// Verify Dynamic properties survive
	FString RestoredVal;
	TestTrue(TEXT("GetDynamicProperty on restored echo"), RestoredComp->GetEchoDynamicProperty(Echo.InstanceId, FName(TEXT("CustomName")), RestoredVal));
	TestEqual(TEXT("DynamicProperty value must match"), RestoredVal, TEXT("ShadowHound"));

	return true;
}

// 12. Required Test: Echo.ReentrancyDuringSummonRejected
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveEchoReentrancyDuringSummonRejectedTest,
	"ShadowSlave.Echoes.ReentrancyDuringSummonRejected",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveEchoReentrancyDuringSummonRejectedTest::RunTest(const FString& Parameters)
{
	FShadowSlaveEchoTestFixture Fixture;
	TestTrue(TEXT("Fixture must initialize"), Fixture.Initialize());

	FShadowSlaveEchoInstance Echo1;
	FShadowSlaveEchoInstance Echo2;
	TestTrue(TEXT("Acquire Echo1"), Fixture.EchoComp->AcquireEcho(Fixture.EchoDef, Echo1));
	TestTrue(TEXT("Acquire Echo2"), Fixture.EchoComp->AcquireEcho(Fixture.EchoDef, Echo2));

	bool bCallbackFired = false;
	bool bReentrantSummonResult = true;
	bool bReentrantRemoveResult = true;

	// Case A & C: OnEchoSummoned callback attempts another summon and a removal
	FDelegateHandle Handle = Fixture.EchoComp->OnEchoSummoned.AddLambda(
		[&](const FShadowSlaveEchoInstance& SummonedEcho)
		{
			bCallbackFired = true;
			// Case A: Attempt another summon while transition is active -> must be rejected
			bReentrantSummonResult = Fixture.EchoComp->SummonEcho(Echo2.InstanceId);
			// Case C: Attempt removal while transition is active -> must be rejected
			bReentrantRemoveResult = Fixture.EchoComp->RemoveEcho(Echo2.InstanceId);
		}
	);

	const bool Summon1Result = Fixture.EchoComp->SummonEcho(Echo1.InstanceId);
	TestTrue(TEXT("SummonEcho on Echo1 must succeed"), Summon1Result);
	TestTrue(TEXT("OnEchoSummoned callback must have fired"), bCallbackFired);
	TestFalse(TEXT("Reentrant SummonEcho during transition must be rejected"), bReentrantSummonResult);
	TestFalse(TEXT("Reentrant RemoveEcho during transition must be rejected"), bReentrantRemoveResult);

	// Echo2 must NOT be summoned and must still exist in collection
	TestFalse(TEXT("Echo2 must not be summoned"), Fixture.EchoComp->IsEchoSummoned(Echo2.InstanceId));
	TestEqual(TEXT("Echo count must remain 2"), Fixture.EchoComp->GetEchoCount(), 2);

	Fixture.EchoComp->OnEchoSummoned.Remove(Handle);

	// After transition finishes, normal operations succeed
	TestTrue(TEXT("Post-transition SummonEcho on Echo2 must succeed"), Fixture.EchoComp->SummonEcho(Echo2.InstanceId));
	TestTrue(TEXT("Echo2 is summoned"), Fixture.EchoComp->IsEchoSummoned(Echo2.InstanceId));

	return true;
}

// 13. Required Test: Echo.ReentrancyDuringDismissRejected
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveEchoReentrancyDuringDismissRejectedTest,
	"ShadowSlave.Echoes.ReentrancyDuringDismissRejected",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveEchoReentrancyDuringDismissRejectedTest::RunTest(const FString& Parameters)
{
	FShadowSlaveEchoTestFixture Fixture;
	TestTrue(TEXT("Fixture must initialize"), Fixture.Initialize());

	FShadowSlaveEchoInstance Echo1;
	FShadowSlaveEchoInstance Echo2;
	TestTrue(TEXT("Acquire Echo1"), Fixture.EchoComp->AcquireEcho(Fixture.EchoDef, Echo1));
	TestTrue(TEXT("Acquire Echo2"), Fixture.EchoComp->AcquireEcho(Fixture.EchoDef, Echo2));
	TestTrue(TEXT("Summon Echo1"), Fixture.EchoComp->SummonEcho(Echo1.InstanceId));
	TestTrue(TEXT("Summon Echo2"), Fixture.EchoComp->SummonEcho(Echo2.InstanceId));

	bool bCallbackFired = false;
	bool bReentrantDismissResult = true;
	bool bReentrantDestroyResult = true;

	// Case B & C: OnEchoDismissed callback attempts another dismiss and destruction
	FDelegateHandle Handle = Fixture.EchoComp->OnEchoDismissed.AddLambda(
		[&](const FShadowSlaveEchoInstance& DismissedEcho)
		{
			bCallbackFired = true;
			// Case B: Attempt another dismiss while transition is active -> must be rejected
			bReentrantDismissResult = Fixture.EchoComp->DismissEcho(Echo2.InstanceId);
			// Case C: Attempt destruction while transition is active -> must be rejected
			bReentrantDestroyResult = Fixture.EchoComp->DestroyEcho(Echo2.InstanceId);
		}
	);

	const bool Dismiss1Result = Fixture.EchoComp->DismissEcho(Echo1.InstanceId);
	TestTrue(TEXT("DismissEcho on Echo1 must succeed"), Dismiss1Result);
	TestTrue(TEXT("OnEchoDismissed callback must have fired"), bCallbackFired);
	TestFalse(TEXT("Reentrant DismissEcho during transition must be rejected"), bReentrantDismissResult);
	TestFalse(TEXT("Reentrant DestroyEcho during transition must be rejected"), bReentrantDestroyResult);

	// Echo2 must still be summoned and not destroyed
	TestTrue(TEXT("Echo2 must still be summoned"), Fixture.EchoComp->IsEchoSummoned(Echo2.InstanceId));
	EShadowSlaveEchoState State2 = EShadowSlaveEchoState::Dormant;
	TestTrue(TEXT("GetEchoState on Echo2"), Fixture.EchoComp->GetEchoState(Echo2.InstanceId, State2));
	TestEqual(TEXT("Echo2 state must still be Summoned"), State2, EShadowSlaveEchoState::Summoned);

	Fixture.EchoComp->OnEchoDismissed.Remove(Handle);

	// After transition finishes, normal operations succeed
	TestTrue(TEXT("Post-transition DismissEcho on Echo2 must succeed"), Fixture.EchoComp->DismissEcho(Echo2.InstanceId));
	TestFalse(TEXT("Echo2 is now dormant"), Fixture.EchoComp->IsEchoSummoned(Echo2.InstanceId));

	return true;
}

// 14. Required Test: Echo.TeardownDoesNotTriggerGameplayDismiss
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveEchoTeardownDoesNotTriggerGameplayDismissTest,
	"ShadowSlave.Echoes.TeardownDoesNotTriggerGameplayDismiss",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveEchoTeardownDoesNotTriggerGameplayDismissTest::RunTest(const FString& Parameters)
{
	FShadowSlaveEchoTestFixture Fixture;
	TestTrue(TEXT("Fixture must initialize"), Fixture.Initialize());

	FShadowSlaveEchoInstance Echo;
	TestTrue(TEXT("Acquire Echo"), Fixture.EchoComp->AcquireEcho(Fixture.EchoDef, Echo));

	AActor* SpawnedPawn = NewObject<AShadowSlavePlayerCharacter>(Fixture.Player);
	TestNotNull(TEXT("Spawned representation must be valid"), SpawnedPawn);

	TestTrue(TEXT("SummonEcho with transient actor"), Fixture.EchoComp->SummonEcho(Echo.InstanceId, SpawnedPawn));
	TestTrue(TEXT("Echo must be summoned"), Fixture.EchoComp->IsEchoSummoned(Echo.InstanceId));

	// Track whether any gameplay events fire during teardown
	bool bDismissFired = false;
	bool bStateChangedFired = false;
	bool bCollectionChangedFired = false;
	bool bRemovedFired = false;

	Fixture.EchoComp->OnEchoDismissed.AddLambda([&](const FShadowSlaveEchoInstance&) { bDismissFired = true; });
	Fixture.EchoComp->OnEchoStateChanged.AddLambda([&](const FShadowSlaveEchoInstance&, EShadowSlaveEchoState, EShadowSlaveEchoState) { bStateChangedFired = true; });
	Fixture.EchoComp->OnEchoCollectionChanged.AddLambda([&]() { bCollectionChangedFired = true; });
	Fixture.EchoComp->OnEchoRemoved.AddLambda([&](const FShadowSlaveEchoInstance&) { bRemovedFired = true; });

	// Execute component EndPlay teardown
	Fixture.EchoComp->EndPlay(EEndPlayReason::Destroyed);

	// Invariant: Teardown must NOT broadcast normal gameplay events
	TestFalse(TEXT("OnEchoDismissed must NOT fire during EndPlay teardown"), bDismissFired);
	TestFalse(TEXT("OnEchoStateChanged must NOT fire during EndPlay teardown"), bStateChangedFired);
	TestFalse(TEXT("OnEchoCollectionChanged must NOT fire during EndPlay teardown"), bCollectionChangedFired);
	TestFalse(TEXT("OnEchoRemoved must NOT fire during EndPlay teardown"), bRemovedFired);

	// Invariant: Transient actor is safely cleared
	TestNull(TEXT("Transient actor pointer must be cleared after teardown"), Fixture.EchoComp->GetSummonedActor(Echo.InstanceId));
	TestFalse(TEXT("Echo must not remain summoned"), Fixture.EchoComp->IsEchoSummoned(Echo.InstanceId));

	// Invariant: Echo ownership is NOT lost during teardown
	TestEqual(TEXT("Echo count must still be 1 (ownership preserved during teardown)"), Fixture.EchoComp->GetEchoCount(), 1);
	TestTrue(TEXT("Echo remains owned by instance ID"), Fixture.EchoComp->HasEchoByInstanceId(Echo.InstanceId));

	return true;
}

// 10. DefinitionUsesGenericContentBase Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveEchoDefinitionUsesGenericContentBaseTest,
	"ShadowSlave.Echo.DefinitionUsesGenericContentBase",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveEchoDefinitionUsesGenericContentBaseTest::RunTest(const FString& Parameters)
{
	UShadowSlaveEchoDefinition* Def = NewObject<UShadowSlaveEchoDefinition>();
	TestNotNull(TEXT("Echo definition must be instantiable"), Def);

	// Verify C++ inheritance and UClass reflection hierarchy
	UShadowSlaveContentDefinition* ContentDef = Cast<UShadowSlaveContentDefinition>(Def);
	TestNotNull(TEXT("Echo definition must cast to UShadowSlaveContentDefinition"), ContentDef);
	TestTrue(TEXT("Echo definition IsA(UShadowSlaveContentDefinition)"), Def->IsA(UShadowSlaveContentDefinition::StaticClass()));
	TestTrue(TEXT("StaticClass hierarchy is child of UShadowSlaveContentDefinition"),
		UShadowSlaveEchoDefinition::StaticClass()->IsChildOf(UShadowSlaveContentDefinition::StaticClass()));

	return true;
}

// 11. DefinitionUsesEchoContentType Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveEchoDefinitionUsesEchoContentTypeTest,
	"ShadowSlave.Echo.DefinitionUsesEchoContentType",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveEchoDefinitionUsesEchoContentTypeTest::RunTest(const FString& Parameters)
{
	UShadowSlaveEchoDefinition* Def = NewObject<UShadowSlaveEchoDefinition>();
	TestNotNull(TEXT("Echo definition must be instantiable"), Def);
	TestEqual(TEXT("Default ContentType must be EShadowSlaveContentType::Echo"), Def->ContentType, EShadowSlaveContentType::Echo);

	return true;
}

// 12. DefinitionHasSingleAuthoritativeId Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveEchoDefinitionHasSingleAuthoritativeIdTest,
	"ShadowSlave.Echo.DefinitionHasSingleAuthoritativeId",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveEchoDefinitionHasSingleAuthoritativeIdTest::RunTest(const FString& Parameters)
{
	UShadowSlaveEchoDefinition* Def = NewObject<UShadowSlaveEchoDefinition>();
	TestNotNull(TEXT("Echo definition must be instantiable"), Def);

	// 1. ContentId is the only stored authoritative ID; initially NAME_None
	TestTrue(TEXT("ContentId initially None"), Def->ContentId.IsNone());
	TestTrue(TEXT("GetEchoId() initially None"), Def->GetEchoId().IsNone());

	// Verify EchoId is not a stored UPROPERTY, while ContentId is
	TestNull(TEXT("EchoId must not be a stored UPROPERTY on UShadowSlaveEchoDefinition"),
		UShadowSlaveEchoDefinition::StaticClass()->FindPropertyByName(TEXT("EchoId")));
	TestNotNull(TEXT("ContentId must be a stored UPROPERTY on UShadowSlaveEchoDefinition"),
		UShadowSlaveEchoDefinition::StaticClass()->FindPropertyByName(TEXT("ContentId")));

	// 2. GetEchoId() returns ContentId
	const FName IdA(TEXT("Echo_Authoritative_A"));
	Def->ContentId = IdA;
	TestEqual(TEXT("GetEchoId() must return ContentId"), Def->GetEchoId(), IdA);

	// 3. SetEchoId() changes ContentId
	const FName IdB(TEXT("Echo_Authoritative_B"));
	Def->SetEchoId(IdB);
	TestEqual(TEXT("ContentId must be updated by SetEchoId()"), Def->ContentId, IdB);
	TestEqual(TEXT("GetEchoId() must reflect SetEchoId() update"), Def->GetEchoId(), IdB);

	// 4. Changing ContentId is reflected by GetEchoId()
	const FName IdC(TEXT("Echo_Authoritative_C"));
	Def->ContentId = IdC;
	TestEqual(TEXT("GetEchoId() must reflect direct ContentId change"), Def->GetEchoId(), IdC);

	return true;
}

// 13. DefinitionValidation Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveEchoDefinitionValidationTest,
	"ShadowSlave.Echo.DefinitionValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveEchoDefinitionValidationTest::RunTest(const FString& Parameters)
{
	// 1. Valid definition passes validation
	UShadowSlaveEchoDefinition* ValidDef = NewObject<UShadowSlaveEchoDefinition>();
	ValidDef->ContentId = FName(TEXT("Test_Valid_Echo"));
	ValidDef->SummonEssenceCost = 10.0f;

	FString ErrorMsg;
	TestTrue(TEXT("Valid echo passes IsValidDefinition"), ValidDef->IsValidDefinition(&ErrorMsg));
	TestTrue(TEXT("Valid echo passes ValidateDefinition"), ValidDef->ValidateDefinition(ErrorMsg));

	// 2. NAME_None ContentId fails validation
	ValidDef->ContentId = NAME_None;
	TestFalse(TEXT("NAME_None ContentId must fail IsValidDefinition"), ValidDef->IsValidDefinition(&ErrorMsg));
	TestFalse(TEXT("Error message must be populated for None ContentId"), ErrorMsg.IsEmpty());

	// 3. Version < 1 fails validation
	ValidDef->ContentId = FName(TEXT("Test_Valid_Echo"));
	ValidDef->Version = 0;
	TestFalse(TEXT("Version 0 must fail IsValidDefinition"), ValidDef->IsValidDefinition(&ErrorMsg));

	// 4. Incorrect ContentType fails validation
	ValidDef->Version = 1;
	ValidDef->ContentType = EShadowSlaveContentType::Memory;
	TestFalse(TEXT("ContentType != Echo must fail IsValidDefinition"), ValidDef->IsValidDefinition(&ErrorMsg));

	// 5. Negative SummonEssenceCost fails validation
	ValidDef->ContentType = EShadowSlaveContentType::Echo;
	ValidDef->SummonEssenceCost = -5.0f;
	TestFalse(TEXT("Negative SummonEssenceCost must fail IsValidDefinition"), ValidDef->IsValidDefinition(&ErrorMsg));

	// Restored definition passes again
	ValidDef->SummonEssenceCost = 0.0f;
	TestTrue(TEXT("Restored definition passes validation"), ValidDef->IsValidDefinition());

	return true;
}

// 14. DefinitionRegistryIntegration Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveEchoDefinitionRegistryIntegrationTest,
	"ShadowSlave.Echo.DefinitionRegistryIntegration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveEchoDefinitionRegistryIntegrationTest::RunTest(const FString& Parameters)
{
	UShadowSlaveContentRegistrySubsystem* Registry = NewObject<UShadowSlaveContentRegistrySubsystem>();
	TestNotNull(TEXT("Registry subsystem must be created"), Registry);

	UShadowSlaveEchoDefinition* EchoDef = NewObject<UShadowSlaveEchoDefinition>();
	EchoDef->ContentId = FName(TEXT("Echo_Test_Scout"));
	EchoDef->DisplayName = FText::FromString(TEXT("Test Scout Echo"));
	EchoDef->Rank = EShadowSlaveEchoRank::Awakened;
	EchoDef->Class = EShadowSlaveEchoClass::Monster;
	EchoDef->SummonEssenceCost = 15.0f;

	// Register with generic registry subsystem
	const bool bRegistered = Registry->RegisterDefinition(EchoDef);
	TestTrue(TEXT("RegisterDefinition must succeed for UShadowSlaveEchoDefinition"), bRegistered);

	// Query existence
	TestTrue(TEXT("HasContent must return true for registered Echo"), Registry->HasContent(EchoDef->ContentId));
	TestEqual(TEXT("Total registered count must be 1"), Registry->GetRegisteredContentCount(), 1);
	TestEqual(TEXT("Echo count by type must be 1"), Registry->GetRegisteredContentCountByType(EShadowSlaveContentType::Echo), 1);
	TestEqual(TEXT("Memory count by type must be 0"), Registry->GetRegisteredContentCountByType(EShadowSlaveContentType::Memory), 0);

	// Generic resolution
	UShadowSlaveContentDefinition* ResolvedGeneric = Registry->ResolveContentDefinition(EchoDef->ContentId);
	TestNotNull(TEXT("Resolved generic definition must not be null"), ResolvedGeneric);
	TestEqual(TEXT("Resolved generic definition must match EchoDef"), ResolvedGeneric, Cast<UShadowSlaveContentDefinition>(EchoDef));

	// Typed resolution
	UShadowSlaveEchoDefinition* ResolvedEcho = Registry->ResolveContentDefinition<UShadowSlaveEchoDefinition>(EchoDef->ContentId);
	TestNotNull(TEXT("Resolved typed echo definition must not be null"), ResolvedEcho);
	TestEqual(TEXT("Resolved typed echo must match original EchoDef"), ResolvedEcho, EchoDef);
	TestEqual(TEXT("Resolved Rank matches"), ResolvedEcho->Rank, EchoDef->Rank);
	TestEqual(TEXT("Resolved Class matches"), ResolvedEcho->Class, EchoDef->Class);
	TestEqual(TEXT("Resolved SummonEssenceCost matches"), ResolvedEcho->SummonEssenceCost, EchoDef->SummonEssenceCost);

	// PrimaryAssetId verification
	const FPrimaryAssetId ExpectedAssetId(TEXT("Echo"), EchoDef->ContentId);
	TestEqual(TEXT("GetPrimaryAssetId must match expected PrimaryAssetId"), EchoDef->GetPrimaryAssetId(), ExpectedAssetId);

	return true;
}

// 15. ExistingRuntimeCompatibility Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveEchoExistingRuntimeCompatibilityTest,
	"ShadowSlave.Echo.ExistingRuntimeCompatibility",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveEchoExistingRuntimeCompatibilityTest::RunTest(const FString& Parameters)
{
	AShadowSlavePlayerCharacter* Player = NewObject<AShadowSlavePlayerCharacter>();
	TestNotNull(TEXT("Player must be instantiable"), Player);

	UShadowSlaveEchoComponent* EchoComp = Player->GetEchoComponent();
	UShadowSlaveAttributeComponent* AttrComp = Player->GetAttributeComponent();
	TestNotNull(TEXT("EchoComp must exist on Player"), EchoComp);
	TestNotNull(TEXT("AttrComp must exist on Player"), AttrComp);

	AttrComp->SetCurrentEssence(100.0f);

	UShadowSlaveEchoDefinition* EchoDef = NewObject<UShadowSlaveEchoDefinition>(Player);
	EchoDef->ContentId = FName(TEXT("Compat_Echo"));
	EchoDef->Rank = EShadowSlaveEchoRank::Awakened;
	EchoDef->Class = EShadowSlaveEchoClass::Monster;
	EchoDef->SummonEssenceCost = 10.0f;

	// 1. Acquisition
	FShadowSlaveEchoInstance Acquired;
	TestTrue(TEXT("AcquireEcho succeeds"), EchoComp->AcquireEcho(EchoDef, Acquired));
	TestTrue(TEXT("Acquired is valid"), Acquired.IsValid());
	TestEqual(TEXT("Acquired definition matches"), Acquired.EchoDefinition.Get(), EchoDef);
	TestEqual(TEXT("Acquired Rank matches"), Acquired.GetRank(), EShadowSlaveEchoRank::Awakened);
	TestEqual(TEXT("Acquired Class matches"), Acquired.GetClass(), EShadowSlaveEchoClass::Monster);
	TestTrue(TEXT("EchoComp has echo by instance ID"), EchoComp->HasEchoByInstanceId(Acquired.InstanceId));
	TestEqual(TEXT("EchoComp echo count is 1"), EchoComp->GetEchoCount(), 1);

	// 2. Summon and Dismiss
	TestTrue(TEXT("SummonEcho succeeds"), EchoComp->SummonEcho(Acquired.InstanceId));
	TestTrue(TEXT("IsEchoSummoned is true"), EchoComp->IsEchoSummoned(Acquired.InstanceId));
	TestTrue(TEXT("DismissEcho succeeds"), EchoComp->DismissEcho(Acquired.InstanceId));
	TestFalse(TEXT("IsEchoSummoned is false after dismiss"), EchoComp->IsEchoSummoned(Acquired.InstanceId));

	// 3. Destruction and Removal
	TestTrue(TEXT("DestroyEcho succeeds"), EchoComp->DestroyEcho(Acquired.InstanceId));
	FShadowSlaveEchoInstance DestroyedInst;
	TestTrue(TEXT("FindEcho finds destroyed echo"), EchoComp->FindEcho(Acquired.InstanceId, DestroyedInst));
	TestEqual(TEXT("Echo state is Destroyed"), DestroyedInst.State, EShadowSlaveEchoState::Destroyed);

	// 4. Save Normalization
	UShadowSlaveSaveSubsystem* SaveSubsystem = NewObject<UShadowSlaveSaveSubsystem>();
	TestNotNull(TEXT("SaveSubsystem must be created"), SaveSubsystem);
	FShadowSlaveEchoCollectionSaveData SaveData;
	SaveSubsystem->CaptureEchoes(EchoComp, SaveData);
	TestTrue(TEXT("CaptureEchoes succeeds"), SaveData.bIsValid);
	TestEqual(TEXT("Saved echoes count must be 1"), SaveData.Echoes.Num(), 1);
	TestEqual(TEXT("Saved echo state must be Destroyed"), SaveData.Echoes[0].State, EShadowSlaveEchoState::Destroyed);
	TestEqual(TEXT("Saved EchoId must match definition GetEchoId()"), SaveData.Echoes[0].EchoId, EchoDef->GetEchoId());

	// 5. Removal
	TestTrue(TEXT("RemoveEcho succeeds"), EchoComp->RemoveEcho(Acquired.InstanceId));
	TestEqual(TEXT("Echo count is 0 after remove"), EchoComp->GetEchoCount(), 0);

	return true;
}

#endif // WITH_AUTOMATION_TESTS
