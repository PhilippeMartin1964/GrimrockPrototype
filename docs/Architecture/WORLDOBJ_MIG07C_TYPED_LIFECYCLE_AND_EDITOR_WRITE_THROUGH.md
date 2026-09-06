# WORLDOBJ-MIG07-C — Cycle de vie et écriture typée du LevelAsset

## Statut

MIG07-C ferme le cycle de vie C++ du stockage typé introduit par MIG07-A/B.

La règle est désormais :

```text
WorldObjectInstances
LooseItemInstances
MonsterSpawns
ItemSpawns
LogicObjects
        │
        ├── source persistante autoritaire si
        │   bTypedPlacementStorageAuthoritative = true
        │
        └── Objects = miroir de compatibilité MIG09
```

`FGridLevelObjectData` reste temporairement exposé aux anciens consommateurs runtime et à plusieurs widgets du Grid Editor, mais il n'est plus une deuxième source de vérité lorsqu'un niveau est passé en stockage typé.

---

## 1. PostLoad

Un LevelAsset typé ne fait plus confiance à la copie sérialisée de `Objects`.

Au chargement :

```text
bTypedPlacementStorageAuthoritative = false
    -> comportement historique
    -> Objects reste autoritaire

bTypedPlacementStorageAuthoritative = true
    -> collections typées autoritaires
    -> Objects est reconstruit immédiatement
```

Cela évite qu'un ancien miroir stale puisse reprendre l'autorité après réouverture d'un asset migré.

---

## 2. Cycle de vie structurel

Les opérations suivantes sont maintenant conscientes des cinq collections typées :

- `AddObject()` ;
- `RemoveObjectById()` ;
- `ClearLevel()` ;
- `EnsureObjectIds()`.

En mode legacy, leur comportement historique est conservé.

En mode typé :

- `AddObject()` convertit l'objet de compatibilité vers le bucket correct ;
- `RemoveObjectById()` supprime l'instance dans le bucket typé et ses liens ;
- `ClearLevel()` vide les cinq collections, le miroir, les liens et les marqueurs sparse ;
- `EnsureObjectIds()` répare directement les `InstanceId` / `SpawnId` typés.

Le booléen `bTypedPlacementStorageAuthoritative` n'est pas remis à `false` par `ClearLevel()` : vider un niveau typé ne doit pas le faire revenir à l'ancien schéma.

---

## 3. Write-through du Grid Editor

Le Grid Editor possède encore des setters historiques qui modifient un `FGridLevelObjectData` temporaire.

MIG07-C introduit :

```cpp
bool UGridLevelAsset::CommitCompatibilityObjectEdit(const FGuid& ObjectId);
```

Cette opération fusionne les champs représentables par le miroir vers l'instance typée correspondante.

Le point de sortie commun `AGridLevelEditorActor::RebuildPreview()` effectue automatiquement :

```text
compatibility mirror edit
        -> CommitCompatibilityObjectEdit
        -> typed authority
        -> RefreshLegacyObjectMirrorFromTyped
        -> preview rebuild
```

L'éditeur de patrol ne passe pas par `RebuildPreview()` à chaque modification ; il effectue donc le commit typé explicitement avant de redessiner les viewports.

---

## 4. Préservation des données exclusivement typées

La fusion n'effectue volontairement pas un remplacement naïf de l'instance typée.

Des champs nouveaux n'existent pas dans `FGridLevelObjectData` :

```text
LooseItemInstance.Quantity
LooseItemInstance.LocalOffset
ItemSpawnInstance.Quantity
WorldObjectInstance.LocalTransformOverride complet
```

Si un ancien widget change seulement un `Tag`, une cellule ou un yaw, ces valeurs doivent survivre.

MIG07-C préserve donc :

- la quantité d'un loose item ;
- son offset local ;
- la quantité d'un ItemSpawn ;
- translation, scale, pitch et roll du transform local d'un WorldObject ;
- le yaw est mis à jour lorsqu'il est modifié par le pont historique.

Cette règle est essentielle pour que le pont de compatibilité soit sans perte d'information.

---

## 5. Suppression dans le Grid Editor

Les anciens chemins faisaient directement :

```cpp
LevelAsset->Objects.RemoveAt(...);
```

Ce comportement est interdit pour un niveau typé car il ne supprimerait que le miroir.

Les suppressions de sélection et les conflits de placement passent maintenant par :

```cpp
UGridLevelAsset::RemoveObjectById(...)
```

Ainsi l'instance typée et les liens associés sont supprimés ensemble.

---

## 6. Nouveaux niveaux

À partir de MIG07-C, un LevelAsset créé depuis le Grimrock Grid Editor naît directement avec :

```text
bTypedPlacementStorageAuthoritative = true
```

Un nouveau niveau ne doit plus produire du stockage legacy à la veille de MIG08.

Les niveaux existants restent en mode historique tant qu'ils n'ont pas été explicitement migrés.

---

## 7. Frontière avec MIG08

MIG07-C ne modifie aucun `.uasset` ou `.umap` existant.

Après validation MIG07-C, le code sait :

- charger un niveau typé ;
- maintenir son miroir de compatibilité ;
- ajouter/supprimer/vider des placements typés ;
- appliquer les éditions actuelles du Grid Editor sans perdre les champs typés ;
- créer de nouveaux niveaux directement dans le nouveau schéma.

MIG08 pourra donc se concentrer sur la migration réelle du contenu Unreal :

```text
assets historiques
    -> projection typée
    -> bTypedPlacementStorageAuthoritative = true
    -> réenregistrement UE
```

La suppression physique de `Objects` et des autres ponts reste réservée à MIG09.

---

## 8. Validation

Famille principale :

```text
Grimrock.WorldObjects
```

Nouveaux contrats MIG07-C :

```text
Grimrock.WorldObjects.MIG07.TypedLifecycle
Grimrock.WorldObjects.MIG07.TypedLifecycleSchema
Grimrock.WorldObjects.MIG07.EditorTypedWriteThrough
```

Ils couvrent notamment :

- Add / Remove / Clear en stockage typé ;
- réparation d'identifiants typés ;
- fusion d'une édition legacy vers le typé ;
- préservation de `Quantity`, `LocalOffset` et transform local enrichi ;
- write-through des setters Grid Editor ;
- suppression d'un loose item via l'éditeur ;
- write-through de la route de patrol.
