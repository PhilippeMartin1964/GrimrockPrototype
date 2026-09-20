# UI Architecture Current State

Statut : **CURRENT — UI-CHAR01**  
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

`WBP_GrimrockMenu` est un menu RPG multipage fonctionnel. Son parent natif est `UGrimrockMenuWidget`.

### UI-FOUNDATION01 — workspace Inventaire / Personnage

La refonte conserve le shell et les pages existants. `Page_Inventory / WBP_GridInventory` devient explicitement le workspace réunissant deux panneaux indépendants :

```text
Panel_CharacterSheet   gauche
zone centrale          vue 3D laissée visible
Panel_InventoryBag     droite
```

Les deux wrappers et leurs boutons de fermeture sont `BindWidgetOptional` afin de permettre une transition UMG sans casser l'asset actuel. `I` continue d'utiliser `ToggleInventoryWidget()`, mais la réouverture appelle maintenant `OpenInventoryWorkspace()`, revient sur la page Inventory et restaure les deux panneaux.

Référence : `docs/Design/UI_FOUNDATION01_UNIFIED_INVENTORY_CHARACTER_WORKSPACE.md`.

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

`WBP_GridInventory` peut exposer six instances canoniques `PartyMember_1..6`, enregistrées nativement sur les indices `0..5`. `WBP_PartyMember` reçoit en option `Image_Portrait` et `Border_Selected` pour projeter le portrait et l'état de sélection sans logique Blueprint.

Référence : `docs/Design/UI_CHAR01_SINGLE_SELECTED_CHARACTER.md`.

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
    navigation / shell / ouverture du workspace Inventory

UGridInventoryWidget
    workspace feuille personnage + inventaire
    visibilité indépendante des deux panneaux
    présentation inventaire / équipement existante

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
