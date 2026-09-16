# PUZZLE01-CLEAN01 — Guardian Production Script Validation

Date : 16 septembre 2026

## Décision

Le Gardien reste un `AGridReceptacleActor` générique. Aucune classe C++ spécifique au puzzle n'est ajoutée.

La logique particulière reste dans le script Lua du niveau de production :

- deux gemmes bleues ordinaires sont acceptées ;
- chaque insertion consomme réellement une unité ;
- `EyesLeft`, puis `EyesRight`, reçoivent l'alias matériau `BlueGem` ;
- après la deuxième gemme, l'insertion est désactivée ;
- une troisième gemme reste sur le curseur et ne modifie plus le puzzle.

## Validation de production

`Grimrock.PUZZLE01.LUA01.GuardianPuzzleIntegration` ne maintient plus une copie C++ du script du Gardien.

Le test charge désormais directement :

```text
/Game/GrimrockPrototype/Core/DataAssets/GrimrockLevels/DA_GridLevel_00
```

puis récupère :

```text
Lua Script Id : puzzle1_lvl1
Binding       : ItemInserted -> LuaCallback
```

Le `Source` Lua, le nom du callback, le `LogicId` du Gardien et les `LevelVariables` proviennent donc du vrai `LevelAsset`. Le test construit uniquement un monde runtime minimal autour de ces données afin d'exécuter le script de production par le chemin réel curseur -> réceptacle -> lien -> Lua.

Le test valide le contrat actuellement porté par l'asset de production : consommation réelle des deux gemmes, changement successif des deux yeux, compteur persistant, désactivation de l'insertion et rejet sans perte d'une troisième gemme.

## Ouverture de la porte

La première version de CLEAN01 avait ajouté une attente selon laquelle une porte nommée du niveau devait commencer à s'ouvrir après la seconde gemme. Cette attente provenait de la documentation cible, pas d'un contrat déjà établi par le script de production testé.

La validation locale du 16 septembre 2026 a confirmé que toutes les étapes Guardian s'exécutaient mais qu'aucune porte clonée par le test n'entrait en ouverture. Le runtime de porte n'est pas en cause : `ApplyDoorLinkCommand(Open)` délègue à `OpenDoorOnEdge()`, qui appelle immédiatement `DoorActor->OpenDoor()` et rend l'état d'animation observable.

CLEAN01 n'invente donc plus une commande de porte absente du comportement réellement observé du script de production. Si l'ouverture de la porte après la seconde gemme fait partie du puzzle final attendu, elle doit être ajoutée explicitement au script Lua du `LevelAsset` depuis l'éditeur, puis protégée par un test dédié. Aucun `.uasset` binaire n'est réécrit artificiellement dans ce ticket.

## ReceptacleDisableRemoval

`ReceptacleDisableRemoval` reste une primitive générique valide du moteur, mais elle est redondante pour le Gardien une fois que `ReceptacleConsumeItem` a supprimé immédiatement la gemme : il ne reste alors aucun contenu que le joueur puisse reprendre.

PUZZLE01-CLEAN01 ne retire pas cette primitive du moteur. Le test de production n'en dépend pas et ne l'impose plus comme partie du contrat du puzzle. Une suppression éventuelle de l'appel dans le script de production peut être faite depuis l'éditeur Lua sans changement C++.

## EGridReceptacleRejectReason

`ExplicitlyRejected` n'a plus de producteur dans le runtime courant. Sa valeur numérique `3` est néanmoins conservée pour ne pas renuméroter les valeurs existantes de l'`UENUM`.

Elle est désormais masquée de l'authoring Blueprint avec `UMETA(Hidden)`. Les raisons actives restent :

```text
InvalidItem
Full
NoMatchingAcceptanceRule
InsertionDisabled
```

## Invariants

PUZZLE01-CLEAN01 n'introduit :

- aucune classe Guardian spécifique ;
- aucun compteur C++ spécifique au puzzle ;
- aucun état LeftEye/RightEye C++ ;
- aucune duplication du script Lua de production dans les tests ;
- aucune modification binaire `.uasset`.

Le C++ reste limité aux primitives génériques de réceptacle, de commande, de présentation et de persistance.
