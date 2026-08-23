# MON19.8 — Suite d’énigmes de production, régression et clôture

Statut : **VALIDÉ UE5.5.4**  
Date : **23 août 2026**  
Référence de départ : `81d6f5dc6a0730864f68ef86382a34f0a1a50f39` (`Valider MON19.7.1 authoring Lua et LogicId`)

## 1. Objectif

MON19.8 clôt la construction technique de MON19 en vérifiant que les briques Event/Command, variables persistantes, nœuds Logic et Lua permettent d’assembler plusieurs familles d’énigmes sans C++ spécifique au puzzle.

Contrat final :

```text
événement existant
    -> FGridObjectLink
        -> commande directe / nœud Logic / callback Lua
            -> commande runtime normale
```

## 2. Puzzles de clôture

```text
A. Lever.Activated -> Door.Open
B. Lever A/B -> AddInt(RuneCount,+1) -> CompareInt >= 2 -> Door.Open
C. persistent.RuneCount -> condition Lua -> grid.command("SecretDoor","Open")
D. EncounterCompleted -> Lua -> Door.Open
```

Le cas simple reste sans Lua ; les variables et nœuds Logic couvrent les puzzles à état ; Lua est réservé à l’orchestration plus complexe.

## 3. Implémentation

Tests :

```text
Source/GrimrockPrototype/Private/Tests/GridMON198ProductionPuzzleTests.cpp
```

Suite :

```text
Grimrock.MON19.8.ProductionPuzzles.A_DirectLeverDoor
Grimrock.MON19.8.ProductionPuzzles.B_LogicCounterThreshold
Grimrock.MON19.8.ProductionPuzzles.C_LuaConditional
Grimrock.MON19.8.ProductionPuzzles.D_EncounterLuaDoor
```

Aucun changement de production n’a été nécessaire dans `UGridActivationComponent`, `GridLogicRuntime`, le VM Lua, le système de portes, MON13, SaveGame ou le Grid Editor.

## 4. Correctif Unity Build

La première compilation a révélé une collision de helpers de tests (`MakeIntVariable`, `MakeLogicNode`) avec MON19.2 dans le Unity Build Unreal.

Correctif : isolation des tests MON19.8 dans :

```cpp
namespace GridMON198Tests
{
    ...
}
```

Commit :

```text
74986ef8540f47edd15a408fc153aafe6b2472f3
Corriger le conflit Unity des tests MON19.8
```

## 5. Validation UE5.5.4

### Compilation

```text
Development Editor / Win64    OK
```

### MON19.8 ciblé

```text
A_DirectLeverDoor          Success
B_LogicCounterThreshold    Success
C_LuaConditional           Success
D_EncounterLuaDoor         Success

4/4 Success
```

### Régression complète MON19

```text
Grimrock.MON19
55/55 Success
0 Fail
0 Error
```

Les warnings des tests de protection (cycle Logic, variable absente, budget partagé, source Lua invalide) sont intentionnels et leurs tests terminent en `Success`.

## 6. PIE représentatif final

Le niveau réel de travail a été validé avec la Secret Door portant :

```text
LogicId = SecretDoor
```

Script :

```lua
persistent = {
    RuneCount = 0
}

function on_secret_button(event)
    persistent.RuneCount = persistent.RuneCount + 1

    if persistent.RuneCount >= 2 then
        local ok, err = grid.command("SecretDoor", "Open")
        assert(ok, err)
    end
end
```

Comportement validé par l’utilisateur :

```text
1er clic -> RuneCount = 1 -> porte fermée
2e clic -> RuneCount = 2 -> SecretDoor ouverte
```

La variable `RuneCount` est déclarée/synchronisée via `persistent` dans le Data Asset et la porte est adressée par `LogicId`, sans GUID dans le script.

## 7. Conclusion

Tous les critères de MON19.8 sont satisfaits :

```text
Compilation         OK
MON19.8             4/4 Success
MON19 global       55/55 Success
PIE                  VALIDÉ
```

MON19.8 est **VALIDÉ** et permet la clôture de MON19 dans `MON19_CLOSURE.md`.
