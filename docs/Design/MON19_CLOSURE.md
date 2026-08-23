# MON19 — Advanced Dungeon Logic / Scripting — CLOSURE

Statut : **VALIDÉ ET CLOS sous UE5.5.4**  
Date : **23 août 2026**  
Référence de clôture technique : `d6eb728e83af26f9e70804f5e5b094afbb818b87`

## 1. Objectif atteint

MON19 devait permettre au level designer de construire des énigmes et mécanismes riches sans ajouter du C++ spécifique à chaque puzzle, tout en réutilisant l'architecture Event -> Command et `UGridLevelAsset`.

L'objectif est atteint avec une hiérarchie volontairement simple :

```text
cas simple
    -> Event -> Command direct

état / compteur / comparaison
    -> variables persistantes + nœuds Logic

orchestration complexe
    -> Lua sandboxé
       -> commandes runtime normales
```

Lua complète donc le modèle data-driven ; il ne le remplace pas.

## 2. Architecture finale

Le chemin autoritaire reste :

```text
objet de grille / événement gameplay
    -> FGridObjectLink
        -> commande existante
        -> nœud Logic
        ou
        -> LuaCallback
             -> grid.command(...)
        -> runtime existant
```

MON19 n'introduit ni second bus d'événements, ni Actor de script générique, ni Tick permanent.

## 3. Capacités livrées

MON19 apporte notamment :

- audit et consolidation du contrat Event -> Command existant ;
- conditions de liens typées ;
- variables de niveau persistantes `Bool` et `Int32` ;
- primitives Logic data-driven pour mutation, comparaison, latch/relay et chaînage ;
- intégration SaveGame et migration des variables de niveau ;
- VM Lua embarquée et sandboxée ;
- quotas mémoire et budget d'instructions ;
- isolation des scripts ;
- pont Event -> Lua -> Command ;
- accès Lua contrôlé aux variables persistantes ;
- persistance reconstruite depuis le Data Asset et l'état runtime, sans sérialiser la VM ;
- outils d'authoring Lua dans le Grid Editor ;
- validation/références des bindings Lua ;
- durcissement sandbox et packaging source-only ;
- déclaration Lua `persistent = { ... }` synchronisée avec les `LevelVariables` ;
- `LogicId` lisible pour adresser les objets depuis Lua sans GUID écrit dans les scripts ;
- suite finale de puzzles d'intégration MON19.8.

## 4. Validation finale UE5.5.4

### Compilation

```text
Development Editor / Win64    OK
```

### Tests ciblés MON19.8

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

Les warnings observés dans les tests négatifs sont attendus : protection contre les cycles Logic, variable absente, budget partagé épuisé et source Lua invalide. Les tests concernés terminent tous en `Success`.

### PIE représentatif final

Le puzzle réel utilisant le Secret Button et la Secret Door avec `LogicId = SecretDoor` a été validé.

Script représentatif :

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

Comportement observé :

```text
1er clic : RuneCount 0 -> 1, porte fermée
2e clic : RuneCount 1 -> 2, condition vraie, SecretDoor ouverte
```

La déclaration `persistent` crée/synchronise bien la variable `RuneCount` dans le Data Asset et `LogicId` résout la porte sans GUID dans le script.

## 5. Puzzles de clôture couverts

```text
A. Lever.Activated -> Door.Open
B. Deux leviers -> AddInt -> CompareInt >= 2 -> Door.Open
C. Variable persistante -> condition Lua -> LogicId -> Door.Open
D. EncounterCompleted -> Lua -> Door.Open
```

Ces quatre scénarios démontrent que les mécanismes simples, les puzzles à état et l'orchestration Lua coexistent dans la même architecture.

## 6. Documents de référence MON19

```text
MON19_1_EVENT_COMMAND_AUDIT_LUA_FEASIBILITY.md
MON19_2_1A_EVENT_COMMAND_CONNECTOR_CONTRACT.md
MON19_2_3_LOGIC_PRIMITIVES.md
MON19_4_EVENT_LUA_COMMAND_BRIDGE.md
MON19_6_LUA_EDITOR_VALIDATION.md
MON19_7_1_LUA_AUTHORING_API.md
MON19_8_PRODUCTION_PUZZLES_CLOSURE.md
```

Les autres documents MON19 restent les preuves détaillées de leurs sous-jalons respectifs.

## 7. Décision de clôture

MON19 est **VALIDÉ ET CLOS**.

Le projet possède désormais un système d'énigmes avancées data-driven extensible, persistant et authorable dans l'éditeur, avec Lua réservé aux cas où les primitives Event/Command/Logic deviennent insuffisantes.

## 8. Prochain jalon

```text
MON20 — Recruitment / Skills / Talents — PROCHAIN
```

MON20 devra commencer par un audit de l'existant afin de réutiliser la création de personnage, les classes, les statistiques, la progression MON15, le catalogue d'actions et la persistance déjà en place avant d'ajouter de nouveaux contrats.
