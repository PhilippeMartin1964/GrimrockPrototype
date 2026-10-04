# MON21.6.13 — Map Regression / Closure + Final Fit / Polish

Date : **28 septembre 2026**  
Statut : **VALIDÉ — CLOS (4 octobre 2026)**

## 1. Objectif

MON21.6.13 clôt la tranche Map sans ajouter d’autorité.

Le ticket regroupe volontairement :

- adaptation finale du rendu à la surface réelle de `WBP_GridMap` ;
- polish graphique reporté depuis MON21.6.11 ;
- clipping du zoom/pan à la zone cartographique ;
- Automation de clôture ciblée ;
- régression complète de la famille MON21.6 ;
- smoke PIE final.

Le pipeline reste inchangé :

```text
LevelAsset + DungeonRuntimeState
        -> MapExploration
        -> filtered read model
        -> FGridMapFloorView
        -> transient view transform
        -> NativePaint()
```

## 2. Validation finale MON21.6.12

La première validation fournie par l’utilisateur :

```text
Filter                  : Grimrock.Map.MON21_6_12
Succeeded               : 4
Succeeded with warnings : 0
Failed                  : 0
Not run                 : 0
Process exit code       : 0
Report                  : TD04-20260928-124412
```

Le cas `Stairs_Up` a ensuite montré que l’arrivée inter-level ne révélait pas immédiatement la cellule d’arrivée.

Le correctif pré-6.13 :

```text
TravelToDungeonLevel()
    -> SetGridStart(...)
    -> RevealMapAroundCell(arrival)
```

est confirmé **VALIDÉ par l’utilisateur en Automation/PIE**. La sortie détaillée du second run à cinq tests n’a pas été recopiée dans le thread ; aucun compteur ou Report supplémentaire n’est inventé ici.

MON21.6.12 est donc **VALIDÉ**.

## 3. Fit canvas final

L’ancien prototype calculait d’abord un fit sur le canvas, puis le plafonnait à :

```text
MaxCellPixels = 64
```

Sur une petite zone explorée, ce plafond expliquait directement pourquoi la carte restait petite au centre de `WBP_GridMap`.

MON21.6.13 remplace ce comportement par :

```text
surface disponible = AllottedGeometry - MapDrawPadding
span de fit         = contenu connu + 2 * AutoFitMarginCells
cell size           = min(width/spanX, height/spanY)
optional cap        = seulement si MaxCellPixels > 0
zoom                = appliqué après le fit
```

Valeurs finales par défaut :

```text
MapDrawPadding     = 48 / 72 / 48 / 96
AutoFitMarginCells = 0.75
MaxCellPixels      = 0   // aucun cap fixe
```

Une carte de quelques cellules peut donc réellement exploiter la surface disponible, tandis qu’une carte minuscule conserve une marge de respiration.

`MaxCellPixels` reste disponible pour un designer qui voudrait volontairement réintroduire un plafond.

## 4. Clipping zoom / pan

Le Paint crée maintenant une clipping zone exactement sur la surface cartographique déduite de `MapDrawPadding`.

Conséquence :

- zoom élevé : géométrie hors viewport masquée ;
- pan : murs/symboles ne débordent plus sur les contrôles ;
- clic-glisser et molette ne démarrent que lorsque le pointeur est dans la zone cartographique ;
- le read model n’est pas modifié ;
- aucun hit-test ou widget par cellule n’est ajouté.

## 5. Polish graphique final

Le langage graphique de MON21.6.11 est conservé, mais finalisé :

### Parchemin

- bordure manuscrite `ParchmentEdgeColor` ;
- `ParchmentEdgeThickness = 2` ;
- grain toujours déterministe ;
- aucun asset texture supplémentaire obligatoire.

### Murs

Les murs ont maintenant deux niveaux :

```text
underlay brun large
    +
trait d’encre sombre
```

Réglages :

```text
WallThickness = 3.5
WallUnderlayThicknessScale = 2.35
```

Le résultat doit moins ressembler à une ligne de debug et davantage à un mur cartographique encré.

### Portes

Les portes conservent leur interruption centrale mais reçoivent deux jambages graphiques perpendiculaires.

```text
DoorJambLengthScale = 2.15
```

Les portes ouvertes/fermées et secrets découverts restent différenciés par le même read model qu’avant.

### Texture intérieure

Le premier pass artistique était volontairement assez visible. La clôture réduit légèrement l’effet :

```text
CellHatchLineCount    = 2
CellHatchColor alpha  = 0.14
HandDrawnJitterPixels = 1.10
SecondaryStrokeAlpha  = 0.24
```

### Groupe

`PartyMarkerScale = 0.78` réduit légèrement la flèche du groupe afin de mieux laisser respirer un symbole présent sur la même cellule.

## 6. Autorités inchangées

MON21.6.13 ne change pas :

- `UGridDungeonAsset` ;
- `UGridLevelAsset` ;
- `FGridDungeonRuntimeState` ;
- `FGridMapExplorationState` ;
- `FGridMapFloorView` comme projection transient ;
- la convention de coordonnées Map ;
- le filtrage des secrets ;
- `MapSymbolStyle` ;
- SaveGame v23 exact-match.

Le miroir X visuel validé en MON21.6.8 reste inchangé. Sa clarification conceptuelle reste différée et ne bloque pas la clôture fonctionnelle.

## 7. Aucun changement UMG requis

MON21.6.13 n’ajoute aucun `BindWidget` et ne modifie aucun `.uasset`.

`WBP_GridMap` utilise directement son `AllottedGeometry` comme référence de taille.

Si le Blueprint avait explicitement surchargé d’anciennes valeurs héritées, utiliser `Reset to Default` pour :

```text
MapDrawPadding
MaxCellPixels
CellHatchColor
CellHatchLineCount
HandDrawnJitterPixels
SecondaryStrokeAlpha
WallThickness
```

## 8. Automation ciblée MON21.6.13

Filtre :

```text
Grimrock.Map.MON21_6_13
```

Quatre tests :

```text
Layout.CanvasAutoFitUsesAvailableSurface
Layout.BreathingRoomAndZoom
Presentation.FinalPolishDefaults
Closure.TransientPresentationContract
```

Ils vérifient notamment que le fit d’une carte 5×5 sur un canvas représentatif dépasse bien l’ancien plafond de 64 px par cellule.

## 9. Régression globale Map

Après le filtre ciblé vert, lancer :

```text
Grimrock.Map.MON21_6
```

Le repository contient à ce stade **50 tests Map MON21.6** :

```text
6.2   4
6.3   4
6.4   4
6.5   4
6.6   4
6.7   4
6.8   4
6.9   5
6.10  4
6.11  4
6.12  5
6.13  4
TOTAL 50
```

Le total décrit l’état du repository au commit MON21.6.13 ; la clôture exige le retour réel du harness utilisateur.

## 10. Smoke PIE final

Checklist :

- ouverture `M` ;
- petite zone explorée : carte sensiblement plus grande et bien centrée ;
- zone explorée étendue : fit toujours contenu dans le parchemin ;
- zoom avant : contenu correctement clippé ;
- pan extrême : aucun dessin sur les contrôles ;
- Up/Down : étages corrects ;
- Recentrer : retour étage + groupe ;
- murs plus graphiques et lisibles ;
- portes avec jambages identifiables ;
- symboles Up/Down/Relocation/Pit visibles selon leurs règles ;
- arrival inter-level : symbole de la cellule d’arrivée immédiatement visible ;
- secret non découvert toujours indistinguable d’un mur normal.

## 11. Stop condition

La stop condition MON21.6 est atteinte depuis le **4 octobre 2026**, après :

1. `Grimrock.Map.MON21_6_13` vert ;
2. `Grimrock.Map.MON21_6` vert ;
3. smoke PIE final confirmé par l’utilisateur.

La roadmap produit peut reprendre les autres tranches MON21 sans nouvelle couche Map.

## 12. Validation finale — 4 octobre 2026

Validation fournie depuis le harness local UE5.5.4 :

```text
Grimrock.Map.MON21_6_13
Succeeded               : 4
Succeeded with warnings : 0
Failed                  : 0
Not run                 : 0
```

```text
Grimrock.Map.MON21_6
Succeeded               : 50
Succeeded with warnings : 0
Failed                  : 0
Not run                 : 0
```

Le smoke PIE final a été confirmé **OK** par l’utilisateur.

La régression globale finale sur la baseline runtime/content canonique `9045ef2db75c09997db4fc65dbf99d4598f4df5c` a ensuite confirmé :

```text
Grimrock
Succeeded               : 1026
Succeeded with warnings : 0
Failed                  : 0
Not run                 : 0
Process exit code        : 0
```

**Décision : MON21.6 est VALIDÉ et CLOS.**
