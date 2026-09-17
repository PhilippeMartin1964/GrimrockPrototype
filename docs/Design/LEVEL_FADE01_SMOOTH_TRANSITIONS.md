# LEVEL-FADE01 — Smooth stair and level transitions

## Goal

Generic grid relocations entered by normal party movement use a short black camera fade so stairs, passages and teleport-style level transitions do not cut abruptly between locations.

## Runtime flow

`AGrimrockPartyPawn::UpdateMove()` keeps the existing gameplay order:

1. finish the grid move;
2. resolve Pit first;
3. emit normal cell-change / turn-completion logic;
4. detect a generic Relocation on the destination cell;
5. when a local `PlayerCameraManager` exists, disable party input and fade from visible to black for `0.35 s`;
6. while fully black, execute the existing `AGridLevelRuntimeActor::TryExecuteRelocationAtCell()` authority;
7. fade from black to visible for `0.35 s`;
8. re-enable party input.

The relocation data model and travel authority are unchanged. `TryExecuteRelocationAtCell()` / `TravelToDungeonLevel()` still decide and execute the actual destination.

## Scope

The fade applies to generic Relocation gameplay reached through party movement, including Stairs Up / Stairs Down and other relocation definitions using the same runtime path.

Open Pit transitions are intentionally excluded. Pit owns its existing fall, landing sound and landing camera-impact presentation.

## Headless / automation behavior

When no usable local `PlayerController` / `PlayerCameraManager` exists, no asynchronous presentation is started and the existing immediate relocation path is used. This preserves deterministic relocation automation tests while keeping presentation local to playable runtime.

## Safety

- The buffered movement command is cleared before and after relocation.
- Party input is disabled during fade-out, relocation and fade-in.
- Timer callbacks keep weak references to the party and runtime actor.
- No level asset, map or Blueprint asset is modified by LEVEL-FADE01.
