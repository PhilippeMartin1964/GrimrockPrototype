# MON13.2 — Pipeline d'instanciation `MonsterSpawn`

Statut : **contrat courant après WORLDOBJ-MIG10 et refonte des états initiaux sémantiques**, 2026-09-12.

## Objectif

MON13.2 relie le placement typé persistant à son Actor de jeu :

```text
FGridMonsterSpawnInstance
    → MonsterDefinition
    → UGridMonsterDefinitionAsset::MonsterActorClass
    → AGridMonsterActor initialisé
```

Le jalon historique utilisait encore `FGridLevelObjectData`, `MonsterDefinitionId`, `Edge`, `bInitiallyEnabled` et `bInitiallyActive`. Ces champs ne font plus partie du pipeline courant.

## Source de vérité

Le runtime lit directement `UGridLevelAsset::MonsterSpawns`.

Pour chaque `FGridMonsterSpawnInstance` :

- `SpawnId` est l'identité persistante ;
- `MonsterDefinition` est la référence directe à la définition ;
- `CellX`, `CellY` et `Facing` décrivent la pose ;
- `bSpawnAtStart` décide de la création initiale ;
- `InitialMonsterState` décrit l'état initial du monstre créé ;
- `EncounterGroupId` et `EncounterWaveIndex` décrivent son appartenance à une rencontre.

La classe gameplay est toujours résolue depuis :

```text
MonsterDefinition->MonsterActorClass
```

Aucun `RuntimeActorClass` d'un world-object générique n'est utilisé pour créer un monstre.

## Contrat de résolution strict

`AGridLevelRuntimeActor::ResolveMonsterSpawn()` refuse un placement avant création si le contrat minimal n'est pas satisfait :

- `LevelAsset` existe ;
- `SpawnId` est valide ;
- la cellule est dans le niveau et autorise l'occupation ;
- `Facing` est cardinal ;
- `MonsterDefinition` existe ;
- la définition passe `ValidateDefinition()` ;
- `MonsterActorClass` existe ;
- la classe dérive d'`AGridMonsterActor` ;
- la classe n'est pas abstraite.

Il n'existe plus de validation d'un `MonsterDefinitionId` dupliqué dans le placement, puisque ce miroir a été supprimé.

## Création initiale

Au démarrage initial d'un niveau, seuls les placements avec :

```text
bSpawnAtStart = true
```

sont candidats à la création.

`bSpawnAtStart=false` signifie simplement : **pas d'Actor initial**. Le placement reste dans `MonsterSpawns` et peut être utilisé plus tard par les commandes runtime MON13.3.

Le runtime :

1. résout la définition et la classe ;
2. calcule le transform centré sur la cellule depuis `Facing` ;
3. refuse les conflits d'identité ;
4. refuse une cellule occupée par le groupe, un autre monstre ou une réservation ;
5. crée l'Actor avec `SpawnActorDeferred` ;
6. appelle `InitializeMonster()` avec la définition, le `SpawnId`, la cellule, le facing et le groupe de rencontre ;
7. termine le spawn ;
8. initialise/enregistre l'occupation ;
9. applique les métadonnées de placement et l'état runtime attendu ;
10. conserve l'Actor généré dans la table indexée par `SpawnId`.

Un refus est atomique : aucun Actor partiellement initialisé ne doit rester dans le monde.

## Occupation et identité

Avant de créer un monstre vivant, le runtime vérifie notamment :

- qu'aucun Actor existant n'utilise déjà le même `SpawnId` comme identité persistante ;
- qu'aucun monstre généré vivant n'occupe déjà la cellule ;
- que le groupe n'occupe pas la cellule ;
- que `UGridMonsterOccupancySubsystem` n'y signale pas de conflit.

L'identité n'est jamais régénérée pendant un rebuild : `SpawnId` reste stable.

## Présentation

La validité gameplay de la définition est distincte de la présentation. Le runtime peut produire un diagnostic `PresentationWarning` si le setup visuel de l'Actor est incomplet.

Pour le Rat géant de production, la définition doit pointer vers la classe Blueprint réellement équipée des composants requis par le gameplay, par exemple :

```text
/Game/GrimrockPrototype/Monsters/RatGiant/Blueprints/BP_MON_RatGiant.BP_MON_RatGiant_C
```

La classe native `AGridMonsterActor` seule ne garantit pas la composition Blueprint attendue par tous les systèmes de combat/mouvement.

## Aperçu éditeur

`UGridEditorPreviewComponent` traite `FGridMonsterSpawnInstance` comme un placement typé de monstre. Il résout la même `MonsterDefinition`, puis affiche un Actor de preview transitoire à partir de la présentation de la définition.

L'aperçu applique notamment :

- le Skeletal Mesh ;
- l'Animation Class ;
- l'offset et l'échelle de présentation ;
- la cellule ;
- le `Facing` cardinal ;
- les stencils de sélection/survol.

L'Actor de preview n'est pas un `AGridMonsterActor` gameplay et n'entre pas dans l'occupation ou le combat.

## Rebuild et persistance

Les monstres générés depuis le niveau sont suivis par `SpawnId`. Un rebuild complet :

- interrompt les opérations runtime qui ne peuvent pas survivre au rebuild ;
- libère/détruit les Actors générés concernés ;
- reconstruit depuis les placements et l'état runtime persistant ;
- ne crée aucune seconde identité.

Lorsqu'un état runtime a déjà été enregistré, la restauration peut replacer le monstre dans sa dernière cellule/orientation et restaurer son état plutôt que de repartir systématiquement du placement initial.

## Diagnostics

Création réussie :

```text
[GridMonsterSpawn] Spawned SpawnId=... DefinitionId=... Class=... Cell=(X,Y) Facing=... Encounter=...
```

Refus :

```text
[GridMonsterSpawn] Skipped SpawnId=... Cell=(X,Y) Definition=... Reason=...
```

Présentation incomplète :

```text
[GridMonsterSpawn] PresentationWarning SpawnId=... Reason=...
```

`RuntimeMonsterSpawnFailureCount` permet de repérer les créations attendues mais refusées.

## Tests Automation

Le pipeline MON13.2 est couvert notamment par :

```text
Grimrock.Monsters.MON13.2.RuntimePipeline
Grimrock.Monsters.MON13.2.AtomicFailure
Grimrock.Monsters.MON13.2.EditorPreview
```

Validation locale recommandée :

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.Monsters.MON13.2"
```

## Checklist Grid Editor / PIE

### Configuration

- vérifier `MonsterDefinition` et `MonsterActorClass` ;
- vérifier Skeletal Mesh, Animation Class, offset et échelle ;
- vérifier l'entrée de palette et sa `DefaultMonsterDefinition` ;
- lancer `Refresh Validation`.

### Aperçu

1. Placer un monstre sur une cellule libre.
2. Choisir un `Facing` facile à reconnaître.
3. Vérifier l'aperçu hors PIE.
4. Sauvegarder/recharger.

Résultat attendu : même `SpawnId`, même cellule, même orientation et aucune création d'un Actor gameplay dans l'éditeur.

### Création initiale

1. Cocher `Spawn at Start`.
2. Lancer PIE.
3. Vérifier le log `[GridMonsterSpawn]`.

Résultat attendu : un seul Actor, même `SpawnId`, bonne définition, bonne cellule et bonne orientation.

### Spawn différé

1. Décocher `Spawn at Start`.
2. Lancer PIE.

Résultat attendu : aucun Actor initial et aucune erreur simplement parce que le placement est différé.

### Refus contrôlés

Sur une copie de niveau, tester séparément :

- `MonsterDefinition=null` ;
- `Facing=None` ;
- cellule bloquée ;
- `SpawnId` dupliqué ;
- cellule déjà occupée.

Chaque cas doit être refusé sans Actor résiduel ni duplication.

## Suite

MON13.3 ajoute les commandes runtime `Spawn`, `Despawn`, `Teleport` et les événements de cycle de vie associés.
