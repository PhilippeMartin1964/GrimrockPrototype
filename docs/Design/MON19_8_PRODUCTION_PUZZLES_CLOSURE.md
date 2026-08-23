# MON19.8 — Suite d’énigmes de production, régression et clôture

Statut : **implémenté — validation UE5.5.4 en attente**  
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

L’audit MON19.1 avait fixé quatre énigmes représentatives.

### Puzzle A — data-driven directe

```text
Lever.Activated
    -> Door.Open
```

But : démontrer que le chemin historique Event -> Command reste la solution la plus simple lorsqu’aucune logique avancée n’est nécessaire.

Lua n’intervient pas.

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

But : démontrer qu’un puzzle à compteur/seuil reste entièrement data-driven grâce aux variables persistantes MON19.2.2 et aux primitives Logic MON19.2.3.

Le premier levier donne `RuneCount = 1` et la porte reste fermée. Le second donne `RuneCount = 2` et la porte s’ouvre.

### Puzzle C — Lua conditionnel

Le runtime persistant contient `RuneCount`. Le callback Lua lit cette valeur et ne demande `Door.Open` que lorsque le seuil est atteint :

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

But : démontrer qu’un événement de gameplay produit par MON13 peut traverser le pont Lua sans créer une seconde architecture d’événements.

## 3. Tests automatisés ajoutés

Fichier :

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

Résultat attendu :

```text
4/4 Success
```

### 3.1 Puzzle A

Le test crée une vraie porte runtime C++ et vérifie :

1. porte initialement fermée ;
2. événement `Lever.Activated` ;
3. lien direct `Door.Open` ;
4. passage de porte ouvert.

### 3.2 Puzzle B

Le test vérifie :

1. `RuneCount` déclaré à `0` ;
2. premier levier -> `1` ;
3. `CompareInt >= 2` faux -> porte fermée ;
4. second levier -> `2` ;
5. comparaison vraie -> `Door.Open` ;
6. porte ouverte.

Aucun Lua n’est chargé pour cette énigme.

### 3.3 Puzzle C

Le test vérifie deux branches sur le même callback :

```text
RuneCount = 1 -> callback Success -> porte reste fermée
RuneCount = 2 -> callback Success -> SecretDoor.Open -> porte ouverte
```

Il couvre ensemble :

- synchronisation runtime de `persistent` ;
- lecture d’une variable typée ;
- branche conditionnelle Lua ;
- résolution `LogicId` ;
- commande normale de porte.

### 3.4 Puzzle D

Le test utilise un objet `MonsterSpawn` comme source stable de l’événement `EncounterCompleted`, puis valide le trajet :

```text
EncounterCompleted -> LuaCallback -> grid.command -> Door.Open
```

L’émission réelle de `EncounterCompleted` par le système de rencontre reste couverte par MON13 ; MON19.8 vérifie ici son intégration avec le pont Lua et le dispatcher central.

## 4. Ce que MON19.8 ne change pas

Aucun changement de production n’est nécessaire dans :

- `UGridActivationComponent` ;
- `GridLogicRuntime` ;
- le VM Lua ;
- le système de portes ;
- MON13 MonsterSpawn / Encounter ;
- SaveGame ;
- le Grid Editor.

Aucun `.uasset` / `.umap` n’est modifié par cette étape C++/tests.

C’est volontaire : la suite de clôture doit démontrer que l’architecture déjà construite suffit.

## 5. Validation UE5.5.4 demandée

### 5.1 Compilation

Compiler le projet en :

```text
Development Editor / Win64
```

La compilation n’est considérée comme validée qu’après résultat fourni depuis l’environnement utilisateur.

### 5.2 Tests ciblés MON19.8

Dans Session Frontend / Automation :

```text
Grimrock.MON19.8
```

Attendu :

```text
4/4 Success
```

### 5.3 Régression MON19 complète

Ensuite exécuter :

```text
Grimrock.MON19
```

Le nombre exact de tests dépend de l’ensemble actuellement enregistré par UE5.5.4 ; le critère est :

```text
0 Fail
0 Error
```

Les warnings volontairement produits par les tests de budget, sandbox, source Lua invalide ou cycle Logic restent acceptables lorsque leur test termine en `Success`.

## 6. Validation PIE de clôture proposée

Après Automation verte, utiliser le niveau de travail déjà employé pour MON19.7.1, avec la porte située autour de `(28,22)` et son `Logic Id = SecretDoor`.

Le scénario manuel minimal recommandé pour la clôture est le puzzle C :

1. conserver une variable persistante `RuneCount` ;
2. utiliser un bouton/trigger comme source ;
3. appliquer :

```lua
persistent = {
    RuneCount = 0
}

function on_secret_button(event)
    if persistent.RuneCount >= 2 then
        local ok, err = grid.command("SecretDoor", "Open")
        assert(ok, err)
    end
end
```

4. vérifier que la porte ne s’ouvre pas lorsque `RuneCount < 2` ;
5. vérifier qu’elle s’ouvre lorsque la valeur autoritaire atteint `2` ou plus.

Si un scénario Encounter réel est facilement disponible dans le niveau de test, le puzzle D peut être validé en complément :

```text
EncounterCompleted -> Lua -> SecretDoor.Open
```

Ce complément n’est pas nécessaire pour démontrer à nouveau l’émission MON13 si la campagne Automation MON19.8 et les anciennes validations MON13 restent vertes.

## 7. Critères de clôture MON19

MON19 pourra être déclaré **VALIDÉ ET CLOS** lorsque les éléments suivants auront été fournis depuis UE5.5.4 :

```text
Compilation                           OK
Grimrock.MON19.8                      4/4 Success
Grimrock.MON19                        0 Fail / 0 Error
PIE puzzle de production représentatif VALIDÉ
```

À ce moment-là, l’étape de clôture devra :

1. passer ce document en `VALIDÉ UE5.5.4` ;
2. créer `docs/Design/MON19_CLOSURE.md` ;
3. mettre à jour `docs/Design/PROJECT_COMPLETION_ROADMAP.md` avec MON19 CLOS et MON20 PROCHAIN ;
4. mettre à jour la synthèse/overview autoritaire si nécessaire ;
5. pousser le commit de clôture sur `origin/master`.

## 8. Conclusion technique attendue

La conclusion recherchée par MON19 n’est pas « tous les puzzles doivent être écrits en Lua ».

La hiérarchie finale doit être :

```text
cas simple
    -> Event -> Command direct

état / compteur / comparaison
    -> variables + nœuds Logic

orchestration réellement complexe
    -> Lua
       mais sortie par commandes normales
```

Lua est donc un complément au modèle data-driven, pas son remplacement.
