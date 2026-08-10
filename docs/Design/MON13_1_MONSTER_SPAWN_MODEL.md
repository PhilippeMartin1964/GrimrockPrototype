# MON13.1 — Modèle persistant `MonsterSpawn`

## Objectif

MON13.1 complète les données nécessaires au futur pipeline :

```text
MonsterSpawn → MonsterDefinition → AGridMonsterActor
```

Ce jalon ne crée encore aucun Actor. Il définit et valide la source persistante
dans le `UGridLevelAsset`, tout en préservant l'autorité logique de la grille et
les identités MON8/MON9.

## Décision d'architecture

`MonsterSpawn` reste un `FGridLevelObjectData` dans
`UGridLevelAsset::Objects`. Aucun tableau parallèle et aucun second GUID ne sont
ajoutés.

| Concept MON13 | Donnée persistante |
|---|---|
| `SpawnId` | `FGridLevelObjectData::ObjectId` |
| Définition | `MonsterDefinitionAsset` et `MonsterDefinitionId` |
| Cellule | `CellX`, `CellY` |
| Orientation | `InitialFacing` |
| Rencontre | `EncounterGroupId` |
| État initial | `bInitiallyEnabled` |

`ObjectId` reste donc directement compatible avec `SpawnObjectId` et
`ResolvePersistenceId()` de MON9.

## Définition du monstre

`UGridMonsterDefinitionAsset` porte désormais `MonsterActorClass`. Sa valeur par
défaut est `AGridMonsterActor`, ce qui conserve les définitions existantes et
permet à une définition spécialisée de choisir ultérieurement une sous-classe
Blueprint.

La validation d'une définition refuse une classe d'Actor absente. La classe ne
porte pas l'identité de l'instance : l'identité persistante reste exclusivement
le `SpawnId` du niveau.

## Orientation et compatibilité

`InitialFacing` accepte uniquement `North`, `East`, `South` ou `West` et devient
la source de vérité gameplay.

`LocalYaw` demeure un miroir de compatibilité pour l'aperçu générique existant.
Au chargement des anciens assets, une orientation manquante est déduite du yaw
historique, puis le yaw est normalisé sur l'orientation cardinale. Les nouveaux
placements commencent au nord et l'inspecteur met les deux valeurs en cohérence.

## Palette et inspecteur

Une `FGridObjectPaletteEntry` de type `MonsterSpawn` doit renseigner
`DefaultMonsterDefinition`. Lors du placement, l'éditeur copie :

- le DataAsset de définition ;
- son `MonsterId` dans `MonsterDefinitionId` ;
- l'orientation initiale nord ;
- l'état initial de l'archetype.

L'inspecteur Slate expose :

- `SpawnId / ObjectId` en lecture seule ;
- `MonsterDefinitionAsset` ;
- `MonsterDefinitionId` et sa synchronisation ;
- `EncounterGroupId` ;
- `InitialFacing` via les quatre boutons d'orientation ;
- l'état initial ;
- la classe d'Actor résolue par la définition.

Aucun WBP ni asset binaire n'est modifié par MON13.1. Pour chaque entrée de
palette existante représentant un monstre, il faut assigner manuellement
`DefaultMonsterDefinition`, par exemple `DA_MON_RatGiant`.

## Validation

`UGridLevelAsset::ValidateMonsterSpawns()` vérifie :

- un `ObjectId/SpawnId` valide et unique parmi tous les objets du niveau ;
- une cellule dans les limites, présente, non vide et autorisant l'occupation ;
- `Edge=None`, car le spawn est centré sur une cellule ;
- une orientation cardinale ;
- une définition ou un identifiant résolvable ;
- la validité complète du DataAsset lorsqu'il est directement référencé ;
- l'égalité de `MonsterDefinitionId` et `MonsterDefinitionAsset::MonsterId` ;
- l'absence de deux spawns initialement activés sur la même cellule.

Deux spawns désactivés peuvent partager une cellule dans les données. Le futur
spawn runtime devra néanmoins refuser atomiquement toute activation dont la
destination est occupée.

L'outil `Validate Current Level` applique les mêmes règles et rattache chaque
message au `SpawnId` concerné.

## Tests Automation

Trois tests sont ajoutés :

```text
Grimrock.Monsters.MON13.1.PersistentModel
Grimrock.Monsters.MON13.1.Validation
Grimrock.Monsters.MON13.1.PaletteContract
```

Ils couvrent la génération du `SpawnId`, la migration du yaw, la synchronisation
de l'identifiant, la classe d'Actor par défaut, les rejets de placement et le
contrat de palette.

Commande UE 5.5.4 :

```bat
D:\UE_5.5\Engine\Binaries\Win64\UnrealEditor-Cmd.exe D:\Development\GrimrockPrototype\GrimrockPrototype.uproject -unattended -nop4 -nosplash -NullRHI -ExecCmds="Automation RunTests Grimrock.Monsters.MON13.1" -TestExit="Automation Test Queue Empty" -ReportOutputPath="D:\Development\GrimrockPrototype\Saved\TestReports\MON13_1"
```

## Checklist éditeur

- [ ] ouvrir la palette d'objets et assigner `DA_MON_RatGiant` à
  `DefaultMonsterDefinition` de l'entrée Rat géant ;
- [ ] placer deux rats sur deux cellules praticables ;
- [ ] vérifier la génération de deux `SpawnId` distincts ;
- [ ] modifier les quatre orientations et contrôler l'aperçu ;
- [ ] renseigner un même `EncounterGroupId` ;
- [ ] exécuter `Validate Current Level` sans erreur MON13 ;
- [ ] provoquer temporairement un conflit de cellule et vérifier son rejet.

## Hors périmètre

- résolution asynchrone d'un `MonsterDefinitionId` seul ;
- création d'`AGridMonsterActor` ;
- enregistrement dans l'occupation et le combat ;
- `Spawn`, `Despawn` et `Teleport` commandés ;
- reconstruction d'un Actor absent depuis une sauvegarde.

Ces opérations commencent avec MON13.2, qui implémentera la résolution stricte
`MonsterSpawn → MonsterDefinition → MonsterActorClass` et l'initialisation
déterministe de l'Actor, sans encore introduire les commandes runtime complètes.
