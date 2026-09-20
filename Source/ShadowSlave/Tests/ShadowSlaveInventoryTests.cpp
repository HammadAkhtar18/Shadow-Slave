// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_AUTOMATION_TESTS

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Items/ShadowSlaveInventoryComponent.h"
#include "Items/ShadowSlaveItemDefinition.h"
#include "Items/ShadowSlaveItemTypes.h"
#include "Content/ShadowSlaveContentTypes.h"
#include "Content/ShadowSlaveContentDefinition.h"
#include "Content/ShadowSlaveContentRegistrySubsystem.h"
#include "Attributes/ShadowSlaveAttributeTypes.h"
#include "Save/ShadowSlaveSaveSubsystem.h"
#include "Save/ShadowSlaveSaveTypes.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveInventoryEmptyInitialStateTest,
	"ShadowSlave.Inventory.EmptyInventoryInitialState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveInventoryEmptyInitialStateTest::RunTest(const FString& Parameters)
{
	UShadowSlaveInventoryComponent* InvComp = NewObject<UShadowSlaveInventoryComponent>();
	TestNotNull(TEXT("InventoryComponent must instantiate successfully"), InvComp);
	if (!InvComp)
	{
		return false;
	}

	InvComp->SetCapacity(8);

	TestEqual(TEXT("Reported capacity must match set capacity"), InvComp->GetCapacity(), 8);
	TestEqual(TEXT("Initial used slots must be 0"), InvComp->GetUsedSlotCount(), 0);
	TestEqual(TEXT("Free slots must equal capacity"), InvComp->GetFreeSlotCount(), 8);
	TestFalse(TEXT("Inventory must not be full on initialization"), InvComp->IsFull());
	TestEqual(TEXT("Slots array must be empty"), InvComp->GetSlots().Num(), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveInventoryAddItemAndQuantityTrackingTest,
	"ShadowSlave.Inventory.AddItemAndQuantityTracking",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveInventoryAddItemAndQuantityTrackingTest::RunTest(const FString& Parameters)
{
	UShadowSlaveInventoryComponent* InvComp = NewObject<UShadowSlaveInventoryComponent>();
	TestNotNull(TEXT("InventoryComponent must instantiate successfully"), InvComp);
	if (!InvComp)
	{
		return false;
	}

	InvComp->SetCapacity(5);

	// Create non-stackable test item definition
	UShadowSlaveItemDefinition* SingleItemDef = NewObject<UShadowSlaveItemDefinition>();
	SingleItemDef->bIsStackable = false;
	SingleItemDef->MaxStackSize = 1;

	int32 Remainder = 0;
	const bool bAdded = InvComp->AddItem(SingleItemDef, 1, Remainder);

	TestTrue(TEXT("AddItem must return true on success"), bAdded);
	TestEqual(TEXT("Remainder must be 0 when slot is available"), Remainder, 0);
	TestEqual(TEXT("Used slot count must be 1"), InvComp->GetUsedSlotCount(), 1);
	TestEqual(TEXT("Total item count must be 1"), InvComp->GetTotalItemCount(SingleItemDef), 1);
	TestTrue(TEXT("HasItem(1) must return true"), InvComp->HasItem(SingleItemDef, 1));
	TestFalse(TEXT("HasItem(2) must return false"), InvComp->HasItem(SingleItemDef, 2));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveInventoryStackingBehaviorTest,
	"ShadowSlave.Inventory.StackingBehavior",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveInventoryStackingBehaviorTest::RunTest(const FString& Parameters)
{
	UShadowSlaveInventoryComponent* InvComp = NewObject<UShadowSlaveInventoryComponent>();
	TestNotNull(TEXT("InventoryComponent must instantiate successfully"), InvComp);
	if (!InvComp)
	{
		return false;
	}

	InvComp->SetCapacity(5);

	// Create stackable test item definition
	UShadowSlaveItemDefinition* StackDef = NewObject<UShadowSlaveItemDefinition>();
	StackDef->bIsStackable = true;
	StackDef->MaxStackSize = 10;

	int32 Remainder = 0;
	// Add initial quantity of 4
	TestTrue(TEXT("First addition must succeed"), InvComp->AddItem(StackDef, 4, Remainder));
	TestEqual(TEXT("Remainder must be 0"), Remainder, 0);
	TestEqual(TEXT("Used slots must be 1"), InvComp->GetUsedSlotCount(), 1);
	TestEqual(TEXT("Total count must be 4"), InvComp->GetTotalItemCount(StackDef), 4);

	// Add 5 more of the same stackable item; should merge into the same slot
	TestTrue(TEXT("Second addition must succeed"), InvComp->AddItem(StackDef, 5, Remainder));
	TestEqual(TEXT("Remainder must be 0"), Remainder, 0);
	TestEqual(TEXT("Used slots must remain 1 after stacking"), InvComp->GetUsedSlotCount(), 1);
	TestEqual(TEXT("Total count must be 9"), InvComp->GetTotalItemCount(StackDef), 9);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveInventoryCapacityAndRemainderTest,
	"ShadowSlave.Inventory.CapacityAndRemainderHandling",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveInventoryCapacityAndRemainderTest::RunTest(const FString& Parameters)
{
	UShadowSlaveInventoryComponent* InvComp = NewObject<UShadowSlaveInventoryComponent>();
	TestNotNull(TEXT("InventoryComponent must instantiate successfully"), InvComp);
	if (!InvComp)
	{
		return false;
	}

	// Strictly 2 slots capacity
	InvComp->SetCapacity(2);

	UShadowSlaveItemDefinition* StackDef = NewObject<UShadowSlaveItemDefinition>();
	StackDef->bIsStackable = true;
	StackDef->MaxStackSize = 5;

	// Requesting to add 13 units into an inventory with 2 slots of capacity 5 (max capacity = 10)
	int32 Remainder = 0;
	const bool bAdded = InvComp->AddItem(StackDef, 13, Remainder);

	TestTrue(TEXT("AddItem must return true if at least one item was added"), bAdded);
	TestEqual(TEXT("Remainder must be 3 because capacity is 10"), Remainder, 3);
	TestEqual(TEXT("Total stored items must be 10"), InvComp->GetTotalItemCount(StackDef), 10);
	TestEqual(TEXT("Used slots must be 2"), InvComp->GetUsedSlotCount(), 2);
	TestEqual(TEXT("Free slots must be 0"), InvComp->GetFreeSlotCount(), 0);
	TestTrue(TEXT("Inventory must be full"), InvComp->IsFull());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveInventoryAtomicRemovalTest,
	"ShadowSlave.Inventory.AtomicRemovalWhenUnavailable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveInventoryAtomicRemovalTest::RunTest(const FString& Parameters)
{
	UShadowSlaveInventoryComponent* InvComp = NewObject<UShadowSlaveInventoryComponent>();
	TestNotNull(TEXT("InventoryComponent must instantiate successfully"), InvComp);
	if (!InvComp)
	{
		return false;
	}

	InvComp->SetCapacity(5);

	UShadowSlaveItemDefinition* ItemDef = NewObject<UShadowSlaveItemDefinition>();
	ItemDef->bIsStackable = true;
	ItemDef->MaxStackSize = 20;

	int32 Remainder = 0;
	InvComp->AddItem(ItemDef, 5, Remainder);
	TestEqual(TEXT("Starting quantity must be 5"), InvComp->GetTotalItemCount(ItemDef), 5);

	// Request removal of 10 units (more than available)
	const bool bRemoved = InvComp->RemoveItem(ItemDef, 10);

	TestFalse(TEXT("RemoveItem must return false when insufficient items exist"), bRemoved);
	TestEqual(TEXT("Total quantity must remain untouched (atomic guarantee)"), InvComp->GetTotalItemCount(ItemDef), 5);
	TestEqual(TEXT("Used slots must remain 1"), InvComp->GetUsedSlotCount(), 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveInventorySuccessfulRemovalAndInstanceIdTest,
	"ShadowSlave.Inventory.SuccessfulRemovalAndInstanceId",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveInventorySuccessfulRemovalAndInstanceIdTest::RunTest(const FString& Parameters)
{
	UShadowSlaveInventoryComponent* InvComp = NewObject<UShadowSlaveInventoryComponent>();
	TestNotNull(TEXT("InventoryComponent must instantiate successfully"), InvComp);
	if (!InvComp)
	{
		return false;
	}

	InvComp->SetCapacity(5);

	UShadowSlaveItemDefinition* ItemDef = NewObject<UShadowSlaveItemDefinition>();
	ItemDef->bIsStackable = true;
	ItemDef->MaxStackSize = 20;

	int32 Remainder = 0;
	InvComp->AddItem(ItemDef, 8, Remainder);

	FShadowSlaveItemInstance FoundInstance;
	TestTrue(TEXT("FindItem must find the stored item instance"), InvComp->FindItem(ItemDef, FoundInstance));
	TestTrue(TEXT("Found instance must have a valid GUID"), FoundInstance.InstanceId.IsValid());
	TestTrue(TEXT("HasItemByInstanceId must return true"), InvComp->HasItemByInstanceId(FoundInstance.InstanceId));

	// Remove partial quantity by definition
	TestTrue(TEXT("RemoveItem(3) must succeed"), InvComp->RemoveItem(ItemDef, 3));
	TestEqual(TEXT("Remaining quantity must be 5"), InvComp->GetTotalItemCount(ItemDef), 5);

	// Remove remainder by instance GUID
	TestTrue(TEXT("RemoveItemByInstanceId(5) must succeed"), InvComp->RemoveItemByInstanceId(FoundInstance.InstanceId, 5));
	TestEqual(TEXT("Total quantity must now be 0"), InvComp->GetTotalItemCount(ItemDef), 0);
	TestFalse(TEXT("HasItemByInstanceId must now return false"), InvComp->HasItemByInstanceId(FoundInstance.InstanceId));
	TestEqual(TEXT("Used slot count must be 0"), InvComp->GetUsedSlotCount(), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveInventoryClearInventoryTest,
	"ShadowSlave.Inventory.ClearInventoryEmptiesAllSlots",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveInventoryClearInventoryTest::RunTest(const FString& Parameters)
{
	UShadowSlaveInventoryComponent* InvComp = NewObject<UShadowSlaveInventoryComponent>();
	TestNotNull(TEXT("InventoryComponent must instantiate successfully"), InvComp);
	if (!InvComp)
	{
		return false;
	}

	InvComp->SetCapacity(10);

	UShadowSlaveItemDefinition* ItemDefA = NewObject<UShadowSlaveItemDefinition>();
	UShadowSlaveItemDefinition* ItemDefB = NewObject<UShadowSlaveItemDefinition>();

	int32 Remainder = 0;
	InvComp->AddItem(ItemDefA, 1, Remainder);
	InvComp->AddItem(ItemDefB, 1, Remainder);
	TestEqual(TEXT("Used slots before clear must be 2"), InvComp->GetUsedSlotCount(), 2);

	InvComp->ClearInventory();

	TestEqual(TEXT("Used slots after ClearInventory must be 0"), InvComp->GetUsedSlotCount(), 0);
	TestEqual(TEXT("Free slots after ClearInventory must be 10"), InvComp->GetFreeSlotCount(), 10);
	TestEqual(TEXT("Slots array must be empty"), InvComp->GetSlots().Num(), 0);

	return true;
}

// =============================================================================
// Step 42: Item Definition Content Pipeline Integration Tests
// =============================================================================

// 1. DefinitionUsesGenericContentBase Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveItemDefinitionUsesGenericContentBaseTest,
	"ShadowSlave.ItemDefinition.DefinitionUsesGenericContentBase",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveItemDefinitionUsesGenericContentBaseTest::RunTest(const FString& Parameters)
{
	UShadowSlaveItemDefinition* ItemDef = NewObject<UShadowSlaveItemDefinition>();
	TestNotNull(TEXT("Item definition must instantiate"), ItemDef);

	// 1. Inheritance verification
	TestTrue(TEXT("Item definition must be a UShadowSlaveContentDefinition"),
		ItemDef->IsA(UShadowSlaveContentDefinition::StaticClass()));

	UShadowSlaveContentDefinition* BaseDef = Cast<UShadowSlaveContentDefinition>(ItemDef);
	TestNotNull(TEXT("Cast to UShadowSlaveContentDefinition must succeed"), BaseDef);

	// 2. Generic content properties inherited and accessible
	ItemDef->ContentId = FName(TEXT("Test_Generic_Item"));
	ItemDef->DisplayName = FText::FromString(TEXT("Generic Test Item"));
	ItemDef->Description = FText::FromString(TEXT("Test description for content pipeline."));
	ItemDef->Version = 2;
	ItemDef->ProvenanceNote = TEXT("Test Provenance Note");

	TestEqual(TEXT("Base ContentId matches"), BaseDef->ContentId, FName(TEXT("Test_Generic_Item")));
	TestEqual(TEXT("Base DisplayName matches"), BaseDef->DisplayName.ToString(), TEXT("Generic Test Item"));
	TestEqual(TEXT("Base Description matches"), BaseDef->Description.ToString(), TEXT("Test description for content pipeline."));
	TestEqual(TEXT("Base Version matches"), BaseDef->Version, 2);
	TestEqual(TEXT("Base ProvenanceNote matches"), BaseDef->ProvenanceNote, TEXT("Test Provenance Note"));
	TestEqual(TEXT("Base ContentType is Item"), BaseDef->ContentType, EShadowSlaveContentType::Item);

	return true;
}

// 2. DefinitionUsesItemContentType Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveItemDefinitionUsesItemContentTypeTest,
	"ShadowSlave.ItemDefinition.DefinitionUsesItemContentType",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveItemDefinitionUsesItemContentTypeTest::RunTest(const FString& Parameters)
{
	UShadowSlaveItemDefinition* ItemDef = NewObject<UShadowSlaveItemDefinition>();
	TestNotNull(TEXT("Item definition must instantiate"), ItemDef);

	// 1. Top-level generic content type must default to Item
	TestEqual(TEXT("Default ContentType must be EShadowSlaveContentType::Item"),
		ItemDef->ContentType, EShadowSlaveContentType::Item);

	// 2. Item-specific taxonomy (ItemType, EquipmentSlot) remains separate from generic ContentType
	TestEqual(TEXT("Default ItemType must be Miscellaneous"),
		ItemDef->ItemType, EShadowSlaveItemType::Miscellaneous);
	TestEqual(TEXT("Default EquipmentSlot must be None"),
		ItemDef->EquipmentSlot, EShadowSlaveEquipmentSlot::None);

	// Mutating Item-specific taxonomy must not affect generic ContentType
	ItemDef->ItemType = EShadowSlaveItemType::Consumable;
	ItemDef->EquipmentSlot = EShadowSlaveEquipmentSlot::Weapon;
	TestEqual(TEXT("ContentType remains Item after mutating ItemType"),
		ItemDef->ContentType, EShadowSlaveContentType::Item);
	TestEqual(TEXT("ItemType is Consumable"),
		ItemDef->ItemType, EShadowSlaveItemType::Consumable);
	TestEqual(TEXT("EquipmentSlot is Weapon"),
		ItemDef->EquipmentSlot, EShadowSlaveEquipmentSlot::Weapon);

	ItemDef->ItemType = EShadowSlaveItemType::Equipment;
	ItemDef->EquipmentSlot = EShadowSlaveEquipmentSlot::Armor;
	TestEqual(TEXT("ContentType remains Item after setting Equipment/Armor"),
		ItemDef->ContentType, EShadowSlaveContentType::Item);

	return true;
}

// 3. DefinitionHasSingleAuthoritativeId Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveItemDefinitionHasSingleAuthoritativeIdTest,
	"ShadowSlave.ItemDefinition.DefinitionHasSingleAuthoritativeId",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveItemDefinitionHasSingleAuthoritativeIdTest::RunTest(const FString& Parameters)
{
	UShadowSlaveItemDefinition* ItemDef = NewObject<UShadowSlaveItemDefinition>();
	TestNotNull(TEXT("Item definition must instantiate"), ItemDef);

	// 1. Initial state: ContentId is NAME_None, GetItemId() returns NAME_None
	TestEqual(TEXT("Initial ContentId is NAME_None"), ItemDef->ContentId, NAME_None);
	TestEqual(TEXT("Initial GetItemId() returns NAME_None"), ItemDef->GetItemId(), NAME_None);

	// 2. Set ContentId directly -> GetItemId() must return the same ID
	const FName Id1(TEXT("Item_Authoritative_01"));
	ItemDef->ContentId = Id1;
	TestEqual(TEXT("GetItemId() returns authoritative ContentId"), ItemDef->GetItemId(), Id1);

	// 3. Set via SetItemId() -> ContentId must be updated
	const FName Id2(TEXT("Item_Authoritative_02"));
	ItemDef->SetItemId(Id2);
	TestEqual(TEXT("ContentId reflects SetItemId()"), ItemDef->ContentId, Id2);
	TestEqual(TEXT("GetItemId() reflects SetItemId()"), ItemDef->GetItemId(), Id2);

	// 4. Verification that GetPrimaryAssetId() uses authoritative ContentId
	const FPrimaryAssetId ExpectedAssetId(TEXT("Item"), Id2);
	TestEqual(TEXT("GetPrimaryAssetId() uses ContentId"), ItemDef->GetPrimaryAssetId(), ExpectedAssetId);

	return true;
}

// 4. DefinitionValidation Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveItemDefinitionValidationTest,
	"ShadowSlave.ItemDefinition.DefinitionValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveItemDefinitionValidationTest::RunTest(const FString& Parameters)
{
	UShadowSlaveItemDefinition* ValidDef = NewObject<UShadowSlaveItemDefinition>();
	ValidDef->ContentId = FName(TEXT("Test_Valid_Item"));
	ValidDef->DisplayName = FText::FromString(TEXT("Valid Test Item"));
	ValidDef->Version = 1;
	ValidDef->ContentType = EShadowSlaveContentType::Item;
	ValidDef->bIsStackable = true;
	ValidDef->MaxStackSize = 10;
	ValidDef->Weight = 0.5f;
	ValidDef->BaseValue = 25;

	FString ErrorMsg;

	// 1. Valid definition passes
	TestTrue(TEXT("Valid item definition must pass IsValidDefinition"), ValidDef->IsValidDefinition(&ErrorMsg));
	TestTrue(TEXT("Valid item definition must pass ValidateDefinition"), ValidDef->ValidateDefinition(ErrorMsg));

	// 2. Generic validation: ContentId None fails
	ValidDef->ContentId = NAME_None;
	TestFalse(TEXT("None ContentId must fail IsValidDefinition"), ValidDef->IsValidDefinition(&ErrorMsg));
	TestFalse(TEXT("Error message must not be empty on failure"), ErrorMsg.IsEmpty());
	ValidDef->ContentId = FName(TEXT("Test_Valid_Item"));

	// 3. Generic validation: Empty DisplayName fails
	ValidDef->DisplayName = FText::GetEmpty();
	TestFalse(TEXT("Empty DisplayName must fail IsValidDefinition"), ValidDef->IsValidDefinition(&ErrorMsg));
	ValidDef->DisplayName = FText::FromString(TEXT("Valid Test Item"));

	// 4. Generic validation: Version < 1 fails
	ValidDef->Version = 0;
	TestFalse(TEXT("Version 0 must fail IsValidDefinition"), ValidDef->IsValidDefinition(&ErrorMsg));
	ValidDef->Version = 1;

	// 5. Generic validation: Wrong ContentType fails
	ValidDef->ContentType = EShadowSlaveContentType::Memory;
	TestFalse(TEXT("Wrong ContentType must fail IsValidDefinition"), ValidDef->IsValidDefinition(&ErrorMsg));
	ValidDef->ContentType = EShadowSlaveContentType::Item;

	// 6. Item-specific validation: Stackable with MaxStackSize < 1 fails
	ValidDef->bIsStackable = true;
	ValidDef->MaxStackSize = 0;
	TestFalse(TEXT("Stackable with MaxStackSize 0 must fail IsValidDefinition"), ValidDef->IsValidDefinition(&ErrorMsg));
	ValidDef->MaxStackSize = 10;

	// Non-stackable with MaxStackSize 1 passes
	ValidDef->bIsStackable = false;
	ValidDef->MaxStackSize = 1;
	TestTrue(TEXT("Non-stackable with MaxStackSize 1 passes"), ValidDef->IsValidDefinition(&ErrorMsg));

	// 7. Item-specific validation: Negative Weight fails
	ValidDef->Weight = -0.1f;
	TestFalse(TEXT("Negative Weight must fail IsValidDefinition"), ValidDef->IsValidDefinition(&ErrorMsg));
	ValidDef->Weight = 0.5f;

	// 8. Item-specific validation: Negative BaseValue fails
	ValidDef->BaseValue = -10;
	TestFalse(TEXT("Negative BaseValue must fail IsValidDefinition"), ValidDef->IsValidDefinition(&ErrorMsg));
	ValidDef->BaseValue = 25;

	// Restored definition passes again
	TestTrue(TEXT("Restored definition passes IsValidDefinition"), ValidDef->IsValidDefinition(&ErrorMsg));

	return true;
}

// 5. DefinitionRegistryIntegration Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveItemDefinitionRegistryIntegrationTest,
	"ShadowSlave.ItemDefinition.DefinitionRegistryIntegration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveItemDefinitionRegistryIntegrationTest::RunTest(const FString& Parameters)
{
	UShadowSlaveContentRegistrySubsystem* Registry = NewObject<UShadowSlaveContentRegistrySubsystem>();
	TestNotNull(TEXT("Registry subsystem must be created"), Registry);

	UShadowSlaveItemDefinition* ItemDef = NewObject<UShadowSlaveItemDefinition>();
	ItemDef->ContentId = FName(TEXT("Item_Registry_Test"));
	ItemDef->DisplayName = FText::FromString(TEXT("Registry Test Item"));
	ItemDef->Version = 1;
	ItemDef->ItemType = EShadowSlaveItemType::Consumable;

	// 1. Register with generic registry subsystem
	const bool bRegistered = Registry->RegisterDefinition(ItemDef);
	TestTrue(TEXT("RegisterDefinition must succeed for UShadowSlaveItemDefinition"), bRegistered);

	// 2. Query existence & count
	TestTrue(TEXT("HasContent must return true for registered ItemDef"), Registry->HasContent(ItemDef->ContentId));
	TestEqual(TEXT("Total registered count must be 1"), Registry->GetRegisteredContentCount(), 1);
	TestEqual(TEXT("Item count by type must be 1"), Registry->GetRegisteredContentCountByType(EShadowSlaveContentType::Item), 1);
	TestEqual(TEXT("Memory count by type must be 0"), Registry->GetRegisteredContentCountByType(EShadowSlaveContentType::Memory), 0);
	TestEqual(TEXT("Echo count by type must be 0"), Registry->GetRegisteredContentCountByType(EShadowSlaveContentType::Echo), 0);
	TestEqual(TEXT("Story count by type must be 0"), Registry->GetRegisteredContentCountByType(EShadowSlaveContentType::Story), 0);
	TestEqual(TEXT("Custom count by type must be 0"), Registry->GetRegisteredContentCountByType(EShadowSlaveContentType::Custom), 0);

	// 3. Generic resolution
	UShadowSlaveContentDefinition* ResolvedGeneric = Registry->ResolveContentDefinition(ItemDef->ContentId);
	TestNotNull(TEXT("Resolved generic definition must not be null"), ResolvedGeneric);
	TestEqual(TEXT("Resolved generic definition must match ItemDef"), ResolvedGeneric, Cast<UShadowSlaveContentDefinition>(ItemDef));

	// 4. Typed resolution
	UShadowSlaveItemDefinition* ResolvedItem = Registry->ResolveContentDefinition<UShadowSlaveItemDefinition>(ItemDef->ContentId);
	TestNotNull(TEXT("Resolved typed item definition must not be null"), ResolvedItem);
	TestEqual(TEXT("Resolved typed item must match original ItemDef"), ResolvedItem, ItemDef);
	TestEqual(TEXT("Resolved ItemType matches"), ResolvedItem->ItemType, EShadowSlaveItemType::Consumable);

	// 5. PrimaryAssetId verification
	const FPrimaryAssetId ExpectedAssetId(TEXT("Item"), ItemDef->ContentId);
	TestEqual(TEXT("GetPrimaryAssetId must match expected PrimaryAssetId"), ItemDef->GetPrimaryAssetId(), ExpectedAssetId);

	return true;
}

// 6. ExistingRuntimeCompatibility Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShadowSlaveItemDefinitionExistingRuntimeCompatibilityTest,
	"ShadowSlave.ItemDefinition.ExistingRuntimeCompatibility",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FShadowSlaveItemDefinitionExistingRuntimeCompatibilityTest::RunTest(const FString& Parameters)
{
	UShadowSlaveInventoryComponent* InvComp = NewObject<UShadowSlaveInventoryComponent>();
	TestNotNull(TEXT("InventoryComponent must instantiate"), InvComp);
	InvComp->SetCapacity(5);

	// 1. Create item definitions using compatibility accessors and verify runtime behavior
	UShadowSlaveItemDefinition* StackDef = NewObject<UShadowSlaveItemDefinition>();
	StackDef->SetItemId(FName(TEXT("Compat_Stackable_Draught")));
	StackDef->DisplayName = FText::FromString(TEXT("Compatibility Draught"));
	StackDef->Version = 1;
	StackDef->ItemType = EShadowSlaveItemType::Consumable;
	StackDef->bIsStackable = true;
	StackDef->MaxStackSize = 5;
	StackDef->Weight = 0.2f;
	StackDef->BaseValue = 20;

	// Add items and verify stacking
	int32 Remainder = 0;
	TestTrue(TEXT("AddItem initial quantity 3 succeeds"), InvComp->AddItem(StackDef, 3, Remainder));
	TestEqual(TEXT("Remainder is 0"), Remainder, 0);
	TestEqual(TEXT("Used slots is 1"), InvComp->GetUsedSlotCount(), 1);
	TestEqual(TEXT("Total item count is 3"), InvComp->GetTotalItemCount(StackDef), 3);

	// Add more into the same stack
	TestTrue(TEXT("AddItem additional quantity 2 succeeds"), InvComp->AddItem(StackDef, 2, Remainder));
	TestEqual(TEXT("Remainder is 0"), Remainder, 0);
	TestEqual(TEXT("Used slots remains 1 after stacking"), InvComp->GetUsedSlotCount(), 1);
	TestEqual(TEXT("Total item count is 5"), InvComp->GetTotalItemCount(StackDef), 5);

	// Atomic removal
	TestFalse(TEXT("RemoveItem for 10 units fails atomically"), InvComp->RemoveItem(StackDef, 10));
	TestEqual(TEXT("Total item count unchanged after failed removal"), InvComp->GetTotalItemCount(StackDef), 5);

	TestTrue(TEXT("RemoveItem for 2 units succeeds"), InvComp->RemoveItem(StackDef, 2));
	TestEqual(TEXT("Total item count is 3"), InvComp->GetTotalItemCount(StackDef), 3);

	// 2. Equipment / Granted Modifiers compatibility
	UShadowSlaveItemDefinition* EquipDef = NewObject<UShadowSlaveItemDefinition>();
	EquipDef->SetItemId(FName(TEXT("Compat_Equip_Sword")));
	EquipDef->DisplayName = FText::FromString(TEXT("Compatibility Sword"));
	EquipDef->Version = 1;
	EquipDef->ItemType = EShadowSlaveItemType::Equipment;
	EquipDef->EquipmentSlot = EShadowSlaveEquipmentSlot::Weapon;

	FAttributeModifier AttackMod;
	AttackMod.AttributeName = FName(TEXT("AttackPower"));
	AttackMod.ModifierType = EAttributeModifierType::Flat;
	AttackMod.Value = 15.0f;
	EquipDef->GrantedModifiers.Add(AttackMod);

	TestEqual(TEXT("GrantedModifiers count is 1"), EquipDef->GrantedModifiers.Num(), 1);
	TestEqual(TEXT("Granted modifier attribute matches"), EquipDef->GrantedModifiers[0].AttributeName, FName(TEXT("AttackPower")));
	TestEqual(TEXT("Granted modifier value matches"), EquipDef->GrantedModifiers[0].Value, 15.0f);

	// 3. Save/Load capture and snapshot compatibility
	UShadowSlaveSaveSubsystem* SaveSub = NewObject<UShadowSlaveSaveSubsystem>();
	TestNotNull(TEXT("SaveSubsystem must instantiate"), SaveSub);

	FShadowSlaveInventorySaveData SaveData;
	SaveSub->CaptureInventory(InvComp, SaveData);
	TestTrue(TEXT("CaptureInventory produces valid save data"), SaveData.bIsValid);
	TestEqual(TEXT("Saved items count is 1"), SaveData.Items.Num(), 1);
	TestEqual(TEXT("Saved item quantity is 3"), SaveData.Items[0].Quantity, 3);
	TestEqual(TEXT("Saved item PrimaryAssetId Type is Item"),
		SaveData.Items[0].ItemPrimaryAssetId.PrimaryAssetType, FPrimaryAssetType(TEXT("Item")));

	// 4. Test factory items compatibility
	UShadowSlaveItemDefinition* FactoryConsumable = UShadowSlaveItemDefinition::CreateTestConsumableDefinition();
	TestNotNull(TEXT("CreateTestConsumableDefinition returns valid object"), FactoryConsumable);
	TestEqual(TEXT("Factory consumable ContentId is TestConsumableItem"),
		FactoryConsumable->ContentId, FName(TEXT("TestConsumableItem")));
	TestEqual(TEXT("Factory consumable GetItemId() is TestConsumableItem"),
		FactoryConsumable->GetItemId(), FName(TEXT("TestConsumableItem")));
	TestTrue(TEXT("Factory consumable passes IsValidDefinition"), FactoryConsumable->IsValidDefinition());

	UShadowSlaveItemDefinition* FactoryQuest = UShadowSlaveItemDefinition::CreateTestQuestItemDefinition();
	TestNotNull(TEXT("CreateTestQuestItemDefinition returns valid object"), FactoryQuest);
	TestEqual(TEXT("Factory quest ContentId is TestQuestItem"),
		FactoryQuest->ContentId, FName(TEXT("TestQuestItem")));
	TestEqual(TEXT("Factory quest GetItemId() is TestQuestItem"),
		FactoryQuest->GetItemId(), FName(TEXT("TestQuestItem")));
	TestTrue(TEXT("Factory quest passes IsValidDefinition"), FactoryQuest->IsValidDefinition());

	return true;
}

#endif // WITH_AUTOMATION_TESTS
