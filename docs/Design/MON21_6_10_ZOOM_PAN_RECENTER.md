# MON21.6.10 — Zoom / Pan / Recenter

Date : **28 septembre 2026**  
Statut : **C++ IMPLÉMENTÉ — VALIDATION AUTOMATION + UMG/PIE UTILISATEUR REQUISES**

## 1. Objectif

MON21.6.10 ajoute une caméra de présentation transitoire au renderer natif de la Map.

Le read model reste inchangé :

```text
FGridMapFloorView
    -> caméra UI transient
       -> NativePaint()
```

Zoom, pan et recentrage ne deviennent jamais des autorités gameplay.

## 2. État de caméra

`UGridMapWidget` porte uniquement :

```text
ZoomScale
PanOffsetPixels
bCenterViewOnParty
bIsPanning
```

Ces champs sont `Transient` et ne portent pas `SaveGame`.

Le schéma reste **v23 exact-match**.

## 3. Zoom

La molette appelle `AdjustZoom(WheelDelta)`.

Règle :

```text
ZoomScale = clamp(
    ZoomScale + WheelDelta * ZoomStep,
    MinZoomScale,
    MaxZoomScale)
```

Valeurs fonctionnelles par défaut :

```text
MinZoomScale = 0.50
MaxZoomScale = 4.00
ZoomStep     = 0.20
```

Le zoom ne reconstruit pas `FGridMapFloorView`. Il invalide uniquement le Paint.

## 4. Pan

Le bouton gauche de la souris démarre le pan sur la surface Map.

Pendant le drag :

```text
PanOffsetPixels += CurrentLocalMouse - PreviousLocalMouse
```

Le widget capture la souris pendant l’opération et libère la capture au relâchement.

Le pan ne reconstruit pas `FGridMapFloorView` et n’affecte aucune coordonnée Map canonique.

## 5. Changement d’étage

`NavigateFloorUp()` et `NavigateFloorDown()` conservent le niveau de zoom mais remettent le pan à zéro.

L’étage nouvellement sélectionné repart donc centré selon le fit automatique du renderer.

## 6. Recentrer

`RecenterMap()` respecte le contrat MON21.6.1 :

```text
1. revenir au LogicalPosition.Z du CurrentDungeonLevelId
2. reconstruire cet étage
3. remettre PanOffsetPixels à zéro
4. centrer la cellule du groupe dans la surface Map
5. conserver le ZoomScale courant
```

Le centrage du groupe est appliqué dans `BuildRenderMetrics()` au moment du Paint. Aucun état dérivé supplémentaire n’est sauvegardé.

## 7. Projection écran

La convention visuelle validée en MON21.6.8 reste inchangée :

```text
North / Y+ -> haut écran
East  / X+ -> gauche écran
South / Y- -> bas écran
West  / X- -> droite écran
```

MON21.6.10 ne rouvre pas la discussion sur le miroir X ; ce point reste différé.

## 8. Intégration UMG

Un binding optionnel supplémentaire est introduit :

```text
Button_Recenter : UButton
```

Le C++ bind automatiquement `OnClicked` vers `RecenterMap()` dans `NativeConstruct()`.

Aucun Graph Blueprint n’est nécessaire.

Le zoom molette et le pan clic-glisser ne requièrent aucun nouveau widget enfant.

## 9. Validation MON21.6.9

MON21.6.9 est confirmé **VALIDÉ** par l’utilisateur après :

- navigation Up/Down fonctionnelle en PIE ;
- libellé final `Niveau <SelectedFloorZ>` validé ;
- abandon de l’utilisation de `DisplayName` pour nommer un étage multi-dalles.

La sortie détaillée du dernier run à cinq tests n’a pas été recopiée dans le thread ; aucun compteur n’est donc inventé dans ce document.

## 10. Automation MON21.6.10

Filtre :

```text
Grimrock.Map.MON21_6_10
```

Tests :

```text
View.ZoomClampsWithoutReadModelRebuild
View.PanIsTransientAndFloorNavigationResetsIt
View.RecenterReturnsToPartyFloorAndPosition
View.OptionalUMGAndTransientContract
```

Les tests vérifient notamment que zoom et pan ne réallouent pas le buffer `FloorView.Cells`.

Aucun résultat MON21.6.10 n’est déclaré avant retour du harness UE5.5.4 local.

## 11. Modification UMG après validation C++

Dans `WBP_GridMap`, ajouter manuellement :

```text
Button_Recenter : Button
```

Le placement et le style restent fonctionnels à ce stade. Le polish artistique appartient à MON21.6.11.

## 12. Smoke PIE

À vérifier :

- molette : zoom avant/arrière borné ;
- clic gauche + glisser : déplacement de la carte ;
- Up/Down : changement d’étage avec pan remis à zéro ;
- Recentrer depuis un autre étage : retour au niveau du groupe ;
- Recentrer après pan : groupe centré ;
- zoom conservé après Recentrer.

## 13. Hors périmètre

- texture/fond parchemin : MON21.6.11 ;
- irrégularités manuscrites déterministes : MON21.6.11 ;
- symboles supplémentaires : MON21.6.12 ;
- passe de régression/closure Map : MON21.6.13.

Prochaine tranche après validation : **MON21.6.11 — Hand-Drawn Parchment Artistic Pass**.
