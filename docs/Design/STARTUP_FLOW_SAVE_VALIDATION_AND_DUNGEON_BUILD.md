# Startup flow, save validation and dungeon build

## Objectif

Le flux de démarrage du jeu doit garantir que le donjon n'est jamais considéré comme une partie jouable tant que le joueur n'a pas :

- soit validé la création initiale du personnage ;
- soit chargé une sauvegarde existante réellement chargeable.

Ce document consolide les règles mises en place après les corrections CC7.1 autour de `Nouvelle partie`, `Continuer`, `Charger`, `Annuler` et la construction du donjon.

## Principes validés

### 1. Nouvelle partie

`Nouvelle partie` prépare une nouvelle session, mais ne doit pas créer une partie jouable complète avant validation du personnage.

Flux attendu :

```text
Menu principal
-> Nouvelle partie
-> ouverture du niveau runtime existant dans le flux actuel
-> désactivation/vidage du runtime visible
-> affichage du wizard de création de personnage
```

Tant que le personnage n'est pas validé :

- le runtime du donjon ne doit pas être jouable ;
- les entrées de mouvement/interaction doivent rester bloquées par l'état modal de création ;
- aucune sauvegarde jouable ne doit être considérée comme chargeable.

### 2. Annuler la création du personnage

`Annuler` dans le wizard ne valide pas la partie.

Flux attendu :

```text
Wizard de création
-> Annuler
-> retour au menu principal
-> aucune partie jouable créée
-> aucun slot non finalisé ne doit activer Continuer
```

Règle fonctionnelle : après annulation, le bouton `Continuer` doit être désactivé si aucune sauvegarde principale réellement chargeable n'existe.

### 3. Validation du personnage

La validation du personnage est le premier moment où une nouvelle partie peut devenir jouable.

Flux attendu :

```text
Wizard de création
-> Valider personnage
-> PartyInventoryState.bInitialCharacterCreationCompleted = true
-> affichage de la progression de construction du donjon
-> reconstruction du runtime du donjon
-> placement du groupe
-> sauvegarde possible
-> runtime jouable
```

La construction du donjon doit être visible via un widget de progression.

### 4. Continuer

`Continuer` ne doit cibler que la sauvegarde principale par défaut.

Condition d'activation :

```text
HasDefaultPartySaveGame() == true
```

Mais `HasDefaultPartySaveGame()` doit signifier :

```text
la sauvegarde principale existe ET elle est réellement chargeable
```

Elle ne doit pas signifier simplement :

```text
un fichier de sauvegarde existe sur disque
```

### 5. Charger

`Charger` ne doit pas nécessairement dépendre de la sauvegarde principale.

Condition d'activation :

```text
au moins un slot configuré est réellement chargeable
```

Le menu de chargement ne doit afficher que les slots chargeables.

### 6. Sauvegarde chargeable

Une sauvegarde est chargeable uniquement si :

- le fichier existe ;
- il peut être chargé comme `UGrimrockPartySaveGame` ;
- sa version est compatible ;
- `PartyInventoryState.bInitialCharacterCreationCompleted == true` ;
- elle contient au moins un personnage actif ;
- `MaxActiveCharacters` est cohérent avec le nombre de personnages actifs ;
- le nombre d'équipements actifs correspond au nombre de personnages actifs ;
- l'index de personnage sélectionné est valide ;
- chaque personnage actif possède un `CharacterId` valide.

Cette règle évite qu'une annulation de création de personnage, un ancien fichier incomplet ou une sauvegarde corrompue active `Continuer` ou apparaisse comme slot chargeable.

## Rôles des classes C++

### `UGrimrockGameInstance`

Responsabilités :

- stocker le mode de démarrage demandé ;
- stocker le slot de sauvegarde demandé ;
- centraliser la validation des sauvegardes chargeables ;
- refuser toute demande de chargement vers un slot non chargeable ;
- gérer le retour au menu principal.

Méthodes importantes :

```text
HasDefaultPartySaveGame()
HasPartySaveGame(SlotName, UserIndex)
DoesDefaultPartySaveGameExist()
DoesPartySaveGameExist(SlotName, UserIndex)
GetExistingPartySaveSlotInfos()
RequestContinueDefaultPartySaveSlot()
RequestLoadPartySaveSlot(SlotName, UserIndex)
RequestReturnToMainMenu(WorldContextObject)
```

Convention :

- `Does...Exist` = le fichier existe physiquement ;
- `Has...SaveGame` = la sauvegarde existe et est chargeable.

### `UGrimrockMainMenuWidget`

Responsabilités :

- recalculer l'état du menu à l'affichage ;
- activer `Continuer` uniquement pour la sauvegarde principale chargeable ;
- activer `Charger` uniquement s'il existe au moins un slot chargeable ;
- ne pas lancer directement de logique runtime.

Règles UI :

```text
Button_Continue -> bHasValidSaveGame
Button_LoadGame -> bHasLoadableSaveSlot
```

### `UGrimrockLoadGameMenuWidget`

Responsabilités :

- demander au `GameInstance` la liste des slots chargeables ;
- afficher uniquement ces slots ;
- refuser/recharger la liste si une demande de chargement échoue.

### `UGrimrockStartupModeComponent`

Responsabilités :

- consommer le mode de démarrage demandé ;
- différer l'activation du runtime en cas de `NewGame` tant que le personnage n'est pas validé ;
- afficher la progression de construction/chargement ;
- reconstruire le runtime après validation du personnage ;
- terminer la progression après chargement d'une sauvegarde.

### `UGridDungeonBuildProgressWidget`

Responsabilités :

- fournir un widget natif de progression visible même sans Blueprint dédié ;
- exposer les champs optionnels pour un futur `WBP_DungeonBuildProgress` :
  - `ProgressBar_BuildProgress` ;
  - `Text_Title` ;
  - `Text_Status` ;
  - `Text_Percent`.

## Tests de non-régression

### Test A - Annulation de création

1. Lancer le jeu.
2. Cliquer `Nouvelle partie`.
3. Annuler le wizard.
4. Vérifier le retour au menu principal.
5. Vérifier que `Continuer` est désactivé si aucune sauvegarde principale chargeable n'existe.
6. Vérifier que `Charger` est désactivé si aucun slot chargeable n'existe.

### Test B - Création validée

1. Cliquer `Nouvelle partie`.
2. Valider un personnage complet.
3. Vérifier l'affichage de la progression de construction du donjon.
4. Vérifier que le donjon devient jouable après validation.
5. Revenir au menu.
6. Vérifier que `Continuer` est activé.

### Test C - Chargement

1. Avoir au moins une sauvegarde chargeable.
2. Revenir au menu.
3. Vérifier que `Charger` est activé.
4. Ouvrir le menu de chargement.
5. Vérifier que seuls les slots chargeables sont listés.
6. Charger un slot.
7. Vérifier l'affichage de la progression de chargement.

### Test D - Ancienne sauvegarde invalide

1. Placer ou conserver un fichier de sauvegarde incomplet/non finalisé.
2. Lancer le menu principal.
3. Vérifier qu'il n'active pas `Continuer`.
4. Vérifier qu'il n'apparaît pas dans le menu `Charger`.

## Points restant à améliorer

La barre actuelle est une progression par phases synchrones. Pour une progression réellement proportionnelle au coût de construction, il faudra découper `RebuildLevel()` en lots de cellules/objets construits sur plusieurs ticks.

Évolution recommandée :

```text
StartDungeonBuild()
-> BuildNextBatch()
-> BroadcastProgress(CurrentStep / TotalSteps)
-> CompleteDungeonBuild()
```

Cela permettra une vraie barre de progression basée sur la taille du niveau et le nombre d'objets runtime générés.
