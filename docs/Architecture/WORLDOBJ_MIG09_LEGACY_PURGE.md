# WORLDOBJ-MIG09 — Purge des compatibilités legacy

Statut : **MIG09-A à MIG09-D1 validés ; MIG09-D2 candidat — validation locale UE5.5.4 requise ; MIG09-D3/E à faire**.

Date de mise à jour : 2026-09-07.

## 1. But

MIG09 supprime physiquement les ponts temporaires introduits pendant MIG00 à MIG08. La règle reste :

```text
une donnée cible existe
        -> les consommateurs sont migrés
        -> le legacy est supprimé physiquement
        -> un test empêche sa réapparition
```

La référence architecturale prioritaire reste :

```text
docs/Architecture/Maps/Grimrock_MindMap_Architecture_Cible_v2_XMind.md
```

## 2. Tranches validées

| Tranche | État | Résultat |
|---|---|---|
| MIG09-A | ✅ | Le runtime ne dépend plus du marqueur sparse pour choisir l’autorité du Behavior. |
| MIG09-B1 | ✅ | `AGridItemActor::ArchetypeId` supprimé. |
| MIG09-B2A | ✅ | Miroirs `ItemArchetypeId` des réceptacles supprimés. |
| MIG09-B2B1 | ✅ | `FGridRuntimeItemState::ArchetypeId` retiré du SaveGame. |
| MIG09-B2B2 | ✅ | Consommateurs SaveGame migrés vers `ItemDefinitionId`. |
| MIG09-B2B3 | ✅ | Proxy/cache runtime `ItemArchetypeId` supprimés. |
| MIG09-B2C-A | ✅ | Audit Blueprint des anciennes API Item. |
| MIG09-B2C-B | ✅ | `InitializeItem()` et `GetItemArchetypeId()` supprimés. |
| MIG09-C | ✅ | Initializers mécanismes et schéma d’animation spécialisé supprimés ; `MovingParts[].Motion` reste l’autorité visuelle. |
| MIG09-D1 | ✅ | `SparseBehaviorOverrideObjectIds` supprimé ; le caractère sparse est structurel. |

Validation locale de MIG09-D1 :

```text
Build                  : OK
Succeeded              : 35
Succeeded with warnings: 1
Failed                 : 0
Not run                : 0
Process exit code       : 0
```

Rapport :

```text
D:\Development\GrimrockPrototype\Saved\Automation\TD04\TD04-20260907-134249
```

Le warning Visual Studio 2022 sur la version de compilateur préférée est un warning UBT distinct du résultat Automation.

## 3. Autorité Definition / Instance

Le contrat cible reste :

```text
Definition.DefaultBehavior
        +
InstanceConfig strictement propre au placement
        +
Runtime/Save delta mutable
        =
comportement effectif
```

L’instance ne recopie ni mesh, ni géométrie d’animation, ni règle permanente de la définition.

Les données d’instance comprennent notamment :

- destination de téléporteur ;
- transition ;
- état/configuration locale de Pit ;
- contenu initial d’un réceptacle ;
- état initial d’une serrure ;
- position, orientation, état initial, Tag/Notes.

## 4. MIG09-C — mécanismes legacy

MIG09-C est validé.

La présentation des mécanismes est décrite exclusivement par :

```text
WorldObject Definition
└── MovingParts[]
    └── Motion
        ├── Type
        ├── Axis
        ├── Pivot
        ├── Amount
        └── Duration
```

Les anciens champs spécialisés Door/Button/Lever/PressurePlate/Pit ont été supprimés physiquement. Les règles gameplay distinctes restent, par exemple `ButtonHoldTime`, les règles de poids et les paramètres de chaîne de porte.

## 5. MIG09-D1 — suppression du marqueur sparse

MIG09-D1 est validé.

Le stockage historique suivant n’existe plus :

```text
SparseBehaviorOverrideObjectIds
```

Le caractère sparse d’un objet réutilisable est désormais structurel :

```text
FGridWorldObjectInstance
        -> sparse par construction
```

Le test :

```text
Grimrock.WorldObjects.MIG09.SparseMarkerPurge
```

protège l’absence réfléchie du marqueur et distingue correctement WorldObject et LooseItem.

## 6. MIG09-D2 — détacher les écritures Grid Editor du commit de compatibilité

État : **candidat**.

Objectif : le Grid Editor ne doit plus modifier `Objects` puis dépendre de :

```text
CommitCompatibilityObjectEdit()
```

pour recopier l’édition vers les collections typées.

### 6.1. `SetSparseBehaviorOverrides()`

Le no-op conservé après D1 est supprimé physiquement. Aucun call site ne doit subsister.

### 6.2. Chemins d’édition principaux

Les setters de l’Inspector construisent désormais un snapshot temporaire `FGridLevelObjectData` à partir de la vue courante, modifient ce snapshot, puis écrivent l’autorité via :

```text
ApplyGridEditorObjectSnapshotToAuthority(...)
        -> UGridLevelAsset::AddObject(snapshot)
        -> collection typée concernée
```

`AddObject()` est utilisé ici comme chemin transitoire d’écriture parce qu’il met déjà à jour la collection typée correcte et préserve les données typed-only qui n’existent pas dans `FGridLevelObjectData` :

- `FGridWorldObjectInstance::LocalTransformOverride` complet ;
- `FGridLooseItemInstance::Quantity` ;
- `FGridLooseItemInstance::LocalOffset` ;
- `FGridItemSpawnInstance::Quantity`.

Ce helper n’ajoute aucun stockage ni aucune seconde autorité.

Les chemins migrés comprennent notamment :

- Behavior d’instance ;
- reset depuis Definition ;
- Definition/Item/Monster asset ;
- Encounter group/wave ;
- readable content ;
- Tag / Notes ;
- état initial Enabled/Active ;
- déplacement d’objet ;
- placement d’un nouvel objet.

### 6.3. Preview

`RebuildPreview()` n’appelle plus `CommitCompatibilityObjectEdit()`.

Il conserve provisoirement un fallback étroit pour les derniers éditeurs pré-D3 : si le miroir sélectionné a été modifié par un ancien chemin, il en copie le snapshot puis appelle directement `AddObject()` avant de reconstruire la vue.

Ce fallback est explicitement temporaire et doit disparaître en MIG09-D3.

### 6.4. Patrol Route

L’édition des waypoints n’appelle plus `CommitCompatibilityObjectEdit()` ; le helper local copie le snapshot sélectionné et le pousse directement via `AddObject()`.

### 6.5. Tests

`Grimrock.WorldObjects.MIG07.TypedLifecycle` ne teste plus l’ancien write-through `CommitCompatibilityObjectEdit()`.

Il vérifie maintenant que des snapshots édités écrits directement via `AddObject()` :

- mettent à jour les collections typées ;
- préservent les champs typed-only ;
- conservent les règles d’instance.

### 6.6. Symbole dormant

Pour limiter le risque de cette tranche, l’implémentation C++ de :

```text
CommitCompatibilityObjectEdit()
```

reste encore présente mais **sans call site runtime/editor/test actif** après D2. Elle sera supprimée physiquement avec la compatibility view en MIG09-D3.

## 7. MIG09-D3 — supprimer la compatibility view restante

Après validation de D2 :

- migrer les derniers lecteurs/éditeurs encore basés sur `Objects` ;
- supprimer le fallback de `RebuildPreview()` ;
- supprimer physiquement `CommitCompatibilityObjectEdit()` ;
- supprimer `RefreshLegacyObjectMirrorFromTyped()` ;
- supprimer `GetObjectCompatibilityView()` ;
- supprimer les conversions `GridLevelPlacementCompatibility` lorsque plus aucun consommateur n’en dépend.

Le but de D3 est qu’aucun code de production n’ait besoin de reconstruire un `FGridLevelObjectData` pour lire ou éditer un placement typé.

## 8. MIG09-E — supprimer le gros modèle historique

Dernière purge avant MIG10 :

```text
UGridLevelAsset::Objects
FGridLevelObjectData
bTypedPlacementStorageAuthoritative
projection legacy -> typed
```

À la fin de MIG09-E, les seules collections d’authoring sont :

```text
WorldObjectInstances
LooseItemInstances
MonsterSpawns
ItemSpawns
LogicObjects
```

## 9. Ordre opérationnel

```text
MIG09-A       autorité Definition sans marqueur sparse              ✅ validé
MIG09-B*      purge identité Item legacy                            ✅ validé
MIG09-C       purge mécanismes + animation spécialisée              ✅ validé
MIG09-D1      supprimer SparseBehaviorOverrideObjectIds             ✅ validé
MIG09-D2      détacher les écritures du compatibility commit        ⏳ candidat
MIG09-D3      supprimer compatibility view/conversions              ⬜ à faire
MIG09-E       supprimer Objects/FGridLevelObjectData                 ⬜ à faire
MIG10         renommage WorldObjectDefinition + clôture             ⬜ après MIG09
```

## 10. Validation de D2

Commande :

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.WorldObjects"
```

Critères :

```text
[ ] build GrimrockPrototypeEditor OK
[ ] Grimrock.WorldObjects : 0 Failed
[ ] SetSparseBehaviorOverrides absent du source courant
[ ] aucun call site actif vers CommitCompatibilityObjectEdit
[ ] TypedLifecycle préserve les champs typed-only
[ ] EditorTypedWriteThrough reste vert
[ ] Patrol editor reste vert dans le filtre WorldObjects
```

## 11. Règle de non-régression

Aucun nouveau marqueur, miroir mutable ou write-through implicite ne doit être introduit.

Le contrat final reste :

```text
Definition
+ instance typée
+ runtime delta
```
