# Shadow Slave — Automation Testing Foundation

This directory contains the initial Unreal Engine 5 Automation Testing suite for the **Shadow Slave** C++ project.

## 1. Test Directory Layout
```
Source/ShadowSlave/Tests/
├── ShadowSlaveCombatTests.cpp        # Combat state machine, transitions, death terminality, attack/dodge validation
├── ShadowSlaveInventoryTests.cpp     # Inventory capacity, stacking, atomic removal, GUID lookup, clearing, and Item definition content pipeline integration
├── ShadowSlaveQuestTests.cpp         # Quest registration, duplicate rejection, prerequisites, objective progress invariants, and Quest definition content pipeline integration
├── ShadowSlaveStoryTests.cpp         # Story definition and story content save restore, parent/child reconciliation, prerequisite validation, and content pipeline integration
├── ShadowSlaveDialogueTests.cpp      # Conversation consequences, rank conditions, passive dialogue save/restore guards, and Dialogue definition content pipeline integration
├── ShadowSlaveAspectTests.cpp        # Generic Aspect ability activation, resource validation, transient state behavior, and Ability/Flaw definition content pipeline integration
├── ShadowSlaveEchoTests.cpp          # Generic Echo acquisition, summoning/dismissal lifecycle, transient actor representation, duplicate summon rejection, destruction, save/load, reentrancy rejection, teardown safety, and content pipeline integration
├── ShadowSlaveStatusEffectTests.cpp  # Generic status effect application, stacking, duration, cleanup, save/load (with remaining duration and source attribution), authority boundaries, reentrancy rejection, expired-effect-not-restored verification, and content pipeline integration
├── ShadowSlaveGameplayTagTests.cpp   # Native Gameplay Tag container queries, mutation, hierarchical matching, and decoupled architecture
├── ShadowSlaveContentRegistryTests.cpp # Generic content data asset registration, soft reference resolution, duplicate rejection, and decoupled architecture
├── ShadowSlaveMemoryTests.cpp        # Memory definition content pipeline integration, inheritance, single authoritative ID, validation, and runtime compatibility
├── ShadowSlaveGameplayFlowTests.cpp  # High-level gameplay flow state transitions, idempotency, transactional requests, runtime progression bootstrap
├── ShadowSlaveNightmareTests.cpp     # Nightmare Scenario definition content pipeline integration, inheritance, validation, and session lifecycle
└── README.md                         # Documentation and discovery guide
```

## 2. Test Naming Convention
All automation tests use structured, hierarchical namespaces:
- `ShadowSlave.Combat.*`
- `ShadowSlave.Inventory.*`
- `ShadowSlave.ItemDefinition.*`
- `ShadowSlave.Quest.*`
- `ShadowSlave.QuestDefinition.*`
- `ShadowSlave.Story.*`
- `ShadowSlave.StoryContent.*`
- `ShadowSlave.StoryDefinition.*`
- `ShadowSlave.Dialogue.*`
- `ShadowSlave.DialogueDefinition.*`
- `ShadowSlave.NightmareScenarioDefinition.*`
- `ShadowSlave.Aspects.*`
- `ShadowSlave.AbilityDefinition.*`
- `ShadowSlave.FlawDefinition.*`
- `ShadowSlave.Echoes.*`
- `ShadowSlave.Echo.*`
- `ShadowSlave.StatusEffects.*`
- `ShadowSlave.StatusEffect.*`
- `ShadowSlave.GameplayTags.*`
- `ShadowSlave.ContentRegistry.*`
- `ShadowSlave.Memory.*`
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

## 6. Story Definition vs. Story Content Architecture
- **StoryDefinition (`UShadowSlaveStoryDefinition`)**: Static Story-level content defining chapters, progression units, prerequisites, and ordered step IDs.
- **StoryContentDefinition (`UShadowSlaveStoryContentDefinition`)**: Static content-entry/graph data modeling child content entries (Quests, Dialogues, Nightmares, etc.) within a story progression.
- **Generic Pipeline Integration**: Both derive from `UShadowSlaveContentDefinition` with `ContentId` as the stable authoritative identifier and `ContentType = EShadowSlaveContentType::Story`.
- **Runtime Separation**: `UShadowSlaveStorySubsystem` owns all mutable runtime progression states; `UShadowSlaveContentRegistrySubsystem` is used solely for static definition resolution and query.
