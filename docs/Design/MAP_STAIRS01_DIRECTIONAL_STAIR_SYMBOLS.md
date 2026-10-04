# MAP-STAIRS01 — Directional Stair Symbols

## Goal

Orient `StairsUp` and `StairsDown` map symbols from the existing authored orientation of the placed stair object.

The map does not store the player's last approach direction. The stable source of truth is the floor object's existing `LocalTransformOverride.Yaw`, authored by the Grid Editor.

## Authoring contract

The Grid Editor already writes floor-object orientation as:

```text
North =   0 degrees
East  =  90 degrees
South = 180 degrees
West  = 270 degrees
```

The stair textures are authored pointing from South to North, so North requires no map rotation.

Because the map presentation mirrors canonical X, directional artwork uses the same screen rotation contract as the party marker:

```text
North =   0 degrees
East  = -90 degrees
South = 180 degrees
West  = +90 degrees
```

Therefore a stair whose authored facing is West is displayed East -> West.

## Read model

`FGridMapSymbolView` and `FGridMapFloorSymbolView` expose transient presentation-only `Facing`.

Only `StairsUp` and `StairsDown` consume this value for rendering. Other symbols remain unrotated.

No SaveGame field or new gameplay authority is introduced.

## Rendering

Both textured stair symbols and the procedural fallback rotate around the center of their map cell.

## Validation

Targeted automation filter:

```text
Grimrock.Map.MAP_STAIRS01
```

PIE should verify all four authored orientations for both stair styles, in particular West = East-to-West travel.
