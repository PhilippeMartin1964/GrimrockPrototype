# TD07.3.5.2 — Item CombatActions Authority

Date : 28 août 2026
Projet : GrimrockPrototype — Unreal Engine 5.5.4
Parent : TD07.3.5 — Combat Data Schema Reset
Characterization : 4/4 validée
Statut : VALIDÉ ET CLOS — TD07.3.5.3 ACTIVE

## 1. Décision

Cible : UGridItemDefinitionAsset::CombatActions devient la seule autorité de combat item.

À supprimer après réparation authoring :
- bProvidesAttack
- OffensiveProfile au niveau item
- HasValidOffensiveProfile
- MakeLegacyEquipmentAttackDefinition
- tous les fallbacks runtime vers le profil item legacy

FGridOffensiveEquipmentProfile reste légitime comme payload d'une FGridCombatActionDefinition.

## 2. Blocage LFS

L'audit TD07.3.1 contient exactement un ITEM.LEGACY_OFFENSE_ONLY : DA_Weapon_Shuriken.

Chemin :
/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_Shuriken

Le fichier est versionné via Git LFS. Le connecteur GitHub ne possède que le pointeur LFS :
oid sha256:1d55bf319515a08166761f2440213677025da828e88b68239424e807c1b28107
size 5824

La réparation doit donc être effectuée par Unreal sur le vrai asset local.

## 3. Conversion exacte

Ancien profil :
AttackId=Attack_Shuriken
Damage=Physical/Piercing
Damage=1..4
AccuracyBonus=0
FlatDamageBonus=0
Scaling=Dexterity
Range=3
Slot=MainHand
Throwable=true

CombatAction cible :
ActionId=Attack_Shuriken
ActionType=RangedAttack
SourcePolicy=Equipment
TargetingPolicy=FirstAxialTarget
ResolutionProfile=Attack
ActionPointCost=2
SourceItemQuantityCost=1
RangeCells=3
PresentationProfileId=Attack_Shuriken
OffensiveProfile=copie exacte du profil MON11.3

## 4. Outil one-shot

Automation Editor :
Grimrock.TechnicalDebt.TD07_3_5_2.AssetRepair.ShurikenCombatActions

Elle charge le vrai DataAsset, convertit le profil legacy, vérifie l'équivalence, écrit CombatActions, remet bProvidesAttack à false, efface l'OffensiveProfile item-level et sauvegarde le package.

Le test est idempotent.

## 5. Script local

Scripts/RepairTD07352Shuriken.ps1

Le script exige master et Git LFS, refuse un asset déjà modifié, lance l'Automation, stage uniquement DA_Weapon_Shuriken.uasset, commit uniquement cet asset et push origin/master.

## 6. Séquence sûre

repair current LFS asset
-> version repaired asset
-> remove legacy C++ schema
-> remove runtime adapters/fallbacks
-> update tests
-> regressions

Aucun fallback PostLoad/runtime de compatibilité n'est introduit.

## 7. Stop condition intermédiaire

- [x] unique legacy-only item identifié
- [x] conversion exacte définie
- [x] Automation one-shot ajoutée
- [x] script LFS commit/push ajouté
- [x] Automation asset repair verte localement
- [x] repaired DA_Weapon_Shuriken pushed
- [x] legacy schema C++ supprimé
- [x] runtime fallback supprimé
- [x] normalization tests verts


## 8. Réparation LFS validée

Le 28 août 2026, l'outil one-shot a validé :

```text
Grimrock.TechnicalDebt.TD07_3_5_2.AssetRepair
Succeeded              : 1
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0
Report                 : Saved/Automation/TD04/TD04-20260828-084311
```

Le vrai asset LFS a été sauvegardé et poussé :

```text
7a14ca0254de605965e0c41f7933ed70462c6946
Repair Shuriken CombatActions authoring
```

Nouveau pointeur LFS :

```text
oid sha256:316d307faa458756db5d2f132c5fbe0759e0fe359cd7b13588c1f3ad8c56de87
size 8600
```

## 9. Normalisation C++ appliquée

Le schéma item cible est désormais :

```text
UGridItemDefinitionAsset
    CombatActions[]
```

Supprimés :

```text
bProvidesAttack
OffensiveProfile [item-level]
HasValidOffensiveProfile
FGridCombatActionCatalog::MakeLegacyEquipmentAttackDefinition
fallback player catalogue
fallback MON12 player attack profile
fallback hotbar
fallback throwable inventory action
```

Le paramètre désormais mort `DefaultAttackActionPointCost` de
`BuildInventoryCombatActionDefinition()` est également supprimé.

L'outil one-shot de réparation et son script sont supprimés après usage ; ils ne
font pas partie du runtime ni du tooling permanent.

## 10. Gate de normalisation

```text
Grimrock.TechnicalDebt.TD07_3_5_2.Normalization
```

Tests :

```text
SchemaAuthority
CombatActionsOnlyBehavior
ShurikenAssetAuthority
RuntimeFallbackRemoval
```

Attendu : 4/4, zéro warning.


## 11. Validation locale finale

```text
Grimrock.TechnicalDebt.TD07_3_5_2.Normalization
Succeeded              : 4
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0
Process exit code       : 0
Report                 : Saved/Automation/TD04/TD04-20260828-085612

Grimrock.TechnicalDebt.TD07_3_5.Characterization
Succeeded              : 4
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0
Process exit code       : 0
Report                 : Saved/Automation/TD04/TD04-20260828-085625
```

Gate total : 8/8, zéro warning.

TD07.3.5.2 est clos. Prochaine tranche : TD07.3.5.3 — Monster Presentation Authority.
