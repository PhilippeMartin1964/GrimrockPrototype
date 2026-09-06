# WORLDOBJ-MIG08 — Migration des assets Unreal réels

Statut : **MIG08-A — harness de migration source-only**.

## 1. But

MIG08 est la frontière entre la migration du schéma C++ et la migration du contenu Unreal réel.

Les `.uasset` et `.umap` ne doivent jamais être édités comme des fichiers binaires hors Unreal Engine. Le projet fournit donc un commandlet dédié qui charge les assets dans UE5.5.4, applique les conversions autorisées, valide le résultat puis, uniquement en mode `-Apply`, réenregistre les packages.

Le commandlet est :

```text
GridWorldObjectMIG08
```

Le wrapper projet est :

```text
Scripts/MigrateWorldObjectAssets.ps1
```

Le mode par défaut est **DRY-RUN**.

---

## 2. Périmètre MIG08-A

### LevelAssets

Pour chaque `UGridLevelAsset` sous `/Game/GrimrockPrototype` :

- conversion explicite du monolithe `Objects` vers :
  - `WorldObjectInstances` ;
  - `LooseItemInstances` ;
  - `MonsterSpawns` ;
  - `ItemSpawns` ;
  - `LogicObjects` ;
- activation de `bTypedPlacementStorageAuthoritative` ;
- reconstruction de `Objects` uniquement comme miroir de compatibilité MIG09 ;
- vérification du nombre de placements ;
- vérification de la conservation des identifiants persistants ;
- vérification des références directes `ItemDefinition` et `MonsterDefinition` ;
- validation du contrat MonsterSpawn existant.

La migration est idempotente : relancer le migrateur sur un LevelAsset déjà typé ne doit pas recréer une seconde autorité ni dupliquer les placements.

### Palette

Pour chaque `UGridObjectPaletteAsset` :

- une entrée Item historique `DefaultArchetype -> DefaultBehavior.Item.ItemDefinitionAsset` est convertie vers `DefaultItemDefinition` ;
- `DefaultArchetype` est supprimé de l’entrée collectible ;
- l’icône de palette est promue vers `ItemDefinition.Icon` si l’item n’en possède pas encore ;
- `StaticPart.Mesh` de l’ancien archetype Item est promu vers `ItemDefinition.WorldMesh` si l’item n’a pas encore de WorldMesh ;
- une divergence entre l’item actuel et l’ancien companion archetype est signalée mais n’écrase pas la nouvelle définition ;
- les doublons potentiels d’une même ItemDefinition dans plusieurs entrées sont signalés pour revue manuelle.

Les anciens `UGridObjectArchetypeAsset` de type `Item` ne sont pas supprimés automatiquement : ils deviennent des **candidats de suppression MIG09** une fois qu’aucune palette, aucun niveau et aucun runtime ne les référence encore.

### Définitions

Le commandlet charge également :

- `UGridObjectArchetypeAsset` ;
- `UGridItemDefinitionAsset`.

Il exécute les validations courantes afin que MIG08 ne réenregistre pas silencieusement un contenu déjà invalide.

---

## 3. Sécurité

Le commandlet est en lecture/écriture mémoire dans les deux modes, mais :

- sans `-Apply`, aucun package n’est sauvegardé ;
- avec `-Apply`, les packages sont sauvegardés uniquement si aucune erreur de migration/validation bloquante n’a été trouvée ;
- un rapport texte est toujours écrit sous `Saved/Automation/MIG08` ;
- les changements binaires restent visibles dans Git après la sauvegarde et doivent être examinés avant commit.

Le migrateur ne supprime aucun `.uasset` et ne modifie aucun `.umap` par manipulation binaire.

---

## 4. Workflow recommandé

### Étape 1 — valider le code du migrateur

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.WorldObjects"
```

Les tests MIG08 ajoutés sont :

```text
Grimrock.WorldObjects.MIG08.LevelAssetMigration
Grimrock.WorldObjects.MIG08.PaletteItemMigration
```

### Étape 2 — dry-run sur le contenu réel

```powershell
.\Scripts\MigrateWorldObjectAssets.ps1 `
    -EngineRoot D:\UE_5.5 `
    -SkipBuild
```

Aucun `.uasset` n’est sauvegardé.

Le rapport doit être examiné. Les lignes sont classées en :

```text
CHANGE
WARNING
ERROR
```

Toute ligne `ERROR` bloque volontairement le mode Apply.

### Étape 3 — appliquer la migration

Uniquement après validation du dry-run :

```powershell
.\Scripts\MigrateWorldObjectAssets.ps1 `
    -EngineRoot D:\UE_5.5 `
    -SkipBuild `
    -Apply
```

Le script affiche ensuite :

```text
git status --short -- Content
```

Les `.uasset` modifiés sont alors des fichiers produits par UE5.5.4 et peuvent être revus/commités normalement.

### Étape 4 — validations après sauvegarde

Au minimum :

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.WorldObjects"
```

Puis :

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.TechnicalDebt.TD07_3_7.Normalization"
```

Enfin, les niveaux réels doivent être ouverts et testés dans le Grid Editor / runtime pour vérifier portes, boutons, leviers, plaques, pits, téléporteurs, triggers, réceptacles, décorations, monstres et transitions inter-niveaux.

---

## 5. Critère de sortie MIG08

MIG08 pourra être déclaré clos lorsque :

- tous les `UGridLevelAsset` réels sont sauvegardés avec `bTypedPlacementStorageAuthoritative=true` ;
- la palette ne dépend plus d’un companion archetype pour les collectibles ;
- les ItemDefinitions portent directement leur WorldMesh/icon lorsque nécessaire ;
- les définitions du monde utilisent la composition visuelle courante ;
- les niveaux réels et transitions passent les validations ;
- Git contient les `.uasset` réenregistrés par UE ;
- aucune donnée nécessaire n’a été perdue lors de la conversion ;
- les anciens assets Item companion restant sont identifiés pour suppression en MIG09.

MIG09 pourra alors supprimer les miroirs, ponts Transient, fallbacks et contrôles Slate historiques sans devoir conserver une compatibilité de contenu obsolète.
