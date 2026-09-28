# MON21.6.12 — Map Symbols

Date : **28 septembre 2026**  
Statut : **VALIDÉ**

## 1. Objectif

MON21.6.12 ajoute des symboles cartographiques filtrés et data-driven sans réintroduire une classification gameplay parallèle.

Le pipeline devient :

```text
UGridLevelAsset + WorldObjectDefinitions
        +
FGridLevelRuntimeState / MapExploration
        -> FGridMapTileView.Symbols[]
        -> FGridMapFloorView.Symbols[]
        -> UGridMapWidget::NativePaint()
```

## 2. Autorisation explicite par définition

`UGridWorldObjectDefinitionAsset` reçoit une métadonnée de présentation uniquement :

```text
MapSymbolStyle
```

Enum `EGridMapSymbolStyle` :

```text
None
StairsUp
StairsDown
Relocation
Pit
PointOfInterest
```

`None` est la valeur par défaut. Un objet monde n’obtient donc aucun symbole tant que sa définition ne l’autorise pas explicitement.

Cette propriété :

- ne change jamais `SupportedType` ;
- ne crée aucun nouveau Gameplay Type ;
- ne porte pas `SaveGame` ;
- ne devient pas une autorité gameplay.

Le Gameplay Type canonique `Relocation` reste donc unique pour escaliers, portails et passages automatiques.

## 3. Symboles issus directement des cellules

Les sémantiques déjà portées par `EGridCellType` restent prioritaires :

```text
StairsUp   -> StairsUp
StairsDown -> StairsDown
Pit        -> Pit
```

Si une cellule porte déjà un symbole de navigation canonique, une métadonnée d’objet contradictoire ne crée pas un second symbole superposé.

## 4. Symboles d’objets monde

Un `FGridWorldObjectInstance` n’est projeté que si :

- sa cellule est valide ;
- sa cellule est explorée ;
- sa cellule n’est pas `Empty` ;
- l’objet n’est pas marqué `bRemovedFromInitialPlacement` ;
- sa définition existe ;
- `MapSymbolStyle != None`.

Le read model ne transmet jamais :

```text
ObjectId
WorldObjectDefinitionId
LogicId
PaletteEntryId
```

Il transmet uniquement :

```text
LocalCell / MapCell
Kind
```

## 5. Pit

Le symbole `Pit` d’un objet monde respecte l’état réel :

```text
FGridRuntimePitState.bIsOpen
    -> sinon InstanceConfig.Pit.bInitiallyOpen
```

Une trappe fermée n’est donc pas révélée comme un trou sur la carte.

Une cellule authored directement `CellType::Pit` reste un pit cartographique intrinsèque.

## 6. Multi-dalles

`FGridMapFloorSymbolView` applique la même projection globale que les cellules :

```text
MapX = TileX * 32 + LocalX
MapY = TileY * 32 + LocalY
```

Les symboles suivent donc naturellement les dalles adjacentes et les coordonnées négatives.

## 7. Glyphes graphiques

Le renderer natif dessine les symboles avec le vocabulaire manuscrit déterministe de MON21.6.11 :

- `StairsUp` : marches + flèche montante ;
- `StairsDown` : marches + flèche descendante ;
- `Relocation` : double losange/rune ;
- `Pit` : cadre barré en croix, couleur hazard ;
- `PointOfInterest` : étoile/repère en traits d’encre.

Le marqueur du groupe est dessiné au-dessus des symboles.

Les symboles disparaissent sous `SymbolMinCellPixels` afin de ne pas devenir du bruit visuel lors d’un zoom arrière.

Réglages exposés :

```text
NavigationSymbolColor
HazardSymbolColor
SymbolStrokeThickness
SymbolScale
SymbolMinCellPixels
```

## 8. Configuration DataAsset à effectuer dans UE

Après validation C++, les définitions voulues doivent être configurées manuellement dans Unreal Editor.

Exemples recommandés :

```text
DA_Stairs_Up      -> Map Symbol = Stairs Up
DA_Stairs_Down    -> Map Symbol = Stairs Down
DA_Pit_Stone_01   -> Map Symbol = Pit
définitions de portail/téléporteur -> Map Symbol = Relocation
POI explicitement souhaité         -> Map Symbol = Point Of Interest
```

Aucun `.uasset` n’est modifié à l’aveugle par ce ticket.

## 9. Objets volontairement non symbolisés par défaut

MON21.6.12 n’active automatiquement aucun symbole pour :

- Button ;
- Lever ;
- PressurePlate ;
- Trigger ;
- Receptacle ;
- MonsterSpawn ;
- ItemSpawn ;
- Logic ;
- objets décoratifs ordinaires.

Un POI doit être explicitement autorisé sur sa définition.

## 10. Validation MON21.6.11

Validation Automation utilisateur du 28 septembre 2026 :

```text
Filter                  : Grimrock.Map.MON21_6_11
Succeeded               : 4
Succeeded with warnings : 0
Failed                  : 0
Not run                 : 0
Process exit code       : 0
Report                  : TD04-20260928-120834
```

Le contrôle PIE confirme que la fondation parchemin est fonctionnelle et lisible.

Le feedback visuel demande cependant de reporter le polish graphique final et l’adaptation précise au canvas `WBP_GridMap` après l’arrivée des symboles. Ce point est donc différé à la passe de clôture MON21.6.13.

MON21.6.11 est **VALIDÉ comme fondation artistique**.

## 11. Automation MON21.6.12

Filtre :

```text
Grimrock.Map.MON21_6_12
```

Tests initiaux :

```text
Symbols.ExploredOnlyAndNoIdentityLeak
Symbols.PitRequiresOpenState
Symbols.MultiTileGlobalProjection
Symbols.PresentationContract
```

Validation utilisateur du 28 septembre 2026 :

```text
Filter                  : Grimrock.Map.MON21_6_12
Succeeded               : 4
Succeeded with warnings : 0
Failed                  : 0
Not run                 : 0
Process exit code       : 0
Report                  : TD04-20260928-124412
```

La configuration PIE confirme les symboles définis. Le cas `Stairs_Up` a mis en évidence non pas un défaut de symbole mais un défaut de reveal : après une relocation inter-level, la cellule d’arrivée n’était révélée qu’au prochain déplacement du groupe.

Correctif pré-clôture ajouté :

```text
TravelToDungeonLevel()
    -> SetGridStart(...)
    -> RevealMapAroundCell(arrival)
```

Le correctif est volontairement Map-only : il n’appelle pas `HandlePartyCellChanged()`, afin de ne pas émettre une seconde fois les triggers/pressure plates d’entrée.

Nouveau cinquième test :

```text
ArrivalReveal.CrossLevelTravelRevealsDestination
```

Le correctif a ensuite été **validé par l’utilisateur** en Automation et en PIE. La sortie détaillée du second run à cinq tests n’a pas été recopiée dans le thread ; aucun compteur ou identifiant de rapport supplémentaire n’est inventé ici.

MON21.6.12 est **VALIDÉ**.

## 12. Validation PIE

Après configuration des DataAssets concernés, l’utilisateur confirme que les symboles définis sont présents.

Le cas `Stairs_Up` a été compris : avant correctif, il fallait avancer puis reculer après `Stairs_Down` pour déclencher le reveal de la cellule d’arrivée. Ce comportement est corrigé par le hook Map-only ajouté dans `TravelToDungeonLevel()`.

Smoke PIE final confirmé par l’utilisateur :

- descendre par `Stairs_Down` ;
- ouvrir immédiatement la Map sans faire un pas supplémentaire ;
- la cellule d’arrivée doit déjà être explorée ;
- le symbole `Stairs Up` doit être visible immédiatement ;
- zoom/pan/recenter doivent rester fonctionnels.

## 13. Polish final différé

Le feedback visuel de MON21.6.11 est conservé pour MON21.6.13 :

- mieux exploiter la surface réelle du canvas `WBP_GridMap` ;
- recalibrer le fit initial et `MaxCellPixels` ;
- raffiner l’épaisseur/graphisme des murs et portes ;
- harmoniser symboles, géométrie, hachures et marqueur du groupe.

Cette passe est désormais implémentée côté C++ dans MON21.6.13 et attend la validation finale ciblée + globale.

Tranche suivante : **MON21.6.13 — Automation / Regression / Closure + fit/polish final**.
