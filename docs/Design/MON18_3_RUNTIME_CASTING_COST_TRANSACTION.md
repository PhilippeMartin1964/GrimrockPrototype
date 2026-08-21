# MON18.3 — Runtime Casting / Cost Transaction

Statut : **VALIDÉ ET CLOS sous UE5.5.4**  
Date : **21 août 2026**

## Objectif

Introduire la frontière transactionnelle de coût d'un cast runtime sans dupliquer les systèmes de ressources existants et sans anticiper le ciblage MON18.4 ni la résolution d'effets MON18.5.

## Contrat

`FGridSpellCastTransactionService` reçoit :

```text
FGridSpellDefinition
FGridSpellCastRequest
FGridCharacterSpellbookState
FRPGDerivedStats
FGridPlayerCharacterTurnState
```

Il réutilise directement :

- `FRPGDerivedStats::CurrentMana` comme mana autoritaire ;
- `FGridPlayerCharacterTurnState::RemainingActionPoints` comme PA autoritaires ;
- `FGridCharacterSpellbookState` comme connaissance autoritaire du sort ;
- `SpellId` et `CharacterId` comme identités stables.

## Validation avant paiement

La transaction refuse le paiement si :

- la définition du sort est structurellement invalide ;
- la requête de cast ne porte pas un `CasterCharacterId` / `SpellId` valide ;
- le Spellbook est invalide ;
- les identités personnage ne correspondent pas ;
- le personnage ne connaît pas le sort ;
- son tour n'est pas `Active` ;
- ses PA sont insuffisants ;
- son mana est insuffisant.

## Atomicité

Aucune ressource n'est modifiée avant que toutes les validations ci-dessus aient réussi.

En cas d'échec :

```text
CurrentMana               inchangé
RemainingActionPoints     inchangé
Receipt                    vide
```

En cas de succès, les deux coûts sont débités puis un `FGridSpellCastCostReceipt` est produit avec :

```text
CharacterId
SpellId
ActionPointsSpent
ManaSpent
```

## Séparation des responsabilités

MON18.3 ne valide pas encore le payload de cible et n'applique aucun effet. C'est volontaire :

```text
MON18.3  connaissance + identité + ressources + paiement atomique
MON18.4  validation/résolution du ciblage avant engagement de la transaction
MON18.5  résolution des premiers sorts de production
```

Le pipeline intégré de MON18.4 doit toujours valider la cible avant d'appeler `TryCommitCosts`.

## Fichiers

```text
Source/GrimrockPrototype/Public/Magic/GridSpellCastTransaction.h
Source/GrimrockPrototype/Private/Magic/GridSpellCastTransaction.cpp
Source/GrimrockPrototype/Private/Tests/GridMagicMON183CastTransactionTests.cpp
docs/Design/MON18_3_RUNTIME_CASTING_COST_TRANSACTION.md
docs/Design/MON18_3_VALIDATION.md
```

## Validation Automation UE5.5.4

```text
Grimrock.Magic.MON18.3.IdentityMismatchNoMutation          Success
Grimrock.Magic.MON18.3.InsufficientActionPointsNoMutation  Success
Grimrock.Magic.MON18.3.InsufficientManaNoMutation          Success
Grimrock.Magic.MON18.3.SuccessfulCommit                    Success
Grimrock.Magic.MON18.3.TargetingDeferred                   Success
Grimrock.Magic.MON18.3.UnknownSpellNoMutation              Success
Total                                                       6/6 Success
```

MON18.3 est **VALIDÉ ET CLOS**. Aucun Blueprint, DataAsset, WBP ou `.uasset` n'a été requis pour cette étape.
