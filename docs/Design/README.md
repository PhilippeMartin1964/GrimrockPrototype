# GrimrockPrototype Design Docs

This folder is the stable design memory for the GrimrockPrototype grid, object, connector, runtime, and editor workflows. It is meant to keep decisions readable across ChatGPT, Codex, Unreal Editor, and Git sessions.

## Recommended Reading Order

1. `00_PROJECT_OVERVIEW.md` - current project overview.
2. `01_GRID_OBJECT_SYSTEM.md` - current grid object model.
3. `02_OBJECT_ARCHETYPES.md` - current archetype naming and object families.
4. `03_EVENT_COMMAND_LINKS.md` - current connector semantics.
5. `ITEM_CONTEXT_ACTION_SYSTEM.md` - target inventory UX: right-click contextual actions, drag/drop shortcuts, assisted world targets and Cursor deprecation as public model.
6. `INVENTORY_CONTEXT_ACTION_MVP_VALIDATION.md` - validation checklist for the current inventory context action MVP.
7. `INVENTORY_INTERACTION_ROUTING.md` - C++ / Blueprint responsibility map for inventory interactions and context actions.
8. `INVENTORY_BLUEPRINT_CONSTRUCTION_GUIDE.md` - construction guide for inventory UMG widgets, menu, buttons, tooltip, inspect and read panels.
9. `GRIMROCK_LOCK_SYSTEM.md` - prospective lock, key, lockpicking, trap and lockable container design.
10. `WALL_LOCK_MVP_RUNTIME_BEHAVIOR.md` - current MVP wall lock behavior, including explicit inventory key context action, cursor insertion and `Activated -> Door.Open` connector rule.
11. `06_GRID_EDITOR_UX_SPEC.md` - current editor UX target and implemented UI decisions.
12. `07_GRID_OBJECT_ARCHETYPE_ASSET_AUDIT.md` - audit of archetype fields after UI/runtime cleanup.
13. `11_GRID_OBJECT_ARCHETYPE_PARAMETERS_REFERENCE.md` - practical reference explaining each DataAsset / GridObjectArchetypeAsset parameter.
14. `ITEM_AND_PICKUP_ASSET_CREATION_GUIDE.md` - production guide for item definitions, pickup archetypes, palette entries and receptacle content.
15. `08_GRID_OBJECT_ARCHETYPE_DATA_ASSETS_AUDIT.md` - audit/checklist for concrete DataAssets.
16. `09_GRID_OBJECT_ARCHETYPE_NAMING_NORMALIZATION_PLAN.md` - naming normalization plan.
17. `10_GRID_EDITOR_UI_CONSISTENCY_CHECKLIST.md` - UI/runtime consistency checklist.
18. `04_IMPLEMENTATION_ROADMAP.md` - historical implementation roadmap.
19. `05_CODEX_TASKS.md` - historical Codex task templates.
20. `99_DECISIONS_LOG.md` - chronological decision log.
21. `COMBAT_SYSTEM_V2_ACTION_POINTS_INITIATIVE.md` - target combat architecture: global rounds, initiative, personal AP, shared party mobility and action catalogue.
22. `MON12_1_COMBAT_ACTION_PANEL.md` - validated reusable combat-panel vertical slice and V2 migration note.
23. `MON12_2_COMBAT_ACTION_CLICKS.md` - validated MainHand/OffHand compatibility routing and V2 migration note.
24. `MON12_3_CHARACTER_TURN_ACTION_POINTS.md` - authoritative player turn states, four-AP budget, two-AP attacks and UMG migration.
25. `MON12_4_GLOBAL_INITIATIVE_INDIVIDUAL_TURNS.md` - implemented mixed initiative order, one active combatant, end-turn flow and initiative events.

## Document Status

| Document | Status | Role |
|---|---|---|
| `00_PROJECT_OVERVIEW.md` | Current | Project orientation and high-level goals. |
| `01_GRID_OBJECT_SYSTEM.md` | Current | Grid object model and runtime/editor split. |
| `02_OBJECT_ARCHETYPES.md` | Current | Concrete archetype families and naming rules. |
| `03_EVENT_COMMAND_LINKS.md` | Current | Explicit `Source Object + Source Event + Target Object + Command` connector rules. |
| `ITEM_CONTEXT_ACTION_SYSTEM.md` | Design Target / Partial Implementation | Inventory UX target plus the current Patch 1 C++ foundation for contextual action discovery and UMG integration. |
| `INVENTORY_CONTEXT_ACTION_MVP_VALIDATION.md` | Current / Validation | Validation checklist and static audit result for the current context action MVP. |
| `INVENTORY_INTERACTION_ROUTING.md` | Current / Reference | Blueprint versus C++ responsibility map for inventory clicks, drag/drop, context menu execution and UI dismissal. |
| `INVENTORY_BLUEPRINT_CONSTRUCTION_GUIDE.md` | Current / Production Guide | UMG construction guide for inventory slots, context menu, action buttons, tooltip, inspection and reading panels. |
| `GRIMROCK_LOCK_SYSTEM.md` | Design / Prospective | Lock, key, lockpicking, trapped lock and lockable container specification; must stay compatible with the existing Event -> Command model. |
| `WALL_LOCK_MVP_RUNTIME_BEHAVIOR.md` | Current / MVP | Runtime behavior for wall locks: explicit inventory key context action, cursor insertion, visual key attachment and mandatory `Activated -> Door.Open` connector. |
| `04_IMPLEMENTATION_ROADMAP.md` | Historical | Initial roadmap; several phases are done or superseded. |
| `05_CODEX_TASKS.md` | Historical | Prompt/task templates, not the active backlog. |
| `06_GRID_EDITOR_UX_SPEC.md` | Current | Editor UX specification and recent UI cleanup decisions. |
| `07_GRID_OBJECT_ARCHETYPE_ASSET_AUDIT.md` | Audit | Phase 4A archetype field audit, updated after cleanup. |
| `08_GRID_OBJECT_ARCHETYPE_DATA_ASSETS_AUDIT.md` | Audit / Checklist | DataAsset review checklist. |
| `09_GRID_OBJECT_ARCHETYPE_NAMING_NORMALIZATION_PLAN.md` | Checklist | Naming normalization plan. |
| `10_GRID_EDITOR_UI_CONSISTENCY_CHECKLIST.md` | Checklist | UI/runtime consistency checklist for Selected Object, CONNECTORS, orientation, DataAssets and runtime. |
| `11_GRID_OBJECT_ARCHETYPE_PARAMETERS_REFERENCE.md` | Current / Reference | Practical table explaining every current `UGridObjectArchetypeAsset` and `DefaultBehavior` parameter. |
| `ITEM_AND_PICKUP_ASSET_CREATION_GUIDE.md` | Current / Production Guide | Asset creation workflow for inventory item definitions, placeable pickup archetypes, palette entries and receptacle content. |
| `COMBAT_SYSTEM_V2_ACTION_POINTS_INITIATIVE.md` | Design Target | Authoritative target for rounds, global initiative, personal AP, party mobility AP, action catalogue, HUD and revised MON12 roadmap. |
| `MON12_1_COMBAT_ACTION_PANEL.md` | Validated Vertical Slice / Migration | Read-only per-member combat panel, event-driven refresh, Widget Blueprint construction and migration from `AlreadyActed`. |
| `MON12_2_COMBAT_ACTION_CLICKS.md` | Validated Vertical Slice / Migration | MainHand/OffHand click routing kept as a compatibility adapter until the action catalogue replaces slot buttons. |
| `MON12_3_CHARACTER_TURN_ACTION_POINTS.md` | Validated Foundation / Superseded Flow | Authoritative per-character PA state and two-point attack cost; MON12.4 now activates and restores one character at a time. |
| `MON12_4_GLOBAL_INITIATIVE_INDIVIDUAL_TURNS.md` | Implemented / Validation Required | Deterministic global initiative, one active combatant, individual player/monster turns and event model for the future initiative bar. |
| `99_DECISIONS_LOG.md` | Decision Log | Authoritative chronological record of accepted decisions. |

## Documents With Diagrams

The following current design documents include Mermaid diagrams or visual tables:

| Document | Diagram Focus |
|---|---|
| `ITEM_CONTEXT_ACTION_SYSTEM.md` | Context action overview, right-click flow, action execution map, Tooltip / Examiner / Lire distinction. |
| `INVENTORY_CONTEXT_ACTION_MVP_VALIDATION.md` | MVP scope and compact validation matrix. |
| `INVENTORY_INTERACTION_ROUTING.md` | C++ / Blueprint responsibility split, mouse routing, drag/drop and menu dismissal. |
| `INVENTORY_BLUEPRINT_CONSTRUCTION_GUIDE.md` | `WBP_GridInventory`, `WBP_ItemActionMenu`, fullscreen click catcher and `Border_MenuPanel` positioning. |
| `GRIMROCK_LOCK_SYSTEM.md` | Key / lock / door separation, key compatibility and `Activated -> Door.Open` flow. |
| `WALL_LOCK_MVP_RUNTIME_BEHAVIOR.md` | Current WallLock runtime flow and no inventory auto-unlock rule. |
| `ITEM_AND_PICKUP_ASSET_CREATION_GUIDE.md` | `DA_Item_XXX` versus `DA_Object_XXXPickup`, item creation flow and responsibility table. |
| `COMBAT_SYSTEM_V2_ACTION_POINTS_INITIATIVE.md` | Combat lifecycle, action-source catalogue and authoritative action execution sequence. |
| `MON12_3_CHARACTER_TURN_ACTION_POINTS.md` | Player turn-state and action-point lifecycle during the phase-based migration. |
| `MON12_4_GLOBAL_INITIATIVE_INDIVIDUAL_TURNS.md` | Global order, individual turn lifecycle and initiative-bar event contract. |

## Priority Rule

When documents disagree, prefer `99_DECISIONS_LOG.md` and the most recently updated current/audit documents. Historical roadmaps and Codex task templates are useful context, but they do not override later decisions.
