# Grid Editor UI Cleanup

## Principle

`BP_GridLevelEditorActor` remains the technical configuration actor for the editor map.

The Grimrock Grid Editor Mode is the daily editing interface. New workflow controls should move there when they are part of normal level editing.

## Dungeon Levels Panel

The Grid Editor Mode now includes a `DUNGEON LEVELS` panel after the header and before `TOOLS / PALETTE`.

The panel shows:

- dungeon name;
- default level id;
- current dungeon level id;
- current `LevelAsset`;
- level count;
- `Auto PIE Prepare` status;
- a clickable list of levels from the assigned `DungeonAsset`.

Example entries:

```text
(0,0,0) Into The Dark
(0,0,1) Old Tunnels
```

Clicking a valid level sets `CurrentDungeonLevelId`, calls `ApplyCurrentDungeonLevel()`, rebuilds the preview, and redraws editor viewports.

`New Level` opens a simple creation popup for adding a new `UGridLevelAsset` to the current `DungeonAsset`.

## Creating a new level from the editor

Workflow :

1. Ouvrir Grimrock Grid Editor Mode.
2. Dans `DUNGEON LEVELS`, cliquer `New Level`.
3. Renseigner `Level Id`, `Display Name` et `Logical Position`.
4. Cliquer `Create`.
5. Le nouvel `UGridLevelAsset` est créé, ajouté au `DungeonAsset`, sélectionné et affiché automatiquement.

La création ne génère aucun Blueprint. Aucun asset manuel n'est nécessaire dans le Content Browser.

Le niveau créé est un `UGridLevelAsset` sauvegardé dans :

```text
/Game/GrimrockPrototype/Core/DataAssets/GrimrockLevels/
```

Après création, l'utilisateur peut peindre les cellules, les murs, les objets et les transitions comme pour les niveaux existants.

## Workflow

1. Assign `DungeonAsset` once on `BP_GridLevelEditorActor`.
2. Open Grimrock Grid Editor Mode.
3. Click a level in `DUNGEON LEVELS`.
4. Edit the selected level.
5. Use `SetStartFromSelection` if the player start should move.
6. Click Play.

PIE preparation is automatic when `bAutoPreparePIE` is enabled on `BP_GridLevelEditorActor`. `PreparePIETestFromStart` remains available as a debug command, but it is not a normal daily editing step.

## Compatibility

The remaining `CallInEditor` buttons on `BP_GridLevelEditorActor` are kept for now for debugging and compatibility.

A later audit can decide which technical controls should remain visible on the actor.

## Step 1 Applied

The first cleanup step has been applied:

- `PLAYTEST` panel added to Grimrock Grid Editor Mode.
- `Set Start From Selection` is now available from `PLAYTEST`.
- `Auto Prepare PIE` and `Abort PIE On Error` can be toggled from `PLAYTEST`.
- Editor actor Details are partially regrouped.
- Technical fields are advanced where safe.
- No behavior was removed.
- `PreparePIETestFromStart` remains a debug fallback.

## Step 2 Applied

La deuxième étape de nettoyage a été appliquée :

- Les boutons Details redondants ont été retirés par suppression de `CallInEditor`.
- Les fonctions existent toujours, restent publiques et conservent `BlueprintCallable`.
- Le Grimrock Grid Editor Mode est maintenant l'interface quotidienne pour les actions de donjon, de playtest, de palette et d'objet retirées du panneau Details.
- Les boutons `Editor|Advanced Debug` restent temporairement visibles pour les cas de récupération ou d'incohérence.
- `SetStartFromSelection` et `ValidateCurrentLevel` restent visibles temporairement dans le panneau Details.
- Aucun comportement runtime, transition, GameMode ou panneau Slate n'a été modifié.
