# Shadow Slave — Automation Testing Foundation

This directory contains the initial Unreal Engine 5 Automation Testing suite for the **Shadow Slave** C++ project.

## 1. Test Directory Layout
```
Source/ShadowSlave/Tests/
├── ShadowSlaveCombatTests.cpp        # Combat state machine, transitions, death terminality, attack/dodge validation
├── ShadowSlaveInventoryTests.cpp     # Inventory capacity, stacking, atomic removal, GUID lookup, clearing
├── ShadowSlaveQuestTests.cpp         # Quest registration, duplicate rejection, prerequisites, objective progress invariants
├── ShadowSlaveStoryTests.cpp         # Story content save restore, parent/child reconciliation, prerequisite validation
├── ShadowSlaveDialogueTests.cpp      # Conversation consequences, rank conditions, and passive dialogue save/restore guards
├── ShadowSlaveAspectTests.cpp        # Generic Aspect ability activation, resource validation, and transient state behavior
├── ShadowSlaveEchoTests.cpp          # Generic Echo acquisition, summoning/dismissal lifecycle, transient actor representation, duplicate summon rejection, destruction, save/load, reentrancy rejection, and teardown safety
├── ShadowSlaveStatusEffectTests.cpp  # Generic status effect application, stacking, duration, cleanup, save/load (with remaining duration and source attribution), authority boundaries, reentrancy rejection, and expired-effect-not-restored verification
├── ShadowSlaveGameplayTagTests.cpp   # Native Gameplay Tag container queries, mutation, hierarchical matching, and decoupled architecture
├── ShadowSlaveContentRegistryTests.cpp # Generic content data asset registration, soft reference resolution, duplicate rejection, and decoupled architecture
├── ShadowSlaveGameplayFlowTests.cpp  # High-level gameplay flow state transitions, idempotency, transactional requests, runtime progression bootstrap
└── README.md                         # Documentation and discovery guide
```

## 2. Test Naming Convention
All automation tests use structured, hierarchical namespaces:
- `ShadowSlave.Combat.*`
- `ShadowSlave.Inventory.*`
- `ShadowSlave.Quest.*`
- `ShadowSlave.Story.*`
- `ShadowSlave.Dialogue.*`
- `ShadowSlave.Aspects.*`
- `ShadowSlave.Echoes.*`
- `ShadowSlave.StatusEffects.*`
- `ShadowSlave.GameplayTags.*`
- `ShadowSlave.ContentRegistry.*`
- `ShadowSlave.Gameplay.*`

Test names are descriptive and describe specific invariant contracts (e.g. `ShadowSlave.Quest.MissingPrerequisitesFailClosed`, `ShadowSlave.Story.RestoreReconcilesActiveParentWithFailedEntry`).

## 3. Framework & Compilation Guards
- Tests use Unreal Engine's native automation framework (`Misc/AutomationTest.h`).
- Declared with `IMPLEMENT_SIMPLE_AUTOMATION_TEST`.
- Configured with `EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter`.
- Guarded by `#if WITH_AUTOMATION_TESTS ... #endif` to ensure clean separation from shipping game targets.

## 4. How to Discover and Run in UE5

### A. Within Unreal Editor (GUI)
1. Open the project in Unreal Editor.
2. Open the **Test Automation** window: `Tools` -> `Test Automation` (or `Window` -> `Developer Tools` -> `Session Frontend` -> `Automation`).
3. In the test tree, expand the **ShadowSlave** category.
4. Select the desired test suites or individual tests.
5. Click **Start Tests**.

### B. Command-Line / Headless Execution
To execute tests headlessly via command line:
```bash
<UE5_PATH>/Engine/Binaries/Linux/UnrealEditor-Cmd \
  "<PROJECT_PATH>/ShadowSlave.uproject" \
  -ExecCmds="Automation RunTests ShadowSlave; Quit" \
  -nullrhi \
  -unattended \
  -nopause \
  -nosplash \
  -log \
  -ReportExportPath="<PROJECT_PATH>/Saved/Automation/Reports" \
  -testexit="Automation Test Results Exit Code"
```
*(On Windows, replace `UnrealEditor-Cmd` with `UnrealEditor-Cmd.exe`)*.

## 5. Important Environment & Execution Notice
- **Current Environment Status:** The agent development environment is a headless Linux container without an installed Unreal Engine 5 toolchain or C++ compiler.
- **Execution Status:** Execution has **NOT** been verified in this container. Static code inspection and API verification have been performed, but actual runtime execution requires a local Unreal Engine 5.4 environment.
