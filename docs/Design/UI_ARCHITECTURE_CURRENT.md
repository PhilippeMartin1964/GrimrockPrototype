# UI Architecture Current State

Statut : **CURRENT — après TD04 / pendant audit TD05**  
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

## Menu joueur actuel

`WBP_GrimrockMenu` est un menu RPG multipage fonctionnel. Son parent natif est `UGrimrockMenuWidget`.

```text
Inventaire      fonctionnel
Compétences     fonctionnel MON20
Sorts           fonctionnel MON18
Journal         shell présent, métier futur MON21
Carte           shell présent, métier futur MON21
Recettes        shell présent, fonctionnalité future
Codex           shell présent, métier futur MON21
```

La navigation des onglets est portée par le C++. Le Graph de `WBP_GrimrockMenu` ne doit pas recréer une navigation parallèle.

## Spellbook

`WBP_GridSpellbook` utilise `UGridSpellbookWidget` comme parent natif. Le modèle de connaissance autoritaire reste :

```text
UGridPartySpellbookComponent
    -> CharacterId
    -> KnownSpellIds[]
```

La vue est construite depuis les autorités runtime et ne possède aucune copie gameplay autoritaire. La persistance est livrée depuis MON18.8 via `CharacterSpellbookStates`.

## Skills / Talents

MON20 a livré :

```text
SelectedCharacterIndex
    -> FGridSkillsPageService
    -> UGridSkillsWidget
    -> WBP_GridSkills
```

La page Compétences projette Skills + Talents du personnage sélectionné. Les rangs de Skills sont persistés dans le SaveGame courant **v9** ; les Talents réutilisent les `ProgressionChoices` MON15.

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

UGridTurnManagerComponent
    autorité combat
```

## Sélection / held visual

`TD-PARTY-001` est **RÉSOLU**. `SetSelectedCharacterIndex()` déclenche la notification autoritaire ; `AGrimrockPartyPawn` resynchronise son held visual. Les tests `SelectionChange` et `SelectedCharacterFilter` ont été validés sous UE5.5.4.

## Dette technique UI active

Ne pas mélanger dette technique et fonctionnalités futures.

Dette réelle suivie dans `TECHNICAL_DEBT_REGISTER.md` :

- `TD-UI-001` : nommage historique `EInventoryTopTab` / `ToggleInventoryWidget()` ; faible priorité ;
- `TD-LOG-001` : certaines zones utilisent encore `LogTemp` ;
- divergence visuelle potentielle entre nombreuses surfaces UMG : à réduire par conventions/composants partagés lorsqu’une douleur concrète apparaît, sans déplacer le métier en Blueprint.

`TD-PARTY-001` ne fait plus partie de la dette active.

## Travail futur qui n’est pas de la dette technique

- sélection explicite d’un autre allié pour les sorts `Ally` : amélioration fonctionnelle/UX ;
- icônes finales : contenu de production ;
- Journal / Map / Codex : MON21 ;
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
MON21.1    Audit terminé
TD01–TD04  stabilisation réalisée
TD05       audit/réduction ciblée de GridLevelRuntimeActor
MON21.2    peut reprendre après la tranche TD05 jugée utile
```
