# TD07.3.5.5 — Regressions / Shipping / Closure

Date : 28 août 2026
Projet : GrimrockPrototype — Unreal Engine 5.5.4
Parent : TD07.3.5 — Combat Data Schema Reset
Statut : VALIDÉ — CLOS / TD07.3.5 CLOS

## 1. Objectif

Fermer TD07.3.5 avec le schéma combat courant uniquement :

- items : `CombatActions` autoritaire ;
- monster presentation : définitions Audio/VFX courantes uniquement ;
- monster range : `MinRangeCells + MaxRangeCells` ;
- aucun adapter ou champ legacy conservé.

## 2. Régressions historiques découvertes

Un filtre trop large `Grimrock.Monsters.MON1` a exécuté aussi MON11/MON12 et révélé six tests historiques rouges.

Ils n'indiquaient pas un défaut du runtime courant. Les fixtures exprimaient encore des contrats antérieurs :

- validation offensive déduite du type de slot au niveau de l'item ;
- indisponibilité d'une action équipement simulée via `PlayerAttackActionPointCost` ;
- portrait injecté directement dans le cache transient ;
- Save memory round-trip avec `ClassId=Warrior` synthétique non enregistré dans le resolver.

## 3. Normalisation des fixtures

Les tests sont réalignés sur les autorités TD07.3 actuelles :

- `CanProvideAttackFromSlot()` vérifie l'autorité main/off-hand ;
- les tests de coût modifient le `ActionPointCost` du `FGridCombatActionDefinition` enregistré ;
- les portraits synthétiques utilisent `RaceId + PortraitGender + PortraitVariantId` et `FRPGAuthoringIdentityResolver` ;
- le round-trip Save v22 enregistre une classe synthétique résoluble avant désérialisation ;
- les mentions "legacy shuriken adapter" sont remplacées par le contrat `CombatActions` courant.

Aucun changement runtime/gameplay n'est introduit par cette sous-étape.

## 4. Validation suivante

Valider d'abord les six tests précédemment rouges, puis les régressions CombatActions/monster ciblées. Si elles sont vertes, exécuter le packaging Win64 Shipping avant clôture de TD07.3.5.


## 5. Validation des fixtures normalisées

Validation locale du 28 août 2026 :

```text
Grimrock.Monsters.MON11.OffensiveProfileValidation
    1/1, warnings 0, failed 0
    Report: Saved/Automation/TD04/TD04-20260828-094344

Grimrock.Monsters.MON12.11.HotbarValidation
    1/1, warnings 0, failed 0
    Report: Saved/Automation/TD04/TD04-20260828-094357

Grimrock.Monsters.MON12.8.1.SaveMemoryRoundTrip
    1/1, warnings 0, failed 0
    Report: Saved/Automation/TD04/TD04-20260828-094410

Grimrock.Monsters.MON12.ActionCatalog.GenericAttackLifecycle
    1/1, warnings 0, failed 0
    Report: Saved/Automation/TD04/TD04-20260828-094422

Grimrock.Monsters.MON12.CombatActionPanel.LiveData
    1/1, warnings 0, failed 0
    Report: Saved/Automation/TD04/TD04-20260828-094435

Grimrock.Monsters.MON12.CombatHUD.Lifecycle
    1/1, warnings 0, failed 0
    Report: Saved/Automation/TD04/TD04-20260828-094447
```

Les six régressions historiques sont désormais réalignées sur les contrats courants et vertes.

Gates TD07.3.5 revalidés dans la même campagne :

```text
Grimrock.TechnicalDebt.TD07_3_5_2.Normalization
    4/4, warnings 0, failed 0
    Report: Saved/Automation/TD04/TD04-20260828-094500

Grimrock.TechnicalDebt.TD07_3_5_3.Normalization
    4/4, warnings 0, failed 0
    Report: Saved/Automation/TD04/TD04-20260828-094512

Grimrock.TechnicalDebt.TD07_3_5_4.Normalization
    4/4, warnings 0, failed 0
    Report: Saved/Automation/TD04/TD04-20260828-094525

Grimrock.TechnicalDebt.TD07_3_5.Characterization
    4/4, warnings 0, failed 0
    Report: Saved/Automation/TD04/TD04-20260828-094538
```

## 6. Campagne finale de clôture

La campagne finale TD07.3.5 doit maintenant couvrir les domaines directement affectés par le reset du schéma combat :

```text
Grimrock.Monsters.MON10
Grimrock.Monsters.MON11
Grimrock.Monsters.MON12
Grimrock.Monsters.MON17
Grimrock.TechnicalDebt.TD07_3_5
Grimrock.TechnicalDebt.TD07_3_1.CurrentSchemaAssetAudit
```

Les deux tests MON1 directement liés aux définitions monster ont déjà été validés séparément pendant TD07.3.5.4 :

```text
Grimrock.Monsters.MON1.DefinitionValidation   1/1
Grimrock.Monsters.MON1.InvalidData            1/1
```

Après régressions vertes, exécuter :

```powershell
.\Scripts\ValidatePackage.ps1 -EngineRoot D:\UE_5.5
```

Stop condition TD07.3.5 :

- [x] Item CombatActions Authority normalisé ;
- [x] Monster Presentation Authority normalisé ;
- [x] Monster Range Schema normalisé ;
- [x] fixtures MON11/MON12 historiques réalignées ;
- [x] gates TD07.3.5 ciblés verts ;
- [ ] campagne finale combat verte ;
- [ ] Win64 Shipping vert ;
- [ ] documentation de clôture finale ;
- [ ] TD07.3.5 clos.


## 7. Campagne finale validée

Validation locale finale du 28 août 2026 :

```text
Grimrock.Monsters.MON10
    Succeeded              : 36
    Succeeded with warnings: 1
    Failed                 : 0
    Report                 : Saved/Automation/TD04/TD04-20260828-094801

Grimrock.Monsters.MON11
    Succeeded              : 18
    Succeeded with warnings: 3
    Failed                 : 0
    Report                 : Saved/Automation/TD04/TD04-20260828-094814

Grimrock.Monsters.MON12
    Succeeded              : 49
    Succeeded with warnings: 0
    Failed                 : 0
    Report                 : Saved/Automation/TD04/TD04-20260828-094827

Grimrock.Monsters.MON17
    Succeeded              : 43
    Succeeded with warnings: 1
    Failed                 : 0
    Report                 : Saved/Automation/TD04/TD04-20260828-094840

Grimrock.TechnicalDebt.TD07_3_5
    Succeeded              : 16
    Succeeded with warnings: 0
    Failed                 : 0
    Report                 : Saved/Automation/TD04/TD04-20260828-094853

Grimrock.TechnicalDebt.TD07_3_1.CurrentSchemaAssetAudit
    Succeeded              : 1
    Succeeded with warnings: 0
    Failed                 : 0
    Report                 : Saved/Automation/TD04/TD04-20260828-094905
```

Les suites MON10, MON11 et MON17 conservent quelques tests "Succeeded with warnings" historiques, mais aucun test n'échoue et tous les filtres retournent un code processus 0.

## 8. Win64 Shipping final

```text
Target        : GrimrockPrototype
Platform      : Win64
Configuration : Shipping
Executable    : Saved/Packaging/TD04/TD04-Shipping-20260828-094929/Windows/GrimrockPrototype.exe
Pak files     : 1
Archive files : 41
Archive bytes : 905992699
Archive       : Saved/Packaging/TD04/TD04-Shipping-20260828-094929
BUILD SUCCESSFUL
[OK] Cook / package validated.
```

Le cook termine explicitement avec :

```text
Success - 0 error(s), 0 warning(s)
AutomationTool exiting with ExitCode=0 (Success)
```

## 9. Stop condition atteinte

- [x] Item CombatActions Authority normalisé ;
- [x] Monster Presentation Authority normalisé ;
- [x] Monster Range Schema normalisé ;
- [x] fixtures MON11/MON12 historiques réalignées ;
- [x] gates TD07.3.5 ciblés verts ;
- [x] campagne finale combat verte ;
- [x] Win64 Shipping vert ;
- [x] documentation de clôture finale ;
- [x] TD07.3.5 clos.

La tranche suivante est **TD07.3.6 — Remaining Legacy API/Data Purge**.
