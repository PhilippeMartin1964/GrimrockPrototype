# ALIGN-B5 — Placement world-object direct

Statut : **CLOSED — 2026-09-09**.

## Résultat

ALIGN-B5 a terminé la migration du placement world-object vers une autorité de données unique :

```text
PlacementSurface
DefaultLocalPosition.U
DefaultLocalPosition.V
DefaultLocalPosition.N
```

### B5.1 — Runtime

`GridPlacementTransformResolver::ResolveWorldObject()` consomme directement `PlacementSurface + U/V/N` avec parité du comportement existant.

### B5.2 — Editor

La classification de placement, le centre logique éditeur, la validation, l’inspecteur et la carte d’aperçu consomment directement le même contrat permanent. Le dernier lecteur runtime indirect a également été migré.

### B5.3 — Purge

Les projections et helpers suivants ont été supprimés :

```text
PlacementKind
PlacementZOffset
WallInset
LocalOffsetAlongWall
LocalOffsetVertical
RefreshPlacementRuntimeProjection()
IsEdgePlaced()
IsCenterPlaced()
IsWallPlaced()
```

`PostLoad()` reste volontairement présent pour la migration audio legacy des portes.

## Contrat courant

- surfaces d’authoring world-object : `Floor`, `Wall`, `Ceiling` ;
- face concrète d’un placement mural : `FGridWorldObjectInstance::WallSide` / `EGridEdge` ;
- `Wall` consomme directement `U`, `V`, `N` ;
- `Floor` et `Ceiling` exposent `U/V/N` dans le modèle, mais le runtime courant ignore encore `U/V` et garde ces objets centrés en XY ;
- le plan plafond courant reste à 200 cm dans les chemins de résolution existants ;
- aucune modification des semantics de boundary/murs n’a été faite dans ALIGN-B5.

## Validation locale

Les validations locales finales de B5.3 ont réussi, notamment :

```text
Grimrock.WorldObjects.ALIGN_B5_3.PlacementBridgePurge
Grimrock.WorldObjects
Grimrock.Editor
Grimrock.Pit
Grimrock.TechnicalDebt.TD07_3_6.Characterization
Grimrock.Monsters.MON13
Grimrock.TechnicalDebt.TD01_1.ReceptaclePersistence
Grimrock.TechnicalDebt.TD07_5.Recovery.ReceptacleCommands
Grimrock.MON19.7.1.LuaAuthoring.LogicIdCommand
Grimrock.Monsters.MON17.1
Grimrock.MON20.5.CustomRecruit
Grimrock.MON20.4.RecruitmentUI.PalettePlacement
```

Aucun échec. Les warnings observés dans `Pit` et `MON13` correspondent aux warnings déjà attendus de ces suites.

## Suite

ALIGN-B5.4 réconcilie uniquement la documentation active. Les sujets suivants restent séparés :

- consommation de `U/V` sur `Floor` et `Ceiling` ;
- plan plafond dérivé de la hauteur de cellule ;
- `FGridWorldObjectInstance::Type` ;
- registre `WorldObjectDefinitions` ;
- `bCanShareCell` / `bCanShareAnchor` ;
- unification topologique `FGridBoundaryKey` / `FGridEdgeKey` ;
- semantics symétriques ou directionnelles des murs.
