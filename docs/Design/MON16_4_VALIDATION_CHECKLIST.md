# MON16.4 — Validation Checklist

## État

```text
Implémentation C++ : préparée
Documentation      : préparée
Compilation UE5    : EN ATTENTE
Automation MON16.4 : EN ATTENTE
Régressions        : EN ATTENTE
Clôture            : NON
```

Base : `e8ffd4308fe5dc20f2393dc9e1e027b78aaa8eda`.

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

Attendu : 0 erreur C++, UHT ou link.

- [ ] compilation / chargement confirmé par log utilisateur

## Automation ciblée

Exécuter :

```text
Automation RunTests Grimrock.RPG.MON16.4
```

- [ ] `AggregateModifier` — Success
- [ ] `StackScalingAndSaturation` — Success
- [ ] `TurnOrderBroadcastProjection` — Success
- [ ] `FutureHasteReorder` — Success
- [ ] `FutureSlowReorder` — Success
- [ ] `ActiveCombatantStability` — Success
- [ ] `TurnExpiration` — Success
- [ ] `RoundExpiration` — Success
- [ ] `MonsterParity` — Success
- [ ] `ReapplicationUpdatesModifier` — Success
- [ ] `NoParallelSystem` — Success

Attendu : **11/11 Success**.

## Régressions minimales

Après MON16.4 vert :

```text
Automation RunTests Grimrock.RPG.MON16.3
Automation RunTests Grimrock.RPG.MON16.2
Automation RunTests Grimrock.RPG.MON16.1
Automation RunTests Grimrock.RPG.MON15
Automation RunTests Grimrock.Monsters.MON14
```

- [ ] MON16.3 : 11/11 Success
- [ ] MON16.2 : 10/10 Success
- [ ] MON16.1 : 7/7 Success
- [ ] MON15 : 42/42 Success
- [ ] MON14 : 19/19 Success

## Clôture

MON16.4 pourra être marqué **VALIDÉ ET CLOS** après chargement/compilation UE5.5.4, 11/11 MON16.4 et régressions appropriées sans échec sur les logs utilisateur.

Prochaine étape : `MON16.5 — Stun / Silence / Immobilize`.
