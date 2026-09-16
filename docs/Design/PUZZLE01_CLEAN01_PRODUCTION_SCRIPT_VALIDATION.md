# PUZZLE01-CLEAN01 — Guardian Production Script Validation

Date : 16 septembre 2026  
Statut : **historique — validation d'intégration supersédée par PUZZLE01-LUA02 / CLEAN02**

## Décision conservée

Le Gardien reste un `AGridReceptacleActor` générique. Aucune classe C++ spécifique au puzzle n'est ajoutée.

La logique particulière reste dans le script Lua du niveau de production :

- deux gemmes bleues ordinaires sont utilisées ;
- chaque insertion consomme réellement une unité ;
- `EyesLeft`, puis `EyesRight`, reçoivent l'alias matériau `BlueGem` ;
- après la deuxième gemme, l'insertion est désactivée ;
- la progression est portée par `GuardianGemCount` dans Lua.

CLEAN01 a établi ces invariants sans dupliquer le script Lua dans le C++.

## Note historique sur l'ouverture de la porte

Au moment de CLEAN01, le test disponible n'avait pas encore établi de manière fiable l'ouverture de `GuardianDoor`. CLEAN01 avait donc correctement refusé d'inventer une commande de porte dans le test.

Cette incertitude est désormais levée par PUZZLE01-LUA02 :

```text
Grimrock.PUZZLE01.LUA02.ProductionAuthoringAudit
Grimrock.PUZZLE01.LUA02.RuntimeGuardianDoorCompletion
```

La validation locale UE5.5.4 de LUA02 a confirmé que le vrai `puzzle1_lvl1` référence `GuardianDoor` et que la seconde gemme déclenche réellement l'ouverture de la porte jusqu'à son endpoint ouvert.

Le passage de CLEAN01 indiquant que la porte n'était pas encore un contrat établi doit donc être lu comme un état historique du ticket, pas comme l'état courant du projet.

## ReceptacleDisableRemoval

`ReceptacleDisableRemoval` reste une primitive générique valide du moteur. Elle est redondante pour ce Gardien une fois que `ReceptacleConsumeItem` a supprimé immédiatement la gemme : aucun contenu ne reste à reprendre.

CLEAN01 ne retire donc pas cette primitive générique du moteur.

## EGridReceptacleRejectReason

`ExplicitlyRejected` n'avait plus de producteur dans le runtime courant. Sa valeur numérique a été conservée pour éviter de renuméroter l'`UENUM`, mais elle a été masquée de l'authoring Blueprint.

Les raisons actives restent :

```text
InvalidItem
Full
NoMatchingAcceptanceRule
InsertionDisabled
```

## Test historique supprimé par CLEAN02

CLEAN01 utilisait encore :

```text
Grimrock.PUZZLE01.LUA01.GuardianPuzzleIntegration
```

PUZZLE01-CLEAN02 supprime ce test devenu redondant après LUA02. Toute la couverture utile du puzzle de production est consolidée dans `RuntimeGuardianDoorCompletion`, sans `Source.Contains(...)` et sans seconde copie du fixture d'intégration.

## Invariants finaux

Le projet n'introduit :

- aucune classe Guardian spécifique ;
- aucun compteur C++ spécifique au puzzle ;
- aucun état `LeftEye` / `RightEye` en C++ ;
- aucune duplication du script Lua de production dans les tests ;
- aucune mutation binaire de `DA_GridLevel_00` par les tests.

Le C++ reste limité aux primitives génériques de réceptacle, commande, présentation, porte et persistance.
