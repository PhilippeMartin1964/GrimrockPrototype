# MON21.6.7 — Multi-Tile / Floor Projection

Date : **28 septembre 2026**  
Statut : **IMPLÉMENTÉ — VALIDATION LOCALE UTILISATEUR REQUISE**

## 1. Objectif

MON21.6.7 compose les vues locales filtrées de MON21.6.6 en un plan d’étage continu, sans introduire de seconde géométrie autoritaire.

Le pipeline devient :

```text
UGridLevelAsset + FGridLevelRuntimeState
        -> FGridMapTileView
        -> LogicalPosition X/Y/Z
        -> FGridMapFloorView
        -> futur renderer MON21.6.8
```

## 2. Coordonnées globales

Pour une cellule locale `(LocalX, LocalY)` appartenant à une entrée de donjon `(TileX, TileY, Z)` :

```text
MapX = TileX * 32 + LocalX
MapY = TileY * 32 + LocalY
```

Les coordonnées négatives sont conservées sans clamp ni offset artificiel.

Exemple :

```text
TileX=-1, LocalX=31 -> MapX=-1
TileX= 0, LocalX= 0 -> MapX= 0
```

Les deux cellules sont donc adjacentes dans le repère cartographique.

## 3. Floor view

`FGridMapFloorView` expose uniquement une projection transitoire :

```text
SelectedFloorZ
AvailableFloorZs[]
Cells[]
    MapCell
    CellType
Boundaries[]
    MapCell
    Edge
    Kind
    bDoorOpen
bHasPartyMarker
PartyMapCell
PartyFacing
```

Aucun champ ne porte `SaveGame`. Le schéma reste **v23**.

## 4. Sélection des dalles

Un étage est composé uniquement des entrées :

- `bEnabled == true` ;
- `LevelId != None` ;
- `LevelAsset != nullptr` ;
- `LogicalPosition.Z == SelectedFloorZ`.

Une dalle sans état d’exploration produit naturellement zéro géométrie visible : sa géométrie authored n’est jamais transmise directement au floor view.

L’ordre de composition est déterministe : `LogicalPosition.X`, puis `Y`, puis `LevelId`.

## 5. Étages disponibles

`GetAvailableFloorZs()` retourne les valeurs Z distinctes des entrées activées et valides, triées en ordre croissant.

Exemple :

```text
[-3, 0, +2]
```

MON21.6.7 expose cette information mais n’implémente pas encore les commandes Level Up / Level Down. Leur comportement UI appartient à MON21.6.9.

## 6. Limites de dalles

Une frontière technique entre deux `LevelAsset` adjacents ne crée aucun trait cartographique.

Si les deux côtés n’ont aucun mur/porte connu :

```text
Tile A | Tile B
       ^
       aucune couture dessinée
```

Si la même frontière physique est connue depuis les deux dalles, elle n’est émise qu’une fois.

La clé de fusion utilise la ligne/segment de grille global et supporte les coordonnées négatives.

En cas de données contradictoires sur une même frontière, la projection échoue visuellement de manière conservatrice vers `Wall`.

## 7. Marqueur du groupe

La position du groupe reste une donnée runtime fournie au builder ; elle n’est pas stockée dans l’état Map.

Le marqueur est projeté seulement lorsque :

```text
SelectedFloorZ == LogicalPosition.Z du ActiveLevelId
```

Coordonnées :

```text
PartyMapX = ActiveTileX * 32 + CurrentCellX
PartyMapY = ActiveTileY * 32 + CurrentCellY
```

Lorsque l’utilisateur consulte un autre étage, `bHasPartyMarker == false`.

## 8. État des portes

Pour la dalle active uniquement, le `UGridDoorSystemComponent` vivant peut être transmis au tile builder.

Les autres dalles utilisent le fallback MON21.6.6 :

```text
FGridLevelRuntimeState::Doors
    -> sinon état initial authored
```

Aucun scan d’Actors n’est réalisé.

## 9. Validation MON21.6.6 actée

Validation utilisateur du 28 septembre 2026 :

```text
Filter                  : Grimrock.Map.MON21_6_6
Succeeded               : 4
Succeeded with warnings : 0
Failed                  : 0
Not run                 : 0
Process exit code       : 0
Report                  : TD04-20260928-091315
```

MON21.6.6 est **VALIDÉ**.

## 10. Automation MON21.6.7

Filtre : `Grimrock.Map.MON21_6_7`

```text
FloorProjection.GlobalCoordinatesAndPartyMarker
FloorProjection.AvailableFloorsAndSelection
FloorProjection.TechnicalSeamAndBoundaryDedup
FloorProjection.TransientAndInvalidSelection
```

Aucun résultat MON21.6.7 n’est déclaré avant retour du harness UE5.5.4 local.

## 11. Hors périmètre

- rendu natif dans `WBP_GridMap` : MON21.6.8 ;
- commandes Level Up/Down : MON21.6.9 ;
- zoom/pan/recenter : MON21.6.10 ;
- style parchemin : MON21.6.11 ;
- symboles supplémentaires : MON21.6.12.

Prochaine tranche après validation : **MON21.6.8 — Existing WBP + Native Map Rendering**.
