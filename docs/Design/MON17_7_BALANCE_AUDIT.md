# MON17.7 — Balance / Closure — Audit final

Statut : **EN COURS — contrat final et PIE final VALIDÉS sous UE5.5.4, régressions finales à faire**
Date : **21 août 2026**

## Objectif

MON17.7 fige la balance du Gobelin lanceur, vérifie son pacing face au Rat Géant, limite le loot à une valeur raisonnable, puis ferme MON17 après une campagne de régression ciblée.

Aucun nouveau système de combat, profil IA ou refactor de loot/XP n'est introduit.

## Baseline de production validée

```text
Danger              3
HP                  10
Armures             0 / 0
Initiative          12
Accuracy / Evasion  2 / 3
AP                  3
Attaques            1
Dégâts bruts        2..5
Dégâts bruts moyens 3.5
XP                  125
```

Contrat tactique : `RangedKeeper`, distance préférée `3..5`, `ThrowKnife` à portée `2..6`, mouvement `1 AP` puis attaque `2 AP`.

Aucun changement de statistiques de combat n'est justifié. Elles restent figées pour la clôture MON17.

## Pacing XP validé

```text
4 Gobelins = 1 Rat Géant = 500 XP
8 Gobelins atteignent le niveau 2 en solo
```

Pour quatre personnages actifs, `125` se partage en `32/31/31/31`. Le décalage d'un Gobelin entre le premier personnage et les autres ne justifie pas une modification artificielle de la récompense.

Décision :

```text
ExperienceReward = 125  CONSERVÉ
```

## Loot final validé

La configuration temporaire MON17.6 était de `3.000` objets attendus par Gobelin. La balance finale est :

```text
GoblinKnife  DropChance = 0.25
Stone        DropChance = 0.50
EmptyVial    DropChance = 0.25
MinQuantity = 1
MaxQuantity = 1

ExpectedItemsPerKill = 1.000
```

Les tirages restent indépendants.

`ProductionLootBaseline` a été relancé après modification et a confirmé `ExpectedItemsPerKill=1.000`.

## Automation MON17.7 validée

```text
Grimrock.Monsters.MON17.7.ProductionBalanceBaseline  Success
Grimrock.Monsters.MON17.7.ProductionLootBaseline     Success
Grimrock.Monsters.MON17.7.RewardPacingBaseline       Success
Grimrock.Monsters.MON17.7.FinalBalanceContract       Success
```

Sortie finale :

```text
MON17.7 final balance: HP=10 Initiative=12 Accuracy=2 Evasion=3 AP=3 Damage=2..5 XP=125 LootExpected=1.000
```

Le `DA_MON_GoblinThrower.uasset` équilibré est versionné sur `origin/master`.

## PIE final validé

Le combat de production à deux Gobelins confirme :

```text
PerceptionHearing -> ExplorationAlert
PatrolVision -> CombatStarted
2 Gobelins présents dans l'initiative
Attack_ThrowKnife exécuté
ProjectileSource utilisé comme origine du projectile
XP +125 sur chaque mort
OccupancyReleased=true sur chaque mort
Victory=true après la seconde mort
```

Loot observé :

```text
Gobelin 1
  GoblinKnife  Roll=0.743  Chance=0.250  NoDrop
  Stone        Roll=0.035  Chance=0.500  Drop x1
  EmptyVial    Roll=0.329  Chance=0.250  NoDrop
  Summary: Dropped=1 Placed=1 Failed=0

Gobelin 2
  GoblinKnife  Roll=0.787  Chance=0.250  NoDrop
  Stone        Roll=0.505  Chance=0.500  NoDrop
  EmptyVial    Roll=0.379  Chance=0.250  NoDrop
  Summary: Dropped=0 Placed=0 Failed=0
```

L'échantillon donne un objet sur deux Gobelins alors que l'espérance est de deux. Cette variance est normale pour trois tirages indépendants et ne constitue pas une anomalie.

Le combat dure 14 manches dans ce run, essentiellement parce que le personnage de test est seul, niveau 1, à mains nues et rate de nombreuses attaques contre `Defense=13`. Le Gobelin n'est ni buffé ni nerfé sur cette seule observation.

Référence détaillée :

```text
docs/Design/MON17_7_FINAL_PIE_VALIDATION.md
```

## Dernière étape avant clôture MON17

Exécuter la campagne définie dans :

```text
docs/Design/MON17_FINAL_REGRESSION_PLAN.md
```

Porte de sortie : **109/109 Success**, aucun nouvel `Error` de production, puis création de `MON17_CLOSURE.md` et passage de la roadmap à MON18.