// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_AUTOMATION_TESTS

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Content/ShadowSlaveContentTypes.h"
#include "Content/ShadowSlaveContentDefinition.h"
#include "Content/ShadowSlaveContentRegistrySubsystem.h"
#include "Memories/ShadowSlaveMemoryTypes.h"
#include "Memories/ShadowSlaveMemoryDefinition.h"
#include "Memories/ShadowSlaveMemoryComponent.h"

// 1. MemoryDefinitionUsesGenericContentBase Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveMemoryDefinitionUsesGenericContentBaseTest,
	"ShadowSlave.Memory.DefinitionUsesGenericContentBase",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveMemoryDefinitionUsesGenericContentBaseTest::RunTest(const FString& Parameters)
{
	UShadowSlaveMemoryDefinition* Def = NewObject<UShadowSlaveMemoryDefinition>();
	TestNotNull(TEXT("Memory definition must be instantiable"), Def);

	// Verify C++ inheritance and UClass reflection hierarchy
	UShadowSlaveContentDefinition* ContentDef = Cast<UShadowSlaveContentDefinition>(Def);
	TestNotNull(TEXT("Memory definition must cast to UShadowSlaveContentDefinition"), ContentDef);
	TestTrue(TEXT("Memory definition IsA(UShadowSlaveContentDefinition)"), Def->IsA(UShadowSlaveContentDefinition::StaticClass()));
	TestTrue(TEXT("StaticClass hierarchy is child of UShadowSlaveContentDefinition"),
		UShadowSlaveMemoryDefinition::StaticClass()->IsChildOf(UShadowSlaveContentDefinition::StaticClass()));

	return true;
}

// 2. MemoryDefinitionUsesMemoryContentType Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveMemoryDefinitionUsesMemoryContentTypeTest,
	"ShadowSlave.Memory.DefinitionUsesMemoryContentType",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveMemoryDefinitionUsesMemoryContentTypeTest::RunTest(const FString& Parameters)
{
	// Default constructor sets Memory content type
	UShadowSlaveMemoryDefinition* Def = NewObject<UShadowSlaveMemoryDefinition>();
	TestNotNull(TEXT("Memory definition must be instantiable"), Def);
	TestEqual(TEXT("Default ContentType must be EShadowSlaveContentType::Memory"), Def->ContentType, EShadowSlaveContentType::Memory);

	// Test factory definition sets Memory content type
	UShadowSlaveMemoryDefinition* TestDef = UShadowSlaveMemoryDefinition::CreateTestMemoryDefinition();
	TestNotNull(TEXT("Test memory definition must be created"), TestDef);
	TestEqual(TEXT("TestDef ContentType must be EShadowSlaveContentType::Memory"), TestDef->ContentType, EShadowSlaveContentType::Memory);

	// Canon factory definition sets Memory content type
	UShadowSlaveMemoryDefinition* CanonDef = UShadowSlaveMemoryDefinition::CreateCanonMemoryDefinition(FName(TEXT("SilverBell")));
	TestNotNull(TEXT("Canon memory definition must be created"), CanonDef);
	TestEqual(TEXT("CanonDef ContentType must be EShadowSlaveContentType::Memory"), CanonDef->ContentType, EShadowSlaveContentType::Memory);

	return true;
}

// 3. MemoryDefinitionHasSingleAuthoritativeId Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveMemoryDefinitionHasSingleAuthoritativeIdTest,
	"ShadowSlave.Memory.DefinitionHasSingleAuthoritativeId",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveMemoryDefinitionHasSingleAuthoritativeIdTest::RunTest(const FString& Parameters)
{
	UShadowSlaveMemoryDefinition* Def = NewObject<UShadowSlaveMemoryDefinition>();
	TestNotNull(TEXT("Memory definition must be instantiable"), Def);

	// 1. ContentId is the only stored authoritative ID; initially NAME_None
	TestTrue(TEXT("ContentId initially None"), Def->ContentId.IsNone());
	TestTrue(TEXT("GetMemoryId() initially None"), Def->GetMemoryId().IsNone());

	// Verify MemoryId is not a stored UPROPERTY, while ContentId is
	TestNull(TEXT("MemoryId must not be a stored UPROPERTY on UShadowSlaveMemoryDefinition"),
		UShadowSlaveMemoryDefinition::StaticClass()->FindPropertyByName(TEXT("MemoryId")));
	TestNotNull(TEXT("ContentId must be a stored UPROPERTY on UShadowSlaveMemoryDefinition"),
		UShadowSlaveMemoryDefinition::StaticClass()->FindPropertyByName(TEXT("ContentId")));

	// 2. GetMemoryId() returns ContentId
	const FName IdA(TEXT("Memory_Authoritative_A"));
	Def->ContentId = IdA;
	TestEqual(TEXT("GetMemoryId() must return ContentId"), Def->GetMemoryId(), IdA);

	// 3. SetMemoryId() changes ContentId
	const FName IdB(TEXT("Memory_Authoritative_B"));
	Def->SetMemoryId(IdB);
	TestEqual(TEXT("ContentId must be updated by SetMemoryId()"), Def->ContentId, IdB);
	TestEqual(TEXT("GetMemoryId() must reflect SetMemoryId() update"), Def->GetMemoryId(), IdB);

	// 4. Changing ContentId is reflected by GetMemoryId()
	const FName IdC(TEXT("Memory_Authoritative_C"));
	Def->ContentId = IdC;
	TestEqual(TEXT("GetMemoryId() must reflect direct ContentId change"), Def->GetMemoryId(), IdC);

	return true;
}

// 4. MemoryDefinitionValidation Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveMemoryDefinitionValidationTest,
	"ShadowSlave.Memory.DefinitionValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveMemoryDefinitionValidationTest::RunTest(const FString& Parameters)
{
	// 1. Valid test definition passes validation
	UShadowSlaveMemoryDefinition* ValidDef = UShadowSlaveMemoryDefinition::CreateTestMemoryDefinition();
	TestNotNull(TEXT("Test memory definition must be valid"), ValidDef);

	FString ErrorMsg;
	TestTrue(TEXT("Valid test memory passes IsValidDefinition"), ValidDef->IsValidDefinition(&ErrorMsg));
	TestTrue(TEXT("Valid test memory passes ValidateDefinition"), ValidDef->ValidateDefinition(ErrorMsg));

	// 2. NAME_None ContentId fails validation
	ValidDef->ContentId = NAME_None;
	TestFalse(TEXT("NAME_None ContentId must fail IsValidDefinition"), ValidDef->IsValidDefinition(&ErrorMsg));
	TestFalse(TEXT("Error message must be populated for None ContentId"), ErrorMsg.IsEmpty());

	// 3. Version < 1 fails validation
	ValidDef->ContentId = FName(TEXT("Test_Mem_ValidId"));
	ValidDef->Version = 0;
	TestFalse(TEXT("Version 0 must fail IsValidDefinition"), ValidDef->IsValidDefinition(&ErrorMsg));

	// 4. Incorrect ContentType fails validation
	ValidDef->Version = 1;
	ValidDef->ContentType = EShadowSlaveContentType::Echo;
	TestFalse(TEXT("ContentType != Memory must fail IsValidDefinition"), ValidDef->IsValidDefinition(&ErrorMsg));

	// 5. Negative BaseEssenceCost fails validation
	ValidDef->ContentType = EShadowSlaveContentType::Memory;
	ValidDef->BaseEssenceCost = -5.0f;
	TestFalse(TEXT("Negative BaseEssenceCost must fail IsValidDefinition"), ValidDef->IsValidDefinition(&ErrorMsg));

	// 6. Negative enchantment EssenceCost fails validation
	ValidDef->BaseEssenceCost = 0.0f;
	if (ValidDef->Enchantments.Num() > 0)
	{
		ValidDef->Enchantments[0].EssenceCost = -2.0f;
		TestFalse(TEXT("Negative enchantment EssenceCost must fail IsValidDefinition"), ValidDef->IsValidDefinition(&ErrorMsg));
		ValidDef->Enchantments[0].EssenceCost = 0.0f;
	}

	// Restored definition passes again
	TestTrue(TEXT("Restored definition passes validation"), ValidDef->IsValidDefinition());

	return true;
}

// 5. MemoryDefinitionRegistryIntegration Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveMemoryDefinitionRegistryIntegrationTest,
	"ShadowSlave.Memory.DefinitionRegistryIntegration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveMemoryDefinitionRegistryIntegrationTest::RunTest(const FString& Parameters)
{
	UShadowSlaveContentRegistrySubsystem* Registry = NewObject<UShadowSlaveContentRegistrySubsystem>();
	TestNotNull(TEXT("Registry subsystem must be created"), Registry);

	UShadowSlaveMemoryDefinition* TestDef = UShadowSlaveMemoryDefinition::CreateTestMemoryDefinition();
	TestNotNull(TEXT("Test memory definition must be created"), TestDef);

	// Register with generic registry subsystem
	const bool bRegistered = Registry->RegisterDefinition(TestDef);
	TestTrue(TEXT("RegisterDefinition must succeed for UShadowSlaveMemoryDefinition"), bRegistered);

	// Query existence
	TestTrue(TEXT("HasContent must return true for registered Memory"), Registry->HasContent(TestDef->ContentId));
	TestEqual(TEXT("Total registered count must be 1"), Registry->GetRegisteredContentCount(), 1);
	TestEqual(TEXT("Memory count by type must be 1"), Registry->GetRegisteredContentCountByType(EShadowSlaveContentType::Memory), 1);
	TestEqual(TEXT("Echo count by type must be 0"), Registry->GetRegisteredContentCountByType(EShadowSlaveContentType::Echo), 0);

	// Generic resolution
	UShadowSlaveContentDefinition* ResolvedGeneric = Registry->ResolveContentDefinition(TestDef->ContentId);
	TestNotNull(TEXT("Resolved generic definition must not be null"), ResolvedGeneric);
	TestEqual(TEXT("Resolved generic definition must match TestDef"), ResolvedGeneric, Cast<UShadowSlaveContentDefinition>(TestDef));

	// Typed resolution
	UShadowSlaveMemoryDefinition* ResolvedMemory = Registry->ResolveContentDefinition<UShadowSlaveMemoryDefinition>(TestDef->ContentId);
	TestNotNull(TEXT("Resolved typed memory definition must not be null"), ResolvedMemory);
	TestEqual(TEXT("Resolved typed memory must match original TestDef"), ResolvedMemory, TestDef);
	TestEqual(TEXT("Resolved Rank matches"), ResolvedMemory->Rank, TestDef->Rank);
	TestEqual(TEXT("Resolved Tier matches"), ResolvedMemory->Tier, TestDef->Tier);
	TestEqual(TEXT("Resolved Category matches"), ResolvedMemory->Category, TestDef->Category);
	TestEqual(TEXT("Resolved EquipmentSlot matches"), ResolvedMemory->EquipmentSlot, TestDef->EquipmentSlot);
	TestEqual(TEXT("Resolved Enchantment count matches"), ResolvedMemory->GetEnchantmentCount(), TestDef->GetEnchantmentCount());

	// PrimaryAssetId verification
	const FPrimaryAssetId ExpectedAssetId(TEXT("Memory"), TestDef->ContentId);
	TestEqual(TEXT("GetPrimaryAssetId must match expected PrimaryAssetId"), TestDef->GetPrimaryAssetId(), ExpectedAssetId);

	return true;
}

// 6. ExistingMemoryRuntimeCompatibility Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveMemoryExistingRuntimeCompatibilityTest,
	"ShadowSlave.Memory.ExistingRuntimeCompatibility",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveMemoryExistingRuntimeCompatibilityTest::RunTest(const FString& Parameters)
{
	UShadowSlaveMemoryComponent* MemComp = NewObject<UShadowSlaveMemoryComponent>();
	TestNotNull(TEXT("Memory component must be instantiable"), MemComp);

	UShadowSlaveMemoryDefinition* TestDef = UShadowSlaveMemoryDefinition::CreateTestMemoryDefinition();
	TestNotNull(TEXT("Test memory definition must be created"), TestDef);

	// 1. Acquire Memory
	FShadowSlaveMemoryInstance AcquiredInst;
	const bool bAcquired = MemComp->AcquireMemory(TestDef, AcquiredInst);
	TestTrue(TEXT("AcquireMemory must succeed with integrated definition"), bAcquired);
	TestTrue(TEXT("Acquired instance must be valid"), AcquiredInst.IsValid());
	TestEqual(TEXT("Acquired instance definition must match TestDef"), AcquiredInst.MemoryDefinition.Get(), TestDef);
	TestEqual(TEXT("Acquired instance Rank matches definition Rank"), AcquiredInst.GetRank(), EShadowSlaveMemoryRank::Awakened);
	TestEqual(TEXT("Acquired instance Tier matches definition Tier"), AcquiredInst.GetTier(), EShadowSlaveMemoryTier::Tier1);
	TestEqual(TEXT("Acquired instance total enchantment count matches"), AcquiredInst.GetTotalEnchantmentCount(), 2);
	TestTrue(TEXT("MemComp HasMemory returns true"), MemComp->HasMemory(TestDef));
	TestEqual(TEXT("MemComp memory count is 1"), MemComp->GetMemoryCount(), 1);

	// 2. Equip Memory
	const bool bEquipped = MemComp->EquipMemory(AcquiredInst.InstanceId);
	TestTrue(TEXT("EquipMemory must succeed"), bEquipped);
	TestTrue(TEXT("HasEquippedMemoryWithDefinition returns true"), MemComp->HasEquippedMemoryWithDefinition(TestDef));

	// 3. Unequip Memory
	const bool bUnequipped = MemComp->UnequipMemory(AcquiredInst.InstanceId);
	TestTrue(TEXT("UnequipMemory must succeed"), bUnequipped);
	TestFalse(TEXT("HasEquippedMemoryWithDefinition returns false after unequip"), MemComp->HasEquippedMemoryWithDefinition(TestDef));
	TestTrue(TEXT("MemComp still holds the unequipped memory"), MemComp->HasMemory(TestDef));

	// 4. Remove Memory
	const bool bRemoved = MemComp->RemoveMemoryByDefinition(TestDef);
	TestTrue(TEXT("RemoveMemoryByDefinition must succeed"), bRemoved);
	TestFalse(TEXT("MemComp HasMemory returns false after removal"), MemComp->HasMemory(TestDef));
	TestEqual(TEXT("MemComp memory count is 0 after removal"), MemComp->GetMemoryCount(), 0);

	return true;
}

#endif // WITH_AUTOMATION_TESTS
