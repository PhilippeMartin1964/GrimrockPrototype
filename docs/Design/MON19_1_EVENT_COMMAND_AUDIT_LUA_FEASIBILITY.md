# MON19.1 — Event/Command Audit & Lua Feasibility

Status: **audit completed — documentation only**  
Date: **2026-08-22**  
Reference `master` before MON19.1: `208c5316a2375c276753604ea7faf7a0fc3ecf11` (`Close MON17.8 monster presentation and persistence`)

## 1. Scope and method

MON19.1 audits the existing Event -> Command architecture before any new scripting system is introduced.

The audit covered:

- `AGENTS.md` and the repository work rules;
- `docs/Design/PROJECT_COMPLETION_ROADMAP.md`;
- `docs/Design/03_EVENT_COMMAND_LINKS.md`;
- `FGridObjectLink`, `EGridObjectEvent`, `EGridObjectCommand` and `EGridObjectCondition`;
- `UGridActivationComponent` and `AGridLevelRuntimeActor`;
- door, receptacle and monster-spawn specializations;
- editor connector policy, Slate connector panel, editor actor validation and viewport visualization;
- level runtime persistence and SaveGame versioning;
- available automated/manual tests related to links and MON13 encounters;
- historical documentation and remaining TODOs;
- official Lua, sol2 and UnLua, including licensing, exception handling, packaging and sandbox implications.

No production C++ code, `.uasset` or `.umap` is changed by MON19.1.

The repository-visible `master` HEAD was verified immediately before this document was prepared and still matched the reference commit above. The audit environment can inspect and update the GitHub repository but cannot execute `git status` on the user's local `D:\Development\GrimrockPrototype` working tree; no claim is therefore made about uncommitted local files on that machine.

---

# A. Exact current Event -> Command architecture

## A.1 Persistent level data

The authoritative connector data is stored in:

```text
UGridLevelAsset
    Objects : TArray<FGridLevelObjectData>
    Links   : TArray<FGridObjectLink>
```

A link is no longer merely the historical quadruplet:

```text
SourceObjectId + SourceEvent -> TargetObjectId + Command
```

`FGridObjectLink` currently contains:

```text
SourceObjectId
TargetObjectId
SourceEvent
Command
Condition
ConditionItemDefinitionId
ConditionItemTag
ConditionItemType
ConditionCount
ConditionWeight
bInvertCondition
```

Therefore the real model already is:

```text
Source object
    emits SourceEvent
        -> select matching FGridObjectLink
        -> evaluate optional condition
        -> apply Command to target object
```

The existing conditions are specialized to receptacle state. They support:

- no condition;
- receptacle empty;
- receptacle contains any item;
- item definition match;
- item tag match;
- item type match;
- minimum item count;
- minimum total weight;
- inversion of a valid condition result.

This is an important MON19 finding: **the project already contains a small conditional logic layer**. MON19 must extend it rather than recreate it.

## A.2 Runtime dispatcher

`UGridActivationComponent` is the central runtime coordinator.

Its responsibilities are already close to the correct future architecture:

1. index level objects by `ObjectId`;
2. index links by `SourceObjectId`;
3. receive object events;
4. keep only links whose `SourceEvent` matches the emitted event;
5. resolve the target object and runtime actor;
6. evaluate the optional link condition;
7. dispatch the command according to the target type;
8. update the central active-state set where applicable;
9. emit follow-up events for stateful objects that support them;
10. log rejected/missing/unsupported paths.

The component has no permanent Tick (`PrimaryComponentTick.bCanEverTick = false`). This is a good foundation for MON19: script execution does not require introducing a permanent scripting Tick.

## A.3 Event emission paths

The normal object families currently emit:

```text
Button
    Activated

Lever
    Activated
    Deactivated

PressurePlate
    Activated
    Deactivated

Trigger
    Activated      (party enters)
    Deactivated    (party exits)

Receptacle
    ItemInserted
    ItemRemoved
    ItemChanged

MonsterSpawn / Encounter lifecycle
    MonsterDied
    MonsterSpawned
    MonsterDespawned
    MonsterTeleported
    EncounterWaveStarted
    EncounterCompleted
```

Notably, trigger entry/exit is translated to `Activated` / `Deactivated`; the enum values `Entered` / `Exited` are not the active contract.

`MonsterDied` is emitted through `UGridMonsterDeathComponent`, which forwards the stable monster spawn object id to `AGridLevelRuntimeActor::ExecuteLinksFromRuntimeObject()`.

MON13 spawn/encounter lifecycle events also return through the same Event -> Command path. They are not a separate scripting/event bus.

## A.4 Command dispatch paths

### Doors

The effective door commands are:

```text
Toggle
Open / Activate
Close / Deactivate
```

They use the specialized door command path and have real gameplay consequences.

### Receptacles

The effective specialized receptacle commands are:

```text
ReceptacleConsumeItem
ReceptacleConsumeAllItems
ReceptacleEnableRemoval
ReceptacleDisableRemoval
```

The generic state commands can also reach a receptacle through the central state path.

### MonsterSpawn

MON13 extends the central dispatcher with:

```text
Spawn
Despawn
Teleport
Activate   -> Spawn alias
Enable     -> Spawn alias
Deactivate -> Despawn alias
Disable    -> Despawn alias
Toggle     -> Spawn/Despawn according to current presence
StartEncounter
```

These commands are real runtime behavior, not historical enum placeholders.

### Generic state path

For several other object types, the dispatcher accepts:

```text
Open / Activate   -> active=true
Close / Deactivate -> active=false
Toggle             -> invert active state
```

However, **successfully storing an active flag is not the same thing as implementing gameplay behavior**. This distinction is central to the MON19 audit.

`ItemSpawn` currently logs that the state is stored but spawn behavior remains TODO. `Teleporter` likewise logs that the state is stored but teleport behavior remains TODO. Generic mechanisms without a specialized handler can also succeed with state bookkeeping only.

## A.5 Runtime chaining and cycle protection

When a link command changes the state of a Lever or PressurePlate, the new state can emit `Activated` or `Deactivated`, which allows chained connectors.

`DispatchingSourceObjectIds` prevents re-entry of a source that is already being dispatched. It therefore stops a runtime cycle once a connector chain comes back to an already-active source.

This is not a full graph-analysis system:

- the editor does not calculate arbitrary connector cycles ahead of time;
- an extremely long chain of distinct sources is not instruction-budgeted;
- future Lua -> Command -> Event -> Lua recursion will need a shared execution-depth/budget guard in addition to the existing source re-entry guard.

Nevertheless, the current runtime is **not completely unprotected against indirect cycles**.

## A.6 Editor CONNECTORS architecture

The connector editor is correctly split into three layers:

```text
GridEditorLinkPolicy
    -> declares events/commands allowed per object type

SGridEditorLinksPanel
    -> source/event/target/command form and link lists

GridLevelEditorActor
    -> creation/removal/validation and persistent asset mutation

GridLevelEdMode / GridLevelEdModeToolkit
    -> connector visualization and toolkit composition
```

`GridEditorLinkPolicy` is particularly important. It is already the central editor capability table and should remain the authoritative UI filter in MON19.

The Slate panel currently exposes only:

```text
Source Object
Source Event
Target Object
Command
```

It **does not expose the existing `FGridObjectLink` condition fields**.

The viewport mode draws incoming/outgoing connector arrows and labels from the same `UGridLevelAsset::Links` data; it does not duplicate runtime execution.

## A.7 Link creation/removal identity mismatch

The persistent data model allows condition fields to distinguish links during validation, but the editor's `CreateLink()` duplicate test uses only:

```text
SourceObjectId
TargetObjectId
SourceEvent
Command
```

`RemoveExactLink()` also removes by that same four-field identity.

Consequences:

1. a manually edited conditional link can prevent creation of another link with the same quadruplet but a different condition;
2. removing one such connector can remove every variant sharing the same quadruplet;
3. validation considers the condition payload when detecting exact duplicates, while creation/removal does not.

This is a real pre-MON19 editor/data-contract gap and should be corrected before complex logic is layered on top.

---

# B. Current capability matrix

Legend:

- **Yes**: implemented as real gameplay behavior.
- **State only**: dispatcher can store active state but no complete specialized effect exists.
- **No**: not part of the supported current connector contract.
- **Partial**: some state is persisted, but not the whole command-relevant state.
- **Static/manual**: coverage exists in documentation/manual PIE but no dedicated automated reference to the audited behavior was found by the source audit.

| Object type / family | Events actually emitted | Commands officially exposed by editor | Runtime implementation | Editor exposed | Tests found in audit | Persistence relevant to links | Unsupported command failure |
|---|---|---|---|---|---|---|---|
| Door, including secret-door archetypes | none (`Opened`/`Closed` not emitted) | `Open`, `Close`, `Toggle`, `Activate`, `Deactivate` | **Yes** | target yes, source no | existing door foundation/manual coverage; no new MON19 execution performed | **Yes**: open/blocking state | clean central rejection for unsupported command |
| Button | `Activated` | none as target | source **Yes**; generic target path exists only if hand-authored outside policy | source yes | no dedicated general-link automation found | **Yes** in interactive state | editor prevents unsupported target links |
| Lever | `Activated`, `Deactivated` | none as target | source **Yes**; commanded state can re-emit event if hand-authored | source yes | no dedicated general-link automation found | **Yes** | editor prevents unsupported target links |
| PressurePlate | `Activated`, `Deactivated` | none as target | **Yes**, including state-change emission | source yes | no dedicated general-link automation found | **Yes**, then runtime refresh can recompute | editor prevents unsupported target links |
| Trigger | `Activated`, `Deactivated` | none as target | **Yes** for enter/exit translation | source yes | MON13.3 editor policy verifies Trigger source; no dedicated trigger-link suite found | **Yes** only if central active state is used | editor prevents unsupported target links |
| Receptacle / wall-lock family | `ItemInserted`, `ItemRemoved`, `ItemChanged` | consume one/all, enable/disable removal | **Yes** | source and target yes | detailed validated manual PIE protocol; no automated test references to `EGridObjectCondition` found | **Partial**: contents yes, active state yes; `bCanRemoveItem` is not in runtime save state | clean specialized rejection |
| MonsterSpawn / Encounter | `MonsterDied`, `MonsterSpawned`, `MonsterDespawned`, `MonsterTeleported`, `EncounterWaveStarted`, `EncounterCompleted` | Spawn/Despawn/Teleport + aliases + `StartEncounter` | **Yes** | source and target yes | **Strong automated coverage**: MON13.3/13.4 runtime, policy, persistence and atomic-failure tests | **Yes**: monster state, placement, presence and encounters | clean/atomic paths covered by MON13 tests |
| Light | none | `Activate`, `Deactivate`, `Toggle` | **State only** unless a specialized runtime actor handles it | target yes | no dedicated link automation found | **No** for central active flag: Light is not captured in `InteractiveObjects` | command can report success even when only state bookkeeping occurred |
| Teleporter | none | `Activate`, `Deactivate`, `Toggle` | **State only; teleport behavior TODO** | target yes | no dedicated link automation found | **No** for central active flag | command can report success while gameplay teleport remains unimplemented |
| ItemSpawn | none | none | **State only; spawn behavior TODO** if hand-authored | no | no dedicated link automation found | **No** for central active flag | editor excludes it; hand-authored generic state can still return success |
| Decoration / readable | none (`Used` not emitted) | none | readable interaction exists outside connector emission; generic state has no official connector target contract | no connector source/target | no dedicated link automation found | no connector state persistence | editor excludes it |
| Item | none | none | item runtime exists, but not as a connector endpoint | no | item systems have separate tests | **Yes** as item/world state, not connector active state | editor excludes it |

### B.1 Enum values declared but not active as generic contract

Events present in the enum but with no active C++ emitter in the audited Event -> Command system:

```text
Used
Entered
Exited
Opened
Closed
Enabled
Disabled
```

Commands declared but not implemented as generic commands:

```text
Lock
Unlock
ShowMessage
```

`Enable`, `Disable`, `Spawn`, `Despawn` and `Teleport` are **not globally dead** anymore: MON13 implements them for `MonsterSpawn`. They must not be documented as universally inactive.

### B.2 Current editor policy table

`GridEditorLinkPolicy` currently exposes exactly:

```text
Sources
-------
Button          : Activated
Lever           : Activated, Deactivated
PressurePlate   : Activated, Deactivated
Trigger         : Activated, Deactivated
Receptacle      : ItemInserted, ItemRemoved, ItemChanged
MonsterSpawn    : MonsterDied, MonsterSpawned, MonsterDespawned,
                  MonsterTeleported, EncounterWaveStarted,
                  EncounterCompleted

Targets
-------
Door            : Open, Close, Toggle, Activate, Deactivate
Teleporter      : Activate, Deactivate, Toggle
Light           : Activate, Deactivate, Toggle
Receptacle      : ConsumeItem, ConsumeAllItems,
                  EnableRemoval, DisableRemoval
MonsterSpawn    : Spawn, Despawn, Teleport,
                  Activate, Deactivate, Enable, Disable, Toggle,
                  StartEncounter
```

This policy should be **extended, not replaced**, by MON19.

---

# C. Real functional gaps

The gaps below are ordered by architectural importance, not by historical TODO age.

## C.1 P0 — no generic persistent puzzle variables

There is no canonical store for values such as:

```text
Crypt.SecretOpened = true
Crypt.RuneCount = 3
Crypt.Stage = 2
```

`FGridLevelRuntimeState` persists doors, selected interactive objects, object presence, items, receptacles, monsters and encounters, but it has no generic typed level-variable map.

This is the clearest prerequisite for advanced puzzle logic and for safe Lua persistence.

## C.2 P0 — commands have no payload model

`EGridObjectCommand` is an enum with no generic argument payload.

That is sufficient for `Door.Open`, but not for future operations such as:

```text
Counter.Add(2)
Variable.SetBool(true)
Variable.SetInt(3)
Message.Show("Crypt.Warning")
Lua.Invoke("OnRuneActivated")
```

MON19 needs a deliberately small typed command-argument contract or a logical-node representation that carries those parameters in level data. It does **not** need a parallel command bus.

## C.3 P0 — existing conditions are too target-specific

All current non-`None` conditions require the **target actor to be a receptacle**.

This can answer:

```text
Button.Activated
    -> SecretAltar.ConsumeAllItems
       if SecretAltar contains RedGem
```

It cannot directly express:

```text
Button.Activated
    -> Door.Open
       if AnotherReceptacle contains RedGem
```

because the condition is evaluated against the command target, which is the Door.

A generic logic-variable / logic-node mechanism is therefore justified even before Lua.

## C.4 P0 — connector condition UI is missing

The level data and runtime already support receptacle conditions, and validation knows their fields, but `SGridEditorLinksPanel` does not expose them.

This forces designers to edit the `Links` array through generic Unreal asset details for advanced existing links.

MON19.2 should fix this before adding a more complex scripting UI.

## C.5 P0 — editor link identity ignores conditions

`CreateLink()` and `RemoveExactLink()` identify a connector using only the source/target/event/command quadruplet, while validation includes condition parameters in exact-duplicate detection.

Before adding more conditional/payload fields, link identity/edit operations must become consistent.

## C.6 P0 — command success does not always mean gameplay success

Light, Teleporter and ItemSpawn demonstrate a semantic problem:

- a generic state command can update `ActiveObjectIds` and return success;
- the specialized gameplay effect can still be absent;
- Teleporter and ItemSpawn explicitly log TODO behavior.

MON19 validation must distinguish:

```text
state can be stored
```

from:

```text
this target type implements this command as gameplay
```

This matters especially if Lua later receives a success/failure result from `grid.command()`.

## C.7 P0 — persistence is incomplete for some command-relevant state

Examples found by the audit:

1. `FGridRuntimeReceptacleState` persists contained items but not `bCanRemoveItem`; therefore `ReceptacleDisableRemoval` / `ReceptacleEnableRemoval` are not durable across Save/Load.
2. `CaptureCurrentLevelRuntimeState()` captures activation state only for Button, Lever, PressurePlate, Receptacle and Trigger. Light, Teleporter, ItemSpawn, Decoration and Item generic active flags are not captured.
3. This means a successful generic state command can be lost on Save/Load.

MON19 should not build persistent puzzle logic on that ambiguity.

## C.8 P1 — generic link/condition automated coverage is weak

The MON13 MonsterSpawn/Encounter extension has good automated coverage, including:

```text
MON13.3 DeferredSpawnLinks
MON13.3 LifecyclePersistence
MON13.3 AtomicCommands
MON13.3 EditorLinkPolicy
MON13.4 EncounterWaves
MON13.4 AtomicWaveFailure
MON13.4 Validation
MON13.4 EditorLinkPolicy
```

Receptacle behavior has a detailed manual PIE suite and previously validated results.

However, repository source search did not find automated tests directly referencing the `EGridObjectCondition` values or a broad generic suite that exhaustively checks each source/event/target/command pair.

MON19.2 should add table-driven contract tests before Lua is connected.

## C.9 P1 — editor/runtime capability definitions can drift

`GridEditorLinkPolicy` is a good editor authority, but `GridLevelEditorActor` also contains runtime-support helper logic, while `UGridActivationComponent` contains the actual dispatcher.

The audit already finds semantics where a type is considered state-command-compatible although its specialized gameplay behavior is absent.

MON19 should avoid adding a third independently maintained capability table. Prefer shared declarative helpers or tests asserting editor policy against actual runtime capability.

## C.10 P1 — no general execution budget

Current source re-entry protection is useful, but Lua introduces a second class of risk:

```lua
while true do
end
```

and also cross-system chains such as:

```text
Lua -> Command -> Event -> Lua -> Command -> ...
```

A Lua instruction budget plus a central MON19 dispatch-depth/budget guard is required before community scripts are allowed.

---

# D. Historical/documentation drift

## D.1 `docs/Design/03_EVENT_COMMAND_LINKS.md`

This document is useful as historical design intent but is no longer authoritative for the exact enum/runtime contract.

Examples of drift include:

- historical event naming such as `OnActivate`/`OnDeactivate` versus current `Activated`/`Deactivated`;
- historical commands such as timer operations, `Destroy`, animation/sound commands, etc. that are not in the current enum;
- old statements around spawn/teleport support that predate MON13;
- object-source/target lists that no longer describe the full MonsterSpawn/Encounter extension.

MON19 documentation should use the current code and `GridEditorLinkPolicy` as authority.

## D.2 `docs/Architecture/LINK_EVENT_COMMAND_FOUNDATION.md`

This document is much closer to current code but has MON13/persistence drift:

- it lists `Spawn`, `Despawn` and `Teleport` among values not dispatched, while MON13 now implements them for MonsterSpawn;
- it predates `StartEncounter` in its command summary;
- its statement that no link runtime state is saved is now too broad: several target states are persisted, although incompletely;
- its “indirect cycles are not detected” wording should distinguish **editor graph detection** from the existing runtime re-entry guard, which stops a chain when it returns to an already-dispatching source.

## D.3 Receptacle documentation

The receptacle test documentation correctly records the important remaining UI limitation: condition fields exist in `FGridObjectLink` but are not exposed in the Slate connector form.

That is current, not historical.

## D.4 Roadmap implication

The authoritative project roadmap correctly says that a scripting language should only be introduced if Event -> Command is insufficient.

MON19.1 confirms:

- Event -> Command is sufficient for simple puzzles and for a significant class of chained mechanisms;
- generic persistent values and parameterized logic are the first missing pieces;
- Lua is justified only for logic that becomes awkward or combinatorial in data;
- a new proprietary language is not justified.

---

# E. Lua feasibility study

## E.1 Current upstream status on 2026-08-22

Official Lua upstream reports:

- Lua 5.5 released 2025-12-22;
- current Lua release: **5.5.1**, released 2026-08-03;
- current stable 5.4 maintenance release: **5.4.8**, released 2025-06-04;
- Lua 5.4.9 is in release-candidate state during August 2026, therefore it should not be the project baseline yet.

Lua is distributed under the MIT license and is explicitly intended to be embedded in C/C++ applications.

Lua's own version documentation also warns that bytecode/VM compatibility is not guaranteed across different Lua versions. This reinforces the project decision to store scripts as source text, not versioned runtime bytecode.

Official references:

- https://www.lua.org/versions.html
- https://www.lua.org/download.html
- https://www.lua.org/manual/5.4/
- https://www.lua.org/license.html

## E.2 Recommended Lua version: 5.4.8

**Recommendation for MON19: Lua 5.4.8.**

Reasons:

1. it is a mature stable maintenance release;
2. it has long-established embedding behavior and a stable 5.4 API;
3. sol2 v3 explicitly contains Lua 5.4 fixes;
4. current 2026 sol2 issue traffic contains an open “Issues with Lua 5.5” report;
5. Lua 5.5.1 is only a few weeks old at the date of this audit;
6. MON19 does not need any Lua 5.5-specific language feature;
7. source scripts make a later 5.5 migration practical.

Do not use 5.4.9 RC in production. Re-evaluate when 5.4.9 is final or when a future migration to 5.5 is justified and tested.

## E.3 Direct Lua C API

### Advantages

- only one third-party dependency: official Lua;
- smallest attack surface;
- exact control over which functions/tables enter the VM;
- no automatic Unreal reflection binding;
- no template-heavy binding layer;
- no requirement to enable C++ exceptions in `GrimrockPrototype`;
- ideal for a deliberately tiny whitelist API;
- easier to audit for future untrusted player content.

### Costs

- more verbose stack manipulation;
- bindings require disciplined type checking;
- C++ code must respect Lua's C error/longjmp model;
- helper wrappers should be kept tiny and tested.

For this project, the small whitelist is a feature, not a limitation.

## E.4 sol2

sol2 is:

- MIT licensed;
- header-only;
- designed as a C++ <-> Lua binding layer;
- advertised as supporting Lua 5.1+ including 5.4.

Current stable release shown by upstream is `v3.3.0`.

Relevant facts for GrimrockPrototype:

1. the project `Build.cs` does not currently set `bEnableExceptions=true`;
2. Unreal Build Tool exposes `bEnableExceptions`, but the project does not opt in;
3. sol2 supports `SOL_NO_EXCEPTIONS`, but its documentation warns that in this configuration its default panic behavior changes and requires deliberate handling;
4. sol2's own error documentation highlights Lua's `setjmp`/`longjmp` behavior and C++ destructor hazards;
5. the sol2 issue tracker currently contains 2026 issues titled around exception-disabled behavior and Lua 5.5;
6. a large part of sol2's value—automatic usertype/binding ergonomics—is intentionally **not wanted** for the future sandbox.

References:

- https://github.com/ThePhD/sol2
- https://sol2.readthedocs.io/en/latest/exceptions.html
- https://sol2.readthedocs.io/en/latest/safety.html

### sol2 conclusion

**Do not make sol2 a MON19 production dependency.**

A tiny isolated compile proof can still be kept as an optional experiment if desired, but the production recommendation is the direct Lua 5.4 C API.

This is not because sol2 is a poor library. It is because GrimrockPrototype intentionally needs a very small, security-auditable API, while the project's current no-exception build posture makes sol2's convenience layer less compelling and adds another compatibility surface.

No global or module-wide exception enable should be introduced merely to accommodate sol2.

## E.5 UnLua

UnLua is MIT licensed and supports Unreal Engine 5.x.

Its principal features deliberately include direct access to:

```text
UCLASS
UPROPERTY
UFUNCTION
USTRUCT
UENUM
Blueprint events
native UE containers
latent functions/coroutines
```

That is almost the opposite of the desired trust boundary:

```text
Lua
  -> controlled Grimrock API
  -> existing Event/Command and runtime services
```

Therefore:

**UnLua is rejected for MON19**, despite its valid UE integration and MIT license.

Reference:

- https://github.com/Tencent/UnLua

## E.6 UE5.5.4 build and packaging

Epic's documented third-party pattern uses a `.Build.cs` external module (`ModuleType.External`) to expose includes, definitions and static/import libraries.

A good Windows-first layout is:

```text
Source/
  ThirdParty/
    Lua54/
      Lua54.Build.cs
      include/
        lua.h
        lauxlib.h
        lualib.h
        luaconf.h
      lib/
        Win64/
          lua54.lib
      LICENSE.txt
```

Then:

```text
GrimrockPrototype.Build.cs
    -> dependency on Lua54
```

For the first integration, prefer a **static library** built from official Lua C sources, excluding the standalone `lua.c` and `luac.c` programs. A static library avoids DLL staging and runtime search-path issues in Development and Shipping.

The exact MSVC/UBT configuration must be proven in MON19.3 with:

```text
Editor Development
Game Development
packaged Development
Shipping
```

No claim is made by MON19.1 that these builds have already been executed.

Epic reference:

- https://dev.epicgames.com/documentation/unreal-engine/integrating-third-party-libraries-into-unreal-engine

## E.7 Exception model

Current project files:

```text
Source/GrimrockPrototype/GrimrockPrototype.Build.cs
Source/GrimrockPrototypeEditor/GrimrockPrototypeEditor.Build.cs
```

do not enable exceptions.

Recommendation:

- keep the main project module in its current exception posture;
- compile official Lua as C in the third-party library;
- execute all user callbacks through protected Lua calls;
- ensure Lua-facing C/C++ trampolines do not rely on C++ exceptions;
- avoid designs where a Lua `longjmp` can skip important C++ RAII cleanup;
- never allow a Lua panic to become normal control flow.

---

# F. Firm architecture recommendation

## F.1 Use Lua?

**Yes.**

But only for complex logic that is materially clearer in script.

Simple puzzles must remain:

```text
Event -> Command
```

without a Lua VM call.

## F.2 Use sol2?

**No for the MON19 production baseline.**

Use the direct official Lua 5.4 C API behind a small project-owned wrapper.

Revisit sol2 only if a later measured need for richer binding ergonomics appears and its no-exception configuration has been independently proven against the exact UE toolchain.

## F.3 Lua version

**Lua 5.4.8** for MON19.

## F.4 Runtime ownership

Prefer a no-Tick component owned by the level runtime actor, for example:

```text
AGridLevelRuntimeActor
  + UGridActivationComponent
  + UGridLuaRuntimeComponent   // proposed, no Tick
```

rather than a global singleton VM.

Reasons:

- lifetime follows the loaded grid level/runtime actor;
- easy reset on level rebuild/transition;
- no global script state leaking between levels;
- direct integration point beside the existing activation component;
- tests can instantiate a runtime actor and its Lua component in isolation.

The component should own one `lua_State*` for the active level and destroy it on teardown/reload.

## F.5 No direct Unreal exposure

Lua must never receive:

```text
UWorld*
AActor*
UObject*
UClass*
UFunction*
raw filesystem path access
reflection helpers
console/process execution
```

The only exposed surface should be project-defined scalar functions/tables such as the eventual equivalent of:

```text
grid.command(id, command)
grid.getBool(name)
grid.setBool(name, value)
grid.getInt(name)
grid.setInt(name, value)
grid.addInt(name, delta)
grid.log(message)        // rate-limited / development-aware
```

The exact names are MON19.3/19.4 implementation details; they are not frozen by this audit.

## F.6 Lua -> Command must reuse the central dispatcher

Lua must not resolve an Actor and call `OpenDoor()` directly.

The required path is:

```text
Lua callback
    -> Grimrock whitelist binding
        -> ObjectId / validated logical id resolution
            -> UGridActivationComponent / AGridLevelRuntimeActor command entry
                -> existing target-specific command implementation
```

This requires extracting/exposing a safe **single-command execution entry point** from the current central dispatcher, not creating a second command implementation for Lua.

## F.7 Event -> Lua bridge should remain a connector target

The least disruptive conceptual model is a non-gameplay logical/script endpoint that participates in the existing link graph.

Example:

```text
Lever_A.Activated
    -> ScriptLogic_OnLever.Invoke
```

The script endpoint has no world gameplay Actor requirement. It resolves a configured callback in the level Lua environment.

The callback can then request normal commands:

```text
Lua
    -> grid.command(Door_Secret, Open)
    -> grid.command(Teleporter_Exit, Activate)
```

The exact representation—logical object subtype versus a small dedicated logic-node record—should be finalized in MON19.2 after the generic logic-node design is implemented. Do **not** fake Lua invocation by overloading an unrelated Door/Trigger target.

## F.8 Designer-facing identifiers

Internal execution should continue to use `FGuid ObjectId` as the canonical object identity.

For Lua authoring, raw GUIDs are too fragile to type manually. A future script-exposed object should therefore have an optional stable designer-facing `FName` alias (for example `LogicId` / `ScriptId`) that is:

- unique within the level;
- validated by the editor;
- resolved once to `ObjectId`;
- never used to bypass the central object index.

Do not repurpose an arbitrary non-unique Tag without adding uniqueness validation.

---

# G. Final MON19.2 -> MON19.8 proposal

## MON19.2 — Event/Command Hardening & Logic Primitives

### Goal

Cover the common puzzle cases without Lua and remove the inconsistencies discovered by MON19.1.

### Work

1. make connector identity consistent with condition/payload fields;
2. expose existing receptacle conditions in `SGridEditorLinksPanel`;
3. add table-driven tests for every supported editor event/command pair;
4. distinguish “state bookkeeping” from real command implementation in validation;
5. add a small generic typed level-variable model:
   - Bool;
   - Int32;
   - optionally FName only if an immediate production use exists;
6. add minimal data-driven logic primitives, preferably represented without one Actor class per primitive:
   - set/toggle Bool;
   - set/add/subtract/reset Int;
   - threshold/compare;
   - one-shot latch;
   - relay;
7. allow logic nodes to emit normal `Activated` / `Deactivated` or another deliberately small event set back into the existing connector graph;
8. fix command-relevant persistence holes needed by production puzzles, including receptacle removal state if it remains a persistent mechanism command.

### Explicitly out of scope

- Lua VM;
- custom scripting language;
- editor IDE;
- arbitrary expression engine.

### Exit criteria

At least two non-trivial puzzles using variables/counters must work with **zero Lua**.

## MON19.3 — Lua 5.4 Runtime Foundation

### Goal

Embed official Lua 5.4.8 with the smallest safe runtime surface.

### Work

1. add vendored Lua 5.4.8 license/source or reproducible third-party build inputs;
2. create the `Lua54` external UBT module and static Win64 library integration;
3. add the no-Tick Lua runtime component/service;
4. create/destroy one VM per active level runtime;
5. load source text only;
6. execute only protected callbacks;
7. open only approved libraries;
8. add deterministic error reporting;
9. implement custom memory allocator/accounting from the start, even if initial limits are generous;
10. add instruction-count hook infrastructure from the start.

### Exit criteria

A trivial callback can execute in Editor Development without access to Unreal objects, filesystem, OS or package loading.

No sol2 dependency is required.

## MON19.4 — Event -> Lua -> Command Bridge

### Goal

Connect Lua to the existing connector graph without creating a second gameplay dispatcher.

### Work

1. represent a script callback as a valid connector target/logical endpoint;
2. add a single script invocation command/contract;
3. resolve designer-facing script object aliases to canonical `ObjectId`;
4. expose `grid.command()` through the central command execution path;
5. return controlled success/error values, not Actor pointers;
6. share a dispatch depth/budget context across Event -> Lua -> Command -> Event chains;
7. ensure a Lua runtime error fails the current script invocation without crashing or corrupting the level runtime.

### Exit criteria

```text
Button.Activated
    -> Lua callback
        -> Door.Open
```

works while the existing direct:

```text
Button.Activated
    -> Door.Open
```

still uses no Lua.

## MON19.5 — Lua Persistence Contract & Save Version

### Goal

Persist only canonical puzzle results.

### Work

1. expose the MON19.2 typed level-variable store to Lua;
2. never serialize `lua_State`, stack, closures, coroutines, userdata or compiled chunks;
3. capture typed variables in `FGridLevelRuntimeState`;
4. restore variables before script callbacks can depend on them;
5. add SaveGame migration and validation;
6. add Save/Load tests through a partially completed puzzle;
7. ensure script source/version changes do not require bytecode migration.

### Exit criteria

A save made after changing puzzle variables reloads to the same logical state with a newly created Lua VM.

## MON19.6 — Editor Integration & Validation

### Goal

Make advanced logic authorable without turning the dungeon editor into a Lua IDE.

### Work

1. edit link conditions directly in CONNECTORS;
2. create/edit logical variables/nodes;
3. associate a level script source and callback name;
4. validate callback names where source is available;
5. validate unique script-facing object aliases;
6. show Event -> Script connector edges in the same visualization;
7. add diagnostic categories for:
   - missing script;
   - syntax error;
   - missing callback;
   - invalid object alias;
   - unsupported command;
   - security policy violation;
8. provide a small read-only runtime variable inspector for PIE if useful.

### Explicitly out of scope

- debugger;
- breakpoints;
- code completion;
- full source editor.

## MON19.7 — Sandbox / Runtime Limits / Packaging

### Goal

Prove that a community-level script can be treated as untrusted input within the defined game sandbox.

### Work

1. never call `luaL_openlibs()` wholesale;
2. explicitly open only approved libraries;
3. remove/never expose `io`, `os`, `package`, `debug`;
4. do not expose `require`, `dofile`, `loadfile` or arbitrary module loaders;
5. restrict dynamic code loading; use text-only loading mode;
6. use an instruction-count hook to stop infinite loops;
7. use a per-VM memory quota through `lua_newstate` custom allocator;
8. enforce maximum script size;
9. enforce callback/dispatch depth;
10. keep every exposed C++ function bounded and non-blocking;
11. rate-limit script logs/errors;
12. test malformed scripts and deliberate infinite loops;
13. test packaged Development;
14. test Shipping;
15. verify packaged script-source staging/loading policy;
16. verify no direct UE/reflection/filesystem/process surface exists.

### Exit criteria

A deliberately hostile test script can exhaust its own budget and be aborted without freezing or crashing the game thread.

## MON19.8 — Production Puzzle Suite / Regression / Closure

Implement and validate:

### Puzzle A — direct data-driven

```text
Lever -> Door
```

No Lua.

### Puzzle B — variables/counter

Multiple switches/counter/threshold using MON19.2 only.

### Puzzle C — conditional Lua

Lua callback reads persistent variables and issues normal commands.

### Puzzle D — encounter bridge

```text
EncounterCompleted
    -> Lua
        -> Door.Open
        -> Teleporter/Message or another completed production command
```

If Teleporter/Message behavior is still incomplete, it must be completed or the test should use a target whose runtime behavior is real; do not treat state-only success as puzzle completion.

### Puzzle E — Save/Load mid-puzzle

Persist variables and canonical target state, recreate Lua VM, continue puzzle.

### Puzzle F — hostile/broken Lua

Syntax error, runtime error and infinite loop are contained and reported.

Then:

- run targeted automated tests;
- run full relevant regression;
- perform PIE validation supplied/confirmed by the project owner;
- verify packaged Development/Shipping as applicable;
- update authoritative architecture docs;
- close MON19.

---

# H. SaveGame and versioning impact

## H.1 Current state

`UGrimrockPartySaveGame::CurrentSaveVersion` is currently:

```text
6
```

Version 6 was introduced by MON18.8 Spellbook persistence.

`FGridDungeonRuntimeState` contains per-level `FGridLevelRuntimeState` snapshots.

## H.2 Proposed MON19 data

Add a typed persistent logic store to `FGridLevelRuntimeState`, conceptually:

```text
LevelVariables
    Crypt.SecretOpened : Bool=true
    Crypt.RuneCount    : Int=3
```

Use a typed USTRUCT, not a Lua value or serialized Lua table.

The first production version should support only types with a proven gameplay use. Recommended baseline:

```text
Bool
Int32
```

Add FName/String later only when required.

## H.3 Version bump

When the persistent MON19 store is introduced, bump:

```text
CurrentSaveVersion 6 -> 7
```

This project uses explicit save-version migration even for additive domains, so keeping that discipline is preferable to silently changing semantics under version 6.

## H.4 Critical migration detail

`FRPGSaveMigrationService::PrepareLoadedSave()` currently has special handling for v5 and v4, then older migration logic.

Once `CurrentSaveVersion` becomes 7, **v6 must receive its own explicit migration path before the current v5 branch**.

Otherwise a legitimate v6 save would fall through into older reconstruction logic that was not written for v6.

Recommended v6 -> v7 migration:

1. validate the existing v6 domains exactly as today;
2. leave MON19 variable snapshots empty;
3. set `SaveVersion=7`;
4. on level restore, an absent variable snapshot initializes from the current `UGridLevelAsset` variable defaults;
5. validate the resulting typed store.

Do not make the SaveGame migration service instantiate level-specific puzzle defaults itself.

## H.5 Existing persistence gaps to decide before MON19 closure

- `bCanRemoveItem` for receptacles if removal commands are intended to survive Save/Load;
- Light/Teleporter/ItemSpawn central active state if these become production connector targets;
- any one-shot/latch state introduced by MON19.2.

## H.6 Lua-specific SaveGame rule

Never save:

```text
lua_State
stack
closures
coroutines
userdata
registry
instruction counters
open iterators
Lua timers
compiled bytecode
```

Save only canonical game state.

---

# I. Sandbox strategy for future player content

## I.1 Library policy

Do **not** call `luaL_openlibs()` in the player-level VM.

Open explicit safe libraries individually with the Lua C API.

Initial candidate allowlist:

```text
selected base functions
math
string
table
utf8 (optional but low risk)
```

Initial denylist:

```text
io
os
package
debug
coroutine (defer unless a concrete design requires it)
```

Also remove or never expose:

```text
require
dofile
loadfile
arbitrary filesystem module loading
process execution
```

`load` should be omitted initially. Level scripts should not dynamically compile arbitrary secondary chunks unless a future feature proves that need.

## I.2 Text-only scripts

Use Lua's text-mode loading (`luaL_loadbufferx(..., "t")` or equivalent) so binary chunks are rejected.

Keep player-authored scripts as source `.lua` inside the future level package/content boundary.

## I.3 Instruction budget

Use `lua_sethook()` with `LUA_MASKCOUNT` to decrement an instruction budget for each protected callback invocation.

The budget must be deterministic and reset per top-level script invocation.

An optional wall-clock diagnostic can supplement it, but wall-clock timeout alone is not sufficient because:

- it is non-deterministic;
- it cannot safely preempt arbitrary C++ work;
- every exposed C++ function should already be bounded.

## I.4 Memory budget

Create the VM using `lua_newstate(customAllocator, context)`.

Track bytes allocated for that VM and reject allocations over a configured level-script quota.

This provides a real bound against scripts that build unbounded tables/strings.

## I.5 Execution depth

Maintain a MON19 execution context containing at least:

```text
remaining Lua instruction budget
Event/Command/Lua nesting depth
current script/callback identity
current level identity
```

The existing `DispatchingSourceObjectIds` remains useful, but the new depth guard protects long chains involving distinct sources and Lua callbacks.

## I.6 API whitelist

Bindings must traffic in stable scalar IDs and values only.

Good boundary:

```text
FName / validated string id
bool
int32
small result enum/error
```

Bad boundary:

```text
UObject*
AActor*
UWorld*
TSubclassOf
reflection access
raw pointers
arbitrary asset loading
```

## I.7 Error isolation

Every script load and callback must be protected.

On failure:

1. record a concise level/script/callback error;
2. abort that invocation;
3. leave canonical runtime state consistent;
4. do not continue with a corrupted Lua stack;
5. if the VM itself enters a panic/unrecoverable state, destroy and recreate the VM rather than pretending it remains safe.

## I.8 Community file boundary

Future external levels should load Lua only from the selected level package/root. The Lua API must never accept an arbitrary host filesystem path.

A level package loader should resolve script names internally and hand source bytes to the Lua runtime.

---

# J. Files likely involved in MON19.2 / MON19.3

No file below is modified by MON19.1 except this audit document. This is the expected implementation surface based on current ownership.

## J.1 Existing runtime/core files — likely MON19.2

```text
Source/GrimrockPrototype/Public/Core/GridTypes.h
Source/GrimrockPrototype/Public/Core/GridLevelAsset.h
Source/GrimrockPrototype/Private/Core/GridLevelAsset.cpp

Source/GrimrockPrototype/Public/Runtime/GridActivationComponent.h
Source/GrimrockPrototype/Private/Runtime/GridActivationComponent.cpp

Source/GrimrockPrototype/Public/Runtime/GridLevelRuntimeActor.h
Source/GrimrockPrototype/Private/Runtime/GridLevelRuntimeActor.cpp

Source/GrimrockPrototype/Public/Runtime/GridDungeonRuntimeState.h
```

Likely responsibilities:

- logic value/node types;
- command payload or equivalent logical-node parameters;
- safe single-command execution entry point;
- variable runtime state;
- capture/restore;
- capability checks.

## J.2 Save files — when MON19 persistent variables land

```text
Source/GrimrockPrototype/Public/Save/GrimrockPartySaveGame.h
Source/GrimrockPrototype/Public/RPG/RPGSaveMigrationService.h   // only if public contract changes
Source/GrimrockPrototype/Private/RPG/RPGSaveMigrationService.cpp
```

Plus the existing save/migration automation tests relevant to version 6 -> 7.

## J.3 Existing editor files — likely MON19.2 / MON19.6

```text
Source/GrimrockPrototypeEditor/Public/EditorTools/GridEditorLinkPolicy.h
Source/GrimrockPrototypeEditor/Private/EditorTools/GridEditorLinkPolicy.cpp

Source/GrimrockPrototypeEditor/Public/EditorTools/GridLevelEditorActor.h
Source/GrimrockPrototypeEditor/Private/EditorTools/GridLevelEditorActor.cpp

Source/GrimrockPrototypeEditor/Public/EditorTools/Widgets/SGridEditorLinksPanel.h
Source/GrimrockPrototypeEditor/Private/EditorTools/Widgets/SGridEditorLinksPanel.cpp

Source/GrimrockPrototypeEditor/Private/EditorTools/GridLevelEdMode.cpp
Source/GrimrockPrototypeEditor/Private/EditorTools/GridLevelEdModeToolkit.cpp
```

`SGridEditorObjectInspectorPanel` may also be involved if logic/script properties are edited there, but it should not be changed merely to satisfy MON19 naming.

## J.4 Proposed new Lua runtime files — MON19.3

A minimal shape is:

```text
Source/GrimrockPrototype/Public/Runtime/Scripting/GridLuaRuntimeComponent.h
Source/GrimrockPrototype/Private/Runtime/Scripting/GridLuaRuntimeComponent.cpp
```

Potential tiny helper files should be added only if the component becomes too broad, for example:

```text
GridLuaSandboxConfig.h
GridLuaBindings.cpp
```

Do not begin with a large scripting framework hierarchy.

## J.5 Third-party files — MON19.3

```text
Source/ThirdParty/Lua54/Lua54.Build.cs
Source/ThirdParty/Lua54/include/...
Source/ThirdParty/Lua54/lib/Win64/lua54.lib
Source/ThirdParty/Lua54/LICENSE.txt
```

`Source/GrimrockPrototype/GrimrockPrototype.Build.cs` then adds the dependency.

No editor module dependency on Lua is required merely to run scripts; editor syntax validation can call a runtime-safe validation service later if needed.

## J.6 Tests to add

Suggested new tests:

```text
Source/GrimrockPrototype/Private/Tests/GridMON19LogicTests.cpp
Source/GrimrockPrototype/Private/Tests/GridMON19LuaTests.cpp
Source/GrimrockPrototypeEditor/Private/Tests/GridEditorMON19LinkPolicyTests.cpp
Source/GrimrockPrototypeEditor/Private/Tests/GridEditorMON19ValidationTests.cpp
```

Do not fragment these into one test file per tiny primitive unless file size genuinely requires it.

---

# Final decision

The answer to the MON19.1 central question is:

> **Event -> Command already knows how to do considerably more than the old design document suggests: typed object events, central dispatch, chaining, receptacle conditions, MON13 monster/encounter lifecycle, editor capability filtering, validation and partial persistence. It should remain the authoritative gameplay logic backbone.**

The smallest missing layer is not Lua first. It is:

```text
1. harden existing connector editing/validation;
2. add generic typed persistent level variables and a few data-driven logic primitives;
3. repair command-relevant persistence gaps;
4. then embed a very small Lua VM only for genuinely complex logic.
```

For scripting:

```text
Use Lua            : YES
Version            : Lua 5.4.8
Use sol2 production: NO for MON19 baseline
Use UnLua          : NO
Binding            : direct Lua C API, project-owned whitelist
UE exposure        : NONE
Lua -> gameplay    : through existing central Command path
VM persistence     : NEVER
Persistent data    : typed canonical level variables only
```

This architecture keeps the project simple, data-driven and compatible with the long-term objective of safely loading player-created levels without giving scripts unrestricted access to Unreal Engine internals.
