# MON16.4 — Validation Checklist

## État

```text
Implémentation C++ : VALIDÉE
Documentation      : VALIDÉE
Compilation UE5    : VALIDÉE après correctif include UE5.5.4
Automation MON16.4 : 11/11 SUCCESS
Régressions        : VALIDÉES
Campagne complète  : 134/134 SUCCESS
Clôture            : OUI — 17 août 2026
```

Base : `e8ffd4308fe5dc20f2393dc9e1e027b78aaa8eda`.

Implémentation : `b926dc584a1f38ca0aed3d4a53cd8c2b79ca23e5`.

Correctif compilation UE5.5.4 : `53d86f47cbbb0440f4807e434fe9aaf593a70112`.

## Architecture

- [x] réutilisation exclusive de `FGridCombatantInitiativeEntry::InitiativeModifier`
- [x] réutilisation de `SetCombatantInitiativeModifier()`
- [x] réutilisation du tri MON12
- [x] aucun second ordre d'initiative
- [x] aucun second jet d'initiative
- [x] aucun nouveau système de stats
- [x] personnages et monstres partagent le même resolver
- [x] aucune dépendance UI

## Calcul

- [x] contribution positive supportée
- [x] contribution négative supportée
- [x] plusieurs effets s'additionnent algébriquement
- [x] contribution multipliée par `StackCount`
- [x] saturation `int32`
- [x] `Potency` n'intervient pas dans le calcul
- [x] `InitiativeModifier == 0` reste neutre
- [x] aucun hard-code d'EffectId Haste/Slow dans le moteur

## Ordre runtime

- [x] statut préexistant projeté lors de `OnTurnOrderChanged`
- [x] application pendant combat synchronisée immédiatement
- [x] réapplication/stack synchronisée immédiatement
- [x] actif jamais déplacé rétroactivement
- [x] tours déjà consommés jamais déplacés dans le round courant
- [x] seuls les `Waiting` futurs sont retriés par le TurnManager existant
- [x] `InitiativeRoll` inchangé
- [x] `InitiativeTotal` inchangé
- [x] round suivant naturellement retrié avec le modificateur actif

## Lifecycle

- [x] expiration `Turns` retire la contribution
- [x] expiration `Rounds` retire la contribution
- [x] recalcul après `AdvanceDuration()`
- [x] coexistence avec les DoT MON16.3
- [x] garde de réentrance sur `OnTurnOrderChanged`

## Hors périmètre respecté

- [x] aucun changement PA/PAM
- [x] aucune vitesse d'animation/déplacement modifiée
- [x] aucune action/spell d'application ajoutée
- [x] aucun Stun/Silence/Immobilize effectif
- [x] aucun HUD/icône/WBP
- [x] aucun `.uasset`/`.umap`
- [x] aucune persistance ajoutée

## Compilation UE5.5.4

Le premier build a échoué uniquement sur :

```text
fatal error C1083: 'Misc/GuardValue.h': No such file or directory
```

Correction appliquée :

```text
#include "Templates/UnrealTemplate.h"
```

- [x] correctif poussé sur `master`
- [x] le nouveau code MON16.4 est compilé/chargé, confirmé par l'exécution des tests MON16.4

## Automation ciblée

Namespace :

```text
Grimrock.RPG.MON16.4
```

- [x] `AggregateModifier` — Success
- [x] `StackScalingAndSaturation` — Success
- [x] `TurnOrderBroadcastProjection` — Success
- [x] `FutureHasteReorder` — Success
- [x] `FutureSlowReorder` — Success
- [x] `ActiveCombatantStability` — Success
- [x] `TurnExpiration` — Success
- [x] `RoundExpiration` — Success
- [x] `MonsterParity` — Success
- [x] `ReapplicationUpdatesModifier` — Success
- [x] `NoParallelSystem` — Success

Bilan : **11/11 Success**.

## Vérifications runtime observées

- [x] Haste futur : `0 -> +12`, `EffectiveTotal=22`
- [x] Slow futur : `0 -> -15`, `EffectiveTotal=5`
- [x] actif stable avec `+100`
- [x] réapplication : `+4 -> +8`
- [x] expiration round : `+12 -> 0`
- [x] expiration turn : `+9 -> 0`
- [x] parité monstre : `0 -> -10`, `EffectiveTotal=5`

## Régressions minimales

- [x] MON16.3 : 11/11 Success
- [x] MON16.2 : 10/10 Success
- [x] MON16.1 : 7/7 Success
- [x] MON15 : 42/42 Success
- [x] MON14 : 19/19 Success

## Campagne complète fournie

Analyse du log utilisateur :

```text
Tests terminés : 134
Success         : 134
Fail            : 0
Error Automation: 0
```

- [x] aucune régression Automation détectée dans la campagne complète
- [x] `Grimrock.Monsters.MON13.5.RealPIEIntegration` est également `Success`

## Clôture

**MON16.4 — VALIDÉ ET CLOS le 17 août 2026.**

Contrat gelé : Haste/Slow se projettent exclusivement dans `FGridCombatantInitiativeEntry::InitiativeModifier`, sans reroll, sans ordre parallèle et sans déplacement rétroactif de l'actif ou des tours déjà consommés.

Prochaine étape : `MON16.5 — Stun / Silence / Immobilize`.
