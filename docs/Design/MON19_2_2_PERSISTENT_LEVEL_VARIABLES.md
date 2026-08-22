# MON19.2.2 — Variables logiques persistantes

## Statut

Implémentation C++ préparée. La compilation et les tests UE5.5.4 restent à valider par le propriétaire du projet.

## Objectif

MON19.2.2 introduit le stockage typé et persistant qui servira aux mécanismes logiques de MON19.2.3 puis à l'API Lua des étapes ultérieures.

Le périmètre est volontairement limité à deux types :

- `Bool` ;
- `Int32`.

Cette étape n'ajoute ni nœud logique, ni compteur Event→Command, ni Lua, ni nouveau `Actor`, `Component` ou `Subsystem`.

## Déclarations dans le niveau

Le fichier `Core/GridLevelVariableTypes.h` introduit :

- `EGridLevelVariableType` ;
- `FGridLevelVariableDefinition`.

`UGridLevelAsset::LevelVariables` contient les déclarations data-driven.

Chaque déclaration possède :

- un `VariableId` stable ;
- un type ;
- une valeur par défaut du type correspondant.

Exemple conceptuel :

```text
CryptDoorUnlocked : Bool  = false
RuneCount         : Int32 = 0
```

`VariableId` est unique à l'échelle du niveau, indépendamment du type. Il n'est donc pas permis d'avoir simultanément un Bool et un Int32 portant le même identifiant.

## État runtime

Les valeurs mutables ne sont jamais écrites dans `UGridLevelAsset`.

Elles résident dans `FGridLevelRuntimeState` :

```text
bLevelVariablesInitialized
BoolVariables : TMap<FName, bool>
IntVariables  : TMap<FName, int32>
```

`FGridLevelRuntimeState` était déjà le conteneur persistant par niveau du donjon. Les variables profitent donc du flux existant :

```text
UGridLevelAsset
    │ defaults
    ▼
GridLevelVariableStore
    │
    ▼
FGridLevelRuntimeState
    │
    ▼
FGridDungeonRuntimeState
    │
    ▼
UGrimrockPartySaveGame
```

Aucun second système de sauvegarde n'est créé.

## Service central

`Runtime/GridLevelVariableStore.h/.cpp` concentre les règles :

- validation des déclarations ;
- initialisation depuis les defaults ;
- réconciliation avec une version modifiée du niveau ;
- lecture/écriture typée ;
- remise à zéro explicite ;
- validation structurelle des snapshots SaveGame ;
- nettoyage du domaine variable lors des migrations legacy.

### Initialisation et réconciliation

Au premier accès, le snapshot est construit depuis les defaults.

Si le niveau est modifié ultérieurement :

- une variable toujours déclarée avec le même type conserve sa valeur runtime ;
- une nouvelle variable reçoit son default ;
- une variable dont le type a changé reçoit le default du nouveau type ;
- une variable supprimée du niveau est supprimée du snapshot runtime.

Cette politique permet de faire évoluer un niveau sans réinitialiser inutilement les énigmes déjà commencées.

### Typage strict

Les opérations refusent :

- `VariableId=None` ;
- une variable non déclarée ;
- une lecture/écriture avec le mauvais type ;
- deux déclarations du même `VariableId`.

Il n'existe aucune création implicite par simple écriture.

## Persistance — SaveVersion 7

`UGrimrockPartySaveGame::CurrentSaveVersion` passe de 6 à 7.

### Migration v6 → v7

Une sauvegarde v6 possède déjà les données autoritatives de MON15 à MON18 mais ne possède aucune valeur MON19 fiable.

La migration :

1. valide les données de progression et de Spellbook v6 ;
2. conserve tout le reste du `DungeonRuntimeState` ;
3. vide uniquement les maps de variables ;
4. positionne `bLevelVariablesInitialized=false` ;
5. passe la sauvegarde en v7.

Au premier accès dans le niveau, `GridLevelVariableStore` initialise alors les variables depuis les defaults actuels du `UGridLevelAsset`.

Aucune valeur historique n'est inventée.

Les chemins v1-v5 appliquent le même principe au nouveau domaine après leurs migrations existantes.

### Validation v7

Une sauvegarde courante est rejetée si :

- un snapshot marqué non initialisé contient néanmoins des valeurs ;
- un `VariableId=None` est présent ;
- un même identifiant est stocké à la fois comme Bool et Int32.

La validation SaveGame ne tente volontairement pas de charger les assets de niveau. La réconciliation avec les déclarations réelles est faite lorsque le niveau est disponible au runtime.

## Tests automatisés ajoutés

Préfixes :

```text
Grimrock.MON19.2.Runtime.LevelVariables
Grimrock.MON19.2.Save
```

Tests MON19.2.2 :

```text
Grimrock.MON19.2.Runtime.LevelVariables.DefaultInitialization
Grimrock.MON19.2.Runtime.LevelVariables.ReconcilePersistence
Grimrock.MON19.2.Save.LevelVariableSnapshotValidation
Grimrock.MON19.2.Save.V6ToV7LevelVariables
Grimrock.MON19.2.Save.LevelVariablesRoundTrip
```

Les suites historiques MON15.6, MON16.7, MON16.8 et MON18.8 sont adaptées afin que leurs assertions de version suivent le contrat courant au lieu de figer la valeur historique `6`.

## Hors périmètre

MON19.2.2 ne fournit pas encore :

- Set/Toggle/Add/Sub comme commandes Event→Command ;
- comparateurs ou seuils ;
- latch/relay ;
- édition dédiée des variables dans le panneau custom Grid Editor ;
- Lua ;
- inspection runtime des variables en PIE.

Ces responsabilités appartiennent aux étapes suivantes, en commençant par MON19.2.3.
