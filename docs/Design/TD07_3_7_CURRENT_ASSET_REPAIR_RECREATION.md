# TD07.3.7 — Current Asset Repair / Recreation

Date : 28 août 2026
Projet : GrimrockPrototype — Unreal Engine 5.5.4
Parent : TD07.3 — Prototype Data Model Reset
Statut : CHARACTERIZATION PREPARED — À VALIDER

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
