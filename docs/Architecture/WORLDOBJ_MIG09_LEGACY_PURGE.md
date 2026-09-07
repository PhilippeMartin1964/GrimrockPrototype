# WORLDOBJ-MIG09 — Purge des compatibilités legacy

Statut : **MIG09-A à MIG09-C validés ; MIG09-D1 candidat — validation locale UE5.5.4 requise ; MIG09-D2/D3 et MIG09-E à faire**.

Date de mise à jour : 2026-09-07.

## 1. But de MIG09

MIG09 supprime physiquement les ponts temporaires qui ont permis de migrer progressivement le système d’objets du monde. Le projet étant encore un prototype, ces ponts ne doivent pas devenir une couche de compatibilité permanente.

Règle :

```text
une donnée cible existe
        -> les consommateurs sont migrés
        -> le legacy est supprimé physiquement
        -> un test empêche sa réapparition
```

## 2. Tranches validées

| Tranche | État | Résultat |
|---|---|---|
| MIG09-A | ✅ validé | Le runtime ne dépend plus du marqueur sparse pré-MIG08 pour choisir l’autorité du comportement. |
| MIG09-B1 | ✅ validé | `AGridItemActor::ArchetypeId` supprimé. |
| MIG09-B2A | ✅ validé | Miroirs `ItemArchetypeId` des réceptacles supprimés. |
| MIG09-B2B1 | ✅ validé | `FGridRuntimeItemState::ArchetypeId` retiré du SaveGame. |
| MIG09-B2B2 | ✅ validé | Consommateurs SaveGame migrés vers `ItemDefinitionId`. |
| MIG09-B2B3 | ✅ validé | Proxy/cache runtime `ItemArchetypeId` supprimés. |
| MIG09-B2C-A | ✅ validé | Audit Blueprint des anciens aliases Item. |
| MIG09-B2C-B | ✅ validé | `InitializeItem()` et `GetItemArchetypeId()` supprimés. |
| MIG09-C | ✅ validé | Initializers mécanismes et schéma d’animation spécialisé supprimés ; `MovingParts[].Motion` reste l’autorité visuelle. |

Validation locale de clôture de MIG09-C :

```text
Build                  : OK
Succeeded              : 34
Succeeded with warnings: 1
Failed                 : 0
Not run                : 0
Process exit code       : 0
```

Rapport :

```text
D:\Development\GrimrockPrototype\Saved\Automation\TD04\TD04-20260907-125212
```

Le warning Visual Studio 2022 sur la version de compilateur préférée est un warning de toolchain distinct de l’Automation.

Le filtre de validation de référence reste :

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.WorldObjects"
```

## 3. Autorité Definition / Instance après MIG09-A

Lorsqu’une définition d’objet du monde est disponible :

```text
Definition.DefaultBehavior
        +
overrides strictement propres à l’instance
        =
comportement effectif
```

Les données naturellement propres au niveau restent notamment :

- destination de téléporteur ;
- transition ;
- état/configuration de Pit propre au niveau ;
- contenu initial d’un réceptacle ;
- état initial d’une serrure ;
- autres données locales explicitement typées.

Une instance ne doit pas recopier la géométrie ou la présentation d’un mécanisme.

## 4. MIG09-C — clôture des mécanismes legacy

MIG09-C est validé.

### 4.1. Anciens initializers

L’audit Blueprint `Grimrock.WorldObjects.MIG09.MechanismLegacyBlueprintReferences` protège les noms historiques :

```text
InitializeButton
InitializeLever
InitializeDoor
```

L’état source courant ne contient plus ces API spécialisées dans les acteurs de production.

Le helper :

```text
GridDoorTestUtils::InitializeDoorFromMotion(...)
```

n’est pas un initializer runtime legacy. C’est un utilitaire de fixture automatisée qui construit explicitement une définition de motion pour tester `AGridDoorActor::InitializeGridObject()`.

Le Pit utilise le chemin générique :

```text
InitializeMechanismVisuals()
InitializeGridObject()
```

### 4.2. Autorité unique de l’animation visuelle

La géométrie et la durée d’un mécanisme appartiennent exclusivement à :

```text
UGridObjectArchetypeAsset
└── MovingParts[]
    └── Motion
        ├── Type
        ├── Axis
        ├── Pivot
        ├── Amount
        └── Duration
```

Les anciens champs suivants sont supprimés physiquement du contrat `Behavior` :

```text
FGridPitAnimationParams
FGridButtonAnimationParams.ButtonPressDistance
FGridButtonAnimationParams.ButtonPressDuration
FGridButtonAnimationParams.ButtonReleaseDuration
FGridLeverAnimationParams
FGridPressurePlateAnimationParams
FGridDoorAnimationParams.OpenHeight
FGridDoorAnimationParams.MoveDuration
```

`AGridDoorActor::OpenHeight`, qui n’était plus qu’un cache `Transient` sans autorité, est également supprimé.

Les caches runtime réellement nécessaires à l’exécution restent autorisés. Par exemple `AGridDoorActor::MoveDuration` est une valeur runtime résolue depuis `MovingParts[].Motion.Duration` ; ce n’est pas une seconde donnée d’authoring.

### 4.3. Données gameplay conservées

Le bouton conserve `ButtonAnimation.ButtonHoldTime` car il s’agit d’un délai logique de maintien.

La porte conserve :

```text
DoorAnimation.bHasChainMechanism
DoorAnimation.ChainPullDistance
DoorAnimation.ChainPullDuration
```

La plaque de pression conserve ses règles de poids.

Le Pit conserve notamment :

```text
Pit.bInitiallyOpen
Pit.bUseSameCellCoordinates
Transition
```

## 5. MIG09-D — derniers consommateurs de compatibilité

MIG09-D est volontairement découpé afin de supprimer chaque autorité historique sans mélanger plusieurs axes de risque.

### MIG09-D1 — suppression du marqueur sparse historique

Candidat actuel.

Le stockage suivant est supprimé physiquement de `UGridLevelAsset` :

```text
SparseBehaviorOverrideObjectIds
```

Ce marqueur n’a plus d’autorité depuis MIG09-A. Le caractère sparse d’un objet réutilisable est désormais structurel :

```text
FGridWorldObjectInstance
        -> sparse par construction
```

En mode legacy temporaire, `UsesSparseBehaviorOverrides()` ne consulte plus aucun état de migration sérialisé ; il déduit simplement si l’objet appartient au bucket `WorldObject`.

`SetSparseBehaviorOverrides()` reste provisoirement un no-op uniquement pour permettre de migrer proprement les derniers call sites du Grid Editor dans MIG09-D2. Il ne stocke plus aucune donnée.

Le test :

```text
Grimrock.WorldObjects.MIG09.SparseMarkerPurge
```

protège l’absence réfléchie de `SparseBehaviorOverrideObjectIds` et la sémantique structurelle world-object / loose-item.

### MIG09-D2 — suppression du write-through de compatibilité

À traiter après validation de D1 :

```text
SetSparseBehaviorOverrides()
CommitCompatibilityObjectEdit()
RefreshLegacyObjectMirrorFromTyped()
GetObjectCompatibilityView()
```

Le Grid Editor devra écrire directement dans les collections typées au lieu de modifier le miroir `Objects` puis de recopier cette modification.

### MIG09-D3 — suppression des conversions de compatibilité restantes

À traiter ensuite :

```text
GridLevelPlacementCompatibility
```

Les validations et outils de migration encore basés sur `FGridLevelObjectData` devront consommer les placements typés directement.

## 6. MIG09-E — suppression du modèle objet générique historique

Dernière étape avant MIG10 :

```text
UGridLevelAsset::Objects
FGridLevelObjectData
compatibility projection
bTypedPlacementStorageAuthoritative
```

À la fin de MIG09-E, les collections de placements typées sont la seule source d’authoring du niveau.

## 7. Ordre opérationnel

```text
MIG09-A       autorité Definition sans marqueur sparse                   ✅ validé
MIG09-B1      supprimer AGridItemActor::ArchetypeId                      ✅ validé
MIG09-B2A     supprimer miroirs ItemArchetypeId réceptacles              ✅ validé
MIG09-B2B1    retirer ArchetypeId du SaveGame item                       ✅ validé
MIG09-B2B2    migrer consommateurs SaveGame                              ✅ validé
MIG09-B2B3    supprimer proxy/cache ItemArchetypeId                      ✅ validé
MIG09-B2C-A   audit Blueprint aliases Item                               ✅ validé
MIG09-B2C-B   supprimer InitializeItem/GetItemArchetypeId                ✅ validé
MIG09-C       purge mécanismes + animation spécialisée                   ✅ validé
MIG09-D1      supprimer SparseBehaviorOverrideObjectIds                  ⏳ candidat
MIG09-D2      supprimer write-through/mirror compatibility               ⬜ à faire
MIG09-D3      supprimer GridLevelPlacementCompatibility                  ⬜ à faire
MIG09-E       supprimer Objects/FGridLevelObjectData/projection          ⬜ à faire
MIG10         renommage final WorldObjectDefinition + clôture            ⬜ après MIG09
```

## 8. Critères de validation de MIG09-D1

```text
[ ] build GrimrockPrototypeEditor OK
[ ] Grimrock.WorldObjects : 0 Failed
[ ] SparseBehaviorOverrideObjectIds absent de la réflexion
[ ] nouvel objet WorldObject toujours résolu comme sparse
[ ] loose item non classé comme WorldObject sparse
[ ] aucun changement de contenu binaire requis
```

## 9. Règle de non-régression

Aucun nouveau marqueur de migration par `ObjectId` ne doit être introduit pour choisir entre Definition et Instance.

Le contrat cible reste :

```text
Definition
+ instance typée
+ runtime delta
```

et non :

```text
Definition
+ ancienne structure générique
+ marqueur de migration
+ proxy
```
