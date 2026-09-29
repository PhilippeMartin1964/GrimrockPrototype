# MAP-UI03 — Floor Navigation Overlay

Date : 29 septembre 2026  
Statut : **C++ IMPLÉMENTÉ — CONFIGURATION UMG + VALIDATION LOCALE/PIE REQUISES**

## Objectif

Déplacer le système de navigation entre étages directement **par-dessus la carte** dans `WBP_GridMap`, sans créer une seconde logique de navigation.

Ce ticket réutilise intégralement le contrat MON21.6.9 déjà validé :

- `Button_LevelUp` ;
- `Button_LevelDown` ;
- `Text_FloorLabel` ;
- `NavigateFloorUp()` ;
- `NavigateFloorDown()` ;
- `CanNavigateFloorUp()` ;
- `CanNavigateFloorDown()`.

Aucune logique de Dungeon, Map read model, exploration, secret, SaveGame ou coordonnées n'est modifiée.

## C++ ajouté

`UGridMapWidget` expose un conteneur de présentation optionnel :

```text
Panel_FloorNavigationOverlay : UPanelWidget
```

Son rôle est uniquement visuel :

- `SelfHitTestInvisible` lorsqu'un étage logique est sélectionné ;
- `Collapsed` lorsqu'aucun étage ne peut être résolu.

Les boutons enfants restent interactifs et continuent d'être pilotés par la logique MON21.6.9 existante.

## Modification UMG manuelle requise

Dans `WBP_GridMap`, **reparenter les contrôles existants** au lieu d'en créer de nouveaux.

Structure recommandée :

```text
Root Overlay / Canvas
├── rendu carte / parchemin
├── bandeau MAP-UI02
└── Panel_FloorNavigationOverlay
    └── Border_FloorNavigation
        └── VerticalBox_FloorNavigation
            ├── Button_LevelUp
            │   └── symbole ↑
            ├── Text_FloorLabel
            └── Button_LevelDown
                └── symbole ↓
```

Le nom exact requis pour le conteneur est :

```text
Panel_FloorNavigationOverlay
```

Conserver impérativement les noms existants :

```text
Button_LevelUp
Text_FloorLabel
Button_LevelDown
```

Ne recréez **aucun OnClicked Blueprint** : les handlers sont déjà branchés en C++.

### Placement recommandé

Le panneau doit être superposé au parchemin, pas placé à côté de celui-ci.

Pour un `Canvas Panel` :

- Anchor : haut-droite ;
- Alignment : `X=1.0, Y=0.0` ;
- position approximative : 16–24 px depuis le bord droit du parchemin et 16–24 px sous son bord supérieur ;
- largeur : ~72–96 px ;
- hauteur : automatique ou ~120–150 px ;
- ZOrder supérieur au rendu de la carte ;
- fond discret, semi-transparent, cohérent avec le parchemin.

Le but est d'obtenir visuellement :

```text
┌────────────────────────────────────────────┐
│ CARTE                                   X │
├────────────────────────────────────────────┤
│                                      ┌───┐ │
│                                      │ ↑ │ │
│          plan du donjon              │N 0│ │
│                                      │ ↓ │ │
│                                      └───┘ │
│                                            │
└────────────────────────────────────────────┘
```

Le panneau doit réellement recouvrir la zone cartographique. Il ne doit pas réduire la taille disponible pour le `NativePaint()`.

`Button_Recenter` reste inchangé dans MAP-UI03 ; il pourra être intégré à un futur pass de polish des contrôles si souhaité.

## Interaction

Le parent du panneau est `SelfHitTestInvisible`, ce qui signifie :

- le panneau lui-même n'intercepte pas inutilement la souris ;
- `Button_LevelUp` et `Button_LevelDown` restent cliquables ;
- le pan/zoom de la carte reste disponible hors des boutons.

## Automation

Filtre ciblé :

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.UI.MapUI03"
```

Attendu : **2 tests**.

Régression de la navigation d'étage :

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.Map.MON21_6_9"
```

Attendu : **5 tests**.

## Smoke PIE

- `M` ouvre la carte autonome ;
- le panneau d'étage est affiché au-dessus du parchemin ;
- le libellé indique l'étage logique courant ;
- ↑ passe à l'étage logique supérieur disponible ;
- ↓ passe à l'étage logique inférieur disponible ;
- les boutons impossibles sont désactivés aux bornes ;
- le marqueur de groupe n'apparaît que sur l'étage courant ;
- pan/zoom fonctionnent derrière/autour du panneau ;
- le bandeau et le bouton X de MAP-UI02 restent fonctionnels.

## Stop condition

MAP-UI03 est validé après :

1. build UE 5.5.4 vert ;
2. `Grimrock.UI.MapUI03` vert ;
3. `Grimrock.Map.MON21_6_9` vert ;
4. UMG reconfiguré avec les contrôles existants dans `Panel_FloorNavigationOverlay` ;
5. smoke PIE confirmé par l'utilisateur.


## MAP-UI03-FIX01 — Full-screen surface + UMG above NativePaint

Validation visuelle du 29 septembre 2026 : la capture PIE a mis en évidence deux défauts :

1. les contrôles `Button_LevelUp` / `Button_LevelDown` étaient peints derrière la carte ;
2. le parchemin conservait l'ancien inset fonctionnel `48 / 72 / 48 / 96` et n'occupait donc pas toute la surface de `WBP_GridMap`.

Cause du premier défaut : `UGridMapWidget::NativePaint()` appelait `Super::NativePaint()` avant le rendu procédural, puis dessinait la Map sur des layers supérieurs. Les enfants UMG étaient donc recouverts.

Correction :

- la Map procédurale est peinte en premier ;
- `Super::NativePaint()` est appelé en dernier à `MarkerLayer + 1` ;
- le bandeau MAP-UI02, `Panel_FloorNavigationOverlay` et `Button_Recenter` restent ainsi au-dessus du parchemin ;
- `MapDrawPadding` vaut désormais `FMargin(0)` par défaut ;
- `AutoFitMarginCells` conserve l'espace visuel autour de la géométrie connue sans réduire la surface du parchemin.

Aucune donnée Map, exploration, SaveGame ou navigation d'étage n'est modifiée.

### Blueprint existant

Si `WBP_GridMap` possède un override sérialisé de l'ancienne valeur `MapDrawPadding = 48 / 72 / 48 / 96`, sélectionner la propriété dans les Class Defaults puis utiliser **Reset to Default**. La valeur attendue est désormais `0 / 0 / 0 / 0`.
