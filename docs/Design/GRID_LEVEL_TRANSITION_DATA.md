# Grid Relocation ? Runtime and Editor (RELOC01)

## Contract

Teleporter, Stairs and Passage share the user-facing **Relocation** destination panel.
A normal relocation activates automatically when the party enters its source cell, after
movement interpolation completes. There is no Use-action relocation path.

| Object | Destination Level = None | Facing = None |
| --- | --- | --- |
| Normal relocation | Current/source level | Preserve incoming party facing |
| Pit | Automatic lower dungeon level | Existing Pit facing fallback |

Normal candidates are every `Type=Teleporter` and every non-Pit object with
`InstanceConfig.Transition.bIsTransition=true`. Ordinary objects have no relocation panel.
Pits remain separate gameplay objects: PIT01/PIT03, trapdoor state, safe landing selection
and `bUseSameCellCoordinates` retain their existing behavior.

## Serialized compatibility

The canonical authoring payload is `FGridWorldObjectInstance::InstanceConfig.Transition`.
All existing `FGridObjectTransitionParams` fields remain serialized with their original names:
`bIsTransition`, `TargetLevelId`, `TargetCellX`, `TargetCellY`, `TargetFacing`, `bRequireUseAction`.
The two booleans are internal compatibility fields, hidden from Selected Object authoring.
Normal relocation ignores `bRequireUseAction`, including old assets where it is true.

A legacy Teleporter with `bIsTransition=false` reads its destination from
`InstanceConfig.Teleporter.TargetCellX/Y`, with current level and preserved facing.
Loading, resolving or selecting the object never migrates or dirties the asset.
On a Relocation field edit, the inspector resolves the complete existing destination,
then applies the edit to Transition, sets `bIsTransition=true` and `bRequireUseAction=false`,
and mirrors Teleporter X/Y into both payloads. Other normal relocation edits set the same
internal flags. There is no second persistent relocation structure or PostLoad migration.

## Selected Object workflow

Select a Teleporter, Stairs, Passage or existing transition object. The single **Relocation**
panel contains **Destination Level**, **Destination Cell X**, **Destination Cell Y**, **Facing**.
Destination Level is a text field: enter an enabled dungeon LevelId, or None for current level.
Commit text with Enter or by leaving the field. Facing includes None to preserve facing.
Changes are applied to the selected instance without a separate Apply button.

For Pit the panel explains `None = automatic lower dungeon level`. Its separate Pit section
retains Open at Start and Use Same Cell Coordinates; the latter disables destination X/Y edits.
For Teleporter the existing Enabled initial state remains in the Game Object section.
Runtime enable/disable commands use `UGridActivationComponent`'s active-object state;
an inactive Teleporter never relocates the party, but its destination is still validated.

## Runtime and validation

The public compatibility APIs `FindTransitionAtCell` and `TryExecuteTransitionAtCell` retain
their names and signatures. Lookup excludes Pit, resolves legacy data, ignores the historical
Use gate, and checks the Teleporter activation state through `IsObjectActive`.

Movement completion keeps this order: Pit priority, HandlePartyCellChanged, combat turn
completion, normal relocation. Successful relocation clears buffered movement.

For None or the explicit current LevelId, relocation validates the current level cell,
updates pawn coordinates/facing, calls `SnapToCurrentCell`, and notifies cell change for
pressure plates, triggers and perception. It does **not** call TravelToDungeonLevel,
RebuildLevel or runtime-state capture/restore. A guard covers notifications and prevents
nested relocation; the destination is not recursively checked. One entry produces one hop.

For another LevelId, the existing `TravelToDungeonLevel` path validates an enabled entry
with a LevelAsset, captures source state, changes CurrentDungeonLevelId/LevelAsset,
rebuilds and restores target runtime state. Normal None is resolved before this call.
`TravelToDungeonLevel(None)` is not a Pit-aware API.

Execution requires a valid pawn and an existing in-bounds destination cell which is neither
Empty nor occupancy-blocked. Editor validation and dungeon diagnostics accept normal None
and Facing None, inspect the source asset for current-level destinations, and inspect the
enabled target asset for cross-level destinations. Invalid coordinates/levels are errors;
Empty and occupancy-blocked destinations are warnings in diagnostics. Disabled Teleporters
are counted and validated, using legacy X/Y where applicable. Pit None must resolve a lower
level or produces the existing Pit error; same-cell coordinates retain priority.

The diagnostic API `GetTransitionDiagnostics` keeps its compatibility name; displayed counts
are `RelocationObjects` (including dedicated Pit destinations).

## StairsUp / StairsDown

`Stairs Up` et `Stairs Down` sont des objets de palette dédiés aux transitions entre niveaux.

Ils utilisent le système existant de définitions :

- `DefinitionId = Stairs_Up`
- `DefinitionId = Stairs_Down`
- `SupportedType = Decoration`
- `PlacementSurface = Floor`
- `DefaultLocalPosition = U=0, V=0, N=0` sauf ajustement visuel volontaire de la définition
- `Category = Transitions`
- `RuntimeActorClass = AGridGenericObjectActor`
- `bBlocksMovement = false`

Depuis ALIGN-B5.3, `PlacementSurface + DefaultLocalPosition.U/V/N` est l'unique contrat de placement world-object ; l'ancien `PlacementKind` n'existe plus.

Meshes attendus :

- `Stairs_Up` utilise `SM_Stairs_Up_01`.
- `Stairs_Down` utilise `SM_Stairs_Down_01`.

### Stairs_Down rendering behavior

`Stairs_Down` masque le mesh de sol standard de sa cellule via `UGridWorldObjectDefinitionAsset::bHideCellFloor = true`.

Ce comportement est un override de rendu porté par la définition :

- la cellule reste inchangée dans le `UGridLevelAsset`;
- la cellule ne devient pas `Empty`;
- la cellule reste franchissable si ses données de cellule le permettent;
- la logique de transition runtime ne change pas;
- seul le floor standard, par exemple `SM_Floor_Stone_01`, n'est pas généré sous l'escalier.

`Stairs_Up` conserve le floor standard pour l'instant.

Le traitement du mur ou d'une face sombre devant la descente est prévu séparément. Une option future pourra utiliser un flag du type `bHideForwardWall` ou `bSpawnDarkForwardPlane`, basé sur une orientation fiable de l'objet de sol. Ce comportement n'est pas activé automatiquement dans cette étape.

## Verification

`Grimrock.Relocation.RELOC01` covers legacy and modern data, authoring normalization,
automatic enabled/disabled entry, buffered movement, same-level rebuild/state invariants,
return-teleporter loop prevention, cross-level travel and persistence, facing preservation,
Pit lower-level resolution and both editor/dungeon validation.

Use `Scripts/ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.Relocation.RELOC01"`.
Relevant regressions: `Grimrock.Pit.PIT01`, `Grimrock.WorldObjects`,
`Grimrock.TechnicalDebt.TD03`, `Grimrock.LUAUX03`.
