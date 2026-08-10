# MON9 — persistance des monstres

## Périmètre

MON9 ajoute les monstres au `FGridDungeonRuntimeState` déjà sauvegardé par `UGrimrockPartySaveGame`. La cellule logique, l’orientation, les points de vie, les armures restantes, l’état, l’activation, le groupe de rencontre, la dernière cellule connue et la mort survivent aux transitions et au rechargement disque.

MON9 ne reprend jamais une action ou un combat en cours. Les interpolations, réservations, Montages, timers, chemins et jets aléatoires transitoires ne sont pas sérialisés.

## Architecture de l’état

`FGridRuntimeMonsterState` est déclaré dans `GridDungeonRuntimeState.h` et contient :

- `PersistenceId`, clé durable du monstre ;
- `SpawnObjectId`, renseigné uniquement pour un véritable `MonsterSpawn` ;
- `MonsterDefinitionId` ;
- `DungeonLevelId` ;
- `CellX` et `CellY` ;
- `Facing` ;
- `MonsterState` ;
- `CurrentHealth` ;
- `CurrentPhysicalArmor` et `CurrentMagicalArmor` ;
- `bMonsterEnabled` ;
- `EncounterGroupId` ;
- `bHasLastKnownPartyCell` et `LastKnownPartyCell` ;
- `bIsDead`.

Chaque `FGridLevelRuntimeState` possède une map :

```cpp
TMap<FGuid, FGridRuntimeMonsterState> Monsters;
```

La clé de la map est toujours `PersistenceId`. L’ordre de capture est déterministe par GUID.

## Identité persistante

`AGridMonsterActor::ResolvePersistenceId()` applique cet ordre :

1. `SpawnObjectId` s’il provient réellement d’un placement `MonsterSpawn` ;
2. sinon `PersistentMonsterId` pour un Actor placé directement ;
3. sinon aucune identité.

Un monstre sans identité stable produit un log d’erreur et n’est pas capturé. Aucun GUID aléatoire runtime et aucune association par nom ou cellule ne sont utilisés comme clé durable.

En éditeur, `OnConstruction` appelle `EnsurePersistentMonsterId()` uniquement pour une instance sérialisable, hors CDO, template, Actor transitoire et monde de jeu. La fonction `CallInEditor` reste disponible pour régénérer explicitement un identifiant manquant.

`ValidatePersistenceSetup()` détecte :

- un identifiant invalide ;
- un doublon dans le même niveau logique ;
- l’absence de `HomeDungeonLevelId` pour un Actor directement placé dans un donjon multi-niveaux.

Dans un niveau unique, `HomeDungeonLevelId=None` est résolu vers l’identifiant runtime courant lors de la première capture. Dans un donjon multi-niveaux, chaque monstre directement placé doit déclarer son niveau d’origine.

## Initialisation des monstres directement placés

`AGridMonsterActor::CurrentCell` commence à `(0,0)`. Pour un Actor directement placé dans une carte et qui ne possède encore aucun état MON9 sauvegardé, cette valeur par défaut n’est pas une position logique valide : la position monde initiale de l’Actor est la source de vérité.

Lors de l’activation initiale, `UGridMonsterMovementComponent::InitializeMovement()` :

1. déduit la cellule depuis la position monde lorsque `bInferCellFromActorLocation=true` ;
2. valide que la cellule existe et est praticable ;
3. enregistre le monstre dans le sous-système d’occupation ;
4. snappe ensuite l’Actor au centre exact de la cellule lorsque `bSnapToCellOnInitialize=true`.

`AGridLevelRuntimeActor` ne doit jamais repositionner un monstre vivant depuis une `CurrentCell` non initialisée avant cet appel. Un monstre vivant sans composant de mouvement suit le même principe : sa cellule est explicitement résolue depuis sa position monde, validée, puis enregistrée.

Après une sauvegarde, la cellule stockée dans `FGridRuntimeMonsterState` devient la source de vérité. La restauration place d’abord l’Actor au centre de cette cellule ; l’initialisation du mouvement enregistre donc la cellule sauvegardée sans reprendre une ancienne position monde. Un monstre mort conserve de la même manière sa `DeathCell` ou sa `CurrentCell` sauvegardée, sans nouvelle inférence ni occupation.

## Ordre BeginPlay et initialisation des statistiques

L’ordre de `BeginPlay` entre `AGridLevelRuntimeActor` et `AGridMonsterActor` n’est pas une source fiable. Le RuntimeActor peut appliquer l’état initial du niveau avant que le MonsterActor ait exécuté son propre `BeginPlay`.

`CurrentHealth=0` avec `bCombatStatsInitialized=false` signifie « statistiques non initialisées », pas « monstre mort ». La mort logique exige au moins une des conditions suivantes :

- `MonsterState=Dead` ;
- `bDeathCommitted=true` sur le composant de mort ;
- des statistiques déjà initialisées avec `CurrentHealth<=0`.

`EnsureInitialCombatState()` prépare de manière idempotente les PV et armures d’un monstre neuf. Cette fonction peut être appelée par l’activation du niveau avant `AGridMonsterActor::BeginPlay`, puis rappelée sans écraser un monstre blessé ou un état MON9 restauré. Les états sauvegardés vivants ou morts restent toujours prioritaires.

Pour un niveau jamais visité et pour le chemin legacy v1 sans état de monstre, les statistiques initiales sont garanties avant le premier test de mort. Un Rat directement placé démarre donc vivant à `MaxHealth`, puis sa cellule est déduite depuis sa position monde et enregistrée dans l’occupation.

L’auto-initialisation tardive de `UGridMonsterMovementComponent::BeginPlay()` vérifie que le propriétaire appartient au niveau actif, qu’il est activé et qu’il est vivant. Elle ne réinscrit jamais un monstre mort ou désactivé dans l’occupation.

## Capture

`CaptureCurrentLevelRuntimeState()` vide d’abord `State->Monsters`, collecte uniquement les monstres du niveau courant, vérifie les identifiants et capture les Actors valides.

La cellule sauvegardée est toujours `CurrentCell`. Les valeurs suivantes ne sont jamais sauvegardées :

- déplacement ou rotation en cours ;
- alpha d’interpolation ;
- chemin calculé ;
- réservation ;
- action active ou file d’actions ;
- attaque, timer ou Montage actif ;
- présentation de mort active.

La mort est normalisée si `IsDead()`, `CurrentHealth <= 0` ou `bDeathCommitted`. Un mort est capturé avec `CurrentHealth=0` et `MonsterState=Dead`. Un vivant conserve entre 1 et `MaxHealth`.

## Restauration d’un monstre vivant

Avant application, MON9 annule l’attaque, le déplacement et les réservations, retire l’ancienne occupation et remet les signaux d’animation à zéro.

La définition doit correspondre à `MonsterDefinitionId`. Le monstre est placé exactement avec `GetCellCenterWorld()` puis `ApplyFacingRotation()`. Les PV, armures, activation, groupe de rencontre et dernière cellule connue sont restaurés.

Les états transitoires sont normalisés :

- `Attacking` vers `Pursuing` si une cellule du groupe est connue, sinon `Alert` ;
- `Repositioning` vers `Pursuing` ou `Alert` ;
- `Hurt` vers `Alert` ou `Idle`.

Aucune attaque, animation ou réservation n’est reprise. La collision redevient `QueryOnly` et l’occupation est enregistrée uniquement si le monstre est vivant et activé. Un conflit sur la cellule sauvegardée est journalisé ; aucun déplacement de secours n’est tenté.

## Restauration d’un monstre mort

`UGridMonsterDeathComponent::RestoreCommittedDeathState()` restaure une mort déjà validée sans effet secondaire :

- `bDeathCommitted=true` ;
- `bLootGenerated=true` ;
- `DeathCell` restaurée ;
- attaque, mouvement, réservation et occupation libérés ;
- collision désactivée ;
- corps visible ;
- `MonsterState=Dead` ;
- `CurrentHealth=0` ;
- signaux transitoires remis à zéro.

Cette voie ne lance pas de nouvelle animation de mort, ne génère aucun butin, n’exécute aucun lien, ne diffuse pas `OnMonsterDied` et n’incrémente aucun compteur logique. Les items de butin existants sont restaurés séparément par le système d’items.

`RestoreLivingState()` remet les gardes et compteurs transitoires du composant de mort dans un état vivant propre.

## Persistance de plusieurs objets issus d’une mort

Une même mort peut produire plusieurs objets grâce aux jets indépendants MON8. Chaque objet placé possède son propre `RuntimeObjectId` et reste un `AGridItemActor` indépendant.

Lors de la capture du niveau, ces objets sont enregistrés séparément dans `FGridLevelRuntimeState::Items`, chacun sous la forme d’un `FGridRuntimeItemState`. Ils ne sont pas ajoutés à `FGridRuntimeMonsterState`, car les items au sol possèdent déjà leur propre pipeline de persistance.

Lors de la restauration :

- le monstre mort conserve `bDeathCommitted=true` ;
- il conserve `bLootGenerated=true` ;
- sa `LootTable` n’est jamais réévaluée ;
- aucun nouveau `RuntimeObjectId` n’est créé ;
- aucun second exemplaire n’est placé ;
- `MonsterDied` et les liens ne sont pas réémis ;
- chaque item au sol est restauré indépendamment par son état runtime.

Le nombre d’items au sol provenant de la mort doit être strictement identique avant la sauvegarde et après le chargement.

## Occupation

Le sous-système d’occupation utilise désormais `ResolvePersistenceId()` et ne fabrique plus d’identité à partir du chemin de l’Actor.

Avant une application de niveau :

1. toutes les actions ennemies sont annulées ;
2. les monstres sont désenregistrés ;
3. le registre d’occupation est vidé ;
4. les états sont restaurés par GUID ;
5. chaque vivant activé reprend exactement sa cellule ;
6. aucune réservation n’est reconstruite.

Un corps mort reste visible mais ne bloque pas la grille.

## Transitions entre niveaux

`TravelToDungeonLevel()` :

1. interrompt le combat et les actions ;
2. capture le niveau courant ;
3. désactive et masque ses monstres sans les détruire ;
4. change `CurrentDungeonLevelId` et `LevelAsset` ;
5. reconstruit les objets ;
6. applique l’état cible ou l’état initial si le niveau n’a pas d’état MON9 ;
7. active uniquement les monstres du niveau cible ;
8. place le groupe.

Un monstre d’un autre niveau est masqué, sans collision, sans occupation, sans réservation et exclu des rencontres. Son état logique sauvegardé n’est pas remplacé par `MaxHealth`.

## Sauvegarde disque et compatibilité

`UGrimrockPartySaveGame` utilise :

```text
CurrentSaveVersion = 2
MinimumCompatibleSaveVersion = 1
```

Les versions 1 et 2 sont acceptées. Une version future ou inférieure à 1 est refusée.

Une sauvegarde v1 ne contient pas `Monsters` : les Actors conservent alors leurs valeurs initiales. La prochaine sauvegarde crée une v2 et écrit la map des monstres. Aucun second objet `SaveGame` n’est créé.

La sauvegarde disque contient désormais l’inventaire du groupe, les items, les portes, mécanismes, réceptacles, niveaux du donjon et monstres.

## Combat après chargement

Avant restauration, `AbortCombat()` remet le TurnManager dans l’état suivant :

```text
CurrentPhase = Exploration
bCombatActive = false
CurrentMonster = null
PendingActions = vide
ActiveAction = None
réservations = vides
entrée du groupe = déverrouillée
```

Les états agressifs peuvent redevenir `Alert` ou `Pursuing`, mais seule la perception normale peut démarrer un nouveau combat. `CombatRandomStream`, les timers et les effets temporaires ne sont ni sauvegardés ni restaurés.

## Logs et diagnostics

La catégorie `LogGridMonsterState` produit notamment :

```text
[GridMonsterState] Capture ...
[GridMonsterState] RestoreAlive ...
[GridMonsterState] RestoreDead ...
[GridMonsterState] MissingActor ...
[GridMonsterState] DuplicatePersistenceId ...
[GridMonsterState] DefinitionMismatch ...
[GridMonsterState] ActivateLevel ...
[GridMonsterState] DeactivateLevel ...
```

`GetLevelAssetDiagnostics()` indique le nombre de monstres associés, sauvegardés et morts, ainsi que les identifiants invalides et les doublons. Aucun de ces logs n’est émis à chaque Tick.

## Tests automatisés

MON9 ajoute :

```text
Grimrock.Monsters.MON9.StateRoundTrip
Grimrock.Monsters.MON9.DeadRoundTrip
Grimrock.Monsters.MON9.OccupancyRestore
Grimrock.Monsters.MON9.LevelTransition
Grimrock.Monsters.MON9.SaveVersionCompatibility
Grimrock.Monsters.MON9.DiskSaveRoundTrip
Grimrock.Monsters.MON9.StableIdentity
Grimrock.Monsters.MON9.DirectPlacedInitialCellInference
Grimrock.Monsters.MON9.RestoredCellNotReinferredFromStaleLocation
Grimrock.Monsters.MON9.DirectPlacedNewGameStartsAlive
Grimrock.Monsters.MON9.BeginPlayOrderIndependence
Grimrock.Monsters.MON9.RestoredDeadRemainsDead
Grimrock.Monsters.MON9.MovementAutoInitSkipsDead
```

La régression complète reste `Grimrock.Monsters.MON`, complétée par les tests de sauvegarde `Grimrock.CharacterCreation.CC5`.

## Réglages manuels UE5

Ne modifier aucun asset automatiquement. Pour chaque `BP_MON_RatGiant` directement placé :

```text
Monster | Identity
Persistent Monster Id = GUID unique

Monster | Persistence
Home Dungeon Level Id = Into_The_Dark
```

Le nom doit correspondre exactement à `CurrentDungeonLevelId`.

Pour un futur placement `MonsterSpawn`, `SpawnObjectId` utilisera l’`ObjectId` du placement ; aucun identifiant direct supplémentaire ne sera nécessaire.

## Limitation du pipeline MonsterSpawn

MON13.1 fournit désormais le modèle persistant validé du placement
`MonsterSpawn` (`ObjectId/SpawnId`, définition, cellule, orientation, état
initial et rencontre). MON9 associe toutefois toujours l'état sauvegardé à un
Actor existant. Si une sauvegarde référence un GUID sans Actor, un Warning est
produit et le chargement continue. La création native de l'Actor depuis ce
placement commence en MON13.2.

## Checklist PIE MON9

1. Fermer Unreal Editor.
2. Compiler `GrimrockPrototypeEditor` en Development Win64.
3. Ouvrir la carte.
4. Sélectionner chaque Rat directement placé.
5. Vérifier un `PersistentMonsterId` unique.
6. Renseigner `HomeDungeonLevelId=Into_The_Dark`.
7. Vérifier que le nom correspond à `CurrentDungeonLevelId`.
8. Lancer le PIE.
9. Vérifier un log `[GridMonsterState] InitializeFresh` par Rat neuf avec `HP=MaxHealth` et un état différent de `Dead`.
10. Vérifier que les logs `ActivateLevel` indiquent `Dead=false`, `CombatStatsInitialized=true` et `DeathCommitted=false`.
11. Vérifier un log `[GridMonsterMovement] InferCell` par Rat directement placé.
12. Vérifier que chaque log indique sa cellule réelle et jamais `(0,0)` par défaut.
13. Vérifier que chaque Rat est vivant, visible et visuellement snappé au centre de sa cellule de carte.
14. Déclencher une sauvegarde et vérifier que les logs `Capture` utilisent ces mêmes cellules avec `Dead=false` et `HP=MaxHealth`.
15. Blesser un Rat sans le tuer.
16. Déplacer le Rat ou le laisser se déplacer.
17. Noter sa cellule, son orientation et ses PV.
18. Ouvrir puis fermer l’inventaire pour sauvegarder.
19. Arrêter le PIE.
20. Relancer le PIE avec Continuer.
21. Vérifier la cellule, l’orientation et les PV.
22. Vérifier que la cellule sauvegardée prime sur la position initiale de la carte.
23. Tuer le premier Rat.
24. Vérifier ses éventuels objets de butin et leurs `RuntimeObjectId` distincts.
25. Laisser plusieurs objets au sol, sauvegarder puis arrêter et relancer le PIE.
26. Vérifier que le Rat reste mort avec `bDeathCommitted=true` et `bLootGenerated=true`.
27. Vérifier que le nombre d’items au sol est inchangé, qu’aucun nouveau jet ou butin n’est généré et que sa cellule reste libre.
28. Effectuer une transition vers un autre niveau.
29. Revenir au niveau du Rat.
30. Vérifier les états, les corps et le butin des Rats.
31. Démarrer un nouveau combat et confirmer le fonctionnement MON4 à MON8.
