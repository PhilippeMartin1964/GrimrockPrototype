# GrimrockPrototype Design Docs

This folder is the stable design memory for the GrimrockPrototype project. It keeps accepted decisions, active roadmaps, implementation contracts and validation checklists readable across ChatGPT, Codex, Unreal Editor and Git sessions.

## Active Reading Order

For current development work, read these first:

1. `00_PROJECT_OVERVIEW.md` — current project orientation and validated state.
2. `PROJECT_COMPLETION_ROADMAP.md` — **active backlog from MON15 onward**.
3. `MON14_CLOSURE.md` — closure of automatic engagement, directional perception, patrol, investigation, route editing and alarm coordination.
4. `99_DECISIONS_LOG.md` — chronological authoritative decision log.
5. `COMBAT_SYSTEM_V2_ACTION_POINTS_INITIATIVE.md` — current combat architecture.
6. `GRIMROCK_LOCK_SYSTEM.md` — lock/key/lockpicking/container target design.
7. `06_GRID_EDITOR_UX_SPEC.md` — current editor UX target.

`04_IMPLEMENTATION_ROADMAP.md` and `05_CODEX_TASKS.md` are historical documents. They remain useful context but **do not define the active backlog**.

---

## Current Major Milestones

| Family | Status | Summary |
|---|---|---|
| MON1–MON10 | Validated foundation | Monster definition, actor, movement, pathfinding, perception, combat, death, persistence, presentation, balance. |
| MON11 | Validated | Party attack pipeline, offensive equipment, presentation and thrown weapons. |
| MON12 | Validated | Global initiative, AP/PAM, action catalogue, HUD, hotbar, quick items, spell/action targeting and cooldowns. |
| MON13 | Validated / Closed | Persistent `MonsterSpawn`, runtime Spawn/Despawn/Teleport, encounter groups, waves, production asset contracts and real PIE closure. |
| MON14 | **Validated / Closed** | Automatic visual engagement, directional sight, Idle/Dormant, patrol data/runtime, investigation/search, visual route editor and local alarm coordination. |
| MON15 | **Next** | XP and level progression. |

MON14 closure was validated on 15 August 2026, including:

```text
Grimrock.Monsters.MON14.4.AlarmFiltering           Success
Grimrock.Monsters.MON14.4.HearingAlarmPropagation Success
Grimrock.Monsters.MON14.4.SharingDisabled          Success
```

and a manual functional validation in `L_GrimrockEditor` / PIE.

---

## Active Roadmap

The accepted order is:

```text
MON15 — XP & Level Progression
MON16 — Status Effects
MON17 — Second Monster Family
MON18 — Magic & Spellbook
MON19 — Advanced Dungeon Logic / Scripting
MON20 — Recruitment / Skills / Talents
MON21 — Quests / Journal / Map / Codex
MON22 — 45–90 Minute Vertical Slice
```

The immediate task is:

```text
MON15.1 — XP & Level Model
```

See `PROJECT_COMPLETION_ROADMAP.md` for scope and exit criteria.

---

## Core Design Documents

| Document | Status | Role |
|---|---|---|
| `00_PROJECT_OVERVIEW.md` | Current | Project orientation, validated milestones and next phase. |
| `PROJECT_COMPLETION_ROADMAP.md` | **Current / Active Backlog** | Authoritative MON15→MON22 development order and exit gates. |
| `MON14_CLOSURE.md` | Validated Closure | Consolidated MON14 result and validation evidence. |
| `01_GRID_OBJECT_SYSTEM.md` | Current | Grid object model and runtime/editor split. |
| `02_OBJECT_ARCHETYPES.md` | Current | Concrete archetype families and naming rules. |
| `03_EVENT_COMMAND_LINKS.md` | Current | Explicit `Source Object + Source Event + Target Object + Command` connector rules. |
| `04_IMPLEMENTATION_ROADMAP.md` | Historical | Initial object-system roadmap, superseded as active backlog. |
| `05_CODEX_TASKS.md` | Historical | Historical task templates. |
| `06_GRID_EDITOR_UX_SPEC.md` | Current | Editor UX specification. |
| `07_GRID_OBJECT_ARCHETYPE_ASSET_AUDIT.md` | Audit | Archetype field audit. |
| `08_GRID_OBJECT_ARCHETYPE_DATA_ASSETS_AUDIT.md` | Audit / Checklist | Concrete DataAsset review. |
| `09_GRID_OBJECT_ARCHETYPE_NAMING_NORMALIZATION_PLAN.md` | Checklist | Naming normalization. |
| `10_GRID_EDITOR_UI_CONSISTENCY_CHECKLIST.md` | Checklist | Editor UI/runtime consistency. |
| `11_GRID_OBJECT_ARCHETYPE_PARAMETERS_REFERENCE.md` | Current / Reference | Practical archetype parameter reference. |
| `99_DECISIONS_LOG.md` | Decision Log | Authoritative chronological record of accepted decisions. |

---

## Inventory / Interaction / Lock Documents

| Document | Status | Role |
|---|---|---|
| `ITEM_CONTEXT_ACTION_SYSTEM.md` | Design Target / Partial | Right-click contextual actions and target model. |
| `INVENTORY_CONTEXT_ACTION_MVP_VALIDATION.md` | Validation | Current context action MVP checklist. |
| `INVENTORY_INTERACTION_ROUTING.md` | Reference | C++ / Blueprint responsibility map. |
| `INVENTORY_BLUEPRINT_CONSTRUCTION_GUIDE.md` | Production Guide | Inventory UMG construction guide. |
| `ITEM_AND_PICKUP_ASSET_CREATION_GUIDE.md` | Production Guide | Item definition, pickup archetype and palette workflow. |
| `GRIMROCK_LOCK_SYSTEM.md` | Design / Prospective | Lock, key, lockpicking, traps and lockable containers. |
| `WALL_LOCK_MVP_RUNTIME_BEHAVIOR.md` | Current / MVP | Current wall-lock runtime contract. |

---

## Combat Documents

| Document | Status | Role |
|---|---|---|
| `COMBAT_SYSTEM_V2_ACTION_POINTS_INITIATIVE.md` | Current Target | Global initiative, personal AP, shared mobility and action catalogue. |
| `MON12_1_COMBAT_ACTION_PANEL.md` | Validated / Historical Migration | First reusable combat panel. |
| `MON12_2_COMBAT_ACTION_CLICKS.md` | Validated / Historical Migration | Main/Off-hand compatibility adapter. |
| `MON12_3_CHARACTER_TURN_ACTION_POINTS.md` | Validated | Per-character AP state. |
| `MON12_4_GLOBAL_INITIATIVE_INDIVIDUAL_TURNS.md` | Validated | Mixed initiative and individual turns. |
| `MON12_5_PARTY_MOVEMENT_ACTION_POINTS.md` | Validated | Party translation costs and PAM. |
| `MON12_6_COMBAT_ACTION_CATALOG.md` | Validated Foundation | Generic action definitions and catalogue. |
| `MON12_7_ACTION_ORIENTED_COMBAT_HUD.md` | Validated | Action-oriented HUD. |
| `MON12_7_1_SLIDING_DYNAMIC_INITIATIVE.md` | Validated | Sliding initiative preview and modifiers. |
| `MON12_8_1_PERSISTENT_COMBAT_HOTBAR_MODEL.md` through `MON12_11_HOTBAR_VALIDATION.md` | Validated | Ten persistent shortcuts, execution, targeting, item lifetime and validation. |

---

## Monster Documents

| Document | Status | Role |
|---|---|---|
| `MON13_1_MONSTER_SPAWN_MODEL.md` | Validated | Persistent placement and stable SpawnId. |
| `MON13_2_MONSTER_SPAWN_PIPELINE.md` | Validated | Skeletal editor preview and runtime Actor creation. |
| `MON13_3_MONSTER_RUNTIME_COMMANDS.md` | Validated | Spawn, Despawn, Teleport and lifecycle persistence. |
| `MON13_4_MONSTER_ENCOUNTER_WAVES.md` | Validated | Encounter groups and ordered waves. |
| `MON13_5_MONSTER_SPAWN_CLOSURE.md` | Validated Closure | Production Rat contract and real PIE closure. |
| `MON14_1_AUTOMATIC_PERCEPTION_ENGAGEMENT.md` | Validated | Visual automatic exploration→combat bridge. |
| `MON14_2_DIRECTIONAL_PERCEPTION_PATROL_DATA.md` | Validated | Directional sight, Idle/Dormant and patrol route data. |
| `MON14_3_RUNTIME_PATROL_INVESTIGATION.md` | Validated | Event-driven patrol, investigation and local search. |
| `MON14_3_1_VISUAL_PATROL_ROUTE_EDITOR.md` | Validated | Visual patrol-route editing in the Grid Editor. |
| `MON14_4_EXPLORATION_ALARM_COORDINATION.md` | Validated | Local MON7-based ally alarm and investigation coordination. |
| `MON14_CLOSURE.md` | **Closed** | Cross-cutting MON14 closure and final architecture. |

---

## Documentation Priority Rule

When documents disagree, use this precedence:

1. `99_DECISIONS_LOG.md` and the latest validated milestone/closure document ;
2. `PROJECT_COMPLETION_ROADMAP.md` for what to do next ;
3. `00_PROJECT_OVERVIEW.md` for current project orientation ;
4. current subsystem design/audit documents ;
5. historical roadmaps and task templates only as context.

Do not use an older roadmap to reopen a milestone that a later closure document marks as validated.

---

## Update Rule

At the closure of a major milestone:

1. add a closure or final validation document when useful ;
2. update `00_PROJECT_OVERVIEW.md` ;
3. update `PROJECT_COMPLETION_ROADMAP.md` ;
4. record durable decisions in `99_DECISIONS_LOG.md` ;
5. update the project map/XMind if the architecture or backlog changed materially ;
6. keep old milestone documents as historical evidence instead of rewriting their original scope.
