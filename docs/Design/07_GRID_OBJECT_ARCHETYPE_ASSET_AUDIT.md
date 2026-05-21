# Grid Object Archetype Asset Audit

Phase 4A audit for `UGridObjectArchetypeAsset`.

This document is documentation only. It does not propose an immediate code refactor and does not require changes to enums, runtime logic, editor logic, DataAssets, or serialization.

## 1. Purpose of GridObjectArchetypeAsset

`UGridObjectArchetypeAsset` defines the default identity, classification, placement, visual, runtime actor, light, interaction, and behavior data for a concrete grid object archetype.

An archetype is not the same thing as an object type. `EGridLevelObjectType` describes broad gameplay behavior such as Door, Button, Lever, Receptacle, Trigger, or Item. A concrete archetype describes a usable authored variant such as `Button_Secret`, `Door_Stone`, `Receptacle_TorchHolder`, or an item archetype.

Placed objects store instance data in `FGridLevelObjectData`, including `ArchetypeId`, placement cell/edge, enabled/active state, tag, notes, readable override text, palette entry id, and `FGridObjectBehaviorParams`. At placement time, selected editor values are copied into the placed object. Runtime then resolves `ArchetypeId` back to `UGridObjectArchetypeAsset` to choose meshes, materials, placement transform, actor class, light/readable options, and item metadata.

## 2. Field Audit Table

Recommendations use the following vocabulary:

- `Essential`: core data required by the current editor or runtime.
- `Advanced`: valid data, but should generally be hidden or displayed only in expert/debug contexts.
- `Archetype Only`: should be authored in DataAssets and not edited per placed instance.
- `Legacy`: retained for compatibility or migration.
- `Remove Later`: candidate for removal after migration and verification.
- `Needs Clarification`: current role is valid but naming, usage, or ownership is ambiguous.

### Identity

| Field | Category | Type | Used In Editor | Used In Runtime | Object Types Concerned | Instance Override? | Recommendation | Notes |
|---|---|---|---|---|---|---|---|---|
| `ArchetypeId` | Archetype | `FName` | Palette validation, object placement, inspector display/edit via placed `FGridLevelObjectData::ArchetypeId`, overview labels. | Runtime archetype lookup, item spawning, readable/item/receptacle lookup. | All archetyped objects and items. | Placed objects store `ArchetypeId`; the archetype field itself is DataAsset-only. | Essential | Stable identifier. Renaming breaks placed objects and references unless migrated. |
| `DisplayName` | Archetype | `FText` | Inspector header and connector/object summaries prefer this when available. | Not directly used by runtime behavior. | All displayable archetypes. | No. | Archetype Only | Designer-facing name. Safe to improve without serialization risk beyond asset text changes. |
| `SupportedType` | Archetype | `EGridLevelObjectType` | Validation, palette/object type assignment, contextual inspector expectations. | Validation and runtime spawn requirements are derived from object type, but placed object also stores `Type`. | All archetypes. | Placed object stores `Type`; archetype remains source of intended type. | Essential | Potentially confusing beside placed object `Type`, `Category`, and `ObjectCategory`. |
| `Description` | Archetype | `FText` | Currently mostly authoring documentation. | Not used by runtime behavior. | All archetypes. | No. | Archetype Only | Useful for DataAsset authoring, not currently surfaced strongly in inspector. |

### Defaults

| Field | Category | Type | Used In Editor | Used In Runtime | Object Types Concerned | Instance Override? | Recommendation | Notes |
|---|---|---|---|---|---|---|---|---|
| `bDefaultInitiallyEnabled` | Defaults | `bool` | Intended default copied into editor paint/placed object state; inspector edits instance `bInitiallyEnabled`. | Runtime checks placed `ObjectData.bInitiallyEnabled` for spawn/activation. | All placed objects. | Yes, through `FGridLevelObjectData::bInitiallyEnabled`. | Essential | Should remain an archetype default, not a live runtime source after placement. |
| `bDefaultInitiallyActive` | Defaults | `bool` | Intended default copied into editor paint/placed object state; inspector edits instance `bInitiallyActive`. | Runtime objects and door/receptacle initial state read placed `ObjectData.bInitiallyActive`. | Doors, receptacles, mechanisms, triggers where active state matters. | Yes, through `FGridLevelObjectData::bInitiallyActive`. | Essential | Meaning varies by type: open, pressed, filled, activated, etc. Needs clear UI labels per type. |
| `DefaultTag` | Defaults | `FName` | Intended default for placed object tag. Inspector edits instance `Tag`. | Receptacle runtime can use placed `ObjectData.Tag` as fallback accepted item tag when acceptance arrays are empty. | Mostly receptacles and tagged interaction objects. | Yes, through `FGridLevelObjectData::Tag`. | Advanced | Useful but technical. Should stay out of primary inspector except advanced/debug or contextual rules. |
| `DefaultBehavior` | Defaults | `FGridObjectBehaviorParams` | Copied to placed objects and reset behavior helper; inspector edits placed `Obj.Behavior`. | Runtime reads placed `ObjectData.Behavior`, not the archetype directly, for activation, trigger, teleporter, receptacle, button, item spawn. | Trigger, pressure plate, button, lever, receptacle, teleporter, item spawn. | Yes, through `FGridLevelObjectData::Behavior`. | Essential | This is the main archetype-to-instance default bridge. It contains several behavior groups that are only relevant to certain types. |
| `ItemTags` | Item | `TArray<FName>` | Validation and item/receptacle authoring context. | Item actors are initialized with tags; receptacles inspect item archetype tags for acceptance. | Items and item-like archetypes. | No placed-object override currently. | Essential | Runtime-relevant for inventory/receptacle matching. |

### Palette / Classification

| Field | Category | Type | Used In Editor | Used In Runtime | Object Types Concerned | Instance Override? | Recommendation | Notes |
|---|---|---|---|---|---|---|---|---|
| `Category` | Palette | `FName` | Palette grouping and validation hints. Inspector displays as read-only archetype info. | Not used for gameplay. | All palette entries/archetypes. | No. | Archetype Only | Confusing with `ObjectCategory`; this is UI grouping only. |
| `ObjectCategory` | Archetype | `EGridObjectCategory` | Validation, overview/inspector classification, contextual readability/light expectations. | Not directly used for runtime behavior, except helper methods use it to decide whether params are relevant. | All archetypes. | No. | Needs Clarification | Functional/editor classification, not the same as `SupportedType` or palette `Category`. |

### Placement

| Field | Category | Type | Used In Editor | Used In Runtime | Object Types Concerned | Instance Override? | Recommendation | Notes |
|---|---|---|---|---|---|---|---|---|
| `PlacementKind` | Placement | `EGridObjectPlacementKind` | Determines edge/cell placement, validation, inspector display, conflict logic, viewport connector center fallback. | Runtime placement transform chooses wall/edge/center/ceiling path through helpers. | All placed archetypes. | No direct instance override; placed object stores cell/edge and `LocalYaw`. | Essential | Source of truth for placement. Supersedes legacy placement booleans. |
| `bPlaceOnEdge` | Placement\|Legacy | `bool` | Validation warns when inconsistent; palette validation also warns on legacy use. | Not a runtime source of truth. | Legacy assets only. | No. | Legacy | Already marked legacy/advanced. Remove only after asset migration. |
| `bPlaceAtCellCenter` | Placement\|Legacy | `bool` | Validation warns when inconsistent. | Not a runtime source of truth. | Legacy assets only. | No. | Legacy | Candidate to remove later after migration. |
| `bCanShareCell` | Placement | `bool` | Placement conflict removal checks new and existing archetypes. | Not used by runtime. | Objects that may share a cell, decorations, floor objects, triggers. | No. | Essential | Editor placement rule only, but important for authoring safety. |
| `bCanShareAnchor` | Placement | `bool` | Placement conflict removal checks same anchor/edge; validation warns for doors. | Not used by runtime. | Edge/wall objects, especially doors, buttons, levers, receptacles. | No. | Essential | Editor placement rule. Prevents overlapping anchors where needed. |
| `bBlocksMovement` | Placement | `bool` | Inspector displays read-only; validation/tooltips clarify non-door usage. | `AGridGenericObjectActor` sets mesh collision from this. Door blocking is handled by door system. | Generic non-door blocking props/decorations. | No. | Needs Clarification | Confusing for doors because door blocking is separate. UI should call this "Generic Blocks Movement" or contextualize it. |
| `PlacementZOffset` | Placement | `float` | Used by preview placement and viewport connector center. | Runtime placement transform for wall/center objects. | All placed visual archetypes except doors use special edge transform. | No. | Essential | Important visual placement offset. For doors, runtime currently uses edge transform instead. |
| `WallInset` | Placement\|Wall | `float` | Used by preview placement and connector center for wall/edge objects; validation checks relevance. | Runtime wall-mounted transform. | Wall/edge objects: buttons, levers, wall decor, receptacles, lights. | No. | Essential | Only meaningful when `PlacementKind` is Wall or Edge. |
| `LocalOffsetAlongWall` | Placement\|Wall | `float` | Used by preview placement and connector center; validation checks relevance. | Runtime wall-mounted transform. | Wall/edge objects. | No. | Essential | Defines lateral offset along wall face. |
| `LocalOffsetVertical` | Placement\|Wall | `float` | Used by preview placement and connector center; validation checks relevance. | Runtime wall-mounted transform. | Wall/edge objects. | No. | Essential | Added to `PlacementZOffset`. |
| `RotationStepYaw` | Placement\|Rotation | `float` | Selected object rotation step uses archetype if available. | Runtime transform uses placed object `LocalYaw`, not this field directly. | Center/floor/ceiling objects or any object that supports local yaw editing. | No; influences editor operation that changes instance `LocalYaw`. | Advanced | Useful as authoring/tool behavior, but not direct gameplay data. |

### Interaction

| Field | Category | Type | Used In Editor | Used In Runtime | Object Types Concerned | Instance Override? | Recommendation | Notes |
|---|---|---|---|---|---|---|---|---|
| `bIsInteractable` | Interaction | `bool` | Inspector and validation display/verify interactable expectations. | Runtime interaction is mostly actor/component driven; this is not currently the only source of interaction behavior. | Buttons, levers, receptacles, readable decorations. | No. | Needs Clarification | Useful classification, but runtime authority should be documented more clearly. |
| `bIsReadable` | Interaction | `bool` | Inspector selects readable text section when true; validation checks readable consistency. | Generic object actor copies archetype `ReadableText` and readable-only-once flag when true. | Readable decorations/props. | No, but placed object can override text. | Essential | Runtime readable flag. Confusing beside `ObjectCategory == Readable`. |
| `ReadableText` | Interaction | `FText` | DataAsset text; inspector uses placed `OverrideReadableText` for instance editing. | Generic object actor uses this unless placed object override exists. | Readable objects. | Yes, via `FGridLevelObjectData::OverrideReadableText`. | Essential | Archetype default text. Instance override should be the inspector edit path. |
| `bShowReadableOnlyOnce` | Interaction | `bool` | Validation checks it only matters when readable. | Generic object actor stores it as runtime readable-only-once behavior. | Readable objects. | No instance override currently. | Advanced | Runtime handling should be documented with activation/readable flow. |

### Light

| Field | Category | Type | Used In Editor | Used In Runtime | Object Types Concerned | Instance Override? | Recommendation | Notes |
|---|---|---|---|---|---|---|---|---|
| `bIsLightSource` | Light | `bool` | Inspector shows Light section when true; validation checks consistency. | Generic object actor enables point light when true. | Light archetypes and lit props/receptacles. | No. | Essential | Runtime light flag. Confusing beside `SupportedType == Light` and `ObjectCategory == Light`. |
| `LightColor` | Light | `FLinearColor` | Inspector displays read-only light info. | Generic object actor applies point light color. | Light source archetypes. | No. | Essential | Archetype-only visual/runtime light setting. |
| `LightIntensity` | Light | `float` | Inspector displays read-only light info; validation checks positive values for lights. | Generic object actor applies point light intensity. | Light source archetypes. | No. | Essential | May overlap with `GridLightEmitterComponent` defaults in other runtime systems. |
| `LightRadius` | Light | `float` | Inspector displays read-only light info; validation checks positive values for lights. | Generic object actor applies attenuation radius. | Light source archetypes. | No. | Essential | Keep archetype-only for now. |
| `bUseLightFlicker` | Light | `bool` | Inspector displays read-only "Flicker"; validation checks consistency. | Not currently applied by `AGridGenericObjectActor`; flicker exists in `GridLightEmitterComponent` but is not wired here. | Light source archetypes. | No. | Needs Clarification | Field exists and is validated/displayed, but generic runtime light path does not use flicker yet. |

### Visual

| Field | Category | Type | Used In Editor | Used In Runtime | Object Types Concerned | Instance Override? | Recommendation | Notes |
|---|---|---|---|---|---|---|---|---|
| `PreviewMesh` | Visual | `TObjectPtr<UStaticMesh>` | Editor preview object mesh preference; inspector advanced/debug display. | Runtime mesh preference before `MovingMesh` and `FixedMesh` in generic spawn path; validation checks required mesh presence. | Most visible objects. | No. | Essential | Name can confuse: it is not editor-only. It is the main/simple mesh. |
| `PreviewMaterial` | Visual | `TObjectPtr<UMaterialInterface>` | Editor preview material preference; inspector advanced/debug display indirectly by mesh fields only today. | Runtime material preference before moving/fixed material. | Most visible objects. | No. | Essential | Name can confuse for the same reason as `PreviewMesh`. |
| `FixedMesh` | Visual | `TObjectPtr<UStaticMesh>` | Advanced/debug display; validation checks relevance. | Runtime fallback mesh; mechanism/door fixed part; receptacle contained-item visual uses moving mesh separately. | Doors, composite mechanisms, items. | No. | Advanced | Should stay advanced for simple objects. |
| `MovingMesh` | Visual | `TObjectPtr<UStaticMesh>` | Advanced/debug display; validation checks relevance. | Runtime fallback mesh; mechanism/door moving part; item/receptacle visuals. | Doors, buttons, levers, receptacles, items. | No. | Advanced | Important for composite/animated actors. Needs clearer UI distinction from `PreviewMesh`. |
| `FixedMaterial` | Visual | `TObjectPtr<UMaterialInterface>` | Validation checks relevance. | Runtime fallback material; mechanism/door fixed material. | Doors, composite mechanisms, items. | No. | Advanced | Pair with `FixedMesh`. |
| `MovingMaterial` | Visual | `TObjectPtr<UMaterialInterface>` | Validation checks relevance. | Runtime fallback material; mechanism/door moving material; item/receptacle visuals. | Doors, buttons, levers, receptacles, items. | No. | Advanced | Pair with `MovingMesh`. |

### Runtime

| Field | Category | Type | Used In Editor | Used In Runtime | Object Types Concerned | Instance Override? | Recommendation | Notes |
|---|---|---|---|---|---|---|---|---|
| `RuntimeActorClass` | Runtime | `TSubclassOf<AGridRuntimeObjectActor>` | Validation and inspector advanced/debug display. | Runtime spawn class for objects; required for several types; class-specific initialization follows casts. | Doors, buttons, levers, pressure plates, teleporters, receptacles, runtime objects. | No. | Essential | Defines how the archetype is instantiated. Confusing beside `SupportedType`, which defines what it is. |
| `ItemActorClass` | Runtime | `TSubclassOf<AGridItemActor>` | Validation for item archetypes. | Item spawn helper uses it, falling back to `AGridItemActor`. | Item archetypes and item spawns. | No. | Essential | Separate from `RuntimeActorClass`; used for inventory/world item actors. |

### Validation / Helpers

`UGridObjectArchetypeAsset` also exposes helper functions, not UPROPERTY fields:

- `IsEdgePlaced`, `IsCenterPlaced`, `IsWallPlaced`, `IsCeilingPlaced`
- `IsReadable`, `IsLightSource`
- `ValidateArchetype`, `IsValidArchetype`, `GetValidationSummary`
- `RequiresEdgePlacement`, `SupportsCenterPlacement`, `SupportsWallPlacement`
- `RequiresRuntimeActorClass`, `AllowsInvisibleRuntimeObject`
- `UsesWallPlacementParams`, `UsesCenterPlacementParams`, `UsesReadableParams`, `UsesLightParams`
- `UsesItemParams`, `UsesItemSpawnParams`, `UsesReceptacleParams`, `UsesTeleporterParams`
- `UsesButtonAnimationParams`, `UsesTriggerParams`, `UsesMovingMeshParams`, `UsesFixedMeshParams`, `UsesRuntimeActorClass`

These helpers are important because they encode current design intent. Future editor cleanup should reuse them instead of duplicating object-type rules in UI code.

### Legacy Fields

| Field | Category | Type | Used In Editor | Used In Runtime | Object Types Concerned | Instance Override? | Recommendation | Notes |
|---|---|---|---|---|---|---|---|---|
| `bPlaceOnEdge` | Placement\|Legacy | `bool` | Validation and compatibility warnings. | No. | Migrated legacy archetypes. | No. | Legacy | Keep until all assets/palette entries are migrated and warnings are clean. |
| `bPlaceAtCellCenter` | Placement\|Legacy | `bool` | Validation and compatibility warnings. | No. | Migrated legacy archetypes. | No. | Legacy | Same migration path as `bPlaceOnEdge`. |

## 3. Fields Grouped By Responsibility

### Identity

- `ArchetypeId`
- `DisplayName`
- `SupportedType`
- `Description`

### Defaults

- `bDefaultInitiallyEnabled`
- `bDefaultInitiallyActive`
- `DefaultTag`
- `DefaultBehavior`
- `ItemTags`

### Palette / Classification

- `Category`
- `ObjectCategory`

### Placement

- `PlacementKind`
- `bCanShareCell`
- `bCanShareAnchor`
- `bBlocksMovement`
- `PlacementZOffset`
- `WallInset`
- `LocalOffsetAlongWall`
- `LocalOffsetVertical`
- `RotationStepYaw`

### Interaction

- `bIsInteractable`
- `bIsReadable`
- `ReadableText`
- `bShowReadableOnlyOnce`

### Light

- `bIsLightSource`
- `LightColor`
- `LightIntensity`
- `LightRadius`
- `bUseLightFlicker`

### Visual

- `PreviewMesh`
- `PreviewMaterial`
- `FixedMesh`
- `MovingMesh`
- `FixedMaterial`
- `MovingMaterial`

### Runtime

- `RuntimeActorClass`
- `ItemActorClass`

### Validation / Helpers

- Helper methods listed in the previous section.
- Validation messages use the archetype state to identify inconsistent type/category/placement/visual/runtime setups.

### Legacy Fields

- `bPlaceOnEdge`
- `bPlaceAtCellCenter`

## 4. Confusing Fields

### `SupportedType` vs `ObjectCategory` vs `Category`

- `SupportedType` is the broad gameplay type. It should answer "what behavior family is this?"
- `ObjectCategory` is an editor/validation functional category. It helps group semantic roles, but does not directly drive runtime behavior.
- `Category` is palette organization only and does not affect runtime.

The editor should avoid showing all three as equal concepts in primary object UI. `SupportedType` is essential. `ObjectCategory` and `Category` are useful read-only metadata or DataAsset authoring fields.

### `PlacementKind` vs `bPlaceOnEdge` vs `bPlaceAtCellCenter`

`PlacementKind` is the current source of truth. The two booleans are legacy compatibility fields. Showing all three together in normal UI would imply competing placement rules.

### `PreviewMesh` vs `FixedMesh` vs `MovingMesh`

`PreviewMesh` is not only an editor preview mesh. It is currently the primary/simple mesh used by runtime mesh selection before falling back to moving/fixed meshes. `FixedMesh` and `MovingMesh` are for composite or animated actors. The naming can mislead designers into thinking `PreviewMesh` is editor-only.

### `PreviewMaterial` vs `FixedMaterial` vs `MovingMaterial`

Same issue as meshes. `PreviewMaterial` is the primary/simple material, not purely preview-only.

### `RuntimeActorClass` vs `ItemActorClass`

`RuntimeActorClass` spawns grid runtime objects such as doors, buttons, pressure plates, receptacles, and generic objects. `ItemActorClass` spawns carried/world item actors. Item-related archetypes may need one, the other, or neither depending on whether they are placed as grid objects or spawned as inventory/world items.

### `bIsReadable` vs `ObjectCategory == Readable`

`bIsReadable` is the runtime readable flag. `ObjectCategory == Readable` is classification. Validation tries to keep them aligned for readable decorations, but runtime behavior depends on `bIsReadable`.

### `bIsLightSource` vs `SupportedType == Light`

`bIsLightSource` controls runtime point light setup on generic objects. `SupportedType == Light` is broad object type/classification. A non-Light supported type can still be a light source, such as a torch holder or lit decoration.

### `bBlocksMovement` vs door blocking

`bBlocksMovement` controls generic object mesh collision in `AGridGenericObjectActor`. Door passage blocking is handled elsewhere by the door system. For doors, this field can confuse designers if displayed without context.

### `bUseLightFlicker`

The field is validated and displayed, but the generic object light setup currently applies color/intensity/radius only. Flicker appears to belong to `GridLightEmitterComponent`, not the archetype-driven generic light path yet. This needs clarification before exposing it as an important gameplay setting.

## 5. Archetype Only vs Instance Override

### Archetype Only

These should generally be edited only in DataAssets:

- Identity and classification: `ArchetypeId`, `DisplayName`, `SupportedType`, `Description`, `Category`, `ObjectCategory`
- Placement rules and offsets: `PlacementKind`, `bCanShareCell`, `bCanShareAnchor`, `bBlocksMovement`, `PlacementZOffset`, `WallInset`, `LocalOffsetAlongWall`, `LocalOffsetVertical`, `RotationStepYaw`
- Interaction capabilities: `bIsInteractable`, `bIsReadable`, `bShowReadableOnlyOnce`
- Light defaults: `bIsLightSource`, `LightColor`, `LightIntensity`, `LightRadius`, `bUseLightFlicker`
- Visuals: `PreviewMesh`, `PreviewMaterial`, `FixedMesh`, `MovingMesh`, `FixedMaterial`, `MovingMaterial`
- Runtime classes: `RuntimeActorClass`, `ItemActorClass`
- Item metadata: `ItemTags`
- Legacy placement flags: `bPlaceOnEdge`, `bPlaceAtCellCenter`

### Instance Override

These archetype fields are defaults that are copied to or mirrored by placed object data:

- `bDefaultInitiallyEnabled` -> `FGridLevelObjectData::bInitiallyEnabled`
- `bDefaultInitiallyActive` -> `FGridLevelObjectData::bInitiallyActive`
- `DefaultTag` -> `FGridLevelObjectData::Tag`
- `DefaultBehavior` -> `FGridLevelObjectData::Behavior`
- `ReadableText` -> can be overridden by `FGridLevelObjectData::OverrideReadableText`
- `ArchetypeId` -> placed object stores the selected `ArchetypeId`, but should not mutate the DataAsset field

Instance-owned fields that are not archetype fields include object id, cell, edge, local yaw, notes, palette entry id, and links.

### Read-only in Inspector

These are useful to display in object inspector context but should not be edited there in the near term:

- `DisplayName`
- `SupportedType`
- `Category`
- `ObjectCategory`
- `PlacementKind`
- `bIsInteractable`
- `bBlocksMovement`
- `bIsLightSource`
- `LightColor`
- `LightIntensity`
- `LightRadius`
- `bUseLightFlicker`
- `RuntimeActorClass`
- `PreviewMesh`
- `FixedMesh`
- `MovingMesh`

Editable inspector fields should remain focused on placed-instance data: initially enabled/active, behavior params, readable override text, tag/notes/debug fields, and connector editing.

## 6. Immediate Cleanup Opportunities

Safe future improvements, without changing runtime or serialization:

- Keep `bPlaceOnEdge` and `bPlaceAtCellCenter` in `AdvancedDisplay`; remove only after a migration pass confirms no assets or palette entries depend on them.
- Improve tooltips for `SupportedType`, `Category`, and `ObjectCategory` so designers understand gameplay type vs palette grouping vs editor classification.
- Rename display labels, not property names, for `PreviewMesh` and `PreviewMaterial` to "Main Mesh" and "Main Material" in editor-facing UI.
- Clarify `bBlocksMovement` label as generic/non-door blocking where shown.
- Clarify that `bIsReadable` and `bIsLightSource` are runtime capability flags, while `ObjectCategory` is classification.
- Document which object types use each `DefaultBehavior` group:
  - Activation: buttons, levers, pressure plates, triggers, receptacles, teleporters where links are emitted.
  - Trigger: triggers and pressure plates.
  - Teleporter: teleporters.
  - Receptacle: receptacles.
  - ButtonAnimation: buttons.
  - ItemSpawn: item spawns.
- Avoid displaying irrelevant fields per object type in the inspector. Prefer contextual read-only summaries.
- Clarify or wire `bUseLightFlicker` before presenting it as active runtime behavior.
- Consider a future DataAsset editor layout that groups "Designer Basics", "Placement", "Visuals", "Runtime Class", and "Advanced/Validation".

## 7. Risks

Renaming or removing fields has high risk because these are UPROPERTY fields on DataAssets and several are referenced by Blueprint/editor/runtime systems.

- Existing DataAssets may break or silently lose values if UPROPERTY names change without redirects/migration.
- Blueprint references may break for `RuntimeActorClass`, `ItemActorClass`, meshes, materials, placement settings, or helper-exposed fields.
- Serialized level assets may lose data if placed object defaults or copied behavior assumptions change.
- Runtime actor spawning may fail if `RuntimeActorClass`, mesh selection, placement, or `ArchetypeId` lookup changes.
- Receptacle and item behavior may regress if `ItemTags`, `DefaultBehavior.Receptacle`, `MovingMesh`, or `MovingMaterial` semantics change.
- Door behavior may regress if `SupportedType`, `RuntimeActorClass`, or edge placement semantics change.
- Editor placement may regress if `PlacementKind`, sharing flags, or wall offset fields are renamed/removed.

## 8. Final Recommendation

Do not refactor `UGridObjectArchetypeAsset` immediately.

First document the asset and current usage boundaries. Then clean up only display labels, tooltips, categories, and contextual inspector presentation. Treat `PlacementKind`, `ArchetypeId`, runtime classes, visual fields, and behavior defaults as stable serialized API until a deliberate migration plan exists.

Only remove legacy placement fields after migration has been completed and validated across DataAssets, palette entries, placed level objects, Blueprint references, and runtime/editor validation.
