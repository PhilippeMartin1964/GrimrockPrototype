# TD07.3.7 — Current Asset Repair / Recreation

Date : 28 août 2026
Projet : GrimrockPrototype — Unreal Engine 5.5.4
Parent : TD07.3 — Prototype Data Model Reset
Statut : VALIDÉ — CLOS

## 1. Objectif

Réparer ou recréer les DataAssets courants qui portent encore des données incompatibles, redondantes ou legacy après les normalisations TD07.3.2 à TD07.3.6.

Cette tranche ne réintroduit aucune migration backward. Git conserve l'historique ; les assets courants doivent être remis au schéma actuel.

## 2. Important : un audit vert n'implique pas zéro finding

`Grimrock.TechnicalDebt.TD07_3_1.CurrentSchemaAssetAudit` valide que tous les DataAssets se chargent et que le rapport est écrit. Il ne fait pas échouer l'Automation lorsqu'il existe encore des candidats de nettoyage.

TD07.3.7 ajoute donc une caractérisation dédiée qui compte et liste explicitement les assets restant à réparer.

## 3. Candidats recherchés

```text
AUTHORING.DEFINITION_WITHOUT_ID
AUTHORING.ID_ONLY
AUTHORING.ASSET_ID_CONFLICT
AUTHORING.ASSET_ID_DUPLICATE
AUTHORING.LOCK_KEY_IDS
ITEM.INVALID_COMBAT_ACTIONS
```

Ces catégories représentent les seules familles encore pertinentes dans l'audit courant après TD07.3.6.

## 4. Gate de caractérisation

Filtre :

```text
Grimrock.TechnicalDebt.TD07_3_7.Characterization
```

Rapport :

```text
Saved/Diagnostics/TD07/TD07_3_7_CurrentAssetRepairCandidates.txt
```

Le test doit être vert même si des candidats existent : son rôle est de produire la liste autoritaire de réparation.

## 5. Décision après exécution

- **0 candidat** : aucune réparation LFS supplémentaire ; TD07.3.7 peut être clos rapidement avec un gate zéro-candidat durable.
- **>0 candidat** : réparer/recréer uniquement les assets listés, par familles logiques, avec one-shot Editor/LFS si nécessaire.
- après réparation : ajouter `Grimrock.TechnicalDebt.TD07_3_7.Normalization` exigeant strictement zéro candidat.
- ensuite : régressions + Shipping, puis passage à TD07.3.8.


## 6. Characterization validée

Validation locale du 28 août 2026 :

```text
Grimrock.TechnicalDebt.TD07_3_7.Characterization
Succeeded              : 1
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0
Report                 : Saved/Automation/TD04/TD04-20260828-101925
```

Rapport :

```text
Scanned DataAssets: 86
Repair candidates: 32

AUTHORING.ASSET_ID_CONFLICT    2
AUTHORING.ASSET_ID_DUPLICATE  23
AUTHORING.ID_ONLY              2
AUTHORING.LOCK_KEY_IDS         5
```

Aucun `ITEM.INVALID_COMBAT_ACTIONS` ni `AUTHORING.DEFINITION_WITHOUT_ID` n'est présent.

## 7. Diagnostic

Les 32 findings sont concentrés dans quelques assets et relèvent de quatre opérations déterministes :

1. supprimer les IDs miroirs lorsqu'une référence d'asset valide existe ;
2. remplacer les deux IDs seuls du Rat Giant par de vraies références ;
3. migrer `AcceptedKeyIds` vers `AcceptedKeyItems` ;
4. corriger les deux conflits Shuriken en gardant la référence `Shuriken` et en supprimant l'ancien ID `Stone`.

Le runtime MonsterSpawn a également été normalisé afin qu'un `MonsterDefinitionAsset` suffise comme autorité d'authoring. Il ne réécrit plus automatiquement `MonsterDefinitionId`.

Commit :

```text
625ceeca9d39564e8533fd8e8cafd8fe2e56f829
Normalize TD07.3.7 MonsterSpawn authoring identity
```

## 8. Deux assets à recréer

Le repository ne contient aucune définition pour :

```text
Item_RatMeat
Item_RatTooth
```

Les docs MON8 indiquaient explicitement qu'elles étaient à ajouter lorsque leurs DataAssets existeraient.

Le one-shot crée donc :

```text
/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_RatMeat
    ItemDefinitionId = Item_RatMeat
    ItemType = Food

/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_RatTooth
    ItemDefinitionId = Item_RatTooth
    ItemType = Component
```

Ce sont des définitions techniques current-schema minimales, stackables, sans combat ni équipement. Présentation, mesh et équilibrage pourront être enrichis séparément sans réintroduire de dette de schéma.

## 9. One-shot AssetRepair

Automation :

```text
Grimrock.TechnicalDebt.TD07_3_7.AssetRepair
```

Script :

```text
Scripts/RepairTD0737CurrentAssets.ps1
```

Commit de préparation :

```text
5df908bf3612ee97a984a3d5f53f6f3035edef2a
Prepare TD07.3.7 current asset repair
```

Le test :
- construit les registries Item / Readable / Monster par ID canonique ;
- crée les deux définitions Rat manquantes ;
- répare toutes les paires asset/id ;
- migre les clés de serrure ;
- sauvegarde uniquement les DataAssets modifiés ;
- exige zéro candidat résiduel en mémoire.

Le script refuse de démarrer si `Content/` contient déjà des changements locaux, puis commit/push uniquement les `.uasset` modifiés ou créés.


## 10. AssetRepair validé

Exécution locale du 28 août 2026 :

```text
Grimrock.TechnicalDebt.TD07_3_7.AssetRepair
Succeeded              : 1
Succeeded with warnings: 0
Failed                 : 0
Report                 : Saved/Automation/TD04/TD04-20260828-103129
```

Le one-shot a modifié/créé exactement 12 DataAssets et les a poussés en Git LFS :

```text
5576153644e0dc95fa9d948c32830007d337ee96
Repair TD07.3.7 current authoring assets
```

Assets :
- `DA_Lock_CopperWall`
- `DA_Lock_IronWall`
- cinq archetypes pickup Item ;
- `DA_GridLevel_00` ;
- `DA_Item_RatMeat` ;
- `DA_Item_RatTooth` ;
- `DA_MON_GoblinThrower` ;
- `DA_MON_RatGiant`.

Le test de réparation exigeait zéro candidat résiduel en mémoire avant de réussir.

## 11. Normalization durable

Le one-shot `AssetRepair` et son script sont supprimés après succès.

Le gate durable devient :

```text
Grimrock.TechnicalDebt.TD07_3_7.Normalization
```

Sous-tests :

```text
CurrentAssets
RatLootDefinitions
LockAuthoring
MonsterSpawnAuthority
```

Stop condition avant clôture :
- [x] 32 candidats caractérisés ;
- [x] 12 DataAssets réparés/créés et poussés ;
- [x] infrastructure one-shot retirée ;
- [ ] Normalization 4/4 ;
- [ ] Characterization rapporte 0 candidat ;
- [ ] CurrentSchemaAssetAudit vert ;
- [ ] régressions MON1/MON8/MON13/MON14/MON17 vertes ;
- [ ] Shipping Win64 vert ;
- [ ] TD07.3.7 clos.


## 12. Validation finale et clôture

Validation locale du 28 août 2026 :

```text
Grimrock.TechnicalDebt.TD07_3_7.Normalization
    Succeeded              : 4
    Succeeded with warnings: 0
    Failed                 : 0
    Report                 : Saved/Automation/TD04/TD04-20260828-103723

Grimrock.TechnicalDebt.TD07_3_7.Characterization
    Succeeded              : 1
    Succeeded with warnings: 0
    Failed                 : 0
    Report                 : Saved/Automation/TD04/TD04-20260828-103744

TD07_3_7_CurrentAssetRepairCandidates.txt
    Scanned DataAssets     : 88
    Repair candidates      : 0

Grimrock.TechnicalDebt.TD07_3_1.CurrentSchemaAssetAudit
    Succeeded              : 1
    Failed                 : 0

Grimrock.Monsters.MON1.DefinitionValidation
    Succeeded              : 1
    Failed                 : 0

Grimrock.Monsters.MON1.InvalidData
    Succeeded              : 1
    Failed                 : 0

Grimrock.Monsters.MON8
    Succeeded              : 4
    Succeeded with warnings: 3
    Failed                 : 0

Grimrock.Monsters.MON13
    Succeeded              : 13
    Succeeded with warnings: 4
    Failed                 : 0

Grimrock.Monsters.MON14
    Succeeded              : 19
    Failed                 : 0

Grimrock.Monsters.MON17
    Succeeded              : 43
    Succeeded with warnings: 1
    Failed                 : 0
```

Shipping final :

```text
Target        : GrimrockPrototype
Platform      : Win64
Configuration : Shipping
Archive       : Saved/Packaging/TD04/TD04-Shipping-20260828-103955
Pak files     : 1
Archive files : 41
Archive bytes : 905984939
BUILD SUCCESSFUL
Cook Success  : 0 error(s), 0 warning(s)
AutomationTool ExitCode=0
```

Stop condition TD07.3.7 :

- [x] 32 candidats caractérisés ;
- [x] 12 DataAssets réparés/créés et poussés ;
- [x] infrastructure one-shot retirée ;
- [x] Normalization 4/4 ;
- [x] Characterization = 0 candidat sur 88 DataAssets ;
- [x] CurrentSchemaAssetAudit vert ;
- [x] régressions ciblées vertes ;
- [x] Shipping Win64 vert ;
- [x] TD07.3.7 clos.

La tranche suivante est **TD07.3.8 — Strict Current-Schema Validation / stop condition**.
