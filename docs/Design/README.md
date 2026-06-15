# GrimrockPrototype Design Docs

This folder is the stable design memory for the GrimrockPrototype grid, object, connector, runtime, and editor workflows. It is meant to keep decisions readable across ChatGPT, Codex, Unreal Editor, and Git sessions.

## Recommended Reading Order

1. `00_PROJECT_OVERVIEW.md` - current project overview.
2. `01_GRID_OBJECT_SYSTEM.md` - current grid object model.
3. `02_OBJECT_ARCHETYPES.md` - current archetype naming and object families.
4. `03_EVENT_COMMAND_LINKS.md` - current connector semantics.
5. `ITEM_CONTEXT_ACTION_SYSTEM.md` - target inventory UX: right-click contextual actions, drag/drop shortcuts, assisted world targets and Cursor deprecation as public model.
6. `GRIMROCK_LOCK_SYSTEM.md` - prospective lock, key, lockpicking, trap and lockable container design.
7. `WALL_LOCK_MVP_RUNTIME_BEHAVIOR.md` - current MVP wall lock behavior, including inventory key use, cursor insertion and `Activated -> Door.Open` connector rule.
8. `06_GRID_EDITOR_UX_SPEC.md` - current editor UX target and implemented UI decisions.
9. `07_GRID_OBJECT_ARCHETYPE_ASSET_AUDIT.md` - audit of archetype fields after UI/runtime cleanup.
10. `11_GRID_OBJECT_ARCHETYPE_PARAMETERS_REFERENCE.md` - practical reference explaining each DataAsset / GridObjectArchetypeAsset parameter.
11. `ITEM_AND_PICKUP_ASSET_CREATION_GUIDE.md` - production guide for item definitions, pickup archetypes, palette entries and receptacle content.
12. `08_GRID_OBJECT_ARCHETYPE_DATA_ASSETS_AUDIT.md` - audit/checklist for concrete DataAssets.
13. `09_GRID_OBJECT_ARCHETYPE_NAMING_NORMALIZATION_PLAN.md` - naming normalization plan.
14. `10_GRID_EDITOR_UI_CONSISTENCY_CHECKLIST.md` - UI/runtime consistency checklist.
15. `04_IMPLEMENTATION_ROADMAP.md` - historical implementation roadmap.
16. `05_CODEX_TASKS.md` - historical Codex task templates.
17. `99_DECISIONS_LOG.md` - chronological decision log.

## Document Status

| Document | Status | Role |
|---|---|---|
| `00_PROJECT_OVERVIEW.md` | Current | Project orientation and high-level goals. |
| `01_GRID_OBJECT_SYSTEM.md` | Current | Grid object model and runtime/editor split. |
| `02_OBJECT_ARCHETYPES.md` | Current | Concrete archetype families and naming rules. |
| `03_EVENT_COMMAND_LINKS.md` | Current | Explicit `Source Object + Source Event + Target Object + Command` connector rules. |
| `ITEM_CONTEXT_ACTION_SYSTEM.md` | Design Target / Partial Implementation | Inventory UX target plus the current Patch 1 C++ foundation for contextual action discovery and UMG integration. |
| `GRIMROCK_LOCK_SYSTEM.md` | Design / Prospective | Lock, key, lockpicking, trapped lock and lockable container specification; must stay compatible with the existing Event -> Command model. |
| `WALL_LOCK_MVP_RUNTIME_BEHAVIOR.md` | Current / MVP | Runtime behavior for wall locks: inventory key auto-use, cursor insertion, visual key attachment and mandatory `Activated -> Door.Open` connector. |
| `04_IMPLEMENTATION_ROADMAP.md` | Historical | Initial roadmap; several phases are done or superseded. |
| `05_CODEX_TASKS.md` | Historical | Prompt/task templates, not the active backlog. |
| `06_GRID_EDITOR_UX_SPEC.md` | Current | Editor UX specification and recent UI cleanup decisions. |
| `07_GRID_OBJECT_ARCHETYPE_ASSET_AUDIT.md` | Audit | Phase 4A archetype field audit, updated after cleanup. |
| `08_GRID_OBJECT_ARCHETYPE_DATA_ASSETS_AUDIT.md` | Audit / Checklist | DataAsset review checklist. |
| `09_GRID_OBJECT_ARCHETYPE_NAMING_NORMALIZATION_PLAN.md` | Checklist | Naming normalization plan. |
| `10_GRID_EDITOR_UI_CONSISTENCY_CHECKLIST.md` | Checklist | UI/runtime consistency checklist for Selected Object, CONNECTORS, orientation, DataAssets and runtime. |
| `11_GRID_OBJECT_ARCHETYPE_PARAMETERS_REFERENCE.md` | Current / Reference | Practical table explaining every current `UGridObjectArchetypeAsset` and `DefaultBehavior` parameter. |
| `ITEM_AND_PICKUP_ASSET_CREATION_GUIDE.md` | Current / Production Guide | Asset creation workflow for inventory item definitions, placeable pickup archetypes, palette entries and receptacle content. |
| `99_DECISIONS_LOG.md` | Decision Log | Authoritative chronological record of accepted decisions. |

## Priority Rule

When documents disagree, prefer `99_DECISIONS_LOG.md` and the most recently updated current/audit documents. Historical roadmaps and Codex task templates are useful context, but they do not override later decisions.
