# PUZZLE01 — Guardian gem absorption

Implement the Guardian blue-gem interaction correctly, using the existing ordinary receptacle + Lua architecture.

## Functional behavior

1. When the player places a compatible blue gem on the Guardian, exactly one gem leaves the player's cursor/inventory exactly as with an ordinary receptacle (alcove, torch holder, etc.).
2. That gem is immediately and permanently absorbed: it must not remain as a contained physical item and must never be recoverable from the Guardian.
3. The existing Lua puzzle callback detects the successful deposit and changes the material/texture of one of the Guardian's two eyes.
4. The operation can succeed exactly twice on the Guardian: first successful gem -> first eye, second successful gem -> second eye.
5. Starting with the third attempt, NOTHING happens: the Guardian must reject the interaction before ownership transfer. The gem remains exactly where it was (cursor/inventory), no item is inserted/consumed, no Lua ItemInserted callback is fired, no eye/material/state changes, and no repeated puzzle activation occurs.

## Architecture constraints

- Keep the Guardian an ordinary generic `AGridReceptacleActor` (or the existing generic receptacle path). Do NOT create a Guardian-specific C++ actor/class.
- Do NOT reintroduce `ConsumingSlot`, consuming-slot state/configuration, eye-slot volumes, light-slot parameters, or any equivalent specialized subsystem.
- Do NOT hardcode `Guardian`, `Gem_Blue`, eye names, or a Guardian door in generic C++ runtime code.
- Lua remains responsible for puzzle semantics and presentation (counting successful Guardian gem insertions, changing eye materials, opening/completing the puzzle if already defined there).
- C++ may only provide or fix minimal reusable generic receptacle/runtime primitives if the existing API cannot express "accept at most N successful deposits / disable further acceptance" cleanly. Prefer existing generic commands/conditions/state if they already exist.
- The third-attempt rejection must occur BEFORE the item leaves the cursor/inventory. Do not insert then undo/return it as normal gameplay behavior.
- Preserve normal receptacle behavior and all unrelated item/inventory flows.
- Do not modify `.uasset` binary assets automatically.
- Remove this `CODEX_TASK_PUZZLE01.md` task file from the branch before completing the implementation so it does not become part of the product diff.

## Investigation requirements

Before changing code, trace the actual production mouse path from inventory/cursor to `AGridReceptacleActor`, the order of `ItemInserted` / link / Lua dispatch, `ReceptacleConsumeItem`, and any generic receptacle enable/disable or acceptance gating already available. Reuse existing mechanisms instead of adding a parallel architecture.

Pay special attention to stacked vs non-stacked gems and runtime ownership. A successful placement must remove exactly one unit; a rejected third placement must remove zero units.

## Validation

Add/adjust automated tests around the real interaction path. At minimum prove:

- First compatible gem: exactly one gem removed from cursor/inventory, Lua callback executed once, Guardian contains no recoverable item after Lua absorption.
- Second compatible gem: same behavior, callback executed a second time, completion state correct.
- Third compatible gem: rejected before transfer, gem remains on cursor/inventory, callback count unchanged at 2, Guardian remains empty/non-recoverable, no additional activation.
- Wrong item: rejected without ownership change.
- Both individual gems and a stack of compatible gems consume one unit per successful attempt.
- Existing ordinary receptacle behavior remains valid.

Run the relevant UE 5.5.4 build/automation available in the repository. Keep the implementation minimal and data/Lua-driven.

Base branch state is `fe6153f78d6b8ba865b6851d85c7f5aeee787833` after rollback of PUZZLE01.2 and PUZZLE01.3.
