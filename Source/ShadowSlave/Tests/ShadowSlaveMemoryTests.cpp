// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_AUTOMATION_TESTS

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Content/ShadowSlaveContentTypes.h"
#include "Content/ShadowSlaveContentDefinition.h"
#include "Content/ShadowSlaveContentRegistrySubsystem.h"
#include "Memories/ShadowSlaveMemoryTypes.h"
#include "Memories/ShadowSlaveMemoryDefinition.h"
#include "Memories/ShadowSlaveMemoryChronologyRegistry.h"
#include "Memories/ShadowSlaveMemoryChronologyTypes.h"


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

// =============================================================================
// Step 51: Memory Chronology Registry Content Pipeline Integration Tests
// =============================================================================

// 1. DefinitionUsesGenericContentBase Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveMemoryChronologyRegistryUsesGenericContentBaseTest,
	"ShadowSlave.MemoryChronologyRegistry.DefinitionUsesGenericContentBase",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveMemoryChronologyRegistryUsesGenericContentBaseTest::RunTest(const FString& Parameters)
{
	UShadowSlaveMemoryChronologyRegistry* Chronology = NewObject<UShadowSlaveMemoryChronologyRegistry>();
	TestNotNull(TEXT("Chronology registry must instantiate"), Chronology);

	TestTrue(TEXT("Chronology registry must be a UShadowSlaveContentDefinition"),
		Chronology->IsA(UShadowSlaveContentDefinition::StaticClass()));

	UShadowSlaveContentDefinition* BaseDef = Cast<UShadowSlaveContentDefinition>(Chronology);
	TestNotNull(TEXT("Cast to UShadowSlaveContentDefinition must succeed"), BaseDef);
	TestTrue(TEXT("StaticClass hierarchy is child of UShadowSlaveContentDefinition"),
		UShadowSlaveMemoryChronologyRegistry::StaticClass()->IsChildOf(UShadowSlaveContentDefinition::StaticClass()));

	Chronology->ContentId = FName(TEXT("Test_Generic_Chronology"));
	Chronology->DisplayName = FText::FromString(TEXT("Generic Test Chronology"));
	Chronology->Description = FText::FromString(TEXT("Test description for chronology content pipeline."));
	Chronology->Version = 2;
	Chronology->ProvenanceNote = TEXT("Test Provenance Note");

	TestEqual(TEXT("Base ContentId matches"), BaseDef->ContentId, FName(TEXT("Test_Generic_Chronology")));
	TestEqual(TEXT("Base DisplayName matches"), BaseDef->DisplayName.ToString(), TEXT("Generic Test Chronology"));
	TestEqual(TEXT("Base Description matches"), BaseDef->Description.ToString(), TEXT("Test description for chronology content pipeline."));
	TestEqual(TEXT("Base Version matches"), BaseDef->Version, 2);
	TestEqual(TEXT("Base ProvenanceNote matches"), BaseDef->ProvenanceNote, TEXT("Test Provenance Note"));
	TestEqual(TEXT("Base ContentType is Custom"), BaseDef->ContentType, EShadowSlaveContentType::Custom);

	return true;
}

// 2. DefinitionUsesCustomContentType Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveMemoryChronologyRegistryUsesCustomContentTypeTest,
	"ShadowSlave.MemoryChronologyRegistry.DefinitionUsesCustomContentType",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveMemoryChronologyRegistryUsesCustomContentTypeTest::RunTest(const FString& Parameters)
{
	UShadowSlaveMemoryChronologyRegistry* Chronology = NewObject<UShadowSlaveMemoryChronologyRegistry>();
	TestNotNull(TEXT("Chronology registry must instantiate"), Chronology);

	TestEqual(TEXT("Default ContentType must be EShadowSlaveContentType::Custom"),
		Chronology->ContentType, EShadowSlaveContentType::Custom);

	TestTrue(TEXT("ContentType is not Memory"), Chronology->ContentType != EShadowSlaveContentType::Memory);
	TestTrue(TEXT("ContentType is not Story"), Chronology->ContentType != EShadowSlaveContentType::Story);
	TestTrue(TEXT("ContentType is not World"), Chronology->ContentType != EShadowSlaveContentType::World);
	TestTrue(TEXT("ContentType is not Ability"), Chronology->ContentType != EShadowSlaveContentType::Ability);

	return true;
}

// 3. DefinitionHasSingleAuthoritativeId Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveMemoryChronologyRegistryHasSingleAuthoritativeIdTest,
	"ShadowSlave.MemoryChronologyRegistry.DefinitionHasSingleAuthoritativeId",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveMemoryChronologyRegistryHasSingleAuthoritativeIdTest::RunTest(const FString& Parameters)
{
	UShadowSlaveMemoryChronologyRegistry* Chronology = NewObject<UShadowSlaveMemoryChronologyRegistry>();
	TestNotNull(TEXT("Chronology registry must instantiate"), Chronology);

	TestNull(TEXT("RegistryId must not be a stored UPROPERTY"),
		UShadowSlaveMemoryChronologyRegistry::StaticClass()->FindPropertyByName(TEXT("RegistryId")));
	TestNull(TEXT("RegistryName must not be a stored UPROPERTY"),
		UShadowSlaveMemoryChronologyRegistry::StaticClass()->FindPropertyByName(TEXT("RegistryName")));
	TestNotNull(TEXT("ContentId must be a stored UPROPERTY"),
		UShadowSlaveMemoryChronologyRegistry::StaticClass()->FindPropertyByName(TEXT("ContentId")));
	TestNotNull(TEXT("DisplayName must be a stored UPROPERTY"),
		UShadowSlaveMemoryChronologyRegistry::StaticClass()->FindPropertyByName(TEXT("DisplayName")));

	TestEqual(TEXT("Initial ContentId is NAME_None"), Chronology->ContentId, NAME_None);
	TestEqual(TEXT("Initial GetRegistryId() returns NAME_None"), Chronology->GetRegistryId(), NAME_None);

	const FName Id1(TEXT("Chronology_Authoritative_01"));
	Chronology->ContentId = Id1;
	TestEqual(TEXT("GetRegistryId() returns authoritative ContentId"), Chronology->GetRegistryId(), Id1);

	const FName Id2(TEXT("Chronology_Authoritative_02"));
	Chronology->SetRegistryId(Id2);
	TestEqual(TEXT("ContentId reflects SetRegistryId()"), Chronology->ContentId, Id2);
	TestEqual(TEXT("GetRegistryId() reflects SetRegistryId()"), Chronology->GetRegistryId(), Id2);

	Chronology->DisplayName = FText::FromString(TEXT("Mapped Registry Title"));
	TestEqual(TEXT("GetRegistryName() maps to DisplayName"), Chronology->GetRegistryName().ToString(), TEXT("Mapped Registry Title"));

	const FPrimaryAssetId ExpectedAssetId(TEXT("MemoryChronologyRegistry"), Id2);
	TestEqual(TEXT("GetPrimaryAssetId() uses ContentId and MemoryChronologyRegistry type"),
		Chronology->GetPrimaryAssetId(), ExpectedAssetId);

	return true;
}

// 4. DefinitionValidation Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveMemoryChronologyRegistryValidationTest,
	"ShadowSlave.MemoryChronologyRegistry.DefinitionValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveMemoryChronologyRegistryValidationTest::RunTest(const FString& Parameters)
{
	UShadowSlaveMemoryChronologyRegistry* ValidDef = NewObject<UShadowSlaveMemoryChronologyRegistry>();
	ValidDef->ContentId = FName(TEXT("Test_Valid_Chronology"));
	ValidDef->DisplayName = FText::FromString(TEXT("Valid Test Chronology"));
	ValidDef->Version = 1;
	ValidDef->ContentType = EShadowSlaveContentType::Custom;

	FString ErrorMsg;

	TestTrue(TEXT("Valid chronology registry must pass IsValidDefinition"), ValidDef->IsValidDefinition(&ErrorMsg));
	TestTrue(TEXT("Valid chronology registry must pass ValidateDefinition"), ValidDef->ValidateDefinition(ErrorMsg));

	ValidDef->ContentId = NAME_None;
	TestFalse(TEXT("None ContentId must fail IsValidDefinition"), ValidDef->IsValidDefinition(&ErrorMsg));
	TestFalse(TEXT("Error message must not be empty on failure"), ErrorMsg.IsEmpty());
	ValidDef->ContentId = FName(TEXT("Test_Valid_Chronology"));

	ValidDef->DisplayName = FText::GetEmpty();
	TestFalse(TEXT("Empty DisplayName must fail IsValidDefinition"), ValidDef->IsValidDefinition(&ErrorMsg));
	ValidDef->DisplayName = FText::FromString(TEXT("Valid Test Chronology"));

	ValidDef->Version = 0;
	TestFalse(TEXT("Version 0 must fail IsValidDefinition"), ValidDef->IsValidDefinition(&ErrorMsg));
	ValidDef->Version = 1;

	ValidDef->ContentType = EShadowSlaveContentType::Memory;
	TestFalse(TEXT("Wrong ContentType must fail IsValidDefinition"), ValidDef->IsValidDefinition(&ErrorMsg));
	ValidDef->ContentType = EShadowSlaveContentType::Custom;

	TestTrue(TEXT("Restored definition passes IsValidDefinition"), ValidDef->IsValidDefinition(&ErrorMsg));

	return true;
}

// 5. DefinitionPrimaryAssetId Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveMemoryChronologyRegistryPrimaryAssetIdTest,
	"ShadowSlave.MemoryChronologyRegistry.DefinitionPrimaryAssetId",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveMemoryChronologyRegistryPrimaryAssetIdTest::RunTest(const FString& Parameters)
{
	UShadowSlaveMemoryChronologyRegistry* Chronology = NewObject<UShadowSlaveMemoryChronologyRegistry>();
	TestNotNull(TEXT("Chronology registry must instantiate"), Chronology);

	const FName RegistryId(TEXT("Chronology_PrimaryAsset_01"));
	Chronology->SetRegistryId(RegistryId);
	Chronology->DisplayName = FText::FromString(TEXT("Primary Asset Chronology"));
	Chronology->Version = 1;

	const FPrimaryAssetId AssetId = Chronology->GetPrimaryAssetId();
	TestEqual(TEXT("GetPrimaryAssetId must equal FPrimaryAssetId(MemoryChronologyRegistry, ContentId)"),
		AssetId, FPrimaryAssetId(TEXT("MemoryChronologyRegistry"), RegistryId));
	TestTrue(TEXT("PrimaryAssetType must be MemoryChronologyRegistry"),
		AssetId.PrimaryAssetType == FPrimaryAssetType(TEXT("MemoryChronologyRegistry")));
	TestEqual(TEXT("PrimaryAssetName must be ContentId"), AssetId.PrimaryAssetName, RegistryId);

	Chronology->ContentId = NAME_None;
	const FPrimaryAssetId FallbackId = Chronology->GetPrimaryAssetId();
	TestTrue(TEXT("Fallback PrimaryAssetType remains MemoryChronologyRegistry"),
		FallbackId.PrimaryAssetType == FPrimaryAssetType(TEXT("MemoryChronologyRegistry")));
	TestEqual(TEXT("Fallback PrimaryAssetName uses GetFName()"), FallbackId.PrimaryAssetName, Chronology->GetFName());

	return true;
}

// 6. DefinitionRegistryIntegration Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveMemoryChronologyRegistryRegistryIntegrationTest,
	"ShadowSlave.MemoryChronologyRegistry.DefinitionRegistryIntegration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveMemoryChronologyRegistryRegistryIntegrationTest::RunTest(const FString& Parameters)
{
	UShadowSlaveContentRegistrySubsystem* Registry = NewObject<UShadowSlaveContentRegistrySubsystem>();
	TestNotNull(TEXT("Content registry subsystem must be created"), Registry);

	UShadowSlaveMemoryChronologyRegistry* Chronology = NewObject<UShadowSlaveMemoryChronologyRegistry>();
	Chronology->ContentId = FName(TEXT("Chronology_Registry_Test"));
	Chronology->DisplayName = FText::FromString(TEXT("Registry Test Chronology"));
	Chronology->Version = 1;

	const bool bRegistered = Registry->RegisterDefinition(Chronology);
	TestTrue(TEXT("RegisterDefinition must succeed for UShadowSlaveMemoryChronologyRegistry"), bRegistered);

	TestTrue(TEXT("HasContent must return true for registered Chronology"), Registry->HasContent(Chronology->ContentId));
	TestEqual(TEXT("Total registered count must be 1"), Registry->GetRegisteredContentCount(), 1);
	TestEqual(TEXT("Custom count by type must be 1"), Registry->GetRegisteredContentCountByType(EShadowSlaveContentType::Custom), 1);
	TestEqual(TEXT("Memory count by type must be 0"), Registry->GetRegisteredContentCountByType(EShadowSlaveContentType::Memory), 0);
	TestEqual(TEXT("Story count by type must be 0"), Registry->GetRegisteredContentCountByType(EShadowSlaveContentType::Story), 0);

	UShadowSlaveContentDefinition* ResolvedGeneric = Registry->ResolveContentDefinition(Chronology->ContentId);
	TestNotNull(TEXT("Resolved generic definition must not be null"), ResolvedGeneric);
	TestEqual(TEXT("Resolved generic definition must match Chronology"), ResolvedGeneric, Cast<UShadowSlaveContentDefinition>(Chronology));

	UShadowSlaveMemoryChronologyRegistry* ResolvedChronology =
		Registry->ResolveContentDefinition<UShadowSlaveMemoryChronologyRegistry>(Chronology->ContentId);
	TestNotNull(TEXT("Resolved typed chronology registry must not be null"), ResolvedChronology);
	TestEqual(TEXT("Resolved typed chronology must match original"), ResolvedChronology, Chronology);

	const FPrimaryAssetId ExpectedAssetId(TEXT("MemoryChronologyRegistry"), Chronology->ContentId);
	TestEqual(TEXT("GetPrimaryAssetId must match expected PrimaryAssetId"), Chronology->GetPrimaryAssetId(), ExpectedAssetId);

	return true;
}

// 7. ExistingChronologyBehavior Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveMemoryChronologyRegistryExistingBehaviorTest,
	"ShadowSlave.MemoryChronologyRegistry.ExistingChronologyBehavior",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveMemoryChronologyRegistryExistingBehaviorTest::RunTest(const FString& Parameters)
{
	UShadowSlaveMemoryChronologyRegistry* Chronology =
		UShadowSlaveMemoryChronologyRegistry::CreateSunnyBaselineChronologyRegistry();
	TestNotNull(TEXT("CreateSunnyBaselineChronologyRegistry must return a valid object"), Chronology);

	TestEqual(TEXT("Factory ContentId is Registry_Sunny_Baseline"),
		Chronology->ContentId, FName(TEXT("Registry_Sunny_Baseline")));
	TestEqual(TEXT("Factory GetRegistryId matches ContentId"),
		Chronology->GetRegistryId(), FName(TEXT("Registry_Sunny_Baseline")));
	TestEqual(TEXT("Factory DisplayName is preserved"),
		Chronology->DisplayName.ToString(), TEXT("Sunny Baseline Memory Chronology (First Nightmare -> Antarctica)"));
	TestEqual(TEXT("Factory GetRegistryName maps to DisplayName"),
		Chronology->GetRegistryName().ToString(), TEXT("Sunny Baseline Memory Chronology (First Nightmare -> Antarctica)"));
	TestEqual(TEXT("Factory ContentType is Custom"), Chronology->ContentType, EShadowSlaveContentType::Custom);
	TestTrue(TEXT("Factory definition passes IsValidDefinition"), Chronology->IsValidDefinition());

	const FPrimaryAssetId ExpectedAssetId(TEXT("MemoryChronologyRegistry"), FName(TEXT("Registry_Sunny_Baseline")));
	TestEqual(TEXT("Factory PrimaryAssetId uses ContentId, not UObject name"),
		Chronology->GetPrimaryAssetId(), ExpectedAssetId);
	TestTrue(TEXT("Factory UObject name remains Sunny_Baseline_Memory_Chronology_Registry"),
		Chronology->GetFName() == FName(TEXT("Sunny_Baseline_Memory_Chronology_Registry")));

	TestEqual(TEXT("Factory preserves 16 chronology entries"), Chronology->GetEntryCount(), 16);

	FShadowSlaveMemoryChronologyEntry SilverBell;
	TestTrue(TEXT("FindEntryById locates Entry_SilverBell"),
		Chronology->FindEntryById(FName(TEXT("Entry_SilverBell")), SilverBell));
	TestEqual(TEXT("Silver Bell chapter remains 8"), SilverBell.ApproximateChapter, 8);
	TestEqual(TEXT("Silver Bell arc remains FirstNightmare"), SilverBell.StoryArc, EShadowSlaveStoryArc::FirstNightmare);
	TestEqual(TEXT("Silver Bell confidence remains Verified"), SilverBell.CanonConfidence, EShadowSlaveCanonConfidence::Verified);

	FShadowSlaveMemoryChronologyEntry SiegeSouvenir;
	TestTrue(TEXT("FindEntryById locates Entry_SiegeSouvenir"),
		Chronology->FindEntryById(FName(TEXT("Entry_SiegeSouvenir")), SiegeSouvenir));
	TestEqual(TEXT("Siege Souvenir chapter remains 1030"), SiegeSouvenir.ApproximateChapter, 1030);
	TestFalse(TEXT("Siege Souvenir is not retained at arc end"), SiegeSouvenir.bIsRetainedAtArcEnd);

	const TArray<FShadowSlaveMemoryChronologyEntry> FirstNightmareEntries =
		Chronology->GetEntriesByArc(EShadowSlaveStoryArc::FirstNightmare);
	TestEqual(TEXT("First Nightmare entry count remains 2"), FirstNightmareEntries.Num(), 2);

	const TArray<FShadowSlaveMemoryChronologyEntry> SunlessEntries =
		Chronology->GetEntriesForCharacter(FName(TEXT("Sunless")));
	TestEqual(TEXT("Sunless entry count remains 16"), SunlessEntries.Num(), 16);

	const TArray<FShadowSlaveMemoryChronologyEntry> VerifiedEntries = Chronology->GetVerifiedEntries();
	TestEqual(TEXT("Verified entry count remains 15"), VerifiedEntries.Num(), 15);

	FShadowSlaveMemoryChronologyEntry AutumnLeaf;
	TestTrue(TEXT("FindEntryById locates Entry_AutumnLeaf"),
		Chronology->FindEntryById(FName(TEXT("Entry_AutumnLeaf")), AutumnLeaf));
	TestEqual(TEXT("Autumn Leaf confidence remains Unknown"), AutumnLeaf.CanonConfidence, EShadowSlaveCanonConfidence::Unknown);

	return true;
}

#endif // WITH_AUTOMATION_TESTS
