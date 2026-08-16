# MON16.2 — Validation Checklist

## État

```text
Implémentation C++ : VALIDÉE
Documentation      : VALIDÉE
UE5.5.4             : nouveaux tests C++ chargés et exécutés
Automation MON16.2 : 10/10 SUCCESS
Régression MON16.1 :  7/7 SUCCESS
Régression MON15   : 42/42 SUCCESS
Régression MON14   : 19/19 SUCCESS
Campagne fournie   : 78/78 SUCCESS
Clôture            : OUI
```

Base : `83f2630c213ad8e1c0583c326085be4df73de71a`.

Commit d'implémentation : `e038df582acc25bd990d924eec689d7a2b09d231`.

MON16.2 est **VALIDÉ ET CLOS** depuis le 16 août 2026.

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

## Hors périmètre respecté

- [x] aucun DoT
- [x] aucun Haste/Slow effectif
- [x] aucun `InitiativeModifier` appliqué
- [x] aucun Stun/Silence/Immobilize effectif
- [x] aucun HUD/icône/WBP
- [x] aucun `.uasset`/`.umap`
- [x] aucune sauvegarde/restauration

## Automation MON16.2

Résultats fournis le 16 août 2026 :

- [x] `TurnDurationLifecycle` — Success
- [x] `RoundDurationLifecycle` — Success
- [x] `PermanentDurationLifecycle` — Success
- [x] `NoStackAndRefresh` — Success
- [x] `AddStacks` — Success
- [x] `ReplaceIfStronger` — Success
- [x] `AtomicFailure` — Success
- [x] `DeterministicExpiration` — Success
- [x] `TurnManagerEventIntegration` — Success
- [x] `NoUIDependency` — Success

Bilan : **10/10 Success**.

Les nouveaux tests C++ sont découverts et exécutés par Unreal Engine 5.5.4, ce qui confirme que le code MON16.2 utilisé par la campagne est compilé et chargé par l'éditeur.

## Régressions

```text
MON16.1    7/7 Success
MON15     42/42 Success
MON14     19/19 Success
```

- [x] MON16.1 sans régression
- [x] MON15 sans régression
- [x] MON14 sans régression

Campagne fournie : **78/78 Success**.

- [x] aucun `Result={Fail}`
- [x] aucune erreur Automation

Les warnings `FlushRenderingCommands called recursively` observés pendant la campagne ciblée sont des warnings renderer sans échec de test ; aucun défaut MON16.2 n'est établi par ces messages.

## Clôture

MON16.2 est **VALIDÉ ET CLOS**.

Aucune correction C++ n'a été requise après les campagnes de validation.

Prochaine étape : `MON16.3 — DoT Poison / Bleeding / Burning`.
