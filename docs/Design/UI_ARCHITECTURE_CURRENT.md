# UI Architecture Current State

Statut : **CURRENT — après MON20 / avant implémentation MON21**  
Date : **25 août 2026**

## Référence canonique

Le détail technique autoritaire du menu joueur reste :

```text
docs/Design/UI_GRIMROCK_MENU_CURRENT.md
```

Pour la dette technique transversale, la référence est :

```text
docs/Architecture/TECHNICAL_DEBT_REGISTER.md
```

## Menu joueur actuel

`WBP_GrimrockMenu` est un menu RPG multipage existant et fonctionnel. Son parent natif est `UGrimrockMenuWidget`.

Pages intégrées :

```text
Inventaire      fonctionnel
Compétences     fonctionnel MON20
Sorts           fonctionnel MON18
Journal         shell présent, domaine métier non démarré
Carte           shell présent, domaine métier non démarré
Recettes        shell présent, fonctionnalité future
Codex           shell présent, domaine métier non démarré
```

La navigation des onglets est portée par le C++. Le Graph de `WBP_GrimrockMenu` ne doit pas recréer une navigation parallèle.

## Spellbook

`WBP_GridSpellbook` est intégré dans `Page_Spellbook` et utilise `UGridSpellbookWidget` comme parent natif.

Le modèle de connaissance autoritaire reste :

```text
UGridPartySpellbookComponent
    -> CharacterId
    -> KnownSpellIds[]
```

La vue est construite par `UGridSpellbookUILibrary` et ne possède aucune copie gameplay autoritaire.

La persistance `KnownSpellIds` est **livrée depuis MON18.8** via `CharacterSpellbookStates` dans le SaveGame courant. L'ancienne mention « persistance à faire MON18.8 » est obsolète.

## Skills / Talents

MON20 a livré :

```text
SelectedCharacterIndex
    -> FGridSkillsPageService
    -> UGridSkillsWidget
    -> WBP_GridSkills
```

La page Compétences projette Skills + Talents du personnage sélectionné. Les rangs de Skills sont persistés en SaveGame v8 ; les Talents réutilisent les `ProgressionChoices` MON15.

## Hotbar

Le Spellbook réutilise la hotbar MON12 à dix slots. Aucun second stockage de raccourcis n'existe.

Identité d'un binding Spell :

```text
ActionId           = SpellId
SourcePolicy       = Spell
SourceDefinitionId = SpellId
```

Le drag/drop, le move/swap et la désaffectation réutilisent les API MON12 existantes.

## Exécution des sorts

Le parcours complet reste :

```text
Spellbook
    -> hotbar
    -> catalogue
    -> ciblage MON18.4
    -> transaction PA/mana MON18.3
    -> effets MON18.5
    -> commit runtime
    -> présentation MON18.6
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
    présentation de la page Sorts

UGridPartySpellbookComponent
    connaissance runtime des sorts

UGridPartyInventoryComponent
    autorité groupe/inventaire + sélection + hotbar

UGridTurnManagerComponent
    autorité combat

Services MON18 / MON20
    résolution métier et read-models
```

## Dette technique UI active

Ne pas mélanger dette technique et fonctionnalités futures.

Dette réelle suivie dans `TECHNICAL_DEBT_REGISTER.md` :

- `TD-PARTY-001` : synchronisation held visual encore dépendante des appelants de `SetSelectedCharacterIndex` ;
- `TD-UI-001` : nommage historique `EInventoryTopTab` / `ToggleInventoryWidget()` ; faible priorité ;
- `TD-LOG-001` : plusieurs zones UI utilisent encore `LogTemp` ;
- risque transversal de divergence visuelle entre nombreuses surfaces UMG, à traiter par conventions/composants partagés sans déplacer la logique métier en Blueprint.

## Travail futur qui n'est pas de la dette technique

- sélection explicite d'un autre allié pour les sorts `Ally` depuis la hotbar : amélioration fonctionnelle/UX ;
- icônes finales : contenu de production ;
- Journal / Map / Codex : MON21, actuellement suspendu après MON21.1 ;
- Recipes : fonctionnalité future.

## Phase actuelle

```text
MON20      CLOS
MON21.1    Audit & Architecture Contract terminé
MON21.2    SUSPENDU
Phase      Exploitation / playtest / stabilisation / dette technique ciblée
```
