# MON13.2 — Pipeline d'instanciation `MonsterSpawn`

## Objectif

MON13.2 relie le placement persistant créé en MON13.1 à son Actor de jeu :

```text
FGridLevelObjectData MonsterSpawn
    → UGridMonsterDefinitionAsset
    → MonsterActorClass
    → AGridMonsterActor initialisé
```

Un Rat géant placé dans le Grid Editor doit désormais apparaître avec son mesh
squelettique dans l'aperçu, puis être créé comme véritable monstre au lancement
du niveau.

`DA_MonsterSpawn.RuntimeActorClass` doit rester à `None`. La classe concrète est
toujours lue depuis `DA_MON_RatGiant.MonsterActorClass`.

## Contrat de résolution strict

`AGridLevelRuntimeActor::ResolveMonsterSpawn()` refuse le placement avant toute
création d'Actor si l'une des conditions suivantes n'est pas satisfaite :

- le type est `MonsterSpawn` ;
- `ObjectId`, qui est aussi le `SpawnId`, est valide ;
- la cellule existe, n'est pas vide et autorise l'occupation ;
- `Edge=None` ;
- `InitialFacing` est `North`, `East`, `South` ou `West` ;
- `MonsterDefinitionAsset` est présent ;
- `MonsterDefinitionId` est présent et égal au `MonsterId` du DataAsset ;
- la définition complète est valide ;
- `MonsterActorClass` existe, dérive d'`AGridMonsterActor` et n'est pas
  abstraite.

MON13.2 ne tente pas encore de charger un DataAsset à partir d'un
`MonsterDefinitionId` seul. Le pointeur `MonsterDefinitionAsset` reste donc
obligatoire dans le placement.

## Création runtime

Pour chaque `MonsterSpawn` dont `bInitiallyEnabled=true`, le runtime :

1. résout et valide toutes les données sans modifier le monde ;
2. vérifie l'unicité du `SpawnId` dans les Actors présents ;
3. refuse une cellule occupée par le groupe, un autre monstre ou une
   réservation ;
4. calcule le centre exact de la cellule et la rotation depuis
   `InitialFacing` ;
5. crée `MonsterActorClass` avec `SpawnActorDeferred` ;
6. appelle `InitializeMonster()` avant `FinishSpawningActor()` ;
7. transmet la définition, le `SpawnId`, la cellule, l'orientation et
   `EncounterGroupId` ;
8. initialise les PV, armures, composants visuels et métadonnées de niveau ;
9. enregistre l'Actor généré dans une table indexée par `SpawnId`.

Un placement désactivé ne crée aucun Actor et n'est pas compté comme une erreur.
`bInitiallyActive` reste une propriété générique : il ne commande pas encore un
spawn dynamique.

### Refus atomique

Une erreur de définition, d'identité, de cellule, d'orientation, de classe ou
d'occupation produit un log `Skipped` et aucun Actor n'est conservé. Le compteur
`RuntimeMonsterSpawnFailureCount` permet de repérer immédiatement le nombre de
placements activés qui n'ont pas pu être créés.

Une configuration de présentation incomplète — mesh ou Animation Blueprint
absent — n'invalide pas les données de combat. L'Actor gameplay peut exister,
mais le log contient alors `PresentationWarning` et l'aperçu squelettique est
omis si le mesh manque.

## Rebuild et persistance

Les monstres créés depuis le `LevelAsset` sont distincts des monstres placés
directement dans une carte :

- ils sont suivis dans `SpawnedMonsterActors` par leur `SpawnId` ;
- un rebuild complet interrompt le combat, libère l'occupation et détruit
  uniquement ces Actors générés ;
- le pipeline les recrée ensuite depuis le `LevelAsset` ;
- MON9 restaure leur état sauvegardé en utilisant le même `SpawnId` : cellule,
  orientation, PV, armures, mort, activation et rencontre.

Ce cycle permet de quitter puis revisiter un niveau sans conserver un Actor de
l'ancien niveau ni créer une seconde identité persistante.

## Aperçu éditeur

`UGridEditorPreviewComponent` traite `MonsterSpawn` séparément des objets à mesh
statique. Il crée un `AGridEditorPreviewObjectActor` transitoire avec un
`USkeletalMeshComponent`, puis applique :

- `SkeletalMesh` ;
- `VisualOffset` ;
- `VisualScale` ;
- `AnimationClass` ;
- la position de cellule ;
- la rotation autoritaire de `InitialFacing` ;
- les stencils de survol et de sélection.

Cet Actor de prévisualisation est editor-only, sans collision et sans logique
de combat. Aucun `AGridMonsterActor` gameplay n'est créé hors PIE.

## Diagnostics attendus

Création réussie :

```text
[GridMonsterSpawn] Spawned SpawnId=... DefinitionId=MON_RatGiant Class=... Cell=(X,Y) Facing=... Encounter=... RuntimeLevel=...
```

Refus avant création :

```text
[GridMonsterSpawn] Skipped SpawnId=... Cell=(X,Y) DefinitionId=... Reason=...
```

Présentation incomplète :

```text
[GridMonsterSpawn] PresentationWarning SpawnId=... DefinitionId=... Actor=... Reason=...
```

Le résumé runtime contient également :

```text
Spawned Monsters=N Failures=M
```

Le warning historique suivant ne doit plus apparaître pour un
`MonsterSpawn` :

```text
Runtime object skipped: archetype MonsterSpawn has no RuntimeActorClass.
```

## Automation Tests

MON13.2 ajoute :

- `Grimrock.Monsters.MON13.2.RuntimePipeline` ;
- `Grimrock.Monsters.MON13.2.AtomicFailure` ;
- `Grimrock.Monsters.MON13.2.EditorPreview`.

Commande UE 5.5.4 :

```bat
D:\UE_5.5\Engine\Binaries\Win64\UnrealEditor-Cmd.exe D:\Development\GrimrockPrototype\GrimrockPrototype.uproject -unattended -nop4 -nosplash -NullRHI -ExecCmds="Automation RunTests Grimrock.Monsters.MON13.2" -TestExit="Automation Test Queue Empty" -ReportOutputPath="D:\Development\GrimrockPrototype\Saved\TestReports\MON13_2"
```

## Checklist éditeur et PIE

### 1 — Configuration des assets

- [ ] ouvrir `DA_MonsterSpawn` et vérifier `RuntimeActorClass=None` ;
- [ ] ouvrir `DA_MON_RatGiant` et vérifier `MonsterActorClass` ;
- [ ] vérifier `SkeletalMesh`, `AnimationClass`, `VisualOffset` et
  `VisualScale` ;
- [ ] ouvrir `DA_ObjectPalette_Default` ;
- [ ] vérifier `Default Archetype=DA_MonsterSpawn` ;
- [ ] vérifier `Default Monster Definition=DA_MON_RatGiant` ;
- [ ] sauvegarder les trois DataAssets ;
- [ ] lancer `Refresh Validation` sans erreur MON13.

### 2 — Aperçu dans le Grid Editor

1. Placer un Rat géant sur une cellule praticable et libre.
2. Choisir une orientation facile à vérifier, par exemple `East`.
3. Sélectionner un autre objet puis revenir au rat.
4. Utiliser `Reload Current`.

Résultats attendus :

- [ ] le Rat géant est visible sans lancer PIE ;
- [ ] il est centré sur la cellule ;
- [ ] son échelle et son offset correspondent à `DA_MON_RatGiant` ;
- [ ] son orientation reste `East` après rechargement ;
- [ ] le contour de sélection/survol fonctionne ;
- [ ] le World Outliner ne contient pas d'`AGridMonsterActor` gameplay créé par
  l'aperçu ;
- [ ] aucun warning `RuntimeActorClass` n'est produit.

### 3 — Création au lancement du niveau

1. Noter le `SpawnId`, la cellule, l'orientation et `EncounterGroupId`.
2. Lancer PIE depuis le niveau runtime normal.
3. Rechercher `[GridMonsterSpawn]` dans l'Output Log.
4. Sélectionner le Rat géant dans le World Outliner pendant PIE.

Résultats attendus :

- [ ] une ligne `Spawned` existe pour le `SpawnId` noté ;
- [ ] un seul Actor de la classe indiquée par `MonsterActorClass` existe ;
- [ ] `SpawnObjectId` est égal au `SpawnId` ;
- [ ] `PersistentMonsterId` n'est pas utilisé comme seconde identité ;
- [ ] `MonsterDefinition` vaut `DA_MON_RatGiant` ;
- [ ] `CurrentCell`, `Facing` et `EncounterGroupId` correspondent au placement ;
- [ ] `CurrentHealth=MaxHealth` et `bCombatStatsInitialized=true` ;
- [ ] le rat possède son mesh, son Animation Blueprint, son offset et son
  échelle ;
- [ ] le résumé runtime affiche `Spawned Monsters=1 Failures=0` ;
- [ ] aucun warning `Runtime object skipped` ne concerne `MonsterSpawn`.

### 4 — Rebuild sans duplication

Pendant PIE, utiliser le bouton ou la commande de rebuild complet du runtime,
hors combat.

- [ ] l'ancien Actor est détruit ;
- [ ] un seul nouvel Actor porte le même `SpawnId` ;
- [ ] aucune cellule n'est occupée deux fois ;
- [ ] le résumé reste `Spawned Monsters=1 Failures=0`.

### 5 — Refus contrôlé sur une copie de test

Effectuer une seule mutation à la fois sur une copie du `LevelAsset`, puis
restaurer immédiatement la donnée :

| Mutation | Résultat attendu |
|---|---|
| vider `MonsterDefinitionAsset` | `Skipped`, aucun Actor |
| modifier seulement `MonsterDefinitionId` | `Skipped`, aucun Actor |
| mettre `InitialFacing=None` | `Skipped`, aucun Actor |
| placer le spawn sur une cellule bloquée | `Skipped`, aucun Actor |
| dupliquer le `SpawnId` d'un autre rat | premier Actor conservé, second refusé |
| décocher `Enabled at Start` | aucun Actor et aucune erreur runtime |

Après chaque cas, vérifier que `Failures` augmente uniquement pour les
placements activés invalides et qu'aucun Actor partiel n'apparaît dans le World
Outliner.

### 6 — Résultat à relever

| Contrôle | Résultat observé | Statut |
|---|---|---|
| Compilation Development Editor Win64 |  | [ ] OK / [ ] KO |
| Tests `Grimrock.Monsters.MON13.2.*` |  | [ ] OK / [ ] KO |
| Aperçu squelettique hors PIE |  | [ ] OK / [ ] KO |
| Création unique en PIE |  | [ ] OK / [ ] KO |
| Identité et données transmises |  | [ ] OK / [ ] KO |
| Rebuild sans duplication |  | [ ] OK / [ ] KO |
| Refus atomiques |  | [ ] OK / [ ] KO |
| Absence du warning historique |  | [ ] OK / [ ] KO |

En cas d'échec, relever le `SpawnId`, la cellule, la définition, la classe, la
ligne `[GridMonsterSpawn]` complète et une capture du World Outliner.

## Hors périmètre

- commande `Spawn` pendant le jeu ;
- activation différée d'un placement désactivé ;
- `Despawn` temporaire ou définitif ;
- `Teleport` intra-niveau ou inter-niveaux ;
- résolution Asset Manager d'un `MonsterDefinitionId` sans pointeur d'asset ;
- gestion globale des rencontres par `EncounterGroupId`.

Ces opérations commencent avec MON13.3.
