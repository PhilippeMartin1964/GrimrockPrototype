# TD07.3.2 — SaveGame Reset / No Backward Migration

Date : **27 août 2026**  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**  
Parent : **TD07.3.1 — Prototype Data Model Policy & Current Schema Asset Audit**  
Baseline GitHub : `745850e3441b7fe2778d1dc1cbc3e8333daf0b90`  
Implémentation : `66a64692323e5c66bf1fcf2658dc9603bf160bb2`  
Correctif de validation : `25e59f4a516dbc7dfde043c0a0dc0d0c66113c29`  
Statut : **VALIDÉ — CLOS**

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

## 9. Validation réelle — 27 août 2026

La validation a été exécutée localement sous Unreal Engine 5.5.4 avec les harness TD04.

### Automation

```text
Filter                                                    Succeeded  With warnings  Failed  Not run
Grimrock.TechnicalDebt.TD07_3_2                           6          0              0       0
Grimrock.MON19.2.Save                                      2          0              0       0
Grimrock.MON20.9.SkillPersistence                          7          0              0       0
Grimrock.RPG.MON16.7                                      10          0              0       0
Grimrock.Magic.MON18.8                                    11          0              0       0
Grimrock.TechnicalDebt.TD01_1.ReceptaclePersistence       0          2              0       0
```

Rapports :

```text
TD07.3.2   D:\Development\GrimrockPrototype\Saved\Automation\TD04\TD04-20260827-162012
MON19.2    D:\Development\GrimrockPrototype\Saved\Automation\TD04\TD04-20260827-162722
MON20.9    D:\Development\GrimrockPrototype\Saved\Automation\TD04\TD04-20260827-162746
MON16.7    D:\Development\GrimrockPrototype\Saved\Automation\TD04\TD04-20260827-162841
MON18.8    D:\Development\GrimrockPrototype\Saved\Automation\TD04\TD04-20260827-163050
TD01.1     D:\Development\GrimrockPrototype\Saved\Automation\TD04\TD04-20260827-163621
```

Le filtre TD01.1 termine avec deux tests classés `Succeeded with warnings`. Les warnings proviennent du fixture transient `TD01_Receptacle` ; le harness termine avec `[OK] Automation filter validated`, `Failed=0` et `Not run=0`.

Une première exécution TD01.1 avait révélé une assertion de test encore figée sur SaveGame v9. Elle a été corrigée dans `25e59f4a516dbc7dfde043c0a0dc0d0c66113c29`, puis le test a été recompilé et rejoué avec succès.

### Shipping

```text
Target        : GrimrockPrototype
Platform      : Win64
Configuration : Shipping
Executable    : D:\Development\GrimrockPrototype\Saved\Packaging\TD04\TD04-Shipping-20260827-163358\Windows\GrimrockPrototype.exe
Pak files     : 1
Archive files : 41
Archive bytes : 906015163
Result        : [OK] Cook / package validated.
```

Archive :

```text
D:\Development\GrimrockPrototype\Saved\Packaging\TD04\TD04-Shipping-20260827-163358
```

## 10. Stop condition — ATTEINTE

TD07.3.2 est clos :

1. ✅ le projet compile sans référence à `RPGSaveMigrationService` ;
2. ✅ aucun `MinimumCompatibleSaveVersion` ne subsiste dans le code courant ;
3. ✅ aucun `ResetLegacyDungeonSnapshots` ne subsiste ;
4. ✅ TD07.3.2 et les régressions de persistance sont vertes ;
5. ✅ Shipping Win64 est vert ;
6. ✅ une nouvelle sauvegarde est v10 et une v9 est rejetée sans migration.

Les 41 findings DataAsset TD07.3.1 restent inchangés et réservés aux tranches TD07.3.4–TD07.3.7.

La tranche suivante est :

```text
TD07.3.3 — Character State Normalization
```

TD07.3.3 n'est pas implémenté par cette clôture.
