# WORLDOBJ-MIG09 — Purge des compatibilités legacy

Statut : **MIG09-A à MIG09-B2C-B validés ; MIG09-C consolidé candidat — validation locale UE5.5.4 requise ; MIG09-D/E à faire**.

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

## 2. Tranches déjà validées

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

MIG09-C regroupe désormais en une seule clôture cohérente l’audit des anciennes API, leur suppression et la purge des paramètres d’animation spécialisés.

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

Le Pit utilise déjà le chemin générique :

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

MIG09-C ne supprime pas les règles qui ne sont pas de la géométrie visuelle.

Le bouton conserve :

```text
ButtonAnimation.ButtonHoldTime
```

car il s’agit d’un délai logique de maintien.

La porte conserve :

```text
DoorAnimation.bHasChainMechanism
DoorAnimation.ChainPullDistance
DoorAnimation.ChainPullDuration
```

La plaque de pression conserve ses règles de poids :

```text
PressurePlateWeight
```

Le Pit conserve notamment :

```text
Pit.bInitiallyOpen
Pit.bUseSameCellCoordinates
Transition
```

### 4.4. Grid Editor

L’Inspector ne doit plus proposer des contrôles d’instance pour :

- hauteur d’ouverture d’une porte ;
- durée de déplacement d’une porte ;
- angles Off/On d’un levier ;
- distance/durée d’enfoncement d’un bouton ;
- hauteurs/durée d’une plaque ;
- pivots, angle et durée des volets d’une trappe.

Il affiche à la place l’autorité :

```text
Definition > Moving Parts[].Motion
```

Les contrôles qui correspondent à du gameplay d’instance restent éditables.

### 4.5. Tests migrés

Les fixtures Door et Pit ne remplissent plus les anciens champs `Behavior.*Animation` supprimés. Elles définissent directement leur motion dans une `UGridObjectArchetypeAsset` de test, via `GridDoorTestUtils::InitializeDoorFromMotion()` ou via `MovingParts[].Motion` pour les Pits.

Le test `Grimrock.WorldObjects.MIG04.BehaviorSchemaAuthority` protège l’absence réfléchie des anciens champs et la présence des seules règles gameplay conservées.

Le test `Grimrock.WorldObjects.MIG04.RuntimeGenericMotionContract` protège l’absence du cache `AGridDoorActor::OpenHeight` et le maintien des caches runtime de durée réellement nécessaires.

## 5. Critères de validation de MIG09-C

MIG09-C n’est considéré **validé** qu’après retour d’une exécution locale UE5.5.4 sans échec.

À vérifier :

```text
[ ] build GrimrockPrototypeEditor OK
[ ] Grimrock.WorldObjects : 0 Failed
[ ] anciens champs d’animation absents de la réflexion
[ ] ancien cache AGridDoorActor::OpenHeight absent
[ ] Inspector mécanismes sans contrôles d’animation d’instance
[ ] Door/Pit continuent à utiliser MovingParts[].Motion
```

Les warnings Automation éventuels doivent être distingués des warnings de compilation et examinés séparément ; ils ne sont pas assimilés à un échec sans diagnostic.

## 6. Reste de MIG09 après C

### MIG09-D — derniers consommateurs vers les placements typés

Objectif : supprimer les lectures/écritures qui nécessitent encore la projection de compatibilité du gros objet historique.

À traiter notamment :

```text
SparseBehaviorOverrideObjectIds
CommitCompatibilityObjectEdit()
RefreshLegacyObjectMirrorFromTyped()
GetObjectCompatibilityView()
GridLevelPlacementCompatibility
```

La suppression doit être guidée par les consommateurs réels, pas par un simple renommage.

### MIG09-E — suppression du modèle objet générique historique

Dernière étape avant MIG10 :

```text
UGridLevelAsset::Objects
FGridLevelObjectData
compatibility projection
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
MIG09-C       purge mécanismes + animation spécialisée                   ⏳ candidat
MIG09-D       derniers consommateurs vers placements typés               ⬜ à faire
MIG09-E       supprimer Objects/FGridLevelObjectData/projection           ⬜ à faire
MIG10         renommage final WorldObjectDefinition + clôture             ⬜ après MIG09
```

## 8. Règle de non-régression

Après MIG09-C, aucun nouveau code de production ne doit réintroduire un paramètre visuel spécialisé lorsqu’il peut être décrit par `MovingParts[].Motion`.

Exemple interdit :

```text
DoorOpenHeight
LeverOnPitch
PitOpenAngle
```

Exemple cible :

```text
MovingPart.Motion.Amount
MovingPart.Motion.Pivot
MovingPart.Motion.Duration
```

La variation visuelle d’un mécanisme est une donnée de sa définition, pas un nouveau chemin runtime.