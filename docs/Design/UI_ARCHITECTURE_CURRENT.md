# UI Architecture Current State

Statut : **CURRENT — UI-CLEAN01**  
Date : **20 septembre 2026**

## Références canoniques

Le détail technique du menu joueur reste :

```text
docs/Design/UI_GRIMROCK_MENU_CURRENT.md
```

Pour la dette technique transversale :

```text
docs/Architecture/TECHNICAL_DEBT_REGISTER.md
```

Pour la roadmap active :

```text
docs/Design/PROJECT_COMPLETION_ROADMAP.md
```

## Menu joueur actuel

`WBP_GrimrockMenu` est désormais uniquement le shell temporaire des pages non-Inventaire encore non migrées. Son parent natif est `UGrimrockMenuWidget`. L'Inventaire n'y possède plus aucun fallback C++.

### UI-FOUNDATION01 — fondation historique du workspace

UI-FOUNDATION01 a validé les deux responsabilités CharacterSheet / InventoryBag dans un même WBP. Cette disposition monolithique est désormais **superseded côté présentation par UI-SPLIT01**. Les invariants de données restent valides : un seul `SelectedCharacterIndex`, un seul `UGridPartyInventoryComponent`, aucune duplication de gameplay.

Référence historique : `docs/Design/UI_FOUNDATION01_UNIFIED_INVENTORY_CHARACTER_WORKSPACE.md`.

### UI-SPLIT01 — deux fenêtres viewport indépendantes

L'Inventaire quitte `WBP_GrimrockMenu` lorsque les classes split sont configurées.

```text
Viewport
├── WBP_CharacterSheet       gauche, pleine hauteur utile
├── vue 3D                   centre
├── WBP_InventoryBag         droite, pleine hauteur utile
└── WBP_GridCombatHud        barre basse persistante
```

Les deux fenêtres dérivent de la même implémentation native `UGridInventoryWidget` via deux classes sémantiques fines. Elles partagent le même composant inventaire et se resynchronisent via `OnPartyInventoryChanged`.

`WBP_GrimrockMenu` reste temporairement utilisé pour Skills/Spellbook/Journal/Map/Recipes/Codex. Depuis UI-CLEAN01, il ne possède plus du tout `Page_Inventory`, même comme fallback.

Référence : `docs/Design/UI_SPLIT01_INDEPENDENT_INVENTORY_WINDOWS.md`.

### UI-SPLIT02 — inventaire split non modal

Le workspace split laisse désormais la vue 3D centrale interactive. Les interactions monde restent disponibles pendant que CharacterSheet et InventoryBag sont ouverts. Le menu contextuel d'item, lui, reste modal lorsqu'il est visible.

Le presenter Blueprint de WBP_ItemActionMenu a été migré vers WBP_InventoryBag et validé en PIE.

Référence : `docs/Design/UI_SPLIT02_WORLD_INTERACTION_AND_CONTEXT_MENU.md`.

### UI-SPLIT03 — paper doll dépendant du rôle

UGridInventoryWidget n'enregistre plus automatiquement les 18 slots paper doll dans toutes ses sous-vues.

```text
WBP_CharacterSheet -> paper doll présent -> enregistrement + validation
WBP_InventoryBag   -> aucun paper doll   -> aucun faux warning
```

Référence : `docs/Design/UI_SPLIT03_ROLE_AWARE_PAPERDOLL.md`.

### UI-CLEAN01 — suppression du monolithe Inventory

L'architecture split devient exclusive.

```text
SUPPRIMÉ C++ :
WBP_GrimrockMenu -> Page_Inventory
UGridInventoryWidget -> Panel_CharacterSheet / Panel_InventoryBag
UGridInventoryWidget -> ResetInventoryWorkspace / états internes de panneaux
```

Les boutons de fermeture appartiennent désormais aux classes spécifiques `UGridCharacterSheetWidget` et `UGridInventoryBagWidget`. L'ancien fichier `GridInventoryWidgetWorkspace.cpp` et son test de caractérisation historique sont supprimés.

Référence : `docs/Design/UI_CLEAN01_REMOVE_MONOLITHIC_INVENTORY.md`.

### UI-NAV01 — barre inférieure persistante

La navigation `ESC / I / K / G / M / J / H` n'est pas enfant du menu. Elle est intégrée à la surface HUD runtime persistante déjà portée par `WBP_GridCombatHud`, à côté de la hotbar MON12.

```text
WBP_GridCombatHud
└── BottomBar (ancrée en bas)
    ├── Panel_GlobalNavigation    toujours visible
    └── Panel_Actions             hotbar 1..0 existante
```

Le menu peut être ouvert, fermé ou changer de page sans modifier la visibilité de `Panel_GlobalNavigation`. Les boutons et les touches passent par les mêmes commandes C++.

Référence : `docs/Design/UI_NAV01_PERSISTENT_BOTTOM_NAVIGATION.md`.

### UI-CHAR01 — personnage sélectionné unique

La refonte ne crée aucun état parallèle. `UGridPartyInventoryComponent::SelectedCharacterIndex` reste l'unique autorité pour la feuille, le paper doll et le sac.

`WBP_CharacterSheet` expose six instances canoniques `PartyMember_1..6`, enregistrées nativement sur les indices `0..5`. `WBP_PartyMember` reçoit en option `Image_Portrait` et `Border_Selected` pour projeter le portrait et l'état de sélection sans logique Blueprint.

Référence : `docs/Design/UI_CHAR01_SINGLE_SELECTED_CHARACTER.md`.

### UI-CHAR02 — projection de la feuille personnage

`UGridInventoryWidget` continue de lire directement `FGridInventoryCharacterSummary`. Aucun ViewModel ou calcul gameplay UI supplémentaire n'est ajouté.

La feuille expose désormais, en plus des champs existants :

```text
ProgressBar_CharacterHealth
ProgressBar_CharacterMana
ProgressBar_CharacterCarryWeight
Text_CharacterInventorySlots
Text_CharacterPhysicalArmor
Text_CharacterMagicalArmor
Text_CharacterInitiative
Text_CharacterAccuracy
Text_CharacterEvasion
Text_ResistancePhysical
```

Les ratios sont uniquement des projections visuelles des valeurs canoniques et sont clampés dans `[0..1]`.

Référence : `docs/Design/UI_CHAR02_CHARACTER_SHEET_PROJECTION.md`.

### UI-INV01 — un seul sac pour le personnage sélectionné

Le panneau droit ne représente jamais simultanément les inventaires des six membres. `WBP_InventoryBag` conserve une seule `InventorySlotsGridPanel`, alimentée par `SelectedCharacterIndex`.

Le titre, l'occupation et la charge du sac sont projetés vers :

```text
Text_InventoryBagTitle
Text_InventoryBagSlotUsage
Text_InventoryBagWeight
ProgressBar_InventoryBagWeight
```

`RefreshInventory()` resynchronise aussi le layout de slots afin qu'une même grille puisse suivre des capacités différentes selon le personnage.

Référence : `docs/Design/UI_INV01_SELECTED_CHARACTER_SINGLE_BAG.md`.

### UI-INV02 — transfert par drag vers un portrait

Le drag d'un slot d'inventaire capture désormais le personnage source en plus du slot et de l'identité runtime. Les portraits `UGridPartyMemberWidget` sont des drop targets natifs.

Le routage reste :

```text
slot inventaire
-> UGridInventoryDragDropOperation
-> portrait
-> UGridInventoryWidget
-> UGridItemTransferService
-> UGridPartyInventoryComponent
```

La sélection UI ne change pas à la suite d'un transfert. Un Ctrl-drag transfère une quantité séparée avec une nouvelle identité runtime.

Référence : `docs/Design/UI_INV02_PARTY_DRAG_TRANSFER.md`.

```text
Inventaire      fonctionnel
Compétences     fonctionnel MON20
Sorts           fonctionnel MON18
Journal         shell présent ; read model prévu MON21.5
Carte           shell présent ; exploration prévue MON21.6
Recettes        shell présent ; fonctionnalité future
Codex           shell présent ; discovery prévu MON21.7
```

La navigation des onglets est portée par le C++. Le Graph de `WBP_GrimrockMenu` ne doit pas recréer une navigation parallèle.

## Quest runtime

MON21.2 et MON21.3 ont livré la couche métier avant l’UI Journal :

```text
UGridQuestDefinitionAsset
    -> QuestId / Objectives

UGridQuestSubsystem
    -> état runtime campagne
    -> OnQuestStateChanged

FGridObjectLink / Event -> Command
    -> QuestStart
    -> QuestCompleteObjective
    -> QuestComplete
    -> QuestFail
```

Le Journal futur doit relire cette autorité. Il ne doit pas stocker sa propre copie des quêtes.

L’état Quest n’est pas encore persistant en SaveGame v9 ; **MON21.4 — Quest Persistence / Migration** est la prochaine tranche.

## Spellbook

`WBP_GridSpellbook` utilise `UGridSpellbookWidget` comme parent natif. Le modèle de connaissance autoritaire reste :

```text
UGridPartySpellbookComponent
    -> CharacterId
    -> KnownSpellIds[]
```

La vue est construite depuis les autorités runtime et ne possède aucune copie gameplay autoritaire.

## Skills / Talents

MON20 a livré :

```text
SelectedCharacterIndex
    -> FGridSkillsPageService
    -> UGridSkillsWidget
    -> WBP_GridSkills
```

La page Compétences projette Skills + Talents du personnage sélectionné. Les rangs de Skills sont persistés dans SaveGame v9 ; les Talents réutilisent les `ProgressionChoices` MON15.

## Hotbar

Le Spellbook réutilise la hotbar MON12 à dix slots. Aucun second stockage de raccourcis n’existe.

```text
ActionId           = SpellId
SourcePolicy       = Spell
SourceDefinitionId = SpellId
```

## Responsabilités

```text
UGrimrockMenuWidget
    shell temporaire Skills / Spellbook / Journal / Map / Recipes / Codex

UGridInventoryWidget
    mécanique/projection commune réellement partagée
    sélection / slots / refresh / interactions item

UGridCharacterSheetWidget
    fenêtre gauche / party / stats / paper doll

UGridInventoryBagWidget
    fenêtre droite / grille inventaire / menu contextuel

UGridCombatHudWidget
    HUD runtime persistant
    navigation globale ESC/I/K/G/M/J/H
    hotbar MON12 1..0
    éléments combat conditionnels

UGridSkillsWidget
    projection Skills / Talents

UGridSpellbookWidget
    présentation Sorts

UGridPartySpellbookComponent
    connaissance runtime des sorts

UGridPartyInventoryComponent
    autorité groupe/inventaire + sélection + hotbar

UGridQuestSubsystem
    autorité runtime des quêtes

UGridTurnManagerComponent
    autorité combat
```

## Sélection / held visual

`TD-PARTY-001` est **RÉSOLU**. `SetSelectedCharacterIndex()` déclenche la notification autoritaire ; `AGrimrockPartyPawn` resynchronise son held visual.

TD06.9 a clôturé le découpage PartyInventory ; aucune nouvelle extraction n’est recommandée sans signal concret.

## Dette technique UI active

Ne pas mélanger dette technique et fonctionnalités futures.

Dette réelle suivie dans `TECHNICAL_DEBT_REGISTER.md` :

- `TD-UI-001` : nommage historique `EInventoryTopTab` / `ToggleInventoryWidget()` ; faible priorité ;
- `TD-LOG-001` : certaines zones utilisent encore `LogTemp` ;
- divergence visuelle potentielle entre surfaces UMG : à réduire seulement si une douleur concrète apparaît.

## Travail futur qui n’est pas de la dette technique

- MON21.4 : persistance Quest / migration ;
- MON21.5 : Journal Read Model / intégration WBP ;
- MON21.6 : Map Geometry / Exploration ;
- MON21.7 : Codex Discovery / projections ;
- sélection explicite d’un autre allié pour les sorts `Ally` : amélioration fonctionnelle/UX ;
- icônes finales : contenu de production ;
- Recipes : fonctionnalité future.

## Validation

TD04 a établi :

```text
Scripts/ValidateUE.ps1       -> Editor build + Automation
Scripts/ValidatePackage.ps1  -> Win64 Shipping cook/package
```

Les bindings/Blueprints/UMG/assets réellement touchés restent à vérifier en PIE.

## Phase actuelle

```text
MON20      CLOS
MON21.1    CLOS
MON21.2    VALIDÉ
MON21.3    VALIDÉ
TD05.9     STOP CONDITION ATTEINTE
TD06.9     STOP CONDITION ATTEINTE
MON21.4    PROCHAIN — Quest Persistence / Migration
```


## Jalons de refonte UI validés côté Automation au 20 septembre 2026

```text
UI-NAV01   2/2
UI-CHAR01  2/2
UI-CHAR02  2/2
UI-INV01   2/2
UI-INV02   2/2
```

`WBP_GridCombatHud`, `WBP_CharacterSheet` et `WBP_InventoryBag` ont désormais fait l'objet de la passe UMG/PIE. UI-CLEAN01 retire le chemin monolithique restant avant de poursuivre les fonctionnalités UI.


## Validation UMG UI-NAV01

`WBP_GridCombatHud` a été validé manuellement en PIE le 20 septembre 2026 : barre inférieure réellement collée au viewport, navigation globale fonctionnelle et hotbar MON12 conservée. La prochaine passe UMG concerne `WBP_GridInventory`.
