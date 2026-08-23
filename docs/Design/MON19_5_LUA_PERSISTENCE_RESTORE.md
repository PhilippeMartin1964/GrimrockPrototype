# MON19.5 — Persistance / restauration Lua

Statut : **implémenté — validation UE5.5.4 en attente**  
Date : **23 août 2026**  
Référence de départ : `4d1ba5bc85c102e0c9e04f61f3774e420336c81f` (`Valider MON19.4 pont Event Lua Command`)

## 1. Objectif

MON19.5 formalise et vérifie la persistance du scripting Lua sans sérialiser l'état interne de Lua.

Le principe est volontairement strict :

```text
SaveGame
    -> données gameplay autoritaires
        -> FGridDungeonRuntimeState
            -> FGridLevelRuntimeState
                -> variables Bool / Int32

Load
    -> recharge le LevelAsset courant
    -> recrée un VM Lua neuf depuis LuaScripts
    -> applique le snapshot gameplay
    -> les callbacks Lua relisent les variables restaurées
```

Aucun `lua_State`, global Lua, closure, pile, coroutine, registry ou environnement `_ENV` n'est sérialisé.

## 2. Aucun nouveau domaine de sauvegarde

MON19.5 n'ajoute pas de `LuaSaveState`.

Les seules valeurs de script autorisées à porter un état durable sont les variables de niveau MON19.2.2 :

```text
Bool
Int32
```

Elles sont déjà stockées dans `FGridLevelRuntimeState` et sérialisées dans :

```text
UGrimrockPartySaveGame::DungeonRuntimeState
```

Lua y accède uniquement via :

```text
grid.vars.get_bool
grid.vars.set_bool
grid.vars.get_int
grid.vars.set_int
```

Ces fonctions délèguent à `GridLevelVariableStore`.

## 3. SaveVersion

Aucune évolution de schéma SaveGame n'est nécessaire.

```text
UGrimrockPartySaveGame::CurrentSaveVersion = 7
```

reste inchangé.

La version 7 introduite par MON19.2.2 contient déjà exactement le domaine persistant requis par Lua.

Passer artificiellement à une version 8 aurait créé une migration sans donnée nouvelle et une seconde responsabilité de persistance inutile.

## 4. Pipeline réel de sauvegarde

Le chemin de production existant est conservé :

```text
AGrimrockPartyPawn::SaveCurrentGame
    -> AGridLevelRuntimeActor::CaptureCurrentLevelRuntimeState
    -> UGrimrockPartySaveGame::DungeonRuntimeState
    -> UGameplayStatics::SaveGameToSlot
```

`CaptureCurrentLevelRuntimeState()` reconstruit les domaines portes, objets interactifs, items, réceptacles et monstres, mais conserve le domaine de variables logiques déjà tenu à jour par `GridLevelVariableStore`.

Le `Serialize()` de `UGrimrockPartySaveGame` continue d'assurer validation et migration du snapshot v7.

## 5. Pipeline réel de restauration

Le chemin de production existant est également conservé :

```text
AGrimrockPartyPawn::LoadCurrentGameData
    -> UGameplayStatics::LoadGameFromSlot
    -> DungeonRuntimeState = Saved DungeonRuntimeState
    -> résolution du LevelAsset sauvegardé
    -> AGridLevelRuntimeActor::RebuildLevel
    -> AGridLevelRuntimeActor::ApplyCurrentLevelRuntimeState
```

`RebuildLevel()` réalise, pour un rebuild runtime complet :

```text
ClearVisuals
    -> UGridActivationComponent::ResetRuntimeState
        -> LuaVm.Reset
    -> UGridActivationComponent::Initialize
        -> ReloadLuaRuntime depuis LevelAsset::LuaScripts
```

Le VM précédent est donc détruit avant le chargement du script du niveau courant.

`ApplyCurrentLevelRuntimeState()` réapplique ensuite le snapshot gameplay, dont les variables Bool / Int32.

## 6. VM toujours neuf après restauration

La restauration ne tente jamais de reprendre l'exécution d'un VM sauvegardé.

Exemple :

```lua
session_counter = 0

function on_trigger(event)
    session_counter = session_counter + 1
end
```

Si `session_counter` vaut `17` au moment de sauvegarder, il revient à `0` lors du prochain chargement du script.

Pour rendre une valeur durable, le script doit utiliser une variable déclarée dans le niveau :

```lua
local count, err = grid.vars.get_int("RuneCount")
local ok, err = grid.vars.set_int("RuneCount", count + 1)
```

`RuneCount` survit à la sauvegarde ; le global Lua ordinaire ne survit pas.

## 7. Le source courant du niveau fait autorité

Le texte des scripts appartient au `UGridLevelAsset`, pas au SaveGame.

Cela permet ce scénario :

```text
sauvegarde créée avec script v1
    -> level asset mis à jour vers script v2
    -> chargement de l'ancienne sauvegarde
    -> variables gameplay v7 restaurées
    -> VM construit avec script v2
```

Le SaveGame ne fige donc pas une ancienne copie du code Lua.

Cette règle est indispensable pour les corrections de scripts et les futures mises à jour de niveaux.

## 8. Source courante invalide : aucun VM obsolète

Le cas critique est couvert par le cycle de rebuild existant.

Avant le chargement des scripts courants :

```text
ResetRuntimeState
    -> LuaVm.Reset
```

Ainsi, si le script courant contient une erreur de syntaxe, son chargement échoue mais le VM de l'ancien niveau / ancienne version n'existe déjà plus.

Le runtime peut donc échouer proprement sans exécuter par accident un callback périmé.

Cette règle est différente du hot reload explicite `ReloadLuaRuntime()`, dont le caractère atomique reste utile hors restauration : un hot reload invalide peut conserver le VM précédemment valide. Un rebuild de niveau complet, lui, détruit d'abord ce VM.

## 9. Tests MON19.5

Nouveau fichier :

```text
Source/GrimrockPrototype/Private/Tests/GridMON195LuaPersistenceTests.cpp
```

Suite :

```text
Grimrock.MON19.5.LuaPersistence
```

### 9.1 SaveRoundTripFreshVm

Le test :

1. exécute deux callbacks Lua ;
2. modifie une variable persistante `Count` et un compteur de session Lua ;
3. capture `FGridDungeonRuntimeState` ;
4. effectue un vrai `UGameplayStatics::SaveGameToMemory` ;
5. recharge avec `LoadGameFromMemory` ;
6. instancie un nouveau `AGridLevelRuntimeActor` ;
7. reconstruit le niveau et applique le snapshot ;
8. exécute un callback dans le VM neuf.

Attendus :

```text
Count : 12 avant save -> 13 après restore + callback
session_counter Lua : 2 avant save -> 1 dans le VM restauré
```

Le test prouve simultanément la persistance des variables autoritaires et la non-persistance des globals Lua.

### 9.2 CurrentScriptSourceWins

Le test sauvegarde avec un script v1, remplace le source du `LevelAsset` par une v2, recharge le snapshot puis vérifie que la v2 s'exécute sur les valeurs persistées par la v1.

### 9.3 InvalidCurrentSourceDropsStaleVm

Le test charge d'abord un VM valide, remplace le script par une source syntaxiquement invalide puis appelle un rebuild complet.

Le callback historique ne doit plus pouvoir s'exécuter et ne doit pas modifier la variable `Gate`.

### 9.4 SaveVersionContract

Le test verrouille explicitement :

```text
CurrentSaveVersion == 7
```

## 10. Non-régression attendue

Après compilation UE5.5.4, lancer :

```text
Grimrock.MON19.5.LuaPersistence
Grimrock.MON19.4.LuaBridge
Grimrock.MON19.3.Lua.Foundation
Grimrock.MON19.2.Save
```

MON19.5 est considéré validé uniquement après résultat UE5.5.4 fourni par l'utilisateur.

## 11. Modifications de production

Aucune modification de production n'est nécessaire pour MON19.5.

Ce résultat est intentionnel : les responsabilités nécessaires existaient déjà grâce à :

```text
MON19.2.2 -> variables persistantes v7
MON19.3.1 -> VM recréable depuis les sources
MON19.4   -> grid.vars et bridge runtime
```

MON19.5 ferme le contrat de persistance par des tests de bout en bout plutôt que par l'ajout d'une abstraction parallèle.

## 12. Hors périmètre

MON19.5 n'ajoute pas :

```text
snapshot lua_State
globals Lua persistants
closures persistantes
coroutines persistantes
sérialisation de _ENV
nouveau format SaveGame
SaveVersion 8
éditeur de scripts/bindings
packaging joueur
```

Ces exclusions sont des garanties d'architecture, pas des fonctionnalités manquantes.
