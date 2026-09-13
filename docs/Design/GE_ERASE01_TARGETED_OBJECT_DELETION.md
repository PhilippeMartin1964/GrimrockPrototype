# GE-ERASE01 — Targeted Object Deletion & Confirmation

## Goal

The Grid Editor Erase tool must never delete every object in a cell implicitly.

When one or more typed placements occupy the selected cell/edge, Erase now uses an explicit object workflow:

1. collect only the typed placements eligible at the selected cell/edge;
2. if several objects are eligible, show a modal chooser listing each object by type and readable identity;
3. ask for explicit confirmation for the chosen object;
4. end the originating viewport paint gesture before returning from modal input;
5. warn when deleting the object also removes incoming/outgoing logical links;
6. delete exactly one placement by `FGuid` through `UGridLevelAsset::RemoveObjectById`;
7. rebuild the preview and clear stale selection/link-source state;
8. wrap the mutation in an Unreal editor transaction so `Ctrl+Z` can restore it.

If no object is eligible, the existing Erase fallback remains unchanged: the selected wall can be cleared, then an otherwise empty cell can be cleared.

## Safety contract

- Multiple objects in the same cell are never bulk-deleted by the Erase tool.
- The chooser is shown before any mutation when more than one candidate is present.
- The confirmation dialog is shown for every object deletion, including a single candidate.
- Cancel at either dialog performs no mutation.
- Closing the chooser or confirmation always cancels the current viewport paint gesture, so the same click cannot continue underneath the popup and delete another object.
- Objects placed on another edge of the same cell are not candidates for the currently selected edge.
- Removing an object uses the existing `RemoveObjectById` authority, so links attached to that object are removed consistently.
- Explicit bulk operations such as Clear Cell remain separate authoring commands and are not redefined by GE-ERASE01.

## Manual validation

1. Put two or more different objects in one cell.
2. Activate Erase and click that cell.
3. Verify that a chooser appears and lists the candidates instead of deleting them immediately.
4. Choose one object and press No in the confirmation dialog: nothing changes.
5. Repeat and press Yes: only the chosen object disappears.
6. Verify that the other objects remain present and selectable.
7. Verify that dismissing either modal does not select or erase anything underneath the popup.
8. Delete an object with logical links and verify that the confirmation warns about the link count.
9. Press `Ctrl+Z` and verify that the deleted object and its links are restored.
10. On a cell without object candidates, verify that the previous wall/cell erase fallback still works.
