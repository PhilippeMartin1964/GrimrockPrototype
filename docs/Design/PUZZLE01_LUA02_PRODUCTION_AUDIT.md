# PUZZLE01-LUA02 — Audit et validation runtime du Gardien

Date : 16 septembre 2026

## Statut

**VALIDÉ UE5.5.4**

Commande locale validée :

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.PUZZLE01.LUA02"
```

Résultat obtenu :

```text
Succeeded              : 2
Succeeded with warnings: 0
Failed                 : 0
Process exit code      : 0
```

## Architecture validée

```text
SCRIPT LUA  = intention gameplay / logique du puzzle
COMPILATEUR = validation statique de l'authoring
RUNTIME     = exécution + gardes internes
```

Aucun `must(...)`, `assert(...)`, wrapper `(ok, err)` ou code de validation défensive n'est ajouté au script de production.

Le ticket ne modifie aucune ligne de `puzzle1_lvl1`.

## Test authoring

Filtre :

```text
Grimrock.PUZZLE01.LUA02.ProductionAuthoringAudit
```

Le test charge le vrai `DA_GridLevel_00`, le vrai `puzzle1_lvl1`, son binding `ItemInserted -> LuaCallback` et la vraie cible `GuardianDoor`.

Il vérifie que :

- `puzzle1_lvl1` existe et est activé ;
- son binding de production existe ;
- `GuardianDoor` existe une seule fois ;
- `GuardianDoor` est une porte ;
- le compilateur accepte le script contre les données de production ;
- retirer uniquement le `LogicId` `GuardianDoor` dans une copie transitoire provoque `E201`.

Cette dernière vérification prouve que le script de production référence réellement `GuardianDoor`, sans recherche fragile de chaîne dans le source Lua.

## Test runtime canonique

Filtre :

```text
Grimrock.PUZZLE01.LUA02.RuntimeGuardianDoorCompletion
```

Le test charge directement :

```text
DA_GridLevel_00
puzzle1_lvl1
binding ItemInserted -> LuaCallback
LogicId du Gardien
GuardianDoor LogicId / InstanceId / Type
```

Le niveau runtime utilisé par le test est transitoire et minimal pour isoler le gameplay de la présentation complète de la map. Le source Lua reste celui de production, inchangé.

Le test vérifie le contrat complet du puzzle :

1. un item incompatible est refusé sans perte du curseur ;
2. la première gemme bleue est consommée ;
3. `GuardianGemCount` devient `1` ;
4. `EyesLeft` reçoit l'alias `BlueGem` ;
5. `EyesRight` reste inchangé ;
6. la porte reste complètement fermée après la première gemme ;
7. la seconde gemme est consommée ;
8. `GuardianGemCount` devient `2` ;
9. `EyesRight` reçoit à son tour `BlueGem` ;
10. l'insertion est désactivée ;
11. la seconde gemme déclenche réellement l'ouverture physique de `GuardianDoor` ;
12. `bIsOpen` reste faux pendant le mouvement, car il représente l'état d'endpoint ;
13. le passage reste bloqué pendant l'ouverture ;
14. une troisième gemme est refusée sans être consommée et sans modifier les yeux ni le compteur ;
15. les deux overrides visuels sont persistés dans `ObjectVisuals` ;
16. à la fin du mouvement, `bIsOpen`, `IsFullyOpen()` et le passage deviennent ouverts.

Le chemin runtime validé est :

```text
Guardian.ItemInserted
    -> puzzle1_lvl1
    -> grid.command("GuardianDoor", "Open")
    -> UGridActivationComponent::ExecuteLuaIssuedCommand
    -> ApplyLinkCommand
    -> AGridLevelRuntimeActor::OpenDoorOnEdge
    -> UGridDoorSystemComponent::OpenDoorOnEdge
    -> AGridDoorActor::OpenDoor
    -> animation
    -> endpoint ouvert
```

## Nettoyage CLEAN02

Depuis PUZZLE01-CLEAN02, ce test runtime est l'unique test d'intégration de production du Gardien.

L'ancien :

```text
Grimrock.PUZZLE01.LUA01.GuardianPuzzleIntegration
```

est supprimé car il doublonnait le même fixture et le même chemin runtime, tout en conservant des assertions textuelles `Source.Contains(...)` devenues contraires au principe de validation par le compilateur.

Les tests LUA01 qui restent couvrent uniquement les primitives génériques : API visuelle Lua, erreurs host contrôlées et persistance des aliases de matériaux.

## Invariants

PUZZLE01-LUA02 n'introduit :

- aucune classe C++ spécifique au Gardien ;
- aucun compteur C++ spécifique au puzzle ;
- aucun état C++ `LeftEye` / `RightEye` ;
- aucune duplication du script Lua dans un test ;
- aucune modification binaire du `LevelAsset`.

Le puzzle demeure data-driven et Lua-first.
