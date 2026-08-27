# TD07.3.3.2 — Remove Legacy Attribute Bridge

Date : **27 août 2026**  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**  
Parent : **TD07.3.3 — Character State Normalization**  
Baseline : `32082dac856207d762ad32112ddd7cd3f27fa311`  
Statut : **VALIDÉ — CLOS**

## 1. Objet

TD07.3.3.2 supprime le dernier pont historique entre l'ancien champ de force et le modèle RPG courant.

Le schéma courant possède désormais une seule autorité pour les attributs :

```text
FGridCharacterInventoryState::Attributes
    -> FRPGAttributes
        -> Strength
        -> Dexterity
        -> Constitution
        -> Intelligence
        -> Wisdom
        -> Charisma
```

Aucun champ, marqueur ou fallback historique n'est conservé.

## 2. Suppressions

Supprimés de `FGridCharacterInventoryState` :

```text
bRPGAttributesInitialized
Strength [DeprecatedProperty]
```

Supprimé de `UGridPartyInventoryComponent::InitializeCharacterDefaults()` :

```text
if !bRPGAttributesInitialized
    Strength -> Attributes.Strength
```

Supprimées des créations courantes :

```text
NewCharacter.bRPGAttributesInitialized = true
NewCharacter.Strength = ...

Candidate.bRPGAttributesInitialized = true
Candidate.Strength = ...
```

Les chemins concernés sont :

```text
CreateInitialCharacter
Custom Recruit
Story Companion
Party Recruitment validation
```

## 3. Recrutement

`FRPGPartyRecruitmentService` ne dépend plus d'un marqueur historique pour décider qu'un personnage RPG est initialisé.

La validation conserve uniquement les invariants déjà réellement utilisés par cette tranche :

```text
Level >= 1
Experience >= 0
CharacterId valide
RaceId / ClassId présents
nom valide
hotbar valide
ownership valide
```

TD07.3.3.2 n'introduit pas de nouvelle plage d'attributs et ne change pas le balancing.

## 4. Tests historiques

Le test :

```text
Grimrock.CharacterCreation.CC1.LegacyStrengthMigration
```

est supprimé.

Il est remplacé par :

```text
Grimrock.CharacterCreation.CC1.CurrentAttributeAuthority
```

qui vérifie qu'un `Attributes.Strength=13` reste l'autorité et produit directement la capacité de charge attendue.

Les fixtures MON15 / MON20 touchées n'écrivent plus le champ ou le flag supprimé.

## 5. SaveGame v11

La suppression de deux `UPROPERTY` sérialisés dans `FGridCharacterInventoryState` modifie le schéma SaveGame.

Conformément au contrat TD07.3.2 exact-match :

```text
CurrentSaveVersion = 11
```

Contrat :

```text
SaveVersion == 11
    -> validation du schéma courant
    -> load

SaveVersion != 11
    -> rejet
    -> aucune migration
    -> aucune réécriture de version
```

Une Save v10 est donc volontairement incompatible après TD07.3.3.2.

Le document historique TD07.3.2 reste daté sur la génération v10 qu'il a introduite ; il n'est pas réécrit.

## 6. Nouveau filtre Automation

Ajout :

```text
Grimrock.TechnicalDebt.TD07_3_3_2
```

Tests :

```text
CurrentAttributeAuthority
RecruitmentUsesCurrentAttributes
SaveSchemaVersion
```

Attendu :

```text
Succeeded              : 3
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0
```

## 7. Régressions ciblées requises

À exécuter sous UE5.5.4 :

```powershell
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.TechnicalDebt.TD07_3_3_2"

.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.CharacterCreation.CC1"

.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.RPG.MON15.3"

.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.MON20.2.Recruitment"

.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.MON20.3.StoryCompanion"

.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.MON20.4.RecruitmentUI"

.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.MON20.5.CustomRecruit"

.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.MON20.6.Skills"

.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.MON20.9.ActivePoolPersistence"

.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.TechnicalDebt.TD07_3_2"

.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.RPG.MON16.7"

.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.RPG.MON16.8"

.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.Monsters.MON9"

.\Scripts\ValidatePackage.ps1 -EngineRoot D:\UE_5.5
```

Le premier appel doit compiler le projet ; les suivants peuvent réutiliser ce build.

## 8. Hors périmètre

TD07.3.3.2 ne modifie pas :

```text
DerivedStats
CurrentWeight / MaxCarryWeight
Level / Experience
ClassProgressionStates
SkillRanks / CharacterSkillStates
Spellbook
Status Effects
Pending Level Up
DataAssets
Blueprints
maps
```

Les 41 findings TD07.3.1 restent inchangés.

## 9. Stop condition

TD07.3.3.2 sera clos lorsque :

- [x] `Strength` legacy est supprimé ;
- [x] `bRPGAttributesInitialized` est supprimé ;
- [x] aucun fallback ancien -> courant ne subsiste ;
- [x] création et recrutement utilisent directement `Attributes` ;
- [x] SaveGame courant passe à v11 ;
- [x] Save v10 est rejetée sans migration ;
- [x] compilation UE5.5.4 verte ;
- [x] filtre TD07.3.3.2 vert ;
- [x] régressions ciblées vertes ;
- [x] Shipping Win64 vert.

Prochaine tranche après validation :

```text
TD07.3.3.3 — Normalize Derived Stats / Mutable Resources
```

TD07.3.3.2 est clos. TD07.3.3.3 peut désormais commencer.

## 10. Validation finale du 27 août 2026

Résultats fournis depuis l'environnement UE5.5.4 local :

```text
Development Editor build    OK
TD07.3.3.2                  vert
régressions ciblées         vertes
MON16.7                     10/10
MON16.8                     10/10
warnings Automation         0 sur MON16.8
failed Automation           0
```

Le test MON16.8 `RegressionNamespaceCoverage` a été corrigé après suppression du test legacy CC1 :

```text
MON16.7 frozen count : 11 -> 10
MON16.1-MON16.7 total: 71 -> 70
```

Cette correction est une mise à jour de baseline de test, pas une modification du runtime Status Effects.

Validation Win64 Shipping :

```text
Target        : GrimrockPrototype
Platform      : Win64
Configuration : Shipping
Pak files     : 1
Archive files : 41
Archive bytes : 906014651
Archive       : Saved/Packaging/TD04/TD04-Shipping-20260827-173404
```

Le warning UBT indiquant que Visual Studio 2022 14.44.35227 n'est pas la version préférée reste la baseline toolchain documentée par TD07.1 ; il n'a empêché ni le build Editor ni le Shipping.

Commits de validation associés :

```text
05445f57311639ca0a624c54c699d7934da9047b  Fix MON16.8 frozen regression baseline
129b2e35eac39bcbb6b467fc9ac0b98004539782  Add concise UE automation summaries
```

La stop condition de TD07.3.3.2 est atteinte.
