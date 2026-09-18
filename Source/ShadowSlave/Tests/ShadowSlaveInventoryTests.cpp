// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_AUTOMATION_TESTS

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Items/ShadowSlaveInventoryComponent.h"
#include "Items/ShadowSlaveItemDefinition.h"
#include "Items/ShadowSlaveItemTypes.h"

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

#endif // WITH_AUTOMATION_TESTS
