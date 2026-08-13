# MON13.3 — Commandes runtime des `MonsterSpawn`

## Objectif

MON13.3 rend les placements `MonsterSpawn` pilotables par le système central de
liens :

```text
Objet source + événement
    → MonsterSpawn + commande
    → Spawn / Despawn / Teleport atomique
    → événement de cycle de vie
```

`ObjectId` reste l'unique `SpawnId`. Une commande ne crée jamais une nouvelle
identité persistante et ne modifie pas le `LevelAsset`.

## Commandes prises en charge

| Commande | Effet sur une cible `MonsterSpawn` |
|---|---|
| `Spawn` | crée l'Actor s'il est absent et restaure son dernier état connu |
| `Despawn` | capture l'état, retire l'Actor, libère l'occupation et conserve une absence persistante |
| `Teleport` | replace l'Actor sur la cellule et l'orientation persistantes du placement |
| `Activate`, `Enable` | alias de `Spawn` |
| `Deactivate`, `Disable` | alias de `Despawn` |
| `Toggle` | alterne `Spawn` et `Despawn` |

Les opérations sont idempotentes : `Spawn` sur un Actor déjà présent et
`Despawn` sur un placement déjà absent réussissent sans duplication ni nouvel
événement.

L'API C++/Blueprint
`TeleportSpawnedMonster(SpawnId, TargetCellX, TargetCellY, TargetFacing)` permet
en plus de choisir explicitement une pose intra-niveau. La commande `Teleport`
d'un lien utilise la pose du placement comme destination par défaut.

## Événements

Trois valeurs sont ajoutées à la fin de `EGridObjectEvent`, sans modifier les
valeurs historiques :

- `MonsterSpawned` ;
- `MonsterDespawned` ;
- `MonsterTeleported`.

L'événement est émis seulement après la réussite complète de l'opération. Il
peut déclencher d'autres liens, par exemple une seconde vague :

```text
Trigger.Activated → Rat_A.Spawn
Rat_A.MonsterSpawned → Rat_B.Spawn
```

Une mutation réussie de la population ou de la pose interrompt le combat actif
afin que l'initiative ne conserve jamais un participant absent ou une position
devenue obsolète. Une opération refusée n'interrompt pas le combat.

## Atomicité et occupation

Avant un spawn ou une téléportation, le runtime vérifie :

- l'existence du placement et de sa définition ;
- une cellule valide, praticable et libre ;
- l'absence du groupe sur la cellule ;
- l'absence d'un autre monstre ou d'une réservation ;
- une orientation cardinale ;
- l'unicité du `SpawnId` et de l'identité MON9.

Un refus ne modifie ni l'Actor existant, ni la grille d'occupation, ni l'état
persistant. `Despawn` capture d'abord l'état ; si cette capture échoue, l'Actor
reste présent. `Teleport` utilise le rollback fourni par le composant de
mouvement et interrompt le combat actif seulement après validation de la cible.

## Persistance

`FGridLevelRuntimeState::MonsterPlacements` conserve, par `SpawnId` :

- `bIsSpawned` ;
- la présence éventuelle d'un dernier état ;
- le dernier `FGridRuntimeMonsterState` complet.

Cet état comprend notamment la cellule, l'orientation, les PV, les armures,
l'état IA, la mort, l'activation et `EncounterGroupId`.

Conséquences :

- un placement initialement désactivé, apparu par commande, revient après un
  rebuild ou une sauvegarde/charge ;
- un placement despawné ne réapparaît pas automatiquement lors d'un rebuild ;
- un nouveau `Spawn` restaure sa dernière cellule, son orientation et son état ;
- les anciennes sauvegardes sans `MonsterPlacements` conservent le comportement
  MON9/MON13.2 fondé sur `Monsters` et `bInitiallyEnabled`.

## Automation Tests

MON13.3 ajoute :

- `Grimrock.Monsters.MON13.3.DeferredSpawnLinks` ;
- `Grimrock.Monsters.MON13.3.LifecyclePersistence` ;
- `Grimrock.Monsters.MON13.3.AtomicCommands`.

Commande UE 5.5.4 :

```bat
D:\UE_5.5\Engine\Binaries\Win64\UnrealEditor-Cmd.exe D:\Development\GrimrockPrototype\GrimrockPrototype.uproject -unattended -nop4 -nosplash -NullRHI -ExecCmds="Automation RunTests Grimrock.Monsters.MON13.3" -TestExit="Automation Test Queue Empty" -ReportOutputPath="D:\Development\GrimrockPrototype\Saved\TestReports\MON13_3"
```

## Checklist UE 5.5.4

### 1 — Spawn différé

1. Placer un Rat géant avec `Enabled at Start` décoché.
2. Créer un lien `Trigger.Activated → Rat.Spawn`.
3. Lancer PIE et activer le trigger.

- [ ] aucun Actor n'existe avant l'activation ;
- [ ] un seul Actor apparaît après l'activation ;
- [ ] le `SpawnId`, la définition et `EncounterGroupId` correspondent au
  placement ;
- [ ] le log contient `MonsterSpawned` sans `PresentationWarning`.

### 2 — Despawn et réapparition

1. Ajouter un lien vers `Rat.Despawn`.
2. Déplacer ou blesser le rat, puis exécuter le lien.
3. Rebuilder le niveau, puis exécuter `Rat.Spawn`.

- [ ] le despawn libère immédiatement la cellule ;
- [ ] le rebuild ne recrée pas le rat ;
- [ ] le spawn suivant restaure cellule, orientation, PV et groupe ;
- [ ] aucun Actor partiel ou doublon n'apparaît.

### 3 — Téléportation

Depuis C++ ou Blueprint, appeler `TeleportSpawnedMonster` vers une cellule libre,
puis vers une cellule occupée.

- [ ] la pose libre est appliquée et persiste après rebuild ;
- [ ] la pose occupée est refusée ;
- [ ] après le refus, l'ancienne cellule reste occupée par le même monstre ;
- [ ] `MonsterTeleported` n'est émis que pour la réussite.

### 4 — Résultats à relever

| Contrôle | Résultat observé | Statut |
|---|---|---|
| Compilation Development Editor Win64 |  | [ ] OK / [ ] KO |
| Tests `Grimrock.Monsters.MON13.3` |  | [ ] OK / [ ] KO |
| Tests `Grimrock.Monsters.MON13` |  | [ ] OK / [ ] KO |
| Test `Grimrock.Monsters.MON8.MonsterDiedEvent` |  | [ ] OK / [ ] KO |
| Spawn différé et cascade |  | [ ] OK / [ ] KO |
| Despawn persistant |  | [ ] OK / [ ] KO |
| Téléportation et rollback |  | [ ] OK / [ ] KO |
| Sauvegarde/charge |  | [ ] OK / [ ] KO |

## Hors périmètre

- téléportation inter-niveaux d'un monstre ;
- suppression définitive non réversible d'un placement ;
- résolution Asset Manager depuis `MonsterDefinitionId` seul ;
- gestion globale des rencontres, des vagues et de leur état terminé par
  `EncounterGroupId`.

Ces points appartiennent aux jalons MON13 suivants.
