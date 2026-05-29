# Grid Level Transition Data

## Purpose

Grid transitions are stored as object behavior data in `UGridLevelAsset`. Editor diagnostics validate transition intent, and runtime execution can switch the active `LevelAsset` inside one `AGridLevelRuntimeActor`.

## Supported Uses

A placed grid object can be marked as a transition and configured as:

- `StairsUp`
- `StairsDown`
- `PassageNorth`
- `PassageSouth`
- `PassageEast`
- `PassageWest`
- future `Portal`

These names should be represented by archetypes or palette entries. They should not become new `EGridLevelObjectType` values unless a later runtime requirement proves that necessary.

## StairsUp / StairsDown

`Stairs Up` et `Stairs Down` sont des objets de palette dédiés aux transitions entre niveaux.

Ils utilisent le système existant d'archétypes :

- `ArchetypeId = Stairs_Up`
- `ArchetypeId = Stairs_Down`
- `SupportedType = Decoration`
- `PlacementKind = Floor`
- `Category = Transitions`
- `RuntimeActorClass = AGridGenericObjectActor`
- `bBlocksMovement = false`

Meshes attendus :

- `Stairs_Up` utilise `SM_Stairs_Up_01`.
- `Stairs_Down` utilise `SM_Stairs_Down_01`.

Les escaliers sont placés au centre d'une cellule et ne bloquent pas le déplacement. Ils portent par défaut :

```text
Behavior.Transition.bIsTransition = true
Behavior.Transition.TargetLevelId = None
Behavior.Transition.TargetCellX = 0
Behavior.Transition.TargetCellY = 0
Behavior.Transition.TargetFacing = North
Behavior.Transition.bRequireUseAction = false
```

`TargetLevelId` reste vide par défaut. Le diagnostic de transitions doit donc afficher une erreur tant que la destination n'est pas configurée. C'est attendu.

Workflow recommandé :

1. Créer ou sélectionner un niveau cible dans `DUNGEON LEVELS`.
2. Dans le niveau source, placer `Stairs Down`.
3. Configurer `TargetLevelId`, `TargetCellX`, `TargetCellY` et `TargetFacing`.
4. Dans le niveau cible, placer `Stairs Up`.
5. Configurer la transition retour.
6. Lancer PIE.
7. Marcher sur l'escalier.

Si `bRequireUseAction = false`, la transition se déclenche en entrant sur la cellule. Si `bRequireUseAction = true`, la transition par action `Use` sera traitée dans une étape ultérieure.

## Fields

`FGridObjectTransitionParams` is part of `FGridObjectBehaviorParams`:

- `bIsTransition`
- `TargetLevelId`
- `TargetCellX`
- `TargetCellY`
- `TargetFacing`
- `bRequireUseAction`

`TargetLevelId` should match an enabled entry in the current `UGridDungeonAsset`. `TargetCellX` and `TargetCellY` should point to a valid cell in the target `UGridLevelAsset`.

## Editor Workflow

In `BP_GridLevelEditorActor`:

- Select an object representing stairs, a passage, or a portal.
- In the object inspector, enable `Transition`.
- Set the target level id and target cell.
- Set target facing.
- Run level validation to catch missing target levels or invalid target cells.

The transition inspector saves values directly into the selected object's `Behavior` inside the `LevelAsset`. There is no separate `Apply` button for transition data.

Text fields such as `Target Level Id` use `OnTextCommitted`, so press Enter or leave the field to commit the value.

## Diagnostic Workflow

1. Select the transition object.
2. Check `Is Transition`.
3. Fill `Target Level Id`.
4. Press Enter or leave the field.
5. Fill `Target Cell X` and `Target Cell Y`.
6. Choose `Target Facing`.
7. Save the edited `UGridLevelAsset`.
8. Select `BP_GridLevelEditorActor`.
9. Click `Log Dungeon Transition Diagnostics`.

Expected log shape:

```text
TransitionObjects=1
[0] SourceLevelId=Into_The_Dark SourceDisplayName=Into The Dark SourceLevelAsset=/Game/.../DA_GridLevel_00 ObjectId=... ArchetypeId=Floor_Bones Type=Decoration Cell=(28,25) Edge=None TargetLevelId=Old_Tunnels TargetCell=(28,26) TargetFacing=North bRequireUseAction=false Status=OK
Status=OK
```

Runtime diagnostics report how many transition objects exist in the loaded `LevelAsset`.

## Runtime Transition Execution

`AGridLevelRuntimeActor` can execute automatic dungeon transitions at runtime.

The runtime actor keeps:

- `DungeonAsset`: used to resolve `TargetLevelId` into a target `UGridLevelAsset`.
- `CurrentDungeonLevelId`: the dungeon level id currently loaded.
- `LevelAsset`: the immediate level data currently reconstructed in the runtime actor.

When `AGrimrockPartyPawn` finishes a successful grid move, it asks the runtime actor to execute a transition on the destination cell. The transition is checked only after the movement interpolation has completed.

Automatic transitions execute only when `bRequireUseAction == false`. Transitions that require Use are ignored by movement for now; Use-triggered transitions will be handled later.

Execution flow:

1. `TryExecuteTransitionAtCell` looks for a transition object on the pawn cell.
2. `TravelToDungeonLevel` validates `DungeonAsset`, `TargetLevelId`, the target level asset, target cell, target facing, and the pawn.
3. `CurrentDungeonLevelId` is set to the target level id.
4. `LevelAsset` is replaced by the target level asset.
5. `RebuildLevel()` reconstructs the runtime level.
6. The pawn is placed on `TargetCellX`, `TargetCellY`, `TargetFacing`.

No UE map change, streaming, save game, menu flow, or visual transition is involved in this step.
