# UI Architecture Current State

Statut : **UI01.4.3e VALIDÉ ET CLOS sous UE5.5.4**  
Date : **21 août 2026**

## Référence canonique

Le détail technique autoritaire du menu joueur est désormais :

```text
docs/Design/UI_GRIMROCK_MENU_CURRENT.md
```

Ce document résume uniquement l'état architectural global afin d'éviter de conserver des descriptions devenues obsolètes.

## Menu joueur actuel

`WBP_GrimrockMenu` est un menu RPG multipage existant et fonctionnel. Son parent natif est `UGrimrockMenuWidget`.

Pages intégrées :

```text
Inventaire
Compétences
Sorts
Journal
Carte
Recettes
Codex
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

## Hotbar

Le Spellbook réutilise la hotbar MON12 à dix slots. Aucun second stockage de raccourcis n'existe.

Identité d'un binding Spell :

```text
ActionId           = SpellId
SourcePolicy       = Spell
SourceDefinitionId = SpellId
```

Le drag/drop, le move/swap et la désaffectation réutilisent les API MON12 existantes.

## Exécution

UI01.4.3e a fermé le parcours complet :

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

Validation UE5.5.4 :

- 6/6 tests Automation UI01.4.3e.2 ;
- `Lesser Heal` validé en PIE ;
- `Arcane Bolt` validé en PIE ;
- mort d'un monstre par sort correctement propagée vers les systèmes existants ;
- refus correct pour mana insuffisant.

## Responsabilités

```text
UGrimrockMenuWidget
    navigation / shell

UGridSpellbookWidget
    présentation de la page Sorts

UGridPartySpellbookComponent
    connaissance runtime des sorts

UGridPartyInventoryComponent
    sélection personnage + hotbar

UGridTurnManagerComponent
    autorité combat

Services MON18
    ciblage / coûts / effets / présentation
```

## Dette / travail futur

Les éléments suivants ne remettent pas en cause la clôture UI01.4.3e :

- persistance `KnownSpellIds` : MON18.8 ;
- sélection explicite d'un autre allié depuis la hotbar ;
- icônes finales de sorts ;
- modules complets Skills, Journal, Map, Recipes et Codex selon leurs jalons futurs ;
- éventuel renommage des API historiques encore centrées sur le mot `Inventory`.

## Prochain travail autoritaire

```text
MON18.8 — Persistence / Migration du Spellbook
```
