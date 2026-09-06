# WORLDOBJ-MIG08 — Migration des assets Unreal réels

Statut : **MIG08-B — migration des définitions d’objets + sauvegarde sélective**.

## 1. But

MIG08 est la frontière entre la migration du schéma C++ et la migration du contenu Unreal réel.

Les `.uasset` et `.umap` ne doivent jamais être édités comme des fichiers binaires hors Unreal Engine. Le projet fournit donc un commandlet dédié qui charge les assets dans UE5.5.4, applique les conversions autorisées, valide le résultat puis, uniquement en mode `-Apply`, réenregistre les packages réellement modifiés.

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

---

## 3. MIG08-B — définitions d’objets du monde

Le premier dry-run réel a montré que plusieurs DataAssets gameplay n’avaient jamais été réenregistrés après MIG01/MIG03/MIG04. Le schéma C++ était correct, mais leur contenu sérialisé restait incomplet.

`MigrateArchetypeAsset()` complète désormais ces définitions avant validation.

### Placement imposé par la sémantique

Le migrateur normalise uniquement les cas non ambigus :

```text
Door / Button / Lever      -> Wall
PressurePlate / Trigger    -> Floor
Pit                        -> Floor
bReplacesStandardWall=true -> Wall
```

Les autres types conservent leur `PlacementSurface` existant.

### Contrats visuels historiques restaurés

La migration ne remplace jamais une `StaticPart` ou une `MovingPart` déjà définie. Elle ne complète que les parties absentes pour les IDs canoniques dont le contrat historique est connu.

```text
Button_Normal
  StaticPart  = SM_Button_Mettalic_Static
  MovingPart0 = SM_Button_Mettalic_Mobile
  Motion      = Translation X +6 cm / 0.08 s

Button_Secret
  MovingPart0 = SM_SecretButton_03
  Motion      = Translation X +6 cm / 0.08 s

Lever
  StaticPart  = SM_LeverStatic_01
  MovingPart0 = SM_Lever_01
  Rest        = Pitch 45°
  Motion      = Rotation Y +90° / 0.10 s

PressurePlate
  StaticPart  = SM_Grid_PressurePlate_Static
  MovingPart0 = SM_Grid_PressurePlate_Moving
  Rest        = Z +4 cm
  Motion      = Translation Z -3 cm / 0.08 s

Door_Wood
  MovingPart0 = SM_Door_Wood_Mobile_01
  Motion      = Translation Z +180 cm / 2.5 s

Door_Grating
  MovingPart0 = SM_Door_Grating_Mobile_01
  Motion      = Translation Z +180 cm / 2.5 s

Door_Secret
  StaticPart  = SM_Wall_Stone_SecretDoorStatic-01
  MovingPart0 = SM_Wall_Stone_SecretDoor-01
  Motion      = Translation Z +180 cm / 2.5 s

Pit_Stone_01
  StaticPart  = SM_Pit_Stone_01
```

Ces valeurs correspondent aux contrats runtime historiques validés avant MIG04.

### Cas particulier du pit

MIG08-B ne crée jamais artificiellement une couverture de trappe.

Un pit sans `MovingParts` reste une fosse ouverte valide. Si les deux volets existent déjà mais que leur `Motion` n’est pas configuré, le migrateur restaure seulement les motions validées :

```text
Part0 : Rotation Y, Pivot=(-85,0,-5), Amount=-80°, Duration=0.75 s
Part1 : Rotation Y, Pivot=( 85,0,-5), Amount=+80°, Duration=0.75 s
```

Une paire incomplète reste une erreur de contenu et n’est pas maquillée par le migrateur.

---

## 4. Validation en deux passes

Le commandlet travaille maintenant en deux passes :

1. migration en mémoire de tous les LevelAssets, palettes et WorldObjectDefinitions ;
2. validation du graphe entièrement migré.

Cela évite qu’un résultat dépende de l’ordre alphabétique des packages dans l’Asset Registry, notamment lorsqu’une palette modifie indirectement une `ItemDefinition` chargée ailleurs.

---

## 5. Sauvegarde sélective

En mode `-Apply`, MIG08 ne prépare plus tous les DataAssets scannés pour sauvegarde.

Après migration et validation, le commandlet collecte uniquement les packages `IsDirty()`.

Conséquences :

- les ItemDefinitions modifiées indirectement par la palette sont bien sauvegardées ;
- un DataAsset seulement lu/validé n’est pas réenregistré ;
- le diff Git reste limité au contenu réellement migré.

Aucun package n’est sauvegardé si une erreur bloquante subsiste.

---

## 6. Sécurité

Le commandlet est en lecture/écriture mémoire dans les deux modes, mais :

- sans `-Apply`, aucun package n’est sauvegardé ;
- avec `-Apply`, les packages sont sauvegardés uniquement si aucune erreur de migration/validation bloquante n’a été trouvée ;
- un rapport texte est toujours écrit sous `Saved/Automation/MIG08` ;
- les changements binaires restent visibles dans Git après la sauvegarde et doivent être examinés avant commit.

Le migrateur ne supprime aucun `.uasset` et ne modifie aucun `.umap` par manipulation binaire.

---

## 7. Workflow recommandé

### Étape 1 — valider le code du migrateur

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.WorldObjects"
```

Les tests MIG08 sont :

```text
Grimrock.WorldObjects.MIG08.LevelAssetMigration
Grimrock.WorldObjects.MIG08.PaletteItemMigration
Grimrock.WorldObjects.MIG08.ArchetypeMigration
```

Le test ArchetypeMigration protège notamment :

- la normalisation Door/Lever/Pit ;
- les motions historiques ;
- l’idempotence ;
- l’interdiction d’inventer des volets de pit.

### Étape 2 — second dry-run sur le contenu réel

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

Uniquement lorsque le dry-run retourne `Errors : 0` :

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

Les `.uasset` modifiés sont alors exclusivement des fichiers produits par UE5.5.4 et peuvent être revus/commités normalement.

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

## 8. Critère de sortie MIG08

MIG08 pourra être déclaré clos lorsque :

- tous les `UGridLevelAsset` réels sont sauvegardés avec `bTypedPlacementStorageAuthoritative=true` ;
- la palette ne dépend plus d’un companion archetype pour les collectibles ;
- les ItemDefinitions portent directement leur WorldMesh/icon lorsque nécessaire ;
- les définitions du monde utilisent `PlacementSurface`, `StaticPart`, `MovingParts` et `Motion` courants ;
- les niveaux réels et transitions passent les validations ;
- Git contient uniquement les `.uasset` réellement réenregistrés par UE ;
- aucune donnée nécessaire n’a été perdue lors de la conversion ;
- les anciens assets Item companion restant sont identifiés pour suppression en MIG09.

MIG09 pourra alors supprimer les miroirs, ponts Transient, fallbacks et contrôles Slate historiques sans devoir conserver une compatibilité de contenu obsolète.
