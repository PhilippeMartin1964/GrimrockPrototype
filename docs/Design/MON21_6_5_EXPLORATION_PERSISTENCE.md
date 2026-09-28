# MON21.6.5 — Exploration Persistence / Save Schema

Date : **28 septembre 2026**  
Statut : **IMPLÉMENTÉ — VALIDATION LOCALE UTILISATEUR REQUISE**

## 1. Objectif

MON21.6.5 rend durable la connaissance cartographique introduite par MON21.6.2–6.4, sans créer de snapshot Map parallèle.

Les données persistées sont directement celles de `FGridLevelRuntimeState::MapExploration` :

```text
ExploredCells
DiscoveredSecretObjectIds
```

## 2. Schéma SaveGame

`UGrimrockPartySaveGame::CurrentSaveVersion` passe de **22** à **23**.

Le prototype reste en politique **exact-match** :

```text
SaveVersion == 23 -> schéma courant
SaveVersion != 23 -> rejet
```

Aucune migration v22 -> v23 n’est ajoutée.

## 3. Frontière de sérialisation

Portent maintenant `SaveGame` :

- `FGridLevelRuntimeState::MapExploration` ;
- `FGridMapExplorationState::ExploredCells` ;
- `FGridMapExplorationState::DiscoveredSecretObjectIds`.

Il n’existe toujours ni `MapActor`, ni `MapSaveState`, ni miroir persistant séparé.

## 4. Validation structurelle

`UGrimrockPartySaveGame::ValidateCurrentState()` valide désormais chaque `MapExploration` présent dans `DungeonRuntimeState.LevelStates`.

Un état est valide si :

- `ExploredCells` est vide, ou contient exactement 1024 octets ;
- chaque octet vaut 0 ou 1 ;
- chaque `ObjectId` de secret découvert est un GUID valide.

Un état invalide est rejeté avant application au runtime.

## 5. Atomicité du chargement

Le chemin de production existant est conservé :

```text
LoadGameFromSlot
 -> UGrimrockPartySaveGame::Serialize
 -> ValidateCurrentState
 -> IsCompatible
 -> seulement ensuite LoadCurrentGameData applique DungeonRuntimeState
```

`LoadCurrentGameData()` possède déjà son snapshot de rollback pour les erreurs d’application ultérieures. MON21.6.5 n’ajoute donc aucune seconde transaction.

## 6. Validation MON21.6.4 actée

Validation utilisateur du 28 septembre 2026 :

```text
Filter                  : Grimrock.Map.MON21_6_4
Succeeded               : 4
Succeeded with warnings : 0
Failed                  : 0
Not run                 : 0
Process exit code       : 0
Report                  : TD04-20260928-084526
```

MON21.6.4 est **VALIDÉ**.

## 7. Automation MON21.6.5

Filtre : `Grimrock.Map.MON21_6_5`

```text
Persistence.SchemaFlags
Persistence.RoundTripPerLevel
Persistence.MalformedStateRejected
Persistence.ExactMatchVersion23
```

Le round-trip vérifie simultanément l’isolation par `LevelId`, les cellules explorées et les secrets découverts.

Aucun résultat MON21.6.5 n’est déclaré avant retour du harness UE5.5.4 local.

## 8. Hors périmètre

- Map Read Model : MON21.6.6 ;
- projection multi-dalles / étages : MON21.6.7 ;
- rendu WBP natif : MON21.6.8.

Prochaine tranche après validation : **MON21.6.6 — Map Read Model**.
