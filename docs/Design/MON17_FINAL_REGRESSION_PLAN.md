# MON17 — Final Regression Plan

Statut : **VALIDÉ ET CLOS sous UE5.5.4**  
Date : **21 août 2026**

## Objectif

Valider une dernière fois les contrats propres à MON17 et les systèmes réutilisés par le Gobelin lanceur avant de clore `MON17 — Second Monster Family`.

La campagne est volontairement ciblée : elle ne relance pas tout le projet, uniquement les familles directement impliquées par MON17.

## Filtres demandés

```text
Grimrock.Monsters.MON17
Grimrock.Monsters.MON6
Grimrock.Monsters.MON8
Grimrock.Monsters.MON13
Grimrock.Monsters.MON14
Grimrock.RPG.MON15
```

## Comptage de référence

```text
Grimrock.Monsters.MON17     30/30
Grimrock.Monsters.MON6       3/3
Grimrock.Monsters.MON8       7/7
Grimrock.Monsters.MON13      6/6
Grimrock.Monsters.MON14     21/21
Grimrock.RPG.MON15          42/42
---------------------------------
TOTAL                      109/109
```

### Détail MON17 de référence

```text
MON17.1      3 tests
MON17.2      2 tests
MON17.3     10 tests
MON17.4.1    3 tests
MON17.5.1    4 tests
MON17.6.1    4 tests
MON17.7      4 tests
-------------------
TOTAL       30 tests
```

## Pourquoi ces régressions

```text
MON6   combat monstre de base / resolver
MON8   loot data-driven et placement monde
MON13  MonsterSpawn / encounter / persistence
MON14  perception / patrol / investigation / alarm / engagement
MON15  XP / Level progression / persistence RPG
MON17  seconde famille, projectile, RangedKeeper, reward et balance
```

## Validation finale

La campagne finale est déclarée validée sous UE5.5.4 par le propriétaire du projet : tous les tests demandés sont OK et aucun défaut MON17 restant ne justifie de maintenir le jalon ouvert.

Le seul incident rencontré pendant cette phase a été isolé et corrigé dans la fixture `MON13.5.RealPIEIntegration` : l'ajout des monstres de production sur `L_GrimrockEditor` pouvait occuper une cellule réservée aux Rats générés par le test, ce qui entraînait `GeneratedMonsterCellConflict` puis `AtomicSpawnFailed`.

Après isolation des monstres étrangers à l'encounter de test, le résultat observé est :

```text
Test Completed. Result={Success}
Name={RealPIEIntegration}
Path={Grimrock.Monsters.MON13.5.RealPIEIntegration}
```

Les validations MON17.7 finales ont également été relancées après cette correction :

```text
Grimrock.Monsters.MON17.7.ProductionLootBaseline   Success
Grimrock.Monsters.MON17.7.FinalBalanceContract     Success
```

Les valeurs finales observées restent :

```text
GoblinKnife  0.250
Stone        0.500
EmptyVial    0.250
ExpectedItemsPerKill = 1.000

HP=10 Initiative=12 Accuracy=2 Evasion=3 AP=3
Damage=2..5 XP=125 LootExpected=1.000
```

## Résultat de clôture

```text
Régressions utiles MON17           VALIDÉES
Nouvelle régression bloquante      AUCUNE
MON17.7                             CLOS
MON17                               CLOS
```

La campagne est terminée. Aucun nouveau changement gameplay MON17 n'est prévu.

Référence de clôture :

```text
docs/Design/MON17_CLOSURE.md
```

Prochain jalon :

```text
MON18 — Magic & Spellbook
MON18.1 — Spell Definition / Identity
```