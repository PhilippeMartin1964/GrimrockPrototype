# UI Architecture Current State

Statut : **CURRENT — après TD06.9 / MON21.3**  
Date : **26 août 2026**

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
    navigation / shell

UGridInventoryWidget
    présentation inventaire

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
