# UI-GLOBALHUD01 — Persistent Navigation & Action Bar Split

Date : **26 septembre 2026**  
Statut : **C++ prêt pour validation UE5.5.4 ; migration UMG requise**

## Problème audité

La barre basse historique était hébergée dans `WBP_GridCombatHud` alors qu'elle mélangeait trois responsabilités différentes :

```text
navigation globale ESC / I / K / G / M / J / H
barre générale d'actions
HUD de combat (PAM, initiative, round, fin de tour, ciblage)
```

Les deux premières existent pendant toute l'exploration. Elles ne sont donc pas des éléments de combat.

## Architecture cible

```text
Viewport
├── WBP_GridPersistentHud
│   └── barre basse permanente
│       ├── Panel_GlobalNavigation
│       │   └── ESC / I / K / G / M / J / H
│       └── Panel_ActionBar
│           └── 16 slots Fill, sans espacement
│
└── WBP_GridCombatHud
    └── présentation combat uniquement
        ├── PAM
        ├── initiative / round
        ├── panneaux des combattants
        ├── fin de tour
        └── ciblage
```

Le parent natif du nouveau widget est :

```cpp
UGridPersistentHudWidget
```

Le Pawn possède désormais séparément :

```text
PersistentHudWidgetClass / PersistentHudWidgetInstance
CombatHudWidgetClass     / CombatHudWidgetInstance
```

## Navigation globale

Les boutons conservent leurs commandes C++ historiques. Le nouveau HUD ne crée aucune seconde navigation.

Le bouton actif utilise le même pattern que les boutons de filtre de `WBP_InventoryBag` :

```text
Image_NavInventorySelectionFrame
Image_NavSkillsSelectionFrame
Image_NavCraftingSelectionFrame
Image_NavMapSelectionFrame
Image_NavJournalSelectionFrame
Image_NavHelpSelectionFrame
```

Chaque image est :

- `HitTestInvisible` lorsque la page correspondante est active ;
- `Collapsed` sinon ;
- dessinée avec le même cadre doré que les frames de sélection de l'Inventaire.

`ESC` possède aussi un binding optionnel `Image_NavEscapeSelectionFrame`, mais il reste volontairement non sélectionné tant que le menu pause/main-menu Blueprint n'expose pas une autorité native de visibilité. Aucun faux état n'est inventé.

## Barre générale d'actions

La barre persistante contient désormais **16 slots**.

Les douze premiers reçoivent le profil clavier suisse demandé :

```text
slot  1 -> 1
slot  2 -> 2
slot  3 -> 3
slot  4 -> 4
slot  5 -> 5
slot  6 -> 6
slot  7 -> 7
slot  8 -> 8
slot  9 -> 9
slot 10 -> 0
slot 11 -> '
slot 12 -> ^
slot 13 -> souris uniquement
slot 14 -> souris uniquement
slot 15 -> souris uniquement
slot 16 -> souris uniquement
```

Le choix de layout clavier est volontairement isolé pour une évolution ultérieure ; le jalon actuel implémente le profil suisse.

Les slots sont placés dans un `HorizontalBox` avec :

```text
Size rule = Fill
Padding   = 0
```

Ils sont donc collés les uns aux autres et se partagent toute la largeur laissée disponible après la navigation globale.

Le raccourci clavier est affiché en bas à droite. Le badge de quantité natif est déplacé en haut à droite pour éviter tout chevauchement.

## Persistance / sauvegardes existantes

Le stockage historique s'appelle encore `FGridCombatHotbarBinding` / `CombatHotbarSlots`. Ce nom est désormais historique, mais il reste l'unique autorité de binding afin de ne pas créer une seconde barre.

Le nombre canonique passe de 10 à 16 slots. Lors du chargement, `InitializeCombatHotbarDefaults()` normalise systématiquement les anciennes sauvegardes :

```text
ancienne sauvegarde 10 slots
    -> conservation des slots 1..10
    -> ajout des slots 11..16 vides
```

Aucune migration de schéma SaveGame distincte n'est nécessaire pour ce `TArray`.

Le renommage complet des types historiques `CombatHotbar*` pourra être réalisé dans un ticket de nettoyage après migration UMG ; il ne doit pas créer de seconde autorité.

## Migration UMG requise

Ne pas modifier `WBP_GridCombatHud.uasset` à l'aveugle.

Créer :

```text
WBP_GridPersistentHud
Parent Class = UGridPersistentHudWidget
```

Hiérarchie recommandée :

```text
CanvasPanel_Root
└── HorizontalBox_BottomBar
    ├── Panel_GlobalNavigation   Auto
    │   ├── ESC
    │   ├── I
    │   ├── K
    │   ├── G
    │   ├── M
    │   ├── J
    │   └── H
    └── Panel_ActionBar         Fill
```

Chaque bouton navigation est idéalement un `Overlay` contenant le bouton et son `Image_*SelectionFrame`.

`Panel_ActionBar` doit être un `HorizontalBox` extensible. Les enfants sont générés/réutilisés par le C++.

Dans `BP_GrimrockPartyPawn` :

```text
Persistent Hud Widget Class = WBP_GridPersistentHud
```

Pendant la migration, `WBP_GridCombatHud` peut encore contenir ses anciens `Panel_GlobalNavigation` et `Panel_Actions` : le C++ les collapse automatiquement dès que le Persistent HUD existe. Ils pourront ensuite être supprimés du Designer.

## Compatibilité de migration

Si `PersistentHudWidgetClass` n'est pas encore configuré, le vieux chrome intégré à `WBP_GridCombatHud` reste visible. Cela permet de compiler et valider le C++ avant la modification UMG, sans écran cassé.

## Validation

Filtres principaux :

```text
Grimrock.UI.GlobalHud01
Grimrock.UI.Navigation01
Grimrock.Monsters.MON12.CombatHUD
Grimrock.Monsters.MON12.8.3
```

PIE après migration UMG :

1. la barre basse existe en exploration ;
2. PAM/initiative restent absents hors combat ;
3. le cadre doré suit I/K/G/M/J/H ;
4. la navigation reste fonctionnelle au clavier et à la souris ;
5. les 16 slots sont jointifs et remplissent la largeur disponible ;
6. les labels sont `1 2 3 4 5 6 7 8 9 0 ' ^`, puis vides ;
7. les slots 13..16 restent cliquables à la souris ;
8. en combat, le HUD combat apparaît sans dupliquer navigation ou barre d'actions.
