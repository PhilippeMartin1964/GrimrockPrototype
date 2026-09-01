# HOTBAR01.2 — Remove Legacy Action Palette

Date : 01.09.2026

## Décision

La palette d’actions intermédiaire est supprimée définitivement de l’architecture du HUD.

La seule barre d’actions visible et persistante est désormais la barre fixe :

`1 2 3 4 5 6 7 8 9 0`

Le slot `1` reste réservé à `PrimaryAttack`. Les slots `2–9,0` sont alimentés directement depuis les sources métier :

- inventaire ;
- Spellbook ;
- futures pages de capacités / compétences.

Le système de ciblage `Panel_Targeting` reste indépendant et doit être conservé.

## Code C++ supprimé

HOTBAR01.2 retire du runtime :

- `FGridCombatHudView::ActionPalette` ;
- `UGridCombatHudWidget::Panel_ActionPalette` ;
- `ActionPaletteWidgets` ;
- `RuntimeActionPalettePanel` ;
- `EnsureActionPalettePanel()` ;
- `EnsureActionPaletteWidgets()` ;
- `RefreshActionPaletteWidgets()` ;
- `UGridCombatHudActionWidget::bActionPaletteEntry` ;
- `InitializePaletteAction()` ;
- `BuildPaletteActionToolTip()` ;
- `IsSpellbookManagedAction()` ;
- le payload de drag `bFromActionPalette` ;
- `InitializeFromActionPalette()`.

Le helper de validation des actions directement assignables est conservé sous un nom générique (`IsDirectHotbarActionSource`) car `AssignCombatActionToHotbarSlot()` reste l’API de destination pour les sources métier non-inventaire.

## Nettoyage complémentaire HOTBAR01.2.1

Le binding synthétique historique de lancer MainHand n'est plus conservé. Le projet étant encore en phase prototype, une ancienne sauvegarde qui dépend d'un binding de hotbar supprimé n'est pas un contrat de compatibilité.

Le Spellbook conserve son chemin autonome :

`InitializeFromSpellbookEntry() -> CommitSpellbookDrop()`

Il ne dépend plus d’aucun champ ou état ActionPalette.

## Mise à jour manuelle de WBP_GridCombatHud

Le fichier `.uasset` doit être modifié dans Unreal Editor 5.5.4.

Dans `WBP_GridCombatHud` :

1. ouvrir le Designer ;
2. localiser `Panel_ActionPalette` dans le Widget Tree ;
3. vérifier qu’aucun nœud Blueprint n’y fait référence ;
4. supprimer `Panel_ActionPalette` ;
5. conserver `Overlay_ActionContext` si nécessaire pour `Panel_Targeting` ;
6. conserver `Panel_Targeting`, `Text_TargetingInstructions` et `Text_TargetingCell` ;
7. compiler puis sauvegarder le Widget Blueprint.

Structure cible :

```text
VerticalBox_ActionArea
├─ Overlay_ActionContext
│  └─ Panel_Targeting
└─ ScaleBox_ActionBarPixelLock
   └─ Panel_Actions
```

## Validation

Les filtres prioritaires après compilation sont :

```text
Grimrock.Monsters.MON12.8
Grimrock.Monsters.MON12.10
Grimrock.Monsters.MON12.11
Grimrock.Magic.MON18.7
Grimrock.Hotbar.HOTBAR01_1
```

Les identifiants historiques de certains tests MON12 contiennent encore le mot `ActionPalette` afin de ne pas casser les scripts de validation existants ; leur implémentation ne dépend plus de la palette.
