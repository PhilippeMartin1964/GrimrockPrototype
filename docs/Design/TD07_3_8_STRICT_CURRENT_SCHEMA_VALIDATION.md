# TD07.3.8 — Strict Current-Schema Validation / Stop Condition

Date : 28 août 2026
Projet : GrimrockPrototype — Unreal Engine 5.5.4
Parent : TD07.3 — Prototype Data Model Reset
Statut : STRICT GATE PREPARED — À VALIDER

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

- [ ] StrictCurrentSchema 5/5 ;
- [ ] TD07.3.2 SaveContract vert ;
- [ ] TD07.3.4.4 Normalization vert ;
- [ ] TD07.3.5 normalizations vertes ;
- [ ] TD07.3.6 Normalization vert ;
- [ ] TD07.3.7 Normalization vert ;
- [ ] CurrentSchemaAssetAudit vert ;
- [ ] régressions structurantes vertes ;
- [ ] Shipping Win64 vert.

Après cette stop condition, MON21.4 pourra reprendre sous le schéma prototype courant exact-match.
