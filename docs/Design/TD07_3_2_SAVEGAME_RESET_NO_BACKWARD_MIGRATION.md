# TD07.3.2 — SaveGame Reset / No Backward Migration

Date : **27 août 2026**  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**  
Parent : **TD07.3.1 — Prototype Data Model Policy & Current Schema Asset Audit**  
Baseline GitHub : `745850e3441b7fe2778d1dc1cbc3e8333daf0b90`  
Statut : **IMPLÉMENTÉ — VALIDATION UE / SHIPPING REQUISE**

## 1. Objet

TD07.3.2 supprime toute compatibilité historique de sauvegarde pendant la phase prototype.

Cette tranche ne modifie aucun DataAsset, Blueprint, map ou donnée d'auteur. Les 41 findings TD07.3.1 restent intacts pour TD07.3.4–TD07.3.7.

## 2. Nouveau contrat Save

Le schéma prototype est désormais exact-match :

```text
CurrentSaveVersion = 10

SaveVersion == 10
    -> validation du schéma courant
    -> restore

SaveVersion != 10
    -> rejet
    -> aucune migration
    -> aucune mutation du snapshot
```

La version 10 constitue volontairement une nouvelle génération afin qu'une ancienne sauvegarde v9 ne puisse pas être acceptée par hasard après le changement de politique.

Les futures modifications de schéma pendant le prototype pourront incrémenter 10 -> 11 -> 12, sans chemin de migration arrière.

## 3. Infrastructure supprimée

```text
MinimumCompatibleSaveVersion
FRPGSaveMigrationService
FRPGSaveMigrationReport
RPGSaveMigrationService.h/.cpp
PrepareLoadedSave()
ResetLegacyDungeonSnapshots()
RPGMON156SaveMigrationTests.cpp
```

Les branches historiques v1-v8 disparaissent avec leurs helpers :

- reconstruction Level/XP legacy ;
- reset Status Effects legacy ;
- reset SkillRanks legacy ;
- reset variables de niveau legacy ;
- initialisation de permission de réceptacle legacy ;
- rapports SourceVersion/TargetVersion/Migrated.

## 4. Validation courante conservée

La partie utile de l'ancien service n'est pas supprimée : elle est déplacée vers :

```cpp
UGrimrockPartySaveGame::ValidateCurrentState(FText& OutError) const
```

Elle valide sans mutation :

- version exacte ;
- cohérence Level / Experience ;
- CharacterId actifs ;
- progression de classe persistée ;
- notifications Level Up ;
- Spellbook snapshot ;
- Skill snapshot ;
- variables de niveau.

La restauration Status Effects, progression, Skills et Level Up reste ensuite atomique selon les services existants.

## 5. Lifecycle

### Save

```text
capture état courant
-> SaveVersion = 10
-> ValidateCurrentState()
-> Serialize
```

### Load

```text
Deserialize
-> ValidateCurrentState()
-> si erreur : bLoadValid=false
-> sinon restore des domaines runtime
```

Les anciens noms internes `bProgressionLoadValid / ProgressionLoadError` deviennent :

```text
bLoadValid
LoadError
IsLoadValid()
GetLoadError()
```

car la validation concerne maintenant l'ensemble du SaveGame.

## 6. Tests supprimés / adaptés

Sont supprimés les tests de migration historique :

```text
LegacyExperienceAheadMigration
LegacyStoredLevelAheadMigration
V4MigrationPreservesProgression
V5MigrationCreatesEmptySpellbook
V6ToV7LevelVariables
V7ToV8Migration
V8Migration
MinimumCompatibleSaveVersion == 1
```

Les tests fonctionnels non liés à la migration restent présents.

Le test MON15.4 nommé `LegacyCompatibility` est renommé pour décrire son vrai contrat : une classe sans ProgressionChoices reste valide. Ce comportement fonctionnel n'est pas une compatibilité Save.

## 7. Nouveau contrat Automation

Nouveau fichier :

```text
Source/GrimrockPrototype/Private/Tests/GridTD0732PrototypeSaveContractTests.cpp
```

Filtre :

```text
Grimrock.TechnicalDebt.TD07_3_2
```

Il couvre six contrats :

1. version exacte v10 ;
2. v9 rejetée sans mutation ;
3. v11 rejetée sans mutation ;
4. progression choix round-trip ;
5. Level/XP courant incohérent rejeté ;
6. choix/pending Level Up invalides et pending Level Up round-trip.

## 8. Hors périmètre volontaire

TD07.3.2 ne supprime pas encore :

- les snapshots séparés Skills / Spellbook / Status Effects ;
- les données dérivées du personnage ;
- `bLevelVariablesInitialized`, encore utilisé par le lifecycle runtime courant ;
- la tolérance Spellbook envers un SpellId non résolu ;
- les 41 findings DataAsset de TD07.3.1.

Ces points relèvent de TD07.3.3 et des tranches suivantes.

## 9. Validation requise

### Contrat TD07.3.2

```powershell
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.TechnicalDebt.TD07_3_2"
```

Attendu :

```text
Succeeded              : 6
Succeeded with warnings: 0
Failed                 : 0
```

### Régressions de persistance

```powershell
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.MON19.2.Save"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.MON20.9.SkillPersistence"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.RPG.MON16.7"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.Magic.MON18.8"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.TechnicalDebt.TD01_1.ReceptaclePersistence"
```

### Shipping

```powershell
.\Scripts\ValidatePackage.ps1 -EngineRoot D:\UE_5.5
```

## 10. Stop condition

TD07.3.2 est clos lorsque :

1. le projet compile sans référence à `RPGSaveMigrationService` ;
2. aucun `MinimumCompatibleSaveVersion` ne subsiste dans le code courant ;
3. aucun `ResetLegacyDungeonSnapshots` ne subsiste ;
4. TD07.3.2 et les régressions de persistance sont vertes ;
5. Shipping est vert ;
6. une nouvelle sauvegarde est v10 et une ancienne v9 est rejetée.

La tranche suivante devient alors :

```text
TD07.3.3 — Character State Normalization
```
