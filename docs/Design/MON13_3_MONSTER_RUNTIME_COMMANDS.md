# MON13.3 — Commandes runtime des `MonsterSpawn`

Statut : **contrat courant après WORLDOBJ-MIG10 et refonte des états initiaux sémantiques**, 2026-09-12.

## Objectif

MON13.3 rend les placements typés `FGridMonsterSpawnInstance` pilotables par le système central de liens :

```text
Objet source + événement
    → MonsterSpawn ciblé par SpawnId
    → Spawn / Despawn / Teleport atomique
    → événement de cycle de vie
```

Une commande ne modifie pas le `LevelAsset` et ne crée jamais une nouvelle identité persistante. `SpawnId` reste l'identité du générateur et du monstre associé.

## État initial

Le seul contrôle de présence initiale du générateur est :

```text
bSpawnAtStart
```

- `true` : création initiale du monstre ;
- `false` : placement conservé mais aucun Actor au démarrage.

Il n'existe plus de `Enabled at Start`, `bInitiallyEnabled` ou `bInitiallyActive` pour `MonsterSpawn`.

Un placement avec `Spawn at Start=false` est le cas normal pour un monstre destiné à apparaître plus tard via un lien, une vague de rencontre ou une commande scriptée.

## Commandes prises en charge

| Commande | Effet sur une cible `MonsterSpawn` |
|---|---|
| `Spawn` | crée l'Actor s'il est absent et restaure le dernier état connu lorsqu'il existe |
| `Despawn` | capture l'état, retire l'Actor, libère l'occupation et mémorise l'absence |
| `Teleport` | replace l'Actor sur la pose persistante du placement |
| `Activate`, `Enable` | alias de `Spawn` |
| `Deactivate`, `Disable` | alias de `Despawn` |
| `Toggle` | alterne `Spawn` et `Despawn` |

Les opérations sont idempotentes : une commande qui demande un état déjà atteint ne doit pas créer de doublon ni d'événement parasite.

L'API :

```cpp
TeleportSpawnedMonster(SpawnId, TargetCellX, TargetCellY, TargetFacing)
```

permet de choisir explicitement une destination intra-niveau. La commande de lien `Teleport` utilise la pose du placement lorsqu'aucune autre destination n'est fournie par le chemin concerné.

## Événements de cycle de vie

Les événements associés sont :

```text
MonsterSpawned
MonsterDespawned
MonsterTeleported
MonsterDied
```

Un événement de cycle de vie n'est émis qu'après la réussite complète de l'opération correspondante.

Exemple :

```text
Trigger.Activated
    → Rat_A.Spawn

Rat_A.MonsterSpawned
    → Rat_B.Spawn
```

`MonsterSpawn` peut donc être à la fois cible de commande et source de liens.

## Atomicité et occupation

Avant un `Spawn` ou un `Teleport`, le runtime valide notamment :

- l'existence du placement `FGridMonsterSpawnInstance` ;
- un `SpawnId` valide ;
- une `MonsterDefinition` valide ;
- une cellule valide et praticable ;
- un `Facing` cardinal ;
- l'absence du groupe sur la cellule ;
- l'absence d'un autre monstre ou d'une réservation ;
- l'absence d'une autre identité persistante utilisant le même `SpawnId`.

Un refus ne doit modifier ni l'Actor courant, ni l'occupation, ni l'état persistant.

`Despawn` capture l'état avant destruction lorsqu'une conservation est requise. Si cette capture échoue, l'Actor reste présent.

`Teleport` doit conserver l'ancienne pose lorsque la destination est refusée.

Les mutations réussies de population ou de position interrompent les opérations de combat qui ne peuvent pas conserver une initiative cohérente avec un participant supprimé ou déplacé.

## Persistance

`FGridLevelRuntimeState::MonsterPlacements` conserve, par `SpawnId`, l'état dynamique du générateur :

```text
SpawnId
bIsSpawned
bHasMonsterState
MonsterState
```

`MonsterState` peut notamment conserver :

- cellule ;
- orientation ;
- PV et armures ;
- état IA ;
- état mort/vivant ;
- métadonnées de rencontre.

Conséquences :

- un placement `Spawn at Start=false` peut apparaître plus tard et rester présent après rebuild/restauration ;
- un placement despawné ne doit pas réapparaître simplement à cause d'un rebuild ;
- un `Spawn` ultérieur peut restaurer le dernier état mémorisé ;
- le `LevelAsset` reste inchangé : l'état mutable appartient au runtime/save state.

La compatibilité avec d'anciens schémas de sauvegarde n'est pas une raison pour réintroduire `bInitiallyEnabled` dans le modèle courant.

## Playtest frais et Continue

Un PIE préparé depuis le Grimrock Grid Editor utilise un état de donjon frais pour le playtest. Les états de rencontre et `MonsterPlacements` provenant d'une sauvegarde normale ne doivent pas contaminer ce scénario.

Un lancement normal en mode `Continue` peut au contraire restaurer l'état persistant complet du niveau.

## Grid Editor

Dans les liens, un `MonsterSpawn` doit pouvoir être sélectionné comme cible. Les commandes pertinentes incluent les commandes principales et leurs alias compatibles.

Pour préparer un spawn différé :

```text
Spawn at Start = false
Trigger.Activated → MonsterSpawn.Spawn
```

Aucun `Enabled at Start` générique n'est nécessaire.

## Tests Automation

MON13.3 est couvert notamment par :

```text
Grimrock.Monsters.MON13.3.DeferredSpawnLinks
Grimrock.Monsters.MON13.3.LifecyclePersistence
Grimrock.Monsters.MON13.3.AtomicCommands
Grimrock.Monsters.MON13.3.EditorLinkPolicy
Grimrock.Monsters.MON13.3.FreshPIEPreparation
```

Le test PIE réel de la famille MON13 est complété par :

```text
Grimrock.Monsters.MON13.5.RealPIEIntegration
```

Validation locale recommandée :

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.Monsters.MON13.3"
```

## Checklist UE 5.5.4

### Spawn différé

1. Placer un monstre avec `Spawn at Start` décoché.
2. Créer `Trigger.Activated → Rat.Spawn`.
3. Lancer PIE.
4. Vérifier qu'aucun Actor n'existe avant l'activation.
5. Activer le trigger.

Résultat attendu : un seul Actor apparaît, avec le même `SpawnId`, la bonne définition et les bonnes métadonnées de rencontre ; `MonsterSpawned` est émis après réussite.

### Despawn / Respawn

1. Ajouter une commande `Despawn`.
2. Déplacer ou blesser le monstre.
3. Exécuter `Despawn`.
4. Rebuilder le niveau.
5. Exécuter `Spawn`.

Résultat attendu : cellule libérée après despawn, absence conservée pendant le rebuild, puis dernier état restauré au respawn sans doublon.

### Téléportation

Tester une destination libre puis une destination occupée.

Résultat attendu :

- destination libre appliquée et persistée ;
- destination occupée refusée ;
- ancienne cellule conservée après refus ;
- `MonsterTeleported` émis uniquement après réussite.

## Hors périmètre

- téléportation inter-niveaux d'un monstre ;
- suppression définitive non réversible d'un placement ;
- création d'une nouvelle identité de monstre indépendante du `SpawnId` ;
- réintroduction d'un état générique `InitiallyEnabled/InitiallyActive`.

La gestion des rencontres et des vagues est décrite dans `MON13_4_MONSTER_ENCOUNTER_WAVES.md`.
