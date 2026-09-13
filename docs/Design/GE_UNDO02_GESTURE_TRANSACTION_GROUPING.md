# GE-UNDO02 — Gesture Transaction Grouping

## Goal

Continuous Grid Editor authoring gestures use one native Unreal transaction for the whole mouse gesture instead of one undo step per visited cell/edge.

## Contract

- Press and hold the left mouse button while painting cells, walls, objects, links or erase-fallback edits: the first authored mutation opens one native `FScopedTransaction`.
- Additional mutations produced while the same Grid Editor paint gesture remains active reuse that transaction.
- Releasing/cancelling the gesture closes the transaction.
- A single `Ctrl+Z` therefore removes the whole authored stroke; `Ctrl+Y` restores the whole stroke.
- Non-drag actions invoked from buttons/panels still receive their own immediate transaction.
- GE-ERASE01 targeted object deletion remains single-shot and keeps its dedicated `Delete Grid Object` transaction. Closing its modal chooser/confirmation cancels the paint gesture before deletion.

## Implementation

`RunGridEditorTransaction` now checks the active `FGridLevelEdMode` paint gesture. While `bIsPainting` is active it keeps one scoped transaction alive and a lightweight editor ticker closes it as soon as the gesture ends. This preserves the existing authoring functions and Unreal transaction authority; no custom undo history is introduced.

## Undo/Redo refresh hardening

GE-UNDO02 also hardens the GE-UNDO01 refresh path after native Undo/Redo. The undo bridge schedules its refresh for the next editor tick, after the transaction has fully restored object state, then rebuilds the preview from the restored `UGridLevelAsset` before redrawing viewports. This removes the need to press `Load Default` after `Ctrl+Y` just to refresh the Grid Editor preview.

## Manual validation

1. Paint a line of several cells while holding the mouse button; one `Ctrl+Z` must remove the whole line.
2. `Ctrl+Y` must restore the whole line and refresh the preview without `Load Default`.
3. Repeat with several walls and several placed objects.
4. Verify that separate clicks remain separate undo steps.
5. Delete a typed object through GE-ERASE01 and verify that its modal workflow remains single-shot and independently undoable.
6. Perform several Undo/Redo operations in sequence and verify that the preview always matches the restored `UGridLevelAsset`.
