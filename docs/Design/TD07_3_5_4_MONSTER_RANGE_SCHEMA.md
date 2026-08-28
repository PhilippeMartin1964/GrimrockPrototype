# TD07.3.5.4 — Monster Range Schema

Date : 28 août 2026
Projet : GrimrockPrototype — Unreal Engine 5.5.4
Parent : TD07.3.5 — Combat Data Schema Reset
Statut : LFS RÉPARÉ — NORMALISATION C++ IMPLÉMENTÉE / À VALIDER

## 1. Cible stricte

Le schéma monster attack courant est désormais :

```text
MinRangeCells
MaxRangeCells
```

Le nom historique `RangeCells` est supprimé physiquement.
Aucun alias, `PostLoad`, CoreRedirect, fallback runtime ou migration backward n'est conservé.

## 2. Réparation LFS préalable

Les Monster Definition DataAssets courants avaient sérialisé leur maximum sous `RangeCells`.
Une étape one-shot a introduit temporairement `MaxRangeCells`, copié `RangeCells -> MaxRangeCells`, puis resauvegardé les assets avant suppression de l'ancien champ.

Automation :

```text
Grimrock.TechnicalDebt.TD07_3_5_4.AssetRepair
Succeeded              : 1
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0
Report                 : Saved/Automation/TD04/TD04-20260828-092540
```

Commit LFS :

```text
88c5da1cb5ce12220f727f7166aa26a2604df931
Repair monster range authoring
```

Le commit LFS a modifié un asset. Les deux définitions RatGiant et GoblinThrower ont néanmoins été chargées et vérifiées par l'Automation avant sauvegarde.

## 3. Normalisation C++ finale

Commit :

```text
4d203d3070f638ea1c81acdc5e8bf9f0d569c607
Normalize TD07.3.5.4 monster range schema
```

Modifications :

- suppression physique de `FGridMonsterAttackDefinition::RangeCells` ;
- `SupportsDistance()` utilise `MaxRangeCells` ;
- `IsRangedAttack()` utilise `MaxRangeCells` ;
- validation `MaxRangeCells >= MinRangeCells` ;
- planners et exécution monster utilisent `MaxRangeCells` ;
- tests MON1 / MON17 / MON17.7 adaptés au schéma courant ;
- `MONSTER.RANGE_FIELD_RENAME` supprimé de l'audit TD07.3.1 ;
- Characterization adaptée au schéma final ;
- outil AssetRepair et script one-shot supprimés.

## 4. Préservation de la donnée non-default

Le nouveau gate charge les vrais DataAssets et vérifie notamment :

```text
RatGiant Attack_Bite
MinRangeCells = 1
MaxRangeCells = 1

GoblinThrower Attack_ThrowKnife
MinRangeCells = 2
MaxRangeCells = 6
```

Le cas GoblinThrower prouve que le maximum non-default sérialisé sous `RangeCells` a bien été conservé dans `MaxRangeCells`.

## 5. Gate de normalisation

Filtre :

```text
Grimrock.TechnicalDebt.TD07_3_5_4.Normalization
```

Tests :

```text
SchemaAuthority
RatGiantRangeAuthority
GoblinThrowerRangeAuthority
RuntimeConsumersNormalized
```

Attendu : 4/4, zéro warning, zéro échec.

La Characterization TD07.3.5 doit rester verte et constater que les dernières dettes combat `AttackSound`, `ImpactVFX` et `RangeCells` sont toutes supprimées.

## 6. Suite

Si le gate `.4` et les régressions monster sont verts, passer à :

```text
TD07.3.5.5 — Regressions / Shipping / Closure
```

SaveGame reste **v22 exact-match**.
