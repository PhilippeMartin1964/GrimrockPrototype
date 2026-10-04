# MAP-FLOOR01 — Textured Explored Floor Cells

## Goal

Render explored map cells with an optional semi-transparent floor texture without changing map exploration or gameplay state.

## Contract

`UGridMapVisualThemeAsset` exposes:

```text
FloorTexture
FloorOpacity = 0.35
```

Rendering behavior:

```text
FloorTexture assigned   -> explored cell uses FloorTexture * FloorOpacity
FloorTexture unassigned -> existing ExploredCellColor fallback
```

Only cells already present in `FGridMapFloorView::Cells` are rendered. MAP-FLOOR01 does not change exploration, topology, SaveGame, symbols, doors or wall rendering.

Recommended UE asset:

```text
Content/GrimrockPrototype/UI/Map/Textures/T_Map_Floor
```

Recommended import settings remain `UserInterface2D (RGBA)`, Texture Group `UI`, sRGB enabled.

## Validation

Targeted automation filter:

```text
Grimrock.UI.MapFloor01
```

The final visual check in PIE should confirm that only explored cells receive the floor texture and that `FloorOpacity` affects only that texture.
