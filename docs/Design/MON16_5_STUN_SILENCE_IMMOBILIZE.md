# MON16.5 — Stun / Silence / Immobilize

## Statut

**VALIDÉ ET CLOS — 17 août 2026.**

Base MON16.5 :

```text
65f3c3bae7e52d05a6708be1591351166d823964
Close MON16.4 status initiative modifiers
```

Implémentation MON16.5 validée :

```text
f9429734f249d995dbb173b206d161dc00a3c615
Add MON16.5 status control
```

MON16.5 ajoute les premières restrictions d'action aux effets de statut en réutilisant le lifecycle MON16.2, le catalogue d'actions MON12 et le TurnManager existant. Aucun `EffectId` n'est interprété comme Stun, Silence ou Immobilize par le code de production.

## 1. Modèle data-driven

`UGridStatusEffectDefinitionAsset` porte un profil générique :

```text
FGridStatusEffectControlProfile Control
```

avec trois capacités booléennes :

```text
bSkipActivation
bBlockSpellActions
bBlockTranslation
```

Configurations de référence :

```text
Stun        -> bSkipActivation=true
Silence     -> bBlockSpellActions=true
Immobilize  -> bBlockTranslation=true
```

Plusieurs effets actifs sont agrégés par OR logique via `FGridStatusEffectControlResolver`. Les stacks conservent leur `StackCount`, mais une restriction booléenne n'est pas multipliée par le nombre de stacks.

## 2. Stun — consommation d'activation

`bSkipActivation` est évalué à la frontière d'activation globale.

```text
Waiting
 -> contrôle des statuts
 -> SkipActivation
 -> aucune activation Player/Monster démarrée
 -> party : 0 PA utilisable
 -> initiative Completed
 -> OnCombatantStateChanged
 -> lifecycle MON16.2
 -> décrément Turns éventuel
 -> combattant suivant
```

`Incapacitated` n'est pas détourné pour représenter Stun. L'activation est consommée comme `Completed`, ce qui permet aux durées `Turns` de décrémenter exactement une fois.

Un Stun `Turns=1` retire donc une activation puis expire. Un Stun en `Rounds` reste gouverné par MON16.2. Un `bSkipActivation` permanent est rejeté par validation afin d'éviter un combat impossible à faire progresser.

MON16.5 ne force pas rétroactivement la fin d'un combattant déjà `Active` : la restriction est évaluée à la prochaine frontière d'activation autoritative.

## 3. Silence — blocage des sorts

Le Silence utilise :

```text
bBlockSpellActions=true
```

Le blocage repose sur la taxonomie existante :

```text
EGridCombatActionSourcePolicy::Spell
```

Le sort reste dans le catalogue commun mais devient indisponible :

```text
bEnabled=false
AvailabilityReason=MissingRequirement
DisabledReason="Un effet de statut empêche l'utilisation des sorts."
```

La requête autoritative retourne `ActionUnavailable` avant toute dépense de PA, mana, item ou application d'effet.

Ne sont pas bloqués par Silence :

```text
Ability
Equipment
QuickItem
Universal
```

Les politiques de ciblage réutilisent le même catalogue. MON16.5 n'introduit pas de seconde taxonomie magique pour les monstres.

## 4. Immobilize — translation interdite

L'Immobilize utilise :

```text
bBlockTranslation=true
```

### Party

`RequestPartyTranslation()` bloque la translation avant toute dépense de PA/PAM. Aucun mouvement n'est lancé. La rotation à 90 degrés reste autorisée par `RequestPartyRotation()`.

MON16.5 conserve provisoirement `PartyBusy` comme raison publique générique de rejet ; la présentation dédiée relève de MON16.6.

### Monstres

`StartActiveAction()` refuse uniquement les actions :

```text
EGridCombatActionType::Move
```

Restent possibles :

```text
Turn
MeleeAttack
Wait
```

Le planner existant est conservé.

## 5. Coexistence avec MON16.2 / MON16.3 / MON16.4

MON16.5 n'ajoute aucune horloge ni système parallèle :

- MON16.2 reste l'autorité des durées `Turns/Rounds/Permanent` ;
- MON16.3 reste l'autorité des dégâts périodiques ;
- MON16.4 reste l'autorité des modificateurs d'initiative ;
- MON16.5 ajoute seulement les restrictions de contrôle.

Un même effet peut donc combiner dégâts périodiques, initiative et restrictions de contrôle.

## 6. Hors périmètre

MON16.5 n'ajoute pas :

- d'icône de statut ;
- de widget/HUD/WBP ;
- de feedback visuel final ;
- de résistance/immunité au contrôle ;
- de jet de sauvegarde ;
- de dispel/cleanse ;
- d'application automatique par attaque ou sort ;
- de persistance des statuts ;
- de `.uasset` ou `.umap`.

Le feedback détaillé appartient à MON16.6 et la sauvegarde/restauration à MON16.7.

## 7. Fichiers

Ajoutés :

```text
Source/GrimrockPrototype/Public/RPG/StatusEffects/GridStatusEffectControlResolver.h
Source/GrimrockPrototype/Private/RPG/StatusEffects/GridStatusEffectControlResolver.cpp
Source/GrimrockPrototype/Private/Tests/RPGMON165StatusEffectControlTests.cpp
docs/Design/MON16_5_STUN_SILENCE_IMMOBILIZE.md
docs/Design/MON16_5_VALIDATION_CHECKLIST.md
```

Modifiés :

```text
Source/GrimrockPrototype/Public/RPG/StatusEffects/GridStatusEffectDefinitionAsset.h
Source/GrimrockPrototype/Private/RPG/StatusEffects/GridStatusEffectDefinitionAsset.cpp
Source/GrimrockPrototype/Private/Runtime/Combat/GridTurnManagerInitiative.cpp
Source/GrimrockPrototype/Private/Runtime/Combat/GridTurnManagerPartyMovement.cpp
Source/GrimrockPrototype/Private/Runtime/Combat/GridTurnManagerActions.cpp
Source/GrimrockPrototype/Private/Runtime/Combat/GridTurnManagerPlayerActionCatalog.cpp
```

Aucun `.uasset`, `.umap`, WBP, Build.cs ou SaveGame.

## 8. Validation

Namespace :

```text
Grimrock.RPG.MON16.5
```

Tests MON16.5 :

```text
ControlAggregation
PermanentSkipActivationRejected
StackBooleanSemantics
TurnSkipLifecycle
RoundSkipLifecycle
SilenceCatalogIsolation
SilenceRequestAtomic
PartyImmobilizeTranslation
PartyImmobilizeRotation
TargetParity
NoParallelSystem
```

### Campagne globale initiale

La campagne complète exécutée par l'utilisateur a donné :

```text
145 tests exécutés
143 Success
2 Fail
```

Les deux seuls échecs étaient MON16.5 :

```text
PartyImmobilizeRotation
SilenceCatalogIsolation
```

L'analyse a montré un défaut de fixture : la grille logique du pawn était configurée sans synchroniser sa transform, ce qui provoquait `PartyBusy` avant l'évaluation réelle des règles MON16.5.

La fixture a été corrigée avec `Party->SnapToCurrentCell()` et les assertions Silence ont été renforcées afin de vérifier le rejet réel `MissingRequirement` / `ActionUnavailable`.

### Rerun ciblé final — 17 août 2026

```text
Automation RunTests Grimrock.RPG.MON16.5
11/11 Success
0 Fail
```

Points explicitement confirmés par le log final :

- `PartyImmobilizeRotation` : rotation acceptée ;
- `PartyImmobilizeTranslation` : translation bloquée avant dépense ;
- `SilenceCatalogIsolation` : Success ;
- `SilenceRequestAtomic` : rejet par `MissingRequirement`, donc par Silence et non par `PartyBusy` ;
- les 11 tests MON16.5 sont Success.

Comme les 134 autres tests de la campagne globale étaient déjà Success et que la correction finale ne touche que la fixture de test MON16.5, aucune nouvelle régression n'est introduite par cette correction.

## 9. Clôture

**MON16.5 est VALIDÉ ET CLOS au 17 août 2026.**

Prochaine étape : **MON16.6 — HUD / Combat Feedback des status effects**.
