# GrimrockPrototype Design Docs

This folder is the stable design memory for the GrimrockPrototype grid, object, connector, runtime, and editor workflows. It is meant to keep decisions readable across ChatGPT, Codex, Unreal Editor, and Git sessions.

## Recommended Reading Order

1. `00_PROJECT_OVERVIEW.md` - current project overview.
2. `01_GRID_OBJECT_SYSTEM.md` - current grid object model.
3. `02_OBJECT_ARCHETYPES.md` - current archetype naming and object families.
4. `03_EVENT_COMMAND_LINKS.md` - current connector semantics.
5. `06_GRID_EDITOR_UX_SPEC.md` - current editor UX target and implemented UI decisions.
6. `07_GRID_OBJECT_ARCHETYPE_ASSET_AUDIT.md` - audit of archetype fields after UI/runtime cleanup.
7. `08_GRID_OBJECT_ARCHETYPE_DATA_ASSETS_AUDIT.md` - audit/checklist for concrete DataAssets.
8. `09_GRID_OBJECT_ARCHETYPE_NAMING_NORMALIZATION_PLAN.md` - naming normalization plan.
9. `04_IMPLEMENTATION_ROADMAP.md` - historical implementation roadmap.
10. `05_CODEX_TASKS.md` - historical Codex task templates.
11. `99_DECISIONS_LOG.md` - chronological decision log.

## Document Status

| Document | Status | Role |
|---|---|---|
| `00_PROJECT_OVERVIEW.md` | Current | Project orientation and high-level goals. |
| `01_GRID_OBJECT_SYSTEM.md` | Current | Grid object model and runtime/editor split. |
| `02_OBJECT_ARCHETYPES.md` | Current | Concrete archetype families and naming rules. |
| `03_EVENT_COMMAND_LINKS.md` | Current | Explicit `Source Object + Source Event + Target Object + Command` connector rules. |
| `04_IMPLEMENTATION_ROADMAP.md` | Historical | Initial roadmap; several phases are done or superseded. |
| `05_CODEX_TASKS.md` | Historical | Prompt/task templates, not the active backlog. |
| `06_GRID_EDITOR_UX_SPEC.md` | Current | Editor UX specification and recent UI cleanup decisions. |
| `07_GRID_OBJECT_ARCHETYPE_ASSET_AUDIT.md` | Audit | Phase 4A archetype field audit, updated after cleanup. |
| `08_GRID_OBJECT_ARCHETYPE_DATA_ASSETS_AUDIT.md` | Audit / Checklist | DataAsset review checklist. |
| `09_GRID_OBJECT_ARCHETYPE_NAMING_NORMALIZATION_PLAN.md` | Checklist | Naming normalization plan. |
| `99_DECISIONS_LOG.md` | Decision Log | Authoritative chronological record of accepted decisions. |

## Priority Rule

When documents disagree, prefer `99_DECISIONS_LOG.md` and the most recently updated current/audit documents. Historical roadmaps and Codex task templates are useful context, but they do not override later decisions.
