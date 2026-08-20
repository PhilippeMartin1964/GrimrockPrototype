# MON17.3.4 — Monster Attack Cooldown / Regression / Closure

Statut : **VALIDÉ EN AUTOMATION UE5.5.4 — PIE final Gobelin restant avant clôture MON17.3**

## Objectif

Finaliser le contrat générique de `FGridMonsterAttackDefinition::CooldownTurns` sans modifier le comportement actuel du Gobelin lanceur, puis préparer la clôture de `MON17.3 — Distinct Attack Set`.

Cette étape ne modifie ni portée, LOS, projectile, animation, dégâts, coût PA, `RangedKeeper`, recul ou kiting.

## Sémantique autoritaire

`CooldownTurns` représente le nombre de **tours suivants du même monstre** pendant lesquels l'attaque reste indisponible.

Exemple :

```text
CooldownTurns = 2

Tour N      attaque utilisée
Tour N+1    indisponible
Tour N+2    indisponible
Tour N+3    disponible
```

`CooldownTurns = 0` signifie qu'aucun cooldown runtime n'est créé.

Le Gobelin lanceur conserve donc :

```text
Attack_ThrowKnife.CooldownTurns = 0
```

et son comportement validé en MON17.3.1–17.3.3 ne doit pas changer.

## Architecture

Le cooldown appartient à `UGridMonsterCombatComponent`, qui possède désormais un état runtime pur :

```cpp
FGridMonsterAttackCooldownState
```

Cet état conserve :

```text
CurrentTurnSerial
AttackId -> UnavailableThroughTurnSerial
```

Il est indépendant du framerate et n'utilise ni timer monde ni animation.

### Synchronisation avec le TurnManager

`UGridMonsterCombatComponent` réutilise les événements déjà publics du TurnManager :

```text
OnMonsterTurnStarted
OnPhaseChanged
```

À chaque tour autoritaire du monstre concerné :

```text
CurrentTurnSerial++
```

Le changement vers `StartingCombat`, `Exploration`, `Victory` ou `Defeat` réinitialise l'état runtime de cooldown.

Une vérification de secours observe également le `CurrentMonster` et le `RoundNumber` du TurnManager avant la sélection/résolution d'une attaque afin de ne pas dépendre uniquement de l'ordre de BeginPlay des composants.

Le système suppose le contrat actuel du TurnManager : un monstre possède au plus une activation autoritaire par round. Haste/Slow peut réordonner les activations futures mais ne crée pas actuellement de seconde activation dans un même round.

## Sélection des attaques

`UGridMonsterCombatComponent::GetPreferredAttackForRange()` continue de sélectionner la définition valide de plus haute priorité, mais ignore maintenant une attaque dont l'`AttackId` est en cooldown.

Ainsi, si plusieurs attaques partagent la même portée :

```text
attaque prioritaire en cooldown
        ↓
attaque disponible de priorité inférieure
```

peut être sélectionnée sans hardcoding par famille de monstre.

## Validation d'exécution

Le cooldown est revérifié avant la présentation/résolution afin qu'une attaque indisponible ne puisse pas être exécutée par un appel direct qui contournerait le planner.

Le cooldown est engagé uniquement lorsqu'une attaque a réellement été résolue par `ResolveAndApplyPartyAttack()`.

Un miss compte comme une utilisation de l'attaque et déclenche donc le cooldown, comme un hit.

## Runtime seulement

Le cooldown d'attaque monstre est volontairement un état de combat transient :

- pas de changement SaveGame ;
- pas de migration de sauvegarde ;
- remise à zéro entre combats ;
- aucune modification de `UGridMonsterDefinitionAsset` autre que le champ `CooldownTurns` déjà existant.

## Tests automatisés

Filtre :

```text
Grimrock.Monsters.MON17.3.4
```

Tests :

```text
CooldownLifecycle
CooldownIsolation
GoblinZeroCooldownContract
```

### CooldownLifecycle

Vérifie la sémantique exacte :

```text
CooldownTurns=2
N   utilisé
N+1 bloqué
N+2 bloqué
N+3 disponible
```

### CooldownIsolation

Vérifie :

- isolation par `AttackId` ;
- `CooldownTurns=1` bloque exactement N+1 ;
- `CooldownTurns=0` ne crée aucun cooldown ;
- `Reset()` efface serial et cooldowns.

### GoblinZeroCooldownContract

Reproduit le contrat runtime actuel de `Attack_ThrowKnife` :

```text
AttackId              Attack_ThrowKnife
MinDamage             2
MaxDamage             5
MinRangeCells         2
RangeCells            6
Delivery              Projectile
bRequiresLineOfSight  true
ActionPointCost       2
CooldownTurns         0
Priority              100
```

et vérifie que l'attaque reste disponible après utilisation et au tour suivant.

## Validation UE5.5.4 — 20 août 2026

Compilation `Development_Editor x64` suffisamment valide pour exécuter les campagnes Automation.

### MON17.3.4

Résultat fourni par l'utilisateur : **3/3 Success**.

```text
CooldownIsolation            Success
CooldownLifecycle            Success
GoblinZeroCooldownContract   Success
```

### Régression MON17.3 complète

Résultat fourni par l'utilisateur : **10/10 Success**.

```text
MON17.3.1.LineOfSight                  Success
MON17.3.1.MeleeRegressionPlanner       Success
MON17.3.1.StationaryRangedPlanner      Success
MON17.3.2.ProjectileTiming             Success
MON17.3.2.ProjectileTrajectory         Success
MON17.3.2.ProjectileVisualOptionality  Success
MON17.3.3.ProjectileSourceContract     Success
MON17.3.4.CooldownIsolation            Success
MON17.3.4.CooldownLifecycle            Success
MON17.3.4.GoblinZeroCooldownContract   Success
```

### Régression combat MON6

Résultat fourni par l'utilisateur : **3/3 Success**.

```text
MON6.CombatResolver        Success
MON6.DirectMeleePlanner    Success
MON6.PartyTargetSelector   Success
```

Campagne combinée demandée : **13/13 Success**.

Aucune régression automatisée n'est observée sur le planner mêlée historique, la LOS, le ranged planner, la présentation projectile, le socket de source ou le cooldown zéro du Gobelin.

## Validation PIE finale demandée

Un dernier PIE court avec le Gobelin doit confirmer qu'avec :

```text
Attack_ThrowKnife.CooldownTurns = 0
```

- `Attack_ThrowKnife` reste utilisable à chaque tour où portée/LOS/AP sont valides ;
- `AM_GoblinThrower_ThrowKnife` reste joué ;
- le projectile part toujours de `ProjectileSource` sur la paume droite ;
- les timings restent `ExpectedDuration=2.20 s`, `ImpactTimeSeconds=1.00 s`, `ProjectileTravelDuration=0.20 s`, donc lancement visuel vers `0.80 s` ;
- Hit/Miss/dégâts restent inchangés ;
- aucun log `[GridMonsterCooldown] Attack rejected ... Reason=Cooldown` ne doit apparaître pour `Attack_ThrowKnife` avec `CooldownTurns=0`.

## Porte de clôture MON17.3

État au 20 août 2026 :

```text
1. Compilation UE5.5.4                 VALIDÉE
2. MON17.3.4                            3/3 Success
3. MON17.3.1–17.3.4                     10/10 Success
4. MON6                                 3/3 Success
5. PIE Gobelin cooldown zéro            À VALIDER
```

`MON17.3 — Distinct Attack Set` pourra être déclaré **VALIDÉ / CLOS** dès que le dernier PIE confirme l'absence de régression runtime.

Après cela, le travail autoritaire devient :

```text
MON17.4 — Distinct AI Profile — RangedKeeper
```

Le maintien de distance, la recherche d'une case de tir, le recul et le kiting restent explicitement hors périmètre de MON17.3.4.
