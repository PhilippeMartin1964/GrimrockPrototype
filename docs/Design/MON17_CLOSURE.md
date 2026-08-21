# MON17 — Second Monster Family — Clôture

Statut : **VALIDÉ ET CLOS sous Unreal Engine 5.5.4**  
Date : **21 août 2026**

## Objectif atteint

MON17 devait prouver que l'architecture monstre construite jusqu'à MON16 n'était pas spécifique au Rat Géant.

La seconde famille retenue, le **Gobelin lanceur** (`MON_GoblinThrower`), est désormais intégrée de bout en bout avec un profil tactique distinct `RangedKeeper`, sans fork du système de combat, du système de perception, des encounters, du loot, de l'XP, des effets de statut ou de la persistance.

```text
MON17.1 — Definition / Assets / Spawn Contract          CLOS
MON17.2 — Skeletal Mesh / Skeleton / AnimBP             CLOS
MON17.3 — Distinct Attack Set                            CLOS
MON17.4 — Distinct AI Profile — RangedKeeper            CLOS
MON17.5 — Patrol / Perception / Alarm Integration       CLOS
MON17.6 — Encounter / Loot / XP Integration             CLOS
MON17.7 — Balance / Closure                              CLOS
```

## Contrat final du Gobelin lanceur

```text
MonsterId             MON_GoblinThrower
DangerLevel           3
MaxHealth             10
Initiative            12
Accuracy              2
Evasion               3
ActionPointsPerTurn    3
SightRangeCells       8
HearingRangeCells     4
PrimaryAIProfile      RangedKeeper
PreferredDistance     3..5
ExperienceReward      125
```

Attaque de production :

```text
AttackId               Attack_ThrowKnife
Delivery               Projectile
Damage                 2..5 Physical
MinRangeCells           2
RangeCells              6
bRequiresLineOfSight    true
ActionPointCost         2
CooldownTurns           0
ProjectileTravel       0.20 s
ProjectileSourceSocket ProjectileSource
```

Loot final :

```text
GoblinKnife  DropChance=0.25 Quantity=1..1
Stone        DropChance=0.50 Quantity=1..1
EmptyVial    DropChance=0.25 Quantity=1..1
ExpectedItemsPerKill = 1.000
```

Relation XP retenue :

```text
4 × Gobelin lanceur (125 XP) = 1 × Rat Géant (500 XP)
```

## Architecture validée

MON17 a confirmé que les contrats suivants sont génériques et réutilisables :

- `MonsterSpawn -> MonsterDefinition -> runtime Actor` ;
- attaques de mêlée et attaques à distance data-driven ;
- `EGridMonsterAttackDelivery::Projectile` ;
- projectile visuel de présentation sans logique de dégâts parallèle ;
- source projectile via socket optionnel ;
- cooldown par `AttackId` ;
- planner tactique `RangedKeeper` ;
- déplacement/repositionnement selon distance préférée et LOS ;
- perception directionnelle, ouïe, investigation et patrouille MON14 ;
- alarme locale entre membres d'un même groupe ;
- engagement automatique par vision ;
- EncounterGroup / waves ;
- mort exactly-once ;
- loot data-driven ;
- attribution XP exactly-once ;
- libération d'occupation ;
- Victory après élimination du dernier ennemi ;
- sauvegarde/Continue sans duplication de récompenses.

## Validation de production

Le PIE final avec deux Gobelins de production a confirmé :

```text
Spawn des deux MON_GoblinThrower
ExplorationAlert par ouïe
CombatStarted par PatrolVision
Deux Gobelins dans l'initiative
RangedKeeper actif
Attack_ThrowKnife exécuté
Projectile depuis ProjectileSource
Mort et OccupancyReleased pour chaque Gobelin
Loot final probabiliste
+125 XP exactement une fois par mort
+250 XP après deux morts
Victory après le second décès
```

Aucune modification de balance supplémentaire n'a été retenue après ce run.

## Validation automatisée finale

Les tests dédiés MON17.7 ont été confirmés sous UE5.5.4 :

```text
ProductionBalanceBaseline   Success
ProductionLootBaseline      Success
RewardPacingBaseline        Success
FinalBalanceContract        Success
```

La régression `Grimrock.Monsters.MON13.5.RealPIEIntegration`, affectée pendant la campagne finale par un conflit entre sa fixture et les nouveaux monstres de production de la carte, a été corrigée par isolation de l'encounter de test puis relancée :

```text
Grimrock.Monsters.MON13.5.RealPIEIntegration   Success
```

La campagne de régression finale est déclarée validée sous UE5.5.4 ; tous les tests demandés sont OK.

## Décisions finales

- `MON_GoblinThrower` reste à `HP=10`, dégâts `2..5`, AP `3`, XP `125`.
- `RangedKeeper` reste le profil runtime autoritaire.
- La portée préférée reste `3..5` et `ThrowKnife` reste valide sur `2..6`.
- Le loot reste constitué de trois tirages indépendants `25 % / 50 % / 25 %`.
- Aucun système spécifique au Gobelin n'est ajouté pour les encounters, le loot, l'XP ou la persistance.
- Le défaut MON13.5 observé pendant la clôture était un défaut d'isolation de fixture de test, pas une régression du runtime de production.

## Documents de référence

```text
docs/Design/MON17_3_4_MONSTER_ATTACK_COOLDOWN.md
docs/Design/MON17_4_1_RANGED_KEEPER_POSITIONING.md
docs/Design/MON17_5_CLOSURE.md
docs/Design/MON17_6_CLOSURE.md
docs/Design/MON17_7_BALANCE_AUDIT.md
docs/Design/MON17_7_FINAL_BALANCE_CONTRACT.md
docs/Design/MON17_7_FINAL_PIE_VALIDATION.md
docs/Design/MON17_FINAL_REGRESSION_PLAN.md
```

## Porte de sortie

```text
MON17 — Second Monster Family   CLOS
```

Le prochain jalon autoritaire est :

```text
MON18 — Magic & Spellbook
MON18.1 — Spell Definition / Identity
```
