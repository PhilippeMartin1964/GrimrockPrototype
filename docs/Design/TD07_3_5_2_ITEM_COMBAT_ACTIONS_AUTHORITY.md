# TD07.3.5.2 — Item CombatActions Authority

Date : 28 août 2026
Projet : GrimrockPrototype — Unreal Engine 5.5.4
Parent : TD07.3.5 — Combat Data Schema Reset
Characterization : 4/4 validée
Statut : ASSET REPAIR PREPARED — SHURIKEN LFS REPAIR REQUIRED BEFORE SCHEMA DELETION

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
- [ ] Automation asset repair verte localement
- [ ] repaired DA_Weapon_Shuriken pushed
- [ ] legacy schema C++ supprimé
- [ ] runtime fallback supprimé
- [ ] normalization tests verts
