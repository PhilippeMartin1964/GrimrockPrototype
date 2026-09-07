# WORLDOBJ-MIG09 — Purge des compatibilités legacy

Statut : **MIG09-A à MIG09-D2 validés ; MIG09-D3 candidat ; MIG09-E à faire**.

Date de mise à jour : 2026-09-07.

## 1. But

MIG09 supprime physiquement les ponts temporaires introduits pendant MIG00 à MIG08.

```text
une donnée cible existe
        -> les consommateurs sont migrés
        -> le legacy est supprimé physiquement
        -> un test empêche sa réapparition
```

Référence architecturale prioritaire :

```text
docs/Architecture/Maps/Grimrock_MindMap_Architecture_Cible_v2_XMind.md
```

## 2. État validé

| Tranche | État | Résultat |
|---|---|---|
| MIG09-A | ✅ | Autorité Definition sans marqueur sparse. |
| MIG09-B* | ✅ | Identité Item legacy purgée. |
| MIG09-C | ✅ | Initializers mécanismes et animation spécialisée purgés ; `MovingParts[].Motion` reste l'autorité visuelle. |
| MIG09-D1 | ✅ | `SparseBehaviorOverrideObjectIds` supprimé ; le caractère sparse est structurel. |
| MIG09-D2 | ✅ | Les écritures principales du Grid Editor ne dépendent plus de `CommitCompatibilityObjectEdit()`. `SetSparseBehaviorOverrides()` est supprimé. |

Validation locale de MIG09-D2 :

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
D:\Development\GrimrockPrototype\Saved\Automation\TD04\TD04-20260907-154104
```

Le warning Visual Studio 2022 sur la version du compilateur préférée reste un warning UBT distinct du résultat Automation.

## 3. Contrat Definition / Instance

```text
Definition.DefaultBehavior
        +
InstanceConfig strictement propre au placement
        +
Runtime/Save delta mutable
        =
comportement effectif
```

Une instance ne recopie ni mesh, ni géométrie d'animation, ni règle permanente de la définition.

## 4. MIG09-D2 — résultat

Le Grid Editor ne dépend plus du write-through historique :

```text
Objects mutable
  -> CommitCompatibilityObjectEdit()
  -> collections typées
```

Les principaux setters construisent un snapshot temporaire puis écrivent l'autorité via :

```text
ApplyGridEditorObjectSnapshotToAuthority(...)
        -> UGridLevelAsset::AddObject(snapshot)
        -> collection typée concernée
```

Les champs typed-only sont préservés pendant l'update, notamment :

- `FGridWorldObjectInstance::LocalTransformOverride` ;
- `FGridLooseItemInstance::Quantity` ;
- `FGridLooseItemInstance::LocalOffset` ;
- `FGridItemSpawnInstance::Quantity`.

`CommitCompatibilityObjectEdit()` reste encore présent dans le coeur uniquement, mais sans call site Editor/test voulu.

## 5. MIG09-D3 — confinement du miroir legacy

État : **candidat**.

D3 supprime le dernier comportement d'écriture implicite du Grid Editor :

```text
RebuildPreview()
        -> aucune mutation de LevelAsset
```

Le fallback D2 qui recopiait un snapshot sélectionné pendant `RebuildPreview()` est supprimé. Le preview devient strictement consommateur.

Les chemins Editor suivants ne passent plus par `GetObjectCompatibilityView()` :

- suppression d'objets à la sélection ;
- détection des conflits de frontière ;
- test d'autorité typée MIG07 ;
- test de write-through Editor MIG07.

Ces consommateurs lisent le miroir `Objects` déjà maintenu par les opérations de niveau. Lorsqu'un test modifie directement une collection typée, il appelle explicitement `RefreshLegacyObjectMirrorFromTyped()` avant de lire le miroir ; il n'existe donc plus d'accesseur qui masque un rafraîchissement implicite.

### Pourquoi les primitives core restent jusqu'à MIG09-E

Les éléments suivants sont désormais confinés au coeur `UGridLevelAsset` et au service de migration MIG08 :

```text
CommitCompatibilityObjectEdit()
RefreshLegacyObjectMirrorFromTyped()
GetObjectCompatibilityView()
GridLevelPlacementCompatibility
Objects
FGridLevelObjectData
bTypedPlacementStorageAuthoritative
```

Ils forment une seule couche de projection legacy. Les supprimer ensemble en MIG09-E est plus sûr et plus simple que de recréer une façade temporaire entre D3 et E.

Aucun code Editor de preview ne doit désormais utiliser cette couche pour écrire.

## 6. MIG09-E — purge finale du modèle historique

MIG09-E supprimera en une tranche substantielle :

```text
UGridLevelAsset::Objects
FGridLevelObjectData
bTypedPlacementStorageAuthoritative
CommitCompatibilityObjectEdit()
RefreshLegacyObjectMirrorFromTyped()
GetObjectCompatibilityView()
GridLevelPlacementCompatibility
projection legacy -> typed
```

À la sortie de MIG09-E, l'authoring du niveau repose uniquement sur :

```text
WorldObjectInstances
LooseItemInstances
MonsterSpawns
ItemSpawns
LogicObjects
```

## 7. Ordre opérationnel

```text
MIG09-A       autorité Definition sans marqueur sparse              ✅
MIG09-B*      purge identité Item legacy                            ✅
MIG09-C       purge mécanismes + animation spécialisée              ✅
MIG09-D1      supprimer SparseBehaviorOverrideObjectIds             ✅
MIG09-D2      détacher les écritures du compatibility commit        ✅
MIG09-D3      rendre le preview/read Editor indépendant du helper   ⏳ candidat
MIG09-E       supprimer toute la projection legacy core             ⬜
MIG10         renommage WorldObjectDefinition + clôture             ⬜
```

## 8. Validation de D3

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.WorldObjects"
```

Critères :

```text
[ ] build GrimrockPrototypeEditor OK
[ ] Grimrock.WorldObjects : 0 Failed
[ ] RebuildPreview ne modifie plus LevelAsset
[ ] suppression Editor conserve l'autorité typée
[ ] MIG07.TypedAuthorityBridge reste vert
[ ] MIG07.EditorTypedWriteThrough reste vert
```

## 9. Règle de non-régression

Aucun nouveau marqueur, miroir mutable d'autorité ou write-through implicite ne doit être introduit.

Le contrat final reste :

```text
Definition
+ instance typée
+ runtime delta
```
