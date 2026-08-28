# TD07.3.5.4 — Monster Range Schema

Date : 28 août 2026
Projet : GrimrockPrototype — Unreal Engine 5.5.4
Parent : TD07.3.5 — Combat Data Schema Reset
Statut : ASSET REPAIR PREPARED

## 1. Cible stricte

Schéma final de `FGridMonsterAttackDefinition` :

```text
MinRangeCells
MaxRangeCells
```

Le nom historique `RangeCells` doit disparaître physiquement. Aucun alias, `PostLoad`, CoreRedirect, fallback runtime ou migration backward ne restera.

## 2. Réparation one-shot

Les deux Monster Definition DataAssets courants ont été sérialisés avec leur maximum sous `RangeCells`. Une étape jetable introduit donc temporairement `MaxRangeCells`, copie `RangeCells -> MaxRangeCells`, resauvegarde les assets LFS, puis sera supprimée avant la normalisation finale.

Assets :

```text
/Game/GrimrockPrototype/Monsters/RatGiant/Data/DA_MON_RatGiant
/Game/GrimrockPrototype/Monsters/GoblinThrower/Data/DA_MON_GoblinThrower
```

Automation :

```text
Grimrock.TechnicalDebt.TD07_3_5_4.AssetRepair.MonsterRangeAssets
```

Script :

```text
Scripts/RepairTD07354MonsterRange.ps1
```

Pendant cette seule étape, les helpers runtime continuent à lire `RangeCells`; `MaxRangeCells` est uniquement la cible de resauvegarde.

## 3. Après le commit LFS

- supprimer physiquement `RangeCells` ;
- basculer tous les consommateurs monster sur `MaxRangeCells` ;
- adapter les tests MON17 et assimilés ;
- supprimer `MONSTER.RANGE_FIELD_RENAME` de TD07.3.1 ;
- adapter la Characterization ;
- ajouter `Grimrock.TechnicalDebt.TD07_3_5_4.Normalization` ;
- supprimer le test et le script one-shot ;
- documenter et valider.

SaveGame reste v22 exact-match.
