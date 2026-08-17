# MON16.5 — Stun / Silence / Immobilize

## Statut

**Implémenté — validation UE5 en attente.**

Base :

```text
65f3c3bae7e52d05a6708be1591351166d823964
Close MON16.4 status initiative modifiers
```

MON16.5 ajoute les premières restrictions d'action aux effets de statut en réutilisant le lifecycle MON16.2, le catalogue d'actions MON12 et le TurnManager existant. Aucun `EffectId` n'est interprété comme Stun, Silence ou Immobilize par le code de production.

## 1. Modèle data-driven

`UGridStatusEffectDefinitionAsset` reçoit un profil générique :

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

Un même effet peut combiner plusieurs capacités. Plusieurs effets actifs sont agrégés par OR logique via :

```text
FGridStatusEffectControlResolver
```

Les stacks conservent leur `StackCount` dans le modèle commun, mais une restriction booléenne n'est pas multipliée par le nombre de stacks.

## 2. Stun — consommation d'activation

Le Stun est projeté par `bSkipActivation` au moment où le TurnManager s'apprête à démarrer une activation globale.

La sémantique retenue est :

```text
combatant Waiting
    -> contrôle des statuts
    -> bSkipActivation=true
    -> aucune activation Player/Monster démarrée
    -> 0 PA utilisable pour la party
    -> état initiative Completed
    -> OnCombatantStateChanged
    -> lifecycle MON16.2
    -> décrément Turns éventuel
    -> combattant suivant
```

`Incapacitated` n'est pas utilisé comme état persistant de Stun. Il conserve son rôle historique d'échec/incapacité du TurnManager. Le Stun consomme au contraire une activation réelle comme `Completed`, ce qui permet aux durées `Turns` existantes de décrémenter exactement une fois.

Un Stun `Turns=1` retire donc une activation puis expire. Un Stun en `Rounds` reste actif jusqu'à la frontière de round gérée par MON16.2.

Un `bSkipActivation` permanent est rejeté par la validation de définition : une incapacité permanente pourrait empêcher toute progression d'un combat. Les restrictions Silence/Immobilize peuvent, elles, être permanentes.

MON16.5 ne force pas rétroactivement la fin d'un combattant déjà `Active`. La restriction est évaluée à la prochaine frontière d'activation autoritative.

## 3. Silence — blocage des sorts

Le Silence est défini par :

```text
bBlockSpellActions=true
```

Le blocage repose exclusivement sur la taxonomie existante :

```text
EGridCombatActionSourcePolicy::Spell
```

Après construction normale du catalogue, les actions Spell qui seraient disponibles deviennent indisponibles. Elles restent présentes dans le catalogue commun, mais :

```text
bEnabled=false
AvailabilityReason=MissingRequirement
DisabledReason="Un effet de statut empêche l'utilisation des sorts."
```

Le même catalogue est utilisé par l'UI et par les requêtes autoritatives. Une tentative d'exécution retourne donc `ActionUnavailable` avant toute dépense de PA, mana, item ou application d'effet.

Ne sont pas bloqués par Silence :

```text
Ability
Equipment
QuickItem
Universal
```

Les sorts `Self`, `FirstAxialTarget`, `Cell` et `Area` passent tous par ce même catalogue ; le blocage n'est donc pas limité aux actions non ciblées.

MON16.5 n'invente pas de nouvelle catégorie de « sort monstre ». Les monstres peuvent porter le même profil de statut, mais `bBlockSpellActions` n'aura un consommateur monstre que lorsque leurs actions magiques utiliseront la taxonomie commune.

## 4. Immobilize — translation interdite

L'Immobilize utilise :

```text
bBlockTranslation=true
```

### Party

`RequestPartyTranslation()` vérifie le statut du personnage actif avant toute dépense de PA/PAM ou validation spatiale. En cas de blocage :

- translation refusée ;
- aucun PA personnel dépensé ;
- aucun PAM partagé dépensé ;
- aucun mouvement lancé.

La rotation à 90 degrés reste autorisée : `RequestPartyRotation()` n'est pas bloquée.

MON16.5 conserve provisoirement le contrat public de rejet existant et utilise `PartyBusy` comme raison générique. Une raison/presentation dédiée relève de MON16.6.

### Monstres

`StartActiveAction()` refuse uniquement :

```text
EGridCombatActionType::Move
```

si `bBlockTranslation` est actif.

Restent possibles :

```text
Turn
MeleeAttack
Wait
```

Le planner existant n'est pas remplacé. Si un déplacement déjà planifié est refusé, le pipeline d'échec d'action du TurnManager termine ou poursuit le tour selon ses règles existantes.

## 5. Coexistence avec MON16.2 / MON16.3 / MON16.4

MON16.5 n'ajoute aucune horloge.

- MON16.2 reste l'autorité des durées Turns/Rounds/Permanent ;
- MON16.3 reste l'autorité des DoT ;
- MON16.4 reste l'autorité Haste/Slow via `InitiativeModifier` ;
- MON16.5 ajoute seulement les restrictions de contrôle.

Un même effet peut donc, si le design le souhaite, combiner dégâts périodiques, initiative et restrictions de contrôle sans créer de système parallèle.

## 6. Hors périmètre

MON16.5 n'ajoute pas :

- d'icône de statut ;
- de widget/HUD/WBP ;
- de nouveau message de feedback visuel final ;
- de résistance/immunité au contrôle ;
- de jet de sauvegarde ;
- de dispel/cleanse ;
- d'application automatique de Stun/Silence/Immobilize par une attaque ou un sort ;
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

## 8. Automation

Namespace :

```text
Grimrock.RPG.MON16.5
```

Tests :

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

Attendu : **11/11 Success**.

Régressions après succès ciblé :

```text
Automation RunTests Grimrock.RPG.MON16.4
Automation RunTests Grimrock.RPG.MON16.3
Automation RunTests Grimrock.RPG.MON16.2
Automation RunTests Grimrock.RPG.MON16.1
Automation RunTests Grimrock.RPG.MON15
Automation RunTests Grimrock.Monsters.MON14
```

MON16.5 sera déclaré **VALIDÉ ET CLOS** uniquement après compilation/chargement UE5.5.4 et succès des tests sur logs utilisateur.

Prochaine étape après clôture : **MON16.6 — HUD / Combat Feedback des status effects**.
