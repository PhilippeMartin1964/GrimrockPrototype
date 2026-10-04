# MAP-THEME01 — Textured Map Rendering

Date de clôture : **4 octobre 2026**  
Statut : **VALIDÉ — MERGED / CLOS**  
Baseline runtime/content validée : `9045ef2db75c09997db4fc65dbf99d4598f4df5c`

## Goal

Replace the procedural hand-drawn presentation of the dungeon map with data-driven UI textures while preserving the validated MON21.6 exploration, topology, secret-discovery, floor-navigation, zoom/pan and SaveGame behavior.

## Scope

Production authority remains unchanged:

```text
FGridLevelRuntimeState::MapExploration
        -> FGridMapReadModelBuilder
        -> FGridMapFloorView
        -> UGridMapWidget
        -> UGridMapSurfaceWidget
```

MAP-THEME01 changes only the final presentation step.

## C++ theme asset

`UGridMapVisualThemeAsset` contains strong `UTexture2D` references:

```text
Textures
  ParchmentTexture
  WallTexture
  WallPillarTexture
  DoorClosedTexture
  DoorOpenTexture

Symbols
  StairsUpTexture
  StairsDownTexture
  RelocationTexture
  PitTexture
  PointOfInterestTexture
  PartyMarkerTexture
```

Texture presentation:

```text
ParchmentOpacity        = 1.00
```

`ParchmentOpacity` affects only `ParchmentTexture`; walls, doors, symbols and the party marker keep their own opacity.

Layout values:

```text
BoundaryThicknessRatio = 0.16
WallPillarScale         = 0.24
SymbolScale             = 0.72
PartyMarkerScale         = 0.78
SymbolMinCellPixels      = 12
```

`UGridMapWidget::VisualTheme` selects the theme.

- `VisualTheme == None`: the validated procedural renderer is kept unchanged.
- `VisualTheme != None`: available theme textures are used.
- a missing individual texture falls back to the simple existing primitive for that element.

There is no duplicated map/read-model state.

## Rendering contract

### Parchment

`ParchmentTexture` is stretched to the exact `MapSurface` viewport.

### Walls

`WallTexture` is authored as a horizontal strip. Slate rotates the strip to match the projected boundary.

`WallPillarTexture` is a square RGBA top-view pillar drawn above wall strips. It follows the same modular convention as `SM_Wall_Stone_05`: when facing a wall from its owning cell, the pillar is at the left end.

```text
North -> West end
East  -> North end
South -> East end
West  -> South end
```

Pillars sharing the same grid vertex are deduplicated. They are emitted only for boundaries that are visually solid walls: ordinary walls and closed secret doors. Open secret doors and standard doors do not add wall pillars.

### Doors

Standard doors use two separate textures, both authored for a horizontal boundary:

```text
closed -> DoorClosedTexture
open   -> DoorOpenTexture
```

Slate rotates the selected texture only to match the N/E/S/W boundary orientation. It never rotates the closed-door texture by an additional 90 degrees to fake the open state.

Secret doors have no dedicated texture:

```text
secret closed -> WallTexture
secret open   -> no boundary drawing (normal passage)
```

The discovery state remains persistent. If a discovered secret door closes again, it is drawn as a standard wall again. An undiscovered secret is already projected by the read model as `EGridMapBoundaryKind::Wall`, so it also uses `WallTexture`.

### Cell symbols

The five map symbols are square RGBA textures centered on the projected cell:

```text
StairsUp        -> T_Map_StairsUp
StairsDown      -> T_Map_StairsDown
Relocation      -> T_Map_Relocation
Pit             -> T_Map_Pit
PointOfInterest -> T_Map_PointOfInterest
```

### Party marker

`T_Map_PartyMarker` must be authored pointing upward, which means canonical North.

The renderer rotates it from the existing `PartyFacing`:

```text
North =   0 degrees
East  = -90 degrees
South = 180 degrees
West  =  90 degrees
```

## UE5 content créé et validé sur master

Do not generate or edit these binary assets outside Unreal Editor.

Recommended content structure:

```text
Content/GrimrockPrototype/UI/Map/
├── Themes/
│   └── DA_MapVisualTheme_Default
├── Textures/
│   ├── T_Map_Parchment
│   ├── T_Map_Wall
│   ├── T_Map_WallPillar
│   ├── T_Map_Door
│   └── T_Map_DoorOpen
└── Symbols/
    ├── T_Map_StairsUp
    ├── T_Map_StairsDown
    ├── T_Map_Relocation
    ├── T_Map_Pit
    ├── T_Map_PointOfInterest
    └── T_Map_PartyMarker
```

Create `DA_MapVisualTheme_Default` as a Data Asset of class `GridMapVisualThemeAsset`, assign the ten textures, then assign that Data Asset to `WBP_GridMap -> Class Defaults -> Map|Theme -> VisualTheme`.

No change to the canonical WBP hierarchy is required.

## Texture import recommendations

For map UI textures:

- Compression Settings: `UserInterface2D (RGBA)`
- Texture Group: `UI`
- sRGB: enabled for normal color artwork
- alpha: required for walls, doors and symbols
- avoid fixed-size assumptions; Slate controls the displayed size

The parchment can be opaque. Wall/door/symbol textures should use transparent surroundings.

## Tests

Targeted automation filter:

```text
Grimrock.UI.MapTheme01
```

MAP-THEME01 must not change the exact-match SaveGame version.

## Final validation

Le thème final et ses assets ont été fusionnés par fast-forward dans `master`. La branche locale de travail `MAP-THEME01` a ensuite été supprimée.

Assets présents sous `Content/GrimrockPrototype/UI/Map/` :

```text
Themes/
  DA_MapVisualTheme_Default

Textures/
  T_Map_Parchment
  T_Map_Wall
  T_Map_WallPillar
  T_Map_Door
  T_Map_DoorOpen
  T_Map_Relocation

Symbols/
  T_Map_PartyMarker
  T_Map_Pit
  T_Map_StairsDown
  T_Map_StairsUp
```

`PointOfInterestTexture` reste optionnelle : le renderer conserve son fallback de primitive lorsque cette texture n’est pas assignée.

Validation finale sur la baseline runtime/content `9045ef2d` :

```text
Grimrock global
1026 succeeded / 0 warnings / 0 failed / 0 not run

Win64 Shipping
Build + Cook + Stage + Package + Pak + Archive : OK
Cook : 0 error / 0 warning
AutomationTool ExitCode = 0
```

**Décision : MAP-THEME01 est VALIDÉ et CLOS.**
