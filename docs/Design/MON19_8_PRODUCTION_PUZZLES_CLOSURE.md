# MON19.8 — Suite d’énigmes de production, régression et clôture

Statut : **validation ciblée UE5.5.4 réussie — clôture MON19 en attente**  
Date : **23 août 2026**  
Référence de départ : `81d6f5dc6a0730864f68ef86382a34f0a1a50f39` (`Valider MON19.7.1 authoring Lua et LogicId`)

## 1. Objectif

MON19.8 est l’étape finale de MON19. Il ne crée pas un nouveau système de gameplay : il vérifie que les briques construites de MON19.2 à MON19.7.1 permettent réellement d’assembler plusieurs familles d’énigmes de production sans C++ spécifique au puzzle.

Le contrat reste :

```text
événement existant
    -> FGridObjectLink
        -> commande existante / nœud Logic / callback Lua
            -> commande normale
                -> runtime existant
```

Aucun bus d’événements parallèle, aucun Actor de script générique et aucun Tick permanent ne sont ajoutés.

## 2. Scénarios de clôture issus de MON19.1

### Puzzle A — data-driven directe

```text
Lever.Activated
    -> Door.Open
```

But : démontrer que le chemin historique Event -> Command reste la solution la plus simple lorsqu’aucune logique avancée n’est nécessaire.

### Puzzle B — compteur et seuil sans Lua

```text
LeverA.Activated ----\
                      -> AddInt(RuneCount,+1)
LeverB.Activated ----/           |
                                Activated
                                   |
                                   v
                    CompareInt(RuneCount >= 2)
                         | true
                         v
                      Door.Open
```

Le premier levier donne `RuneCount = 1` et la porte reste fermée. Le second donne `RuneCount = 2` et la porte s’ouvre.

### Puzzle C — Lua conditionnel

```lua
persistent = { RuneCount = 0 }

function on_trigger(event)
    if persistent.RuneCount >= 2 then
        local ok, err = grid.command("SecretDoor", "Open")
        assert(ok, err)
    end
end
```

But : démontrer que Lua reste une couche d’orchestration et qu’il réutilise ensuite la commande normale du runtime.

### Puzzle D — pont rencontre -> Lua -> porte

```text
MonsterSpawn / Encounter
    EncounterCompleted
        -> Lua on_encounter_completed
            -> grid.command("SecretDoor", "Open")
                -> Door.Open
```

But : démontrer qu’un événement MON13 peut traverser le pont Lua sans créer une seconde architecture d’événements.

## 3. Implémentation

Fichier de tests :

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

Aucun `.uasset` / `.umap` n’est modifié par cette étape.

## 4. Correctif Unity Build

La première compilation UE5.5.4 a détecté un conflit de helpers de tests :

```text
MakeIntVariable
MakeLogicNode
```

Ces fonctions existaient déjà dans `GridMON192LogicPrimitiveTests.cpp`. Le Unity Build d’Unreal regroupait les deux `.cpp` dans la même unité de compilation et provoquait les erreurs `C2572`, `C2084` et `C2264`.

Correctif : isolation complète des helpers/tests MON19.8 dans :

```cpp
namespace GridMON198Tests
{
    ...
}
```

Commit du correctif :

```text
74986ef8540f47edd15a408fc153aafe6b2472f3
Corriger le conflit Unity des tests MON19.8
```

## 5. Validation UE5.5.4 ciblée

### 5.1 Compilation

Validation utilisateur du 23 août 2026 :

```text
Development Editor / Win64 : OK
```

### 5.2 Tests MON19.8

Résultats fournis depuis UE5.5.4 :

```text
A_DirectLeverDoor          Success
B_LogicCounterThreshold    Success
C_LuaConditional           Success
D_EncounterLuaDoor         Success
```

Résultat : **4/4 Success**.

Le log confirme les chemins attendus :

- Puzzle A : `Lever.Activated -> Door.Open`, avec vraie commande de porte et `Blocked=false` ;
- Puzzle B : première comparaison `Deactivated`, puis seuil atteint, `Activated -> Door.Open` ;
- Puzzle C : callback Lua exécuté sous le seuil sans ouvrir la porte, puis `grid.command("SecretDoor", "Open")` au seuil ;
- Puzzle D : `EncounterCompleted -> Lua -> Door.Open` avec `AnyApplied=true`.

Aucun échec ni erreur Automation n’est présent dans cette suite ciblée.

## 6. Régression MON19 complète — encore requise

Exécuter :

```text
Grimrock.MON19
```

Le nombre exact de tests dépend de l’ensemble enregistré par UE5.5.4. Critère :

```text
0 Fail
0 Error
```

Les warnings volontairement produits par les tests de budget, sandbox, source Lua invalide ou cycle Logic restent acceptables lorsque leur test se termine en `Success`.

## 7. Validation PIE de clôture — encore requise

Le scénario représentatif recommandé reste le Puzzle C dans le niveau de travail réel :

1. `RuneCount < 2` -> activation du callback -> porte fermée ;
2. `RuneCount >= 2` -> activation du callback -> `SecretDoor` ouverte.

Le test PIE doit confirmer le comportement du Data Asset réel, de l’authoring Lua et du runtime avec les assets du niveau.

Le Puzzle D réel est optionnel si les validations MON13 précédentes restent vertes.

## 8. Critères de clôture MON19

État actuel :

```text
Compilation                             OK
Grimrock.MON19.8                        4/4 Success
Grimrock.MON19                          EN ATTENTE
PIE puzzle de production représentatif EN ATTENTE
```

MON19 sera déclaré **VALIDÉ ET CLOS** lorsque la régression complète et le PIE représentatif seront également fournis depuis UE5.5.4.

À ce moment-là :

1. créer `docs/Design/MON19_CLOSURE.md` ;
2. mettre à jour `docs/Design/PROJECT_COMPLETION_ROADMAP.md` avec MON19 CLOS et MON20 PROCHAIN ;
3. mettre à jour la synthèse/overview autoritaire si nécessaire ;
4. pousser le commit de clôture sur `origin/master`.

## 9. Conclusion technique

La hiérarchie finale recherchée reste :

```text
cas simple
    -> Event -> Command direct

état / compteur / comparaison
    -> variables + nœuds Logic

orchestration réellement complexe
    -> Lua
       mais sortie par commandes normales
```

Lua complète donc le modèle data-driven ; il ne le remplace pas.
