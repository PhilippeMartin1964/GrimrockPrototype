# PUZZLE01-CLEAN02 — Consolidation finale des tests du Gardien

Date : 16 septembre 2026

## Objectif

Après validation de PUZZLE01-LUA02, deux tests d'intégration du Gardien couvraient en grande partie le même chemin runtime :

```text
Grimrock.PUZZLE01.LUA01.GuardianPuzzleIntegration
Grimrock.PUZZLE01.LUA02.RuntimeGuardianDoorCompletion
```

Cette duplication n'avait plus de raison d'exister.

PUZZLE01-CLEAN02 conserve une seule autorité d'intégration de production :

```text
Grimrock.PUZZLE01.LUA02.RuntimeGuardianDoorCompletion
```

## Suppression du test LUA01 dupliqué

Le fichier :

```text
Source/GrimrockPrototype/Private/Tests/GridPUZZLE01Lua01PuzzleIntegrationTests.cpp
```

est supprimé.

Il contenait encore deux assertions textuelles :

```cpp
ProductionGuardianScript->Source.Contains(TEXT("ReceptacleConsumeItem"))
ProductionGuardianScript->Source.Contains(TEXT("ReceptacleDisableInsertion"))
```

Ces contrôles sont désormais inutiles et contraires à l'architecture retenue : la validité du script et de ses références statiques appartient au compilateur, tandis que le comportement effectif appartient au test runtime.

Aucun script Lua n'est modifié pour satisfaire le test.

## Couverture transférée dans LUA02

Le test runtime LUA02 absorbe les invariants utiles qui n'étaient pas encore présents dans sa première version :

- refus d'un item incompatible sans perte du curseur ;
- consommation de la première gemme ;
- activation de `EyesLeft` ;
- consommation de la seconde gemme ;
- activation de `EyesRight` ;
- persistance des deux aliases visuels ;
- désactivation de l'insertion après deux gemmes ;
- refus d'une troisième gemme sans consommation ni mutation ;
- compteur `GuardianGemCount` borné à `2` ;
- ouverture réelle de `GuardianDoor` jusqu'à l'endpoint ouvert.

Le fixture d'intégration n'existe donc plus en double.

## Ce qui reste sous LUA01

Les tests PUZZLE01-LUA01 conservés restent utiles parce qu'ils protègent des primitives moteur génériques indépendantes du puzzle :

```text
LuaVisualApi
LuaVisualApiFailureIsData
RuntimeMaterialAliasPersistence
```

Ils ne doivent pas être supprimés simplement parce que PUZZLE01 est terminé : ces primitives peuvent être réutilisées par d'autres énigmes.

## Ce qui n'est pas modifié

PUZZLE01-CLEAN02 ne change :

- aucun code runtime de production ;
- aucune API Lua ;
- aucun compilateur Lua ;
- aucun DataAsset ;
- aucun Blueprint ;
- aucun `.uasset` ;
- aucun script de niveau.

Le ticket est exclusivement un nettoyage de couverture de tests et de documentation.

## Validation attendue

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.PUZZLE01"
```

Résultat attendu : tous les tests PUZZLE01 restants sont verts et `GuardianPuzzleIntegration` n'apparaît plus dans la liste des tests exécutés.
