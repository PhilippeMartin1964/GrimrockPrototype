# MON17 — Final Regression Plan

Statut : **À EXÉCUTER sous UE5.5.4**  
Date : **21 août 2026**

## Objectif

Valider une dernière fois les contrats propres à MON17 et les systèmes réutilisés par le Gobelin lanceur avant de clore `MON17 — Second Monster Family`.

La campagne est volontairement ciblée : elle ne relance pas tout le projet, uniquement les familles directement impliquées par MON17.

## Filtres à exécuter

Exécuter successivement :

```text
Grimrock.Monsters.MON17
Grimrock.Monsters.MON6
Grimrock.Monsters.MON8
Grimrock.Monsters.MON13
Grimrock.Monsters.MON14
Grimrock.RPG.MON15
```

## Comptage attendu

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

### Détail MON17 attendu

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

## Critères de validation

La clôture exige :

```text
109 tests terminés
109 Success
0 Fail
aucun nouvel Error de production
```

Les warnings explicitement attendus par des fixtures négatives historiques ne bloquent pas la clôture s'ils correspondent aux tests qui les provoquent volontairement.

Si le nombre de tests découverts diffère du comptage ci-dessus sans `Fail`, conserver le log complet : il faudra distinguer un changement de découverte Automation d'une régression réelle avant de conclure.

## Après validation

Après réception du log final :

1. consigner les résultats exacts ;
2. créer `docs/Design/MON17_CLOSURE.md` ;
3. passer `MON17.7` et `MON17` à `CLOS` dans `PROJECT_COMPLETION_ROADMAP.md` ;
4. définir `MON18 — Magic & Spellbook` comme prochain jalon actif.

Aucun nouveau changement gameplay MON17 n'est prévu avant cette campagne.