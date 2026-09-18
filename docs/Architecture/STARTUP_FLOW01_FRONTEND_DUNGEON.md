# STARTUP-FLOW01 — Frontend character creation and canonical dungeon map

## Canonical maps

- `L_MainMenu`: lightweight frontend only.
- `L_Dungeon`: the single Unreal world hosting dungeon authoring, PIE playtests and packaged gameplay.
- `L_GrimrockRuntime`: obsolete historical map, no longer present.

The Unreal map is a host world. Actual dungeon content remains data-driven through the dungeon and level assets. The canonical host package is now L_Dungeon; the rename was performed inside Unreal Editor so the binary package and serialized references were updated safely.

## New Game

```text
L_MainMenu
-> WBP_MainMenu
-> New Game
-> WBP_CharacterCreationWizard
-> transient UGridPartyInventoryComponent
-> completed FGridPartyInventoryState
-> UGrimrockGameInstance.PendingNewPartyState
-> NewGameDungeonBuild pending
-> PendingStartupMode = NewGame
-> Open L_Dungeon
-> AGridLevelRuntimeActor defers its automatic initial rebuild
-> UGrimrockStartupModeComponent consumes PendingNewPartyState
-> "Construction du donjon" progress overlay
-> rebuild runtime + restore initial state
-> placement du groupe
-> initial save
-> "Donjon prêt."
-> gameplay
```

The dungeon map is not opened while the player is creating the initial character.

## Continue / Load

```text
L_MainMenu
-> select save
-> PendingLoadSlot
-> PendingStartupMode = Continue
-> UGrimrockGameInstance::OpenDungeonLevel()
-> L_Dungeon
-> restore save
```

All gameplay travel now uses `UGrimrockGameInstance::DungeonLevelName`. UI classes no longer own independent runtime-map strings.

## Runtime simplification

`UGrimrockStartupModeComponent` no longer clears the dungeon and waits for initial character creation. For New Game it applies the frontend-prepared party, then STARTUP-FLOW02 owns the one-shot initial dungeon build handoff. The runtime actor skips its normal automatic BeginPlay rebuild while that handoff is pending, so the progress overlay wraps the real runtime construction instead of a duplicate rebuild.

The pawn keeps the old initial-character modal only as a direct fresh-PIE fallback for editor playtests; packaged New Game creation belongs to the frontend.
