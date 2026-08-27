# TD07.3.1 — Prototype Data Model Policy & Current Schema Asset Audit

Date : **27 août 2026**  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**  
Baseline GitHub : `11d7609e1c81e844bec6cb0c0c0dc82c14d8fd3b`  
Statut : **IMPLÉMENTÉ — VALIDATION UE / RAPPORT LOCAL REQUIS**

## 1. Décision autoritaire

Tant que GrimrockPrototype est en phase **prototype**, aucune compatibilité arrière n'est exigée.

Cela s'applique à :

- SaveGames ;
- DataAssets ;
- LevelAssets / DungeonAssets ;
- Blueprints ;
- maps ;
- propriétés UPROPERTY renommées ou supprimées ;
- APIs Blueprint/C++ historiques ;
- formats d'état runtime devenus obsolètes.

Git est l'unique mécanisme de conservation de l'historique du prototype.

Une donnée ancienne incompatible peut être supprimée ou recréée. Aucune couche de migration, champ legacy, fallback d'ancien schéma ou duplication d'autorité ne doit être conservé uniquement pour permettre de relire un état antérieur du prototype.

Cette politique sera réévaluée **uniquement lorsque le prototype sera déclaré validé et que le projet entrera en phase de développement produit**.

## 2. Frontières de données cibles

### Données d'auteur

```text
DataAsset / LevelAsset
    -> références directes vers les définitions
    -> pas de paire DefinitionAsset + DefinitionId représentant la même autorité
```

### Runtime / Save

```text
état mutable
    -> identités stables
    -> valeurs réellement mutables
    -> aucune donnée de présentation calculable
    -> aucun pointeur DataAsset persistant
```

### Données dérivées

Les données recalculables sont des projections, pas des autorités persistantes.

Exemples déjà identifiés :

```text
CurrentWeight
MaxCarryWeight
ClassDisplayName
RaceDisplayName
ClassIcon
RequirementIds calculés
```

La séparation future de `FRPGDerivedStats` devra également distinguer les statistiques calculées de `CurrentHealth / CurrentMana`, qui sont des ressources réellement mutables.

## 3. Dette de schéma déjà caractérisée

### Save / migration

```text
CurrentSaveVersion = 9
MinimumCompatibleSaveVersion = 1
FRPGSaveMigrationService
v1-v8 migration paths
ResetLegacyDungeonSnapshots
bLevelVariablesInitialized legacy distinction
```

Cible TD07.3.2 : un schéma prototype courant exact-match, sans migration arrière.

### Character state

```text
bRPGAttributesInitialized
Strength deprecated
CurrentWeight persisté alors que dérivé
MaxCarryWeight persisté alors que dérivé
snapshots séparés progression / skills / spellbook / status effects
```

Cible TD07.3.3 : une autorité personnage canonique, sans doubles représentations runtime/save inutiles.

### Authoring Asset + Id

Cas identifiés :

```text
FGridLevelObjectData
    ItemDefinitionAsset + ItemDefinitionId
    ReadableContentAsset + ReadableContentId
    MonsterDefinitionAsset + MonsterDefinitionId

FGridItemBehaviorParams
    ItemDefinitionAsset + ItemDefinitionId
    DefaultReadableContentAsset + DefaultReadableContentId

FGridLockBehaviorParams
    AcceptedKeyItems + AcceptedKeyIds

FGridMonsterLootEntry
    ItemDefinitionAsset + ItemDefinitionId
```

Cible : référence DataAsset dans l'authoring ; ID stable dans runtime/save.

### Combat item legacy

```text
bProvidesAttack
OffensiveProfile
MakeLegacyEquipmentAttackDefinition
fallback CombatActions -> legacy profile
```

Cible TD07.3.5 : `CombatActions` seule autorité item/combat.

### Monster presentation legacy

```text
AttackSound -> fallback vers AttackAudio
ImpactVFX   -> fallback vers ImpactHitVFXDefinition
RangeCells  -> nom conservé uniquement pour compatibilité sérialisée
```

Cible TD07.3.5 :

```text
AttackAudio
ImpactHitAudio / ImpactMissAudio
AttackVFXDefinition
ImpactHitVFXDefinition / ImpactMissVFXDefinition
MaxRangeCells
```

### Placement / API legacy

```text
bPlaceOnEdge
bPlaceAtCellCenter
GetFacingForLegacyYaw
HasCharacterCommittedAttackThisPhase
bEnableLegacyKeyboardUseAction
EGridRuntimeRebuildMode::ObjectsOnly
```

Cible TD07.3.6 : suppression après caractérisation des usages courants.

## 4. Audit automatisé du contenu courant

Nouveau test Editor :

```text
Grimrock.TechnicalDebt.TD07_3_1.CurrentSchemaAssetAudit
```

Fichier :

```text
Source/GrimrockPrototypeEditor/Private/Tests/GridEditorTD0731CurrentSchemaAssetAuditTests.cpp
```

Le test utilise l'Asset Registry et scanne **tous les UDataAsset sous /Game**, y compris leurs sous-classes.

Il génère ensuite un rapport de caractérisation :

```text
Saved/Diagnostics/TD07/TD07_3_1_CurrentSchemaAssetAudit.txt
```

Catégories :

```text
Conflict            données concurrentes incohérentes
LegacyOnly          contenu dépendant uniquement d'un ancien chemin
DuplicateAuthority  Asset + Id ou ancien + nouveau système simultanés
LegacyField         champ historique encore renseigné
SchemaRename        donnée valide mais propriété destinée à être renommée
```

Le test **ne considère pas ces findings comme des warnings Automation** : leur présence est précisément l'objet de TD07.3.1.

Il échoue uniquement si le scan ne trouve aucun DataAsset, si un DataAsset annoncé par l'Asset Registry ne peut pas être chargé ou si le rapport ne peut pas être écrit.

## 5. Périmètre du rapport TD07.3.1

Le premier audit automatique couvre explicitement :

- paires authoring Item Asset + Id ;
- paires Readable Asset + Id ;
- paires Monster Asset + Id ;
- `AcceptedKeyIds` ;
- miroirs legacy de `PlacementKind` ;
- dépendance item `bProvidesAttack / OffensiveProfile` ;
- `AttackSound` monstre ;
- `ImpactVFX` monstre ;
- `RangeCells` monstre à renommer ;
- loot Item Asset + Id ;
- miroir MonsterSpawn `InitialFacing / LocalYaw`.

Le rapport compte en plus **toutes les classes DataAsset réellement présentes**, ce qui permet de vérifier que l'audit ne se limite pas aux seuls types historiques connus.

## 6. Validation locale

```powershell
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.TechnicalDebt.TD07_3_1"
```

Attendu :

```text
Succeeded              : 1
Succeeded with warnings: 0
Failed                 : 0
```

Puis :

```powershell
Get-Content .\Saved\Diagnostics\TD07\TD07_3_1_CurrentSchemaAssetAudit.txt
```

Le contenu du rapport doit être conservé comme baseline locale de TD07.3.1. Il guidera les suppressions TD07.3.2 à TD07.3.7.

## 7. Sous-tranches TD07.3

```text
TD07.3.1  Data Model Policy & Current Schema Asset Audit        EN COURS
TD07.3.2  SaveGame Reset / no backward migration               À FAIRE
TD07.3.3  Character State Normalization                        À FAIRE
TD07.3.4  Authoring Identity Normalization                     À FAIRE
TD07.3.5  Combat Data Schema Reset                             À FAIRE
TD07.3.6  Remaining Legacy API/Data Purge                      À FAIRE
TD07.3.7  Current Asset Repair / Recreation                    À FAIRE
TD07.3.8  Strict Current-Schema Validation / stop condition    À FAIRE
```

## 8. Stop condition TD07.3.1

TD07.3.1 est validé lorsque :

1. l'Automation Editor est verte ;
2. le rapport local est généré ;
3. le nombre et les catégories de findings sont connus ;
4. aucune suppression de schéma n'a encore été faite ;
5. TD07.3.2 peut être planifié depuis des données réelles, et non depuis des suppositions.
