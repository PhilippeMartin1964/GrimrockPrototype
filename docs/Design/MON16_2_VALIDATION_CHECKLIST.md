# MON16.2 — Validation Checklist

## État

```text
Implémentation C++ : préparée
Documentation      : préparée
Compilation UE5    : EN ATTENTE
Automation MON16.2 : EN ATTENTE
Régression MON16.1 : EN ATTENTE
Clôture            : NON
```

Base : `83f2630c213ad8e1c0583c326085be4df73de71a`.

## Architecture

- [x] aucune horloge en secondes
- [x] aucun tick de durée
- [x] delegates du TurnManager comme autorité
- [x] même collection personnages/monstres
- [x] `TryAdd()` strict conservé
- [x] `TryApply()` pour MON16.2
- [x] `Permanent` immuable dans le lifecycle
- [x] `Potency` limitée à la précédence de stacking
- [x] aucune dépendance UI
- [x] aucune persistance

## Sémantique

- [x] `Completed` consomme une unité `Turns`
- [x] `Incapacitated` consomme une unité `Turns`
- [x] `Round 1` initialise la baseline
- [x] `Round N -> N+1` consomme une unité `Rounds`
- [x] expiration à zéro
- [x] ordre déterministe

## Stacking

- [x] `NoStack` atomique
- [x] `RefreshDuration`
- [x] `AddStacks` borné à `MaxStacks`
- [x] `AddStacks` rafraîchit la durée
- [x] `ReplaceIfStronger` exige une potency strictement supérieure
- [x] potency égale rejetée
- [x] `DurationUnit` incohérent rejeté

## Hors périmètre

- [x] aucun DoT
- [x] aucun Haste/Slow effectif
- [x] aucun `InitiativeModifier` appliqué
- [x] aucun Stun/Silence/Immobilize effectif
- [x] aucun HUD/icône/WBP
- [x] aucun `.uasset`/`.umap`
- [x] aucune sauvegarde/restauration

## Compilation UE5.5.4

Attendu : 0 erreur C++, UHT ou link.

- [ ] compilation confirmée par log utilisateur

## Automation ciblée

Exécuter :

```text
Automation RunTests Grimrock.RPG.MON16.2
```

- [ ] TurnDurationLifecycle — Success
- [ ] RoundDurationLifecycle — Success
- [ ] PermanentDurationLifecycle — Success
- [ ] NoStackAndRefresh — Success
- [ ] AddStacks — Success
- [ ] ReplaceIfStronger — Success
- [ ] AtomicFailure — Success
- [ ] DeterministicExpiration — Success
- [ ] TurnManagerEventIntegration — Success
- [ ] NoUIDependency — Success

Attendu : **10/10 Success**.

## Régression minimale

```text
Automation RunTests Grimrock.RPG.MON16.1
Automation RunTests Grimrock.RPG.MON15
Automation RunTests Grimrock.Monsters.MON14
```

- [ ] MON16.1 : 7/7 Success
- [ ] MON15 : 42/42 Success
- [ ] MON14 : 19/19 Success

## Clôture

MON16.2 pourra être marqué **VALIDÉ ET CLOS** après compilation, 10/10 MON16.2, 7/7 MON16.1, absence de régression pertinente et analyse des logs utilisateur.

Prochaine étape : `MON16.3 — DoT Poison / Bleeding / Burning`.
