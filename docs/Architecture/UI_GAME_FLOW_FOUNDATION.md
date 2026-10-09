# UI et flux de jeu — Fondation d'architecture

> **Contrat courant — DOC-ARCH01, 9 octobre 2026.**
>
> L'ancienne architecture de menu monolithique est superseded. Les grandes
> surfaces sont maintenant indépendantes et partagent les mêmes autorités C++.

## 1. Principe

C++ fournit les données, read models, validations et transactions.
Blueprint/UMG compose et présente ces résultats.

Aucun Widget Blueprint ne doit devenir une seconde autorité gameplay.

## 2. Flux principal

```text
L_MainMenu
    -> New Game
        -> Character Creation
        -> progress "Construction du donjon"
        -> L_Dungeon
    -> Continue / Load
        -> L_Dungeon
        -> restore v24

L_Dungeon
    -> exploration
    -> inventaire / character sheet
    -> skills / talents
    -> map
    -> spellbook / journal / recipes / codex
    -> combat
```

## 3. Surfaces viewport courantes

```text
Viewport
├── WBP_CharacterSheet
├── WBP_InventoryBag
├── WBP_GridSkills
├── WBP_GridMap
├── WBP_GridCombatHud
└── WBP_GridPersistentHud
```

### Character Sheet / Inventory

`WBP_CharacterSheet` et `WBP_InventoryBag` sont indépendants mais lisent le
même `UGridPartyInventoryComponent` et le même
`SelectedCharacterIndex`.

### Skills / Talents

`WBP_GridSkills` est autonome et contient COMPÉTENCES / TALENTS.

Le C++ fournit :

- balance Skill Points ;
- rang/cap/disponibilité ;
- arbre Talent 3×5 ;
- détail Talent canonique ;
- acquisition simple ou à variantes.

### Map

`WBP_GridMap` est une fenêtre autonome. Elle n'appartient plus au shell
`WBP_GrimrockMenu`.

### Shell restant

`WBP_GrimrockMenu`, parent `UGrimrockMenuWidget`, conserve temporairement :

```text
Journal
Recipes
Codex
Spellbook
```

Inventory, Skills et Map n'y sont plus des pages actives.

## 4. Persistent HUD

`UGridPersistentHudWidget` / `WBP_GridPersistentHud` possède :

- navigation globale ;
- sélection visuelle du bouton actif ;
- barre d'actions persistante ;
- toast de progression.

La barre d'actions utilise les bindings persistés dans
`FGridCharacterInventoryState::CombatHotbarSlots` et possède au minimum
**12 slots**.

Navigation actuelle :

```text
ESC  I  K  G  M  J  H
```

Les touches et boutons passent par les mêmes routes C++.

## 5. Combat HUD

`UGridCombatHudWidget` / `WBP_GridCombatHud` est **combat-only** :

- panneaux des membres ;
- initiative ;
- PAM ;
- fin du tour ;
- feedback de rejet ;
- targeting combat.

Il ne possède plus la navigation globale ni la barre d'actions persistante.

UI-COMBAT-UNIFY02 est le contrat C++/UMG actuel :

```text
Panel_CombatHud (Overlay root)
    -> HorizontalBox_CombatBottomBar
        -> Panel_PartyMembers
        -> Spacer Fill
        -> Panel_CombatBottomRight
```

Le runtime ne translate plus ces surfaces. Le padding Bottom est authored une
seule fois dans UMG. La validation UMG/PIE finale de cette hiérarchie reste
explicitement à fournir.

## 6. Level Up

Le Level Up est non modal :

```text
FRPGLevelUpService
    -> URPGLevelUpNotificationSubsystem
    -> WBP_GridPersistentHud
        -> WBP_RPGNotification
```

Aucun état d'acknowledgement n'est persisté.

## 7. Quests / Journal / Codex / Recipes

Quest runtime existe :

```text
UGridQuestDefinitionAsset
UGridQuestSubsystem
FGridCampaignQuestRuntimeState
```

Mais Quest n'est pas encore persisté.

- MON21.4 : Quest Persistence — en attente ;
- MON21.5 : Journal — à faire ;
- MON21.7 : Codex — à faire ;
- Recipes/Crafting : futur chantier distinct.

Journal et Codex doivent rester des projections des autorités gameplay.

## 8. Map

Map est **implémentée et close**, pas un shell :

- exploration persistante ;
- secrets filtrés ;
- multi-dalles/multi-étages ;
- symboles ;
- changement d'étage ;
- zoom/pan/recenter ;
- rendu texturé via `UGridMapVisualThemeAsset`.

## 9. Spellbook

Le Spellbook lit `KnownSpellIds` du personnage courant. Il ne possède pas de
snapshot durable parallèle.

## 10. Règles

1. `SelectedCharacterIndex` unique.
2. Persistent HUD = chrome global.
3. Combat HUD = combat seulement.
4. Fenêtres autonomes pour CharacterSheet, InventoryBag, Skills et Map.
5. UMG ne calcule pas les règles métier.
6. Une modification de WBP exige validation UE/PIE.
7. Aucun ancien `WBP_GridInventory` ou Level-Up modal ne doit être réintroduit.
