# TD05.7 — RuntimeActor post-Feedback re-audit

Date : 26 août 2026  
Baseline auditée : `1edfb86755f51ef74caea47a00073561eef35801` — `Extract TD05.6 RuntimeActor feedback UI`

## Statut

**RÉALISÉ — STOP CONDITION NON ATTEINTE**

TD05.6 a extrait la présentation Feedback UI sans modifier l’API publique ni l’état autoritaire de `AGridLevelRuntimeActor`. La validation UE5.5.4 post-extraction de `Grimrock.TechnicalDebt.TD05_5.RuntimeActorFeedbackUI.Contract` est verte : 1 Success, 0 warning, 0 failure.

## 1. Évolution du fichier principal

```text
TD05.1 baseline
GridLevelRuntimeActor.cpp     3 359 lignes

Après TD05.3 Diagnostics
GridLevelRuntimeActor.cpp     2 951 lignes

Après TD05.6 Feedback UI
GridLevelRuntimeActor.cpp     2 768 lignes
```

Les extractions TD05 ont donc retiré 591 lignes du fichier principal depuis la re-baseline, soit environ 17,6 %, tout en conservant `AGridLevelRuntimeActor` comme façade/orchestrateur unique.

Unités dédiées actuellement :

```text
GridLevelRuntimeActorPersistence.cpp
GridLevelRuntimeActorWorldItems.cpp
GridLevelRuntimeActorDiagnostics.cpp
GridLevelRuntimeActorFeedbackUI.cpp
```

## 2. Responsabilités restantes

Le fichier principal conserve encore plusieurs domaines distincts :

```text
Lifecycle / BeginPlay / EndPlay
Geometry / render transforms / rebuild
Grid queries / movement / walls
Doors / edge interaction
Dungeon transitions
Editor preview façade
Object archetype resolution / placement
Generic runtime object spawning
Monster spawn / lifecycle / encounter façade
RebuildRuntimeObjects orchestration
```

Le volume seul ne justifie pas un découpage. La question est de savoir si une responsabilité peut quitter la translation unit sans créer une seconde autorité ni multiplier les dépendances croisées.

## 3. Frontières réévaluées

### Geometry / rebuild

**Différée.**

La géométrie reste couplée aux ISM, aux transformations, au preview éditeur, à `RebuildLevel`, à `ClearVisuals` et à l’orchestration de spawn. Une extraction maintenant serait principalement volumétrique.

### Doors / interactions / transitions

**Différée.**

Cette zone partage les conventions directionnelles, les helpers d’arêtes et les composants Activation/Door. La frontière existe conceptuellement mais n’est pas aussi autonome que les extractions Diagnostics/Feedback.

### Generic runtime objects / placement

**Conservée dans le fichier principal pour l’instant.**

`RebuildRuntimeObjects()` est l’orchestrateur transversal des items, MonsterSpawn et objets runtime génériques. Il doit rester dans la façade principale tant que les sous-domaines sont extraits comme implémentations spécialisées.

### Monster spawn / lifecycle / encounter façade

**Retenue comme prochaine frontière TD05.8.**

Le domaine possède une cohérence fonctionnelle forte :

```text
AbortActiveCombatAndMonsterActions
SetMonsterRuntimeLevelActive
ApplyInitialMonsterStateForCurrentLevel
ClearSpawnedMonsterActors
IsPartyOnCell
ApplyMonsterPlacementMetadata
ResolveMonsterSpawn
GetMonsterSpawnTransform
FindSpawnedMonsterActor
GetSpawnedMonsterActorCount
StartMonsterEncounter
IsMonsterEncounterCompleted
GetMonsterEncounterActiveWave
NotifyMonsterEncounterDeath
StoreMonsterPlacementState
DespawnMonsterSpawnActor
ExecuteMonsterSpawnCommand
TeleportSpawnedMonster
AddMonsterSpawnActor
```

`RebuildRuntimeObjects()` reste explicitement hors de l’extraction : il continue d’orchestrer tous les types d’objets et appelle la façade Monster existante.

## 4. Pourquoi Monster devient acceptable malgré son couplage

TD05.4 avait différé cette frontière parce qu’elle touche persistance, occupancy, combat, encounter et Event -> Command. TD05.7 ne nie pas ce couplage ; il constate que le contrat est déjà très fortement caractérisé par MON13.

La suite existante `GridMonsterMON13SpawnTests.cpp` couvre notamment :

```text
Grimrock.Monsters.MON13.2.RuntimePipeline
Grimrock.Monsters.MON13.2.AtomicFailure
Grimrock.Monsters.MON13.3.DeferredSpawnLinks
Grimrock.Monsters.MON13.3.LifecyclePersistence
Grimrock.Monsters.MON13.3.AtomicCommands
Grimrock.Monsters.MON13.4.EncounterWaves
Grimrock.Monsters.MON13.4.AtomicWaveFailure
Grimrock.Monsters.MON13.4.Validation
```

Cette suite teste déjà le pipeline de spawn, l’absence de fuite après échec, les événements de lifecycle, la persistance Spawn/Despawn, la téléportation et l’occupation, les waves d’encounter et leurs rollbacks atomiques.

**Décision : ne pas créer une suite TD05 redondante.** MON13 constitue le contrat de caractérisation pré-extraction de TD05.8.

## 5. Contrat TD05.8

Cible :

```text
Source/GrimrockPrototype/Private/Runtime/GridLevelRuntimeActorMonsters.cpp
```

Règles :

1. aucun changement de `GridLevelRuntimeActor.h` ;
2. aucune nouvelle classe ou composant ;
3. aucun nouvel état ;
4. `SpawnedMonsterActors`, runtime state et encounter component restent possédés par `AGridLevelRuntimeActor` ;
5. `RebuildRuntimeObjects()` reste dans `GridLevelRuntimeActor.cpp` ;
6. les helpers locaux nécessaires dans la nouvelle translation unit sont préfixés `GridLevelRuntimeMonsters...` pour éviter les collisions Unity ;
7. aucune modification SaveGame ;
8. aucune modification `.uasset/.umap`.

## 6. Validation après extraction

Validation minimale recommandée :

```text
Grimrock.Monsters.MON13.2.RuntimePipeline
Grimrock.Monsters.MON13.2.AtomicFailure
Grimrock.Monsters.MON13.3
Grimrock.Monsters.MON13.4
```

Le filtre de groupe `Grimrock.Monsters.MON13` peut être utilisé si le temps d’exécution reste raisonnable ; il est préférable car il couvre aussi le modèle persistant et les contrats de production.

## 7. Stop condition

**Non atteinte après TD05.6.**

Une dernière extraction Monster est justifiée parce qu’elle :

- retire un domaine fonctionnel cohérent et important du fichier principal ;
- réutilise une suite de régression existante et mature ;
- ne change ni ownership, ni API, ni modèle de données ;
- laisse `RebuildRuntimeObjects()` comme orchestration centrale.

Après TD05.8 validé, effectuer TD05.9 : re-mesure et stop-condition finale. Aucun split Geometry/Doors supplémentaire ne devra être engagé sauf nouveau risque concret observé.
