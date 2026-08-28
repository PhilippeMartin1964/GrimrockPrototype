# GEUI10 — Plugin / Player Level Editor Readiness Audit

**Date:** 28 August 2026  
**Status:** architecture audit complete  
**Implementation impact:** documentation only — no C++ refactor, no plugin creation, no asset/map modification

## 1. Objective

GEUI10 closes the Grid Editor workspace refactor by answering one long-term architectural question:

> How much of the current Grimrock Grid Editor can eventually support player-authored dungeons in a packaged game, and what must be separated before that becomes safe and maintainable?

This milestone is intentionally an **audit**, not an implementation milestone.

The immediate project objective remains the Unreal Editor authoring workflow. A packaged player-facing editor is a future capability.

GEUI10 therefore does **not**:

- move classes between modules;
- create a plugin;
- create a runtime editor UI;
- introduce a new save/package format;
- refactor `AGridLevelEditorActor`;
- change the existing DataAssets;
- change runtime gameplay;
- modify `.uasset` or `.umap`;
- open MON21.4.

## 2. Executive conclusion

The project is in a favorable position for future player-authored dungeons because the most important architectural decision was already made correctly:

~~~text
level topology and gameplay data
            ↓
UGridLevelAsset / UGridDungeonAsset
            ↓
AGridLevelRuntimeActor
~~~

The Unreal map is not the dungeon source of truth.

The current architecture already separates:

~~~text
Runtime data/gameplay
    GrimrockPrototype
    GrimrockLua

Unreal Editor authoring
    GrimrockPrototypeEditor
~~~

That means a future player editor does **not** require rewriting the dungeon runtime.

However, the current Unreal Grid Editor itself cannot simply be shipped.

The future packaged editor needs a third conceptual layer:

~~~text
Developer Unreal Editor
        ↓
GrimrockPrototypeEditor
        ↓
        ┌───────────────────────────┐
        │ future Authoring Core     │
        │ data mutation/validation  │
        └───────────────────────────┘
                   ↓
       GrimrockPrototype Runtime
                   ↑
        ┌───────────────────────────┐
        │ future Player Editor UI   │
        │ packaged runtime UI/input │
        └───────────────────────────┘
~~~

The recommendation is therefore:

> **Do not convert GrimrockPrototypeEditor into a runtime module.**

Instead, when player authoring becomes an active milestone, extract only the data-oriented authoring rules into a small runtime-compatible authoring layer and build a dedicated packaged UI around it.

## 3. Current module topology

The current `.uproject` declares:

~~~text
GrimrockLua              Runtime
GrimrockPrototype        Runtime
GrimrockPrototypeEditor  Editor
~~~

The game target includes:

~~~text
GrimrockPrototype
~~~

The editor target includes:

~~~text
GrimrockPrototype
GrimrockPrototypeEditor
~~~

This direction is correct:

~~~text
GrimrockPrototypeEditor
          ↓
GrimrockPrototype
          ↓
GrimrockLua
~~~

The runtime does not depend on `GrimrockPrototypeEditor`.

### 3.1 Current runtime dependencies

The current `GrimrockPrototype.Build.cs` contains:

~~~text
Public:
Core
CoreUObject
Engine
InputCore
EnhancedInput
UMG
Niagara
GrimrockLua

Private:
Slate
SlateCore
~~~

This is newer than the historical snapshot in `docs/Architecture_Runtime_Editor_Split.md`, which predates the current runtime UI work.

The presence of runtime `Slate/SlateCore` is not itself a player-editor blocker: those dependencies are used by packaged game UI code and are legal runtime dependencies.

The important forbidden direction remains:

~~~text
GrimrockPrototype -> UnrealEd
GrimrockPrototype -> GrimrockPrototypeEditor
~~~

No such module dependency is currently declared.

### 3.2 Current editor dependencies

`GrimrockPrototypeEditor` owns the expected editor-only dependencies:

~~~text
UnrealEd
EditorFramework
PropertyEditor
ToolMenus
Slate
SlateCore
ApplicationCore
AssetRegistry
~~~

This module is therefore correctly excluded from a packaged game target.

## 4. Readiness matrix

| Area | Current readiness for future player editor | Assessment |
|---|---:|---|
| Grid/cell data model | High | Runtime data already independent of Unreal Editor. |
| Placed object model | High | `FGridLevelObjectData` is runtime and data-driven. |
| Event -> Command links | High | Persistent link model is runtime. |
| Multi-level dungeon model | High | `UGridDungeonAsset` already provides stable level identity and logical position. |
| Runtime reconstruction | High | `AGridLevelRuntimeActor` consumes level data directly. |
| Object archetypes | High | Runtime DataAssets already define placement/render/runtime behavior. |
| Palette metadata | Medium/High | Runtime DataAsset; useful for authoring, but player exposure must be allowlisted. |
| Lua runtime sandbox | High | Dedicated runtime module with hard limits and host-controlled API. |
| Level mutation primitives | Medium | Some are already runtime; much higher-level authoring mutation remains in editor actor/services. |
| Validation | Medium | Many pure rules exist, but complete editor validation is currently editor-module-owned. |
| Runtime authoring persistence | Low | No player dungeon package/document serializer currently exists. |
| Runtime undo/redo | Low | Current workflow relies on Unreal Editor transactions/authoring semantics. |
| Runtime picking/editor camera | Low | Current interaction uses EdMode / editor viewport infrastructure. |
| Runtime authoring UI | Low | Current Slate workspace is editor-only by design. |
| Player content security | Medium | Lua sandbox is strong, but asset/archetype/package allowlisting is not yet a complete player-content boundary. |
| Plugin packaging | Not required now | A plugin is optional organization, not a prerequisite. |

## 5. Already reusable without architectural extraction

### 5.1 Core grid types

The following are already runtime types and are strong future-authoring foundations:

~~~text
EGridCellType
EGridWallType
EGridEdge
FGridLevelCellData
FGridLevelObjectData
FGridObjectLink
EGridObjectEvent
EGridObjectCommand
EGridObjectCondition
FGridObjectBehaviorParams
~~~

They contain the actual dungeon language rather than Unreal Editor UI state.

This is the most important positive finding in GEUI10.

### 5.2 UGridLevelAsset

`UGridLevelAsset` already contains:

- width / height / cell size;
- cell topology;
- walls;
- ceilings and occupancy;
- player start;
- placed objects;
- Event -> Command links;
- quest definitions;
- persistent level variable definitions;
- Lua source scripts.

It also exposes runtime-compatible mutation/helpers such as:

~~~text
EnsureCellCount
ClearLevel
AddObject
RemoveObjectById
RemoveLinksForObject
EnsureObjectIds
~~~

The `Modify()` and `MarkPackageDirty()` calls in some of these methods are guarded by:

~~~cpp
#if WITH_EDITOR
~~~

Therefore those basic mutations are already callable in a runtime build without introducing `UnrealEd`.

Important qualification:

> Runtime mutability is **not the same as packaged persistence**.

A player can modify an in-memory UObject, but a cooked `UDataAsset` is not a suitable writable player-level file format.

That persistence gap is the largest future blocker.

### 5.3 UGridDungeonAsset

`UGridDungeonAsset` already supplies:

~~~text
DungeonName
Author
Version
DefaultLevelId
Levels[]
    LevelId
    DisplayName
    LevelAsset
    LogicalPosition
    bEnabled
~~~

The concepts themselves are directly suitable for player-created dungeon packages.

The UObject reference representation is developer-asset-oriented, but the identity model is sound.

### 5.4 Object archetypes

`UGridObjectArchetypeAsset` is already runtime and contains the central data required by both authoring and execution:

- gameplay type;
- category;
- placement kind;
- cell/anchor sharing policy;
- wall replacement;
- movement blocking;
- interaction/readable/light settings;
- meshes/materials;
- runtime actor class;
- placement transforms;
- default behavior.

This gives a future player editor a very strong curated-content model:

> Players should choose an allowed `ArchetypeId`; they should not author arbitrary Unreal classes.

### 5.5 Object palette

`UGridObjectPaletteAsset` is also runtime.

Its entries already provide:

~~~text
EntryId
DisplayNameOverride
CategoryOverride
Icon
DefaultArchetype
DefaultMonsterDefinition
DefaultStoryCompanionDefinition
~~~

The current developer palette can therefore conceptually become the source for a future player-facing palette.

The player editor should expose a curated subset rather than allowing arbitrary asset browsing.

### 5.6 AGridLevelRuntimeActor

The runtime actor already reconstructs and executes the level from `UGridLevelAsset`.

Its responsibilities include:

- floor / wall / ceiling generation;
- runtime object spawning;
- object placement transforms;
- doors and interaction;
- item pickup/drop;
- monster spawn/runtime state;
- encounters;
- triggers;
- links;
- transitions;
- multi-level runtime state;
- Lua bridge through activation/runtime systems.

This means the future player editor can use the **same runtime renderer/gameplay consumer** for live preview.

No second dungeon renderer should be created.

### 5.7 GrimrockLua

`GrimrockLua` is a dedicated Runtime module depending only on:

~~~text
Core
CoreUObject
~~~

Its `FGridLuaVm` already provides hard non-level-controlled caps:

~~~text
HardMaxScriptCount = 64
HardMaxSourceBytesPerScript = 256 KiB
HardMaxTotalSourceBytes = 1 MiB
MemoryLimitBytes
InstructionBudgetPerCall
~~~

The host API is callback-based and intentionally knows nothing about:

~~~text
UWorld
Actors
gameplay enums
filesystem
network
OS APIs
~~~

This is a very strong foundation for future player-authored scripted mechanisms.

## 6. Strictly Unreal-Editor-only implementation

The following should remain editor-only.

### 6.1 FGridLevelEdMode

Depends on Unreal editor viewport/tool infrastructure:

~~~text
FEdMode
FEditorViewportClient
FViewport
GEditor
FEditorModeRegistry
~~~

A packaged editor must implement its own camera/input/picking path.

### 6.2 FGridLevelEdModeToolkit

Uses editor toolkit infrastructure and global Nomad tabs.

It should not be migrated to runtime.

The GEUI01–09 workspace UX can inspire the player editor, but the Slate toolkit implementation itself is not the reusable layer.

### 6.3 Workspace Nomad tabs

The following are developer-editor presentation:

~~~text
Dungeon Levels
PlayTest & Validation
Tools & Palette
Selected Object
Grimrock Lua Scripts
~~~

They depend on:

~~~text
FGlobalTabmanager
SDockTab
SWindow
editor mode lifetime
Window menu integration
~~~

A player editor needs an in-game workspace/layout model instead.

### 6.4 AGridLevelEditorActor

`AGridLevelEditorActor` is now correctly located in `GrimrockPrototypeEditor`.

It combines several developer-editor concerns:

- current selection;
- hovered selection;
- viewport painting;
- object placement UI state;
- link authoring state;
- preview coordination;
- editor diagnostics;
- PIE preparation;
- transaction-aware mutations;
- selection focus;
- patrol-route editor interaction.

It must **not** be moved wholesale back into the runtime module.

Future extraction should take data-oriented functions out of it one capability at a time.

### 6.5 PIE workflow

~~~text
GridPIEPlaytestRequest
PreBeginPIE
BeginPIE
Debug Prepare PIE
Auto Prepare PIE
~~~

are developer-editor workflows.

A packaged player editor should instead have a direct:

~~~text
Edit Mode
   ↓
Playtest Mode
   ↓
return to Edit Mode
~~~

using the runtime world, not PIE duplication.

## 7. High-value future extraction candidates

These are currently editor-owned but contain logic that a packaged authoring workflow will eventually need.

They should **not** be extracted during GEUI10.

### 7.1 Link policy

Current:

~~~text
GridEditorLinkPolicy
~~~

It determines:

- which objects emit events;
- which targets receive commands;
- supported source events;
- supported target commands;
- supported conditions;
- runtime command support classification;
- exact link identity.

Most of this is pure data policy and does not fundamentally require Unreal Editor.

Future target:

~~~text
GridAuthoringLinkPolicy
~~~

inside a runtime-compatible authoring module/core.

### 7.2 Link mutation service

Current:

~~~text
GridEditorLinkService
~~~

Its low-level functions are already nearly pure:

~~~text
NormalizeLink
IsConditionConfigurationValid
IsLinkSupported
ContainsExactLink
AddExactLink
RemoveExactLink
~~~

The overloads that accept `AGridLevelEditorActor` are editor bridges.

Future extraction should separate:

~~~text
pure link model operations
from
editor transaction/selection bridge
~~~

### 7.3 Lua authoring service

Current:

~~~text
GridEditorLuaService
~~~

Mixed responsibilities include:

- script analysis;
- callback discovery;
- script validation;
- script identity;
- script source mutation;
- persistent declaration synchronization;
- selected-object LogicId mutation;
- full editor level validation.

The VM itself is already runtime.

A future player editor will need a runtime-safe authoring facade for:

~~~text
AnalyzeLevel
ValidateScriptDefinitions
GetCallbacksForScript
Add/Rename/Remove script
SetScriptEnabled
SetScriptSource
~~~

Operations tied to `AGridLevelEditorActor` should remain adapter code.

### 7.4 Level validation

The complete validation presentation currently uses:

~~~text
FGridLevelValidationMessage
EGridLevelValidationSeverity
~~~

declared in the editor actor header.

For player authoring, a neutral validation contract should eventually move out of the editor module, for example:

~~~text
EGridLevelValidationSeverity
FGridLevelValidationIssue
FGridLevelValidationResult
GridLevelValidationService
~~~

The Unreal editor panel and future player editor would both consume the same result.

This is a high-value extraction because invalid player packages must be rejected before play/export.

### 7.5 Level editing mutations

The future reusable authoring core will need explicit operations such as:

~~~text
PaintCell
PaintWall
EraseWall
PlaceObject
RemoveObject
MoveObject
SetObjectProperty
AddLink
RemoveLink
SetStartCell
AddDungeonLevel
RemoveDungeonLevel
RenameDungeonLevel
~~~

Today many of these workflows are embodied in `AGridLevelEditorActor`.

Do not expose the actor itself as the future API.

Instead, extract a command/service layer only when player editing becomes an active milestone.

## 8. Main blocker: player dungeon persistence format

The current source of truth is a `UDataAsset`.

That is ideal for developer-authored cooked content.

It is not sufficient as the file format for player-created levels in a packaged build.

### 8.1 What must not be done

Do not design the player editor around writing modified cooked `.uasset` files.

Do not make player content depend on Unreal Editor asset creation APIs.

Do not serialize arbitrary UObject graphs from untrusted player content.

### 8.2 Recommended future model

Introduce a versioned, engine-controlled player dungeon document, conceptually:

~~~text
FGridPlayerDungeonDocument
    SchemaVersion
    DungeonId
    DungeonName
    Author
    GameContentVersion
    DefaultLevelId
    Levels[]

FGridPlayerLevelDocument
    LevelId
    DisplayName
    LogicalPosition
    Width
    Height
    CellSize
    Cells[]
    Objects[]
    Links[]
    LevelVariables[]
    LuaScripts[]
~~~

This document should contain stable identifiers rather than arbitrary UObject references wherever possible.

Examples:

~~~text
ArchetypeId
ItemDefinitionId
MonsterDefinitionId
ReadableContentId
QuestId
StoryCompanionId
~~~

### 8.3 Runtime materialization

Recommended flow:

~~~text
player package
     ↓ deserialize + schema validation
FGridPlayerDungeonDocument
     ↓ resolve allowed developer content IDs
transient UGridDungeonAsset / UGridLevelAsset
     ↓
AGridLevelRuntimeActor
~~~

A transient UObject representation allows the existing runtime to continue consuming its current model.

### 8.4 Serialization choice

The audit does not mandate JSON, binary or SaveGame.

Required properties are more important than the container:

- explicit schema version;
- deterministic field ownership;
- bounds/size limits;
- safe failure on unknown versions;
- stable IDs;
- no arbitrary class loading;
- round-trip tests;
- optional human-readable format if sharing/modding benefits from it.

## 9. Asset resolution and cooking

Player-authored content can only reference content that exists in the packaged build unless a separate trusted content distribution system is later created.

Therefore a player editor needs a curated authoring catalog.

Recommended rule:

~~~text
player document
    stores ArchetypeId
        ↓
runtime authoring catalog
        ↓
known cooked UGridObjectArchetypeAsset
~~~

The same principle applies to:

- items;
- monsters;
- readables;
- quests;
- story companions;
- icons;
- meshes/materials indirectly referenced by archetypes.

### 9.1 RuntimeActorClass safety

`UGridObjectArchetypeAsset` contains:

~~~text
RuntimeActorClass
ItemActorClass
~~~

This is acceptable because archetypes are developer-controlled cooked content.

Player packages should **not** specify arbitrary class paths.

They should specify only an allowed archetype/definition ID.

## 10. Player-content validation boundary

A future player dungeon must pass validation before:

~~~text
Save
Export
Publish
Playtest
Play
~~~

At minimum validation must cover:

### Structural limits

- permitted grid dimensions;
- exact cell count;
- valid coordinates;
- object count limits;
- link count limits;
- unique ObjectIds;
- unique LogicIds where required;
- valid dungeon LevelIds;
- valid transitions.

### Placement

- placement kind;
- walls required for wall objects;
- occupancy conflicts;
- anchor sharing;
- start cell validity;
- monster spawn validity;
- patrol waypoint validity.

### References

- known ArchetypeIds;
- known definition IDs;
- no forbidden content;
- no arbitrary UObject/class paths.

### Connectors

- valid source;
- valid target or valid targetless command;
- supported event;
- supported command;
- valid condition payload;
- valid Lua callback/script;
- valid quest identifiers.

### Lua

- script count;
- source sizes;
- syntax/load validation;
- callback existence;
- persistent declaration consistency;
- VM memory limit;
- instruction budget.

## 11. Lua security assessment

The current Lua architecture is unusually well positioned for player authoring.

Positive properties already present:

- dedicated Lua module;
- no Lua headers crossing the module boundary;
- no direct world/actor authority in the VM;
- host-controlled `grid` API;
- memory quota;
- instruction budget;
- source count/size limits;
- isolated script environments;
- atomic VM reload;
- callback execution through controlled host functions.

Future player-editor rule:

> Never weaken these caps based on dungeon-authored data.

The package must not be allowed to increase its own:

~~~text
memory quota
instruction budget
script count
source size
host API privileges
~~~

## 12. Runtime editor UI strategy

The current editor UI should be treated as a **UX prototype/reference**, not as code to ship.

Reusable UX concepts from GEUI01–09:

- Dungeon Levels workspace;
- overview map;
- Tools & Palette;
- search/categories/favorites/recent;
- Selected Object Properties/Connectors;
- validation search/filter/focus;
- Lua script workspace;
- clear PlayTest state.

Possible packaged implementation technologies:

~~~text
UMG
runtime Slate
or a combination
~~~

The project already uses both UMG and runtime Slate elsewhere.

Recommendation:

> Prefer the existing game UI architecture unless a later prototype proves that a runtime Slate desktop-style editor is materially better.

Do not add `UnrealEd`, `EditorFramework`, `ToolMenus` or editor docking code to the game target.

## 13. Runtime picking and editor camera

The current Grid Editor relies on:

~~~text
FEditorViewportClient
FSceneView deprojection
FEdMode mouse handling
Unreal Editor camera/navigation
~~~

A player editor will require an explicit runtime authoring controller.

Conceptual responsibilities:

~~~text
AGridAuthoringPawn / Controller
    camera pan/orbit/free-look
    grid raycast
    cell hover
    edge resolution
    object hover
    selection
    paint gesture
    erase gesture
~~~

The existing coordinate/placement math should be reused where possible.

The EdMode itself should not be reused.

## 14. Preview strategy

Two current classes remain in the runtime module:

~~~text
UGridEditorPreviewComponent
AGridEditorPreviewObjectActor
~~~

They are historically named for developer-editor preview, but their implementation is runtime-compilable.

They remain architecturally ambiguous.

GEUI10 recommendation:

- do not move them now;
- do not make the future player editor depend on them as its fundamental model;
- first build the real reference dungeon and continue stabilizing runtime rendering;
- later decide whether their selection/highlight behavior becomes a generic authoring-preview facility or stays developer-only compatibility code.

For a packaged editor, the preferred live preview should still be centered on:

~~~text
AGridLevelRuntimeActor
~~~

plus runtime selection/highlight presentation.

## 15. Undo / redo

The Unreal editor benefits from UObject/editor transaction semantics through authoring calls using `Modify()`.

A packaged editor cannot rely on Unreal Editor transaction infrastructure.

Future authoring core should use explicit commands, conceptually:

~~~text
FGridAuthoringCommand
    Execute()
    Undo()
~~~

Examples:

~~~text
PaintCellCommand
PaintWallCommand
PlaceObjectCommand
DeleteObjectCommand
ChangePropertyCommand
AddLinkCommand
RemoveLinkCommand
~~~

This command layer is another reason not to expose `AGridLevelEditorActor` directly to a packaged editor.

## 16. Dirty state and autosave

A player editor needs explicit document state:

~~~text
Clean
Modified
Saving
Save failed
Validation failed
~~~

This must be separate from:

~~~text
UPackage::MarkPackageDirty()
~~~

Recommended future responsibilities:

- dirty revision counter;
- autosave;
- recovery file;
- Save As;
- package metadata;
- validation-before-export;
- confirmation on destructive close.

## 17. Multi-level implications

The current dungeon model is already advantageous.

A player package should preserve:

~~~text
LevelId
DisplayName
LogicalPosition
DefaultLevelId
~~~

Transition objects can continue to reference stable level IDs.

The future player editor should therefore edit one dungeon document containing multiple levels rather than writing unrelated standalone map files.

This directly matches the project's current data-driven design.

## 18. Plugin decision

### 18.1 Do we need a plugin now?

No.

Creating a plugin today would mostly relocate files without solving a player-editor requirement.

The current module split is sufficient for ongoing game development.

### 18.2 When could a plugin become useful?

A plugin becomes justified if one of these becomes true:

- the authoring system is reused by another project;
- player-authoring modules need an independently versioned package;
- developer Grid Editor and runtime player editor share a mature authoring core;
- distribution/mod SDK boundaries benefit from plugin packaging.

### 18.3 Possible future plugin/module shape

Only when justified:

~~~text
GrimrockAuthoring
├── GrimrockAuthoringCore        Runtime
│     document schema
│     mutation commands
│     validation
│     catalog resolution
│
├── GrimrockAuthoringRuntimeUI   Runtime
│     packaged player editor
│
└── GrimrockAuthoringEditor      Editor
      UE EdMode adapters
      developer-specific Slate
      asset integration
~~~

This is a future direction, not a requested refactor.

## 19. Recommended extraction order when player authoring becomes active

Do not start with UI.

Recommended sequence:

### PLE01 — Player Dungeon Document

Create the versioned runtime-serializable dungeon/level document.

Acceptance criterion:

~~~text
document -> serialize -> deserialize -> identical document
~~~

### PLE02 — Catalog and safe resolution

Map player-visible IDs to allowed cooked assets.

Acceptance criterion:

~~~text
unknown/forbidden IDs are rejected without arbitrary asset loading
~~~

### PLE03 — Runtime Validation Core

Extract neutral validation issues and pure validation rules.

Acceptance criterion:

~~~text
same invalid level produces equivalent diagnostics
in Unreal developer editor and packaged authoring tests
~~~

### PLE04 — Authoring Mutation Commands

Create pure editing commands with undo/redo.

Acceptance criterion:

~~~text
execute -> undo -> original document
execute -> undo -> redo -> edited document
~~~

### PLE05 — Runtime Live Preview

Materialize a transient level and rebuild `AGridLevelRuntimeActor`.

Acceptance criterion:

~~~text
player document edits appear in runtime preview
without map or asset editing APIs
~~~

### PLE06 — Player Editor UI

Implement geometry/object/link/property workflows.

### PLE07 — Lua Authoring

Expose safe script editing/validation using the existing sandbox.

### PLE08 — Import / Export / Sharing

Add package metadata, compatibility checks and safe file management.

## 20. Risk register

| Risk | Severity | Current mitigation / future action |
|---|---:|---|
| Trying to ship `GrimrockPrototypeEditor` | High | Explicitly forbidden by this audit. |
| Writing cooked DataAssets as player files | High | Introduce versioned player document instead. |
| Arbitrary UObject/class references from player package | High | Stable ID + allowlisted catalog only. |
| Stale validation tied to editor actor | Medium | Future neutral validation service extraction. |
| Duplicated developer/player authoring rules | High | Extract pure authoring core before player UI. |
| Runtime editor built before serialization contract | High | PLE01 must precede UI. |
| Lua resource abuse | Medium | Existing hard VM caps retained and non-author-configurable. |
| Invalid/hostile oversized level data | High | Structural hard limits during deserialize/validate. |
| Broken cooked asset availability | Medium | Explicit catalog/cook policy required. |
| Preview class ambiguity | Low/Medium | Leave unchanged until real need appears. |
| Premature plugin refactor | Medium | No plugin work before reuse/distribution requirement exists. |

## 21. Architectural classification summary

### Keep in GrimrockPrototype Runtime

~~~text
GridTypes
GridObjectBehavior
GridLevelAsset
GridDungeonAsset
GridObjectArchetypeAsset
GridObjectPaletteAsset
definition assets
GridLevelRuntimeActor
runtime object actors/components
runtime state/persistence
gameplay interaction
~~~

### Keep in GrimrockLua Runtime

~~~text
FGridLuaVm
script types
sandbox/resource limits
host API bridge
~~~

### Keep in GrimrockPrototypeEditor

~~~text
FGridLevelEdMode
FGridLevelEdModeToolkit
SGridEditorWorkspaceTab
all current Grid Editor Slate workspaces
AGridLevelEditorActor
PIE preparation
Window/NomadTab integration
GEditor/editor viewport integration
~~~

### Future extraction candidates

~~~text
Link policy
pure link mutation
neutral validation model/service
Lua authoring analysis/mutation facade
level/dungeon mutation commands
safe authoring catalog
player dungeon document serialization
~~~

## 22. Immediate recommendation after GEUI10

**Stop the Grid Editor architecture refactor here.**

GEUI01–10 have now produced:

- dockable, focused workspaces;
- compact main dashboard;
- usable palette;
- selected-object workspace;
- PlayTest & Validation workspace;
- Lua workspace;
- editor-mode lifecycle management;
- window restoration;
- focused refresh/state behavior;
- a documented path toward future player authoring.

The next highest-value work is no longer another abstract editor refactor.

It is:

> **Build a real reference dungeon with the editor.**

Recommended reference dungeon scope:

~~~text
3 levels
multiple transitions
doors
secret doors
buttons
levers
pressure plates
receptacles
items / keys / locks
teleporters
triggers
monster spawns
patrols / encounters
readables
Lua mechanisms
quest hooks
validation
playtest
~~~

Using the editor intensively against real content will reveal future UX and data-model gaps far more reliably than continuing speculative interface work.

## 23. Decision record

GEUI10 records the following decisions:

1. `UGridLevelAsset` / `UGridDungeonAsset` remain the developer content authority.
2. The runtime remains independent of `GrimrockPrototypeEditor`.
3. `AGridLevelEditorActor` remains editor-only.
4. Current Slate/EdMode UI is not a future packaged editor implementation.
5. A player editor will use a runtime-compatible authoring core, not the Unreal editor module.
6. Player levels require a separate versioned serializable document/package contract.
7. Player packages reference curated content through stable IDs.
8. The existing Lua sandbox is the foundation for player-authored scripting and its hard limits must remain non-author-controlled.
9. No plugin is created during GEUI10.
10. No further Grid Editor architecture refactor is recommended before building a real reference dungeon.

## 24. GEUI01–GEUI10 closure

The GEUI roadmap can now be considered architecturally complete:

~~~text
GEUI01  Dockable workspace foundation
GEUI02  Dungeon Levels
GEUI03  PlayTest & Validation workspace
GEUI04  Tools & Palette workspace
GEUI05  Selected Object workspace
GEUI06  Slim main Toolkit + window lifecycle
GEUI07  Palette UX
GEUI08  Validation UX
GEUI09  Refresh / state cleanup
GEUI10  Player-editor / plugin readiness audit
~~~

No additional GEUI milestone is required before returning to dungeon content authoring.
