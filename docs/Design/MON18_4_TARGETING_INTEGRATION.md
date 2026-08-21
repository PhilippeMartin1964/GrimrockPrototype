# MON18.4 — Targeting Integration

Statut : **VALIDÉ ET CLOS sous UE5.5.4**  
Date : **21 août 2026**

## Objectif

Valider et résoudre la cible d'un sort avant toute mutation de PA/mana, puis chaîner cette validation vers la transaction MON18.3.

## Contrat de ciblage

`FGridSpellTargetingService` réutilise `EGridCombatTargetingPolicy` et traite :

- `Self` : le lanceur devient la cible résolue ;
- `Ally` : exige une identité résolue déclarée alliée ;
- `FirstAxialTarget` : exige une identité hostile, une cellule résolue et un alignement axial ;
- `Cell` / `Area` : utilisent directement la cellule portée par `FGridSpellCastRequest`.

Tous les modes appliquent `MinRangeCells..MaxRangeCells`. Si `bRequiresLineOfSight` est vrai, le contexte runtime doit fournir un résultat de LOS positif issu de la couche grille autoritaire.

## Identités et contexte runtime

La requête conserve uniquement les identités stables et coordonnées prévues par MON18.1. `FGridSpellTargetingContext` fournit le résultat de résolution effectué par la couche runtime : cellule du lanceur, identité/cellule cible résolue, relation Ally/Hostile et état LOS.

Aucun pointeur d'Actor n'est stocké dans le contrat de sort.

## Pipeline intégré

`FGridSpellCastPipelineService::TryValidateTargetAndCommitCosts` impose l'ordre :

```text
Spell request
    -> Targeting validation / resolution
    -> si échec : arrêt, zéro mutation
    -> MON18.3 cost transaction
    -> si échec : arrêt, zéro mutation
    -> receipt + resolved target
```

MON18.4 n'applique toujours aucun effet de sort. Cette responsabilité commence avec MON18.5.

## Validation UE5.5.4

Campagne reçue le 21 août 2026 :

```text
Grimrock.Magic.MON18.4.AllyRelation                   Success
Grimrock.Magic.MON18.4.AxialTargetSuccess             Success
Grimrock.Magic.MON18.4.CellAreaResolution             Success
Grimrock.Magic.MON18.4.LineOfSightNoMutation          Success
Grimrock.Magic.MON18.4.NonAxialNoMutation             Success
Grimrock.Magic.MON18.4.OutOfRangeNoMutation           Success
Grimrock.Magic.MON18.4.SelfResolution                 Success
Grimrock.Magic.MON18.4.TransactionFailureAfterTarget  Success
Total                                                   8/8 Success
```

## Rejets de ciblage

```text
InvalidSpellDefinition
InvalidRequest
MissingTarget
TargetIdentityMismatch
InvalidTargetRelation
TargetOutOfRange
TargetNotAxial
LineOfSightBlocked
```

Le pipeline expose également le stage de rejet (`Targeting` ou `Transaction`) pour éviter de mélanger les responsabilités.

## Fichiers

```text
Source/GrimrockPrototype/Public/Magic/GridSpellTargeting.h
Source/GrimrockPrototype/Private/Magic/GridSpellTargeting.cpp
Source/GrimrockPrototype/Private/Tests/GridMagicMON184TargetingTests.cpp
docs/Design/MON18_4_TARGETING_INTEGRATION.md
```

## Hors périmètre

- application Damage/Heal/StatusEffect : MON18.5 ;
- VFX/audio/projectiles : MON18.6 ;
- UI de sélection/hotbar : MON18.7 ;
- persistance Spellbook : MON18.8.

Aucun `.uasset` n'est requis pour MON18.4.
