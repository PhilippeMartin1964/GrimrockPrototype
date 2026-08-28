# TD07.3.8 — Strict Current-Schema Validation / Stop Condition

Date : 28 août 2026
Projet : GrimrockPrototype — Unreal Engine 5.5.4
Parent : TD07.3 — Prototype Data Model Reset
Statut : VALIDÉ — TD07.3 STOP CONDITION ATTEINTE

## 1. Objectif

Figer un gate transversal durable pour le schéma prototype courant après TD07.3.2 à TD07.3.7.

TD07.3.8 ne crée aucune nouvelle migration, aucun nouveau modèle de données et aucun one-shot. Il vérifie que les contrats nettoyés ne peuvent pas régresser silencieusement.

## 2. Gate autoritaire

Filtre :

```text
Grimrock.TechnicalDebt.TD07_3_8.StrictCurrentSchema
```

Sous-tests :

```text
SaveExactMatch
CharacterAuthority
LegacySymbolsAbsent
CurrentAssets
MonsterAssets
```

## 3. Contrats figés

### Save

- génération prototype courante au moins v22 ;
- exact-match uniquement ;
- version précédente rejetée ;
- version future rejetée ;
- aucune réécriture silencieuse de version ;
- aucune migration historique.

### Character State

Autorités durables :

```text
ClassId
RaceId
Experience
LastAcknowledgedLevel
SelectedClassProgressionChoiceIds
Attributes
Resources
SkillRanks
KnownSpellIds
StatusEffects
PortraitGender
PortraitVariantId
```

Projections transient :

```text
ClassDisplayName
ClassDefinition
RaceDisplayName
Level
DerivedStats
Portrait
ClassIcon
```

### Legacy schema

Le gate exige notamment l'absence de :

```text
bPlaceOnEdge
bPlaceAtCellCenter
bProvidesAttack
item-level OffensiveProfile
HasCharacterCommittedAttackThisPhase
bEnableLegacyKeyboardUseAction
UseAction
EGridRuntimeRebuildMode::ObjectsOnly
AttackSound
ImpactVFX
MonsterAttack.RangeCells
```

### Current authoring assets

Tous les `UDataAsset` sous `/Game` sont chargés et aucun candidat TD07.3.7 n'est accepté :

```text
AUTHORING.DEFINITION_WITHOUT_ID
AUTHORING.ID_ONLY
AUTHORING.ASSET_ID_CONFLICT
AUTHORING.ASSET_ID_DUPLICATE
AUTHORING.LOCK_KEY_IDS
ITEM.INVALID_COMBAT_ACTIONS
```

### Monster production assets

- `DA_GridLevel_00` charge ;
- MonsterSpawn utilise `MonsterDefinitionAsset` sans miroir `MonsterDefinitionId` ;
- facing cardinal ;
- RatGiant et GoblinThrower valides ;
- schéma MinRangeCells/MaxRangeCells valide.

## 4. Stop condition TD07.3

TD07.3 pourra être clos lorsque :

- [x] StrictCurrentSchema 5/5 ;
- [x] TD07.3.2 SaveContract vert ;
- [x] TD07.3.4.4 Normalization vert ;
- [x] TD07.3.5 normalizations vertes ;
- [x] TD07.3.6 Normalization vert ;
- [x] TD07.3.7 Normalization vert ;
- [x] CurrentSchemaAssetAudit vert ;
- [x] régressions structurantes vertes ;
- [x] Shipping Win64 vert.

Après cette stop condition, MON21.4 pourra reprendre sous le schéma prototype courant exact-match.


## 5. Strict gate validé

Validation locale du 28 août 2026 :

```text
Grimrock.TechnicalDebt.TD07_3_8.StrictCurrentSchema
Succeeded              : 5
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0
Report                 : Saved/Automation/TD04/TD04-20260828-104937
Process exit code       : 0
```

Le gate strict est donc validé. Il reste la campagne finale de stop condition TD07.3 avant clôture de la phase.

Campagne finale autoritaire :

```text
Grimrock.TechnicalDebt.TD07_3_3_10.Normalization     4 tests
Grimrock.TechnicalDebt.TD07_3_2.SaveContract        6 tests
Grimrock.TechnicalDebt.TD07_3_4_4.Normalization     4 tests
Grimrock.TechnicalDebt.TD07_3_5_2.Normalization     4 tests
Grimrock.TechnicalDebt.TD07_3_5_3.Normalization     4 tests
Grimrock.TechnicalDebt.TD07_3_5_4.Normalization     4 tests
Grimrock.TechnicalDebt.TD07_3_6.Normalization       4 tests
Grimrock.TechnicalDebt.TD07_3_7.Normalization       4 tests
Grimrock.TechnicalDebt.TD07_3_1.CurrentSchemaAssetAudit
```

Soit 34 tests de normalisation ciblés + l'audit courant, en plus des 5 tests StrictCurrentSchema déjà validés.

La stop condition exige ensuite un Win64 Shipping réussi.


## 6. Campagne finale et clôture TD07.3

Validation locale du 28 août 2026 :

```text
Grimrock.TechnicalDebt.TD07_3_3_10.Normalization
    Succeeded : 4 / Failed : 0

Grimrock.TechnicalDebt.TD07_3_2.SaveContract
    Succeeded : 6 / Failed : 0

Grimrock.TechnicalDebt.TD07_3_4_4.Normalization
    Succeeded : 4 / Failed : 0

Grimrock.TechnicalDebt.TD07_3_5_2.Normalization
    Succeeded : 4 / Failed : 0

Grimrock.TechnicalDebt.TD07_3_5_3.Normalization
    Succeeded : 4 / Failed : 0

Grimrock.TechnicalDebt.TD07_3_5_4.Normalization
    Succeeded : 4 / Failed : 0

Grimrock.TechnicalDebt.TD07_3_6.Normalization
    Succeeded : 4 / Failed : 0

Grimrock.TechnicalDebt.TD07_3_7.Normalization
    Succeeded : 4 / Failed : 0

Grimrock.TechnicalDebt.TD07_3_1.CurrentSchemaAssetAudit
    Succeeded : 1 / Failed : 0
```

Soit **34 tests de normalisation + 1 audit**, tous verts, en complément du gate :

```text
Grimrock.TechnicalDebt.TD07_3_8.StrictCurrentSchema
    Succeeded : 5 / Failed : 0
```

Le Shipping Win64 validé immédiatement avant le gate strict reste applicable au runtime/content courant :

```text
BUILD SUCCESSFUL
Cook: Success - 0 error(s), 0 warning(s)
Pak files     : 1
Archive files : 41
Archive bytes : 905984939
```

**TD07.3 — Prototype Data Model Reset a atteint sa stop condition et est clos.**

Conséquence fonctionnelle : **MON21.4 — Quest Persistence** peut reprendre sur le schéma prototype courant exact-match, sans migration arrière.
