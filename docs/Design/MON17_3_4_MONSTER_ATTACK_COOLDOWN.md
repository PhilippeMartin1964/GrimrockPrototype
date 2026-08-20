# MON17.3.4 — Monster Attack Cooldown / Regression / Closure

Statut : **IMPLÉMENTÉ — compilation et validation UE5.5.4 à faire**

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

## Validation demandée

Après compilation UE5.5.4 :

```text
Grimrock.Monsters.MON17.3.4
```

Résultat attendu :

```text
CooldownLifecycle            Success
CooldownIsolation            Success
GoblinZeroCooldownContract   Success
```

Puis régressions :

```text
Grimrock.Monsters.MON17.3.1
Grimrock.Monsters.MON17.3.2
Grimrock.Monsters.MON17.3.3
Grimrock.Monsters.MON6
```

Enfin, un PIE court avec le Gobelin doit confirmer qu'avec `CooldownTurns=0` :

- `Attack_ThrowKnife` reste utilisable à chaque tour où portée/LOS/AP sont valides ;
- le montage de lancer reste joué ;
- le projectile part toujours de `ProjectileSource` ;
- Hit/Miss/dégâts restent inchangés.

## Porte de clôture MON17.3

`MON17.3 — Distinct Attack Set` pourra être déclaré **VALIDÉ / CLOS** lorsque :

1. MON17.3.4 compile sous UE5.5.4 ;
2. les 3 tests MON17.3.4 sont verts ;
3. les régressions MON17.3.1/2/3 et MON6 sont vertes ;
4. le PIE Gobelin à cooldown zéro reste inchangé.

Après cela, le travail autoritaire devient :

```text
MON17.4 — Distinct AI Profile — RangedKeeper
```

Le maintien de distance, la recherche d'une case de tir, le recul et le kiting restent explicitement hors périmètre de MON17.3.4.
