# MON21.6.2 — Exploration State

Date : **28 septembre 2026**  
Statut : **IMPLÉMENTÉ — VALIDATION LOCALE UTILISATEUR REQUISE**  
Baseline : `77c9773a1a6cd22997f77b8ea136d5fdc2a251a0`

## 1. Objectif

MON21.6.2 introduit l’autorité de connaissance **cellule par cellule** nécessaire au fog-of-war.

Le ticket reste strictement limité à l’état d’exploration : aucun calcul de rayon, aucune traversée topologique, aucune porte secrète, aucun read model Map, aucun rendu UMG et aucune persistance disque.

## 2. Autorité ajoutée

Nouveau type : `FGridMapExplorationState`, porté directement par `FGridLevelRuntimeState::MapExploration`.

```text
FGridDungeonRuntimeState
    -> LevelStates[LevelId]
        -> FGridLevelRuntimeState
            -> MapExploration
```

L’exploration est isolée par `LevelId`.

## 3. Modèle

`Unknown = 0`, `Explored = 1`, sur la grille canonique `32 x 32 = 1024` cellules.

Le stockage est un `TArray<uint8>` alloué paresseusement : zéro allocation tant qu’aucune cellule n’est découverte, puis exactement 1024 octets.

## 4. API

`IsValidCell`, `IsExplored`, `TryMarkExplored`, `GetExploredCellCount`, `GetStorageCellCount`, `IsStructurallyValid`, `Reset`.

`TryMarkExplored` est idempotent et expose `bOutNewlyExplored` pour distinguer une première découverte d’un marquage répété.

## 5. Frontière de session

`MapExploration` fait partie de `FGridLevelRuntimeState` et survit aux copies/transitions utilisant le même `FGridDungeonRuntimeState`. `CaptureCurrentLevelRuntimeState()` ne le réinitialise pas.

## 6. Frontière SaveGame

MON21.6.2 ne prend pas encore en charge la persistance disque. `MapExploration` et `ExploredCells` ne portent pas le flag `SaveGame`.

`UGrimrockPartySaveGame::CurrentSaveVersion` reste **22**. MON21.6.5 ouvrira la persistance disque et incrémentera alors le schéma exact-match.

## 7. Automation

Filtre : `Grimrock.Map.MON21_6_2`

```text
ExplorationState.DefaultsAndBounds
ExplorationState.MutationIsIdempotent
ExplorationState.PerLevelAuthority
ExplorationState.SaveBoundary
```

Aucun résultat n’est déclaré tant que la sortie locale Unreal n’a pas été fournie par l’utilisateur.

## 8. Stop condition

L’état Unknown/Explored est unique par LevelId, paresseux, idempotent, borné à 32×32 et sans persistance disque prématurée.

Prochaine tranche après validation : **MON21.6.3 — Topology-Aware Reveal**.
