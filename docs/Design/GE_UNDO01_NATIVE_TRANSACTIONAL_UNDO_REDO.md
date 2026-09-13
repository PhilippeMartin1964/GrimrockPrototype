# GE-UNDO01 — Native Transactional Undo/Redo

## Goal

The Grid Editor uses Unreal Editor's native transaction stack instead of maintaining a custom undo history.

The authoring contract is:

- `Ctrl+Z` reverts the most recent transactional Grid Editor mutation;
- repeated `Ctrl+Z` walks backward through earlier transactions;
- Unreal's native redo command (`Ctrl+Y`, and the editor's configured redo shortcut) walks forward again;
- selection/hover changes are not authored into the level data and do not intentionally create Grid Editor undo steps;
- after undo or redo, the runtime preview is rebuilt from the restored `UGridLevelAsset` and stale selected/hovered/link-source GUIDs are cleared.

## Transactional viewport actions

The primary and secondary Grid Editor tool actions now create native `FScopedTransaction` entries around their level mutations:

- Paint Cell;
- Paint Wall;
- Paint Object;
- Erase fallback for wall/cell authoring;
- Link creation;
- Clear Cell;
- Clear Wall;
- explicit Remove Grid Objects.

GE-ERASE01 targeted object deletion already had its own `Delete Grid Object` transaction and keeps that dedicated transaction name.

Selected-object authoring paths that use the shared `EditGridPlacementAuthoring` helper are also transactional through `Edit Grid Object`.

## Preview synchronization

`FGridEditorUndoBridge` is a self-registering Unreal editor undo client. After a successful undo/redo, while the Grimrock Grid Editor mode is active, it:

1. finds the active `AGridLevelEditorActor` instances in the editor world;
2. invalidates selected, hovered or pending-link GUIDs that no longer exist in the restored level asset;
3. calls `RebuildPreview()`;
4. redraws editor viewports.

The level asset remains the authority. The preview is reconstructed from the restored data instead of trying to maintain a parallel editor history.

## Scope note

GE-UNDO01 establishes the native undo/redo foundation and covers the canonical viewport tool mutations plus shared selected-object edits. Existing specialized editor panels that mutate the level asset through separate direct code paths can be migrated incrementally to the same transaction contract without changing the undo architecture.

## Manual validation

1. Paint one cell, then press `Ctrl+Z`: the cell returns to its previous state and the preview refreshes.
2. Paint a wall, then undo and redo it.
3. Place an object, undo it, then redo it.
4. Delete an object with GE-ERASE01, undo it, then redo it; attached links must follow the restored asset state.
5. Create a logical link and undo it.
6. Perform several different edits and press `Ctrl+Z` repeatedly to walk backward through them.
7. Redo several of those operations using Unreal's native redo command.
8. Verify that selecting/hovering cells without changing level data does not intentionally add a Grid Editor transaction.
