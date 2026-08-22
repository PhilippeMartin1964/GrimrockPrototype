# MON19.3.1 — Fondation runtime Lua 5.4.8

Statut : **implémenté — validation UE5.5.4 en attente**  
Date : **22 août 2026**  
Référence de départ : `73b116819e28dac5864cdffb09fafde97dd80e2d` (`Ajouter les conditions de liens sur variables MON19.2.4`)

## 1. Objectif

MON19.3.1 introduit uniquement la fondation d'exécution Lua nécessaire aux étapes suivantes de MON19. Il ne branche pas encore Lua sur le dispatcher Event → Command.

Le contrat recherché est :

```text
un niveau actif
    -> un VM Lua
        -> plusieurs ScriptId
            -> un environnement _ENV isolé par script
```

Cette étape ne crée aucun Actor de gameplay, aucun Tick, aucune nouvelle donnée SaveGame et aucune commande Event → Lua.

## 2. Version et dépendance

Le runtime est épinglé sur **Lua 5.4.8 officiel** :

```text
repository : https://github.com/lua/lua.git
commit     : 6e22fedb74cf0c9b6656e9fce8b7331db847c605
release    : Lua 5.4.8
```

Le dépôt officiel est référencé comme sous-module :

```text
ThirdParty/Lua54
```

Après un premier `git pull` contenant MON19.3.1, initialiser la dépendance avec :

```powershell
git submodule update --init ThirdParty/Lua54
```

Le commit du sous-module est fixe : une mise à jour future de Lua devra être une modification explicite du dépôt principal.

## 3. Module `GrimrockLua`

Lua est encapsulé dans un module runtime séparé :

```text
Source/GrimrockLua/
├── GrimrockLua.Build.cs
├── Public/
│   ├── GridLuaScriptTypes.h
│   └── GridLuaVm.h
└── Private/
    ├── GrimrockLua.cpp
    ├── Lua54.cpp
    ├── GridLuaVm.cpp
    └── Tests/
        └── GridLuaVmTests.cpp
```

Le module utilise :

```text
PCHUsage = NoPCHs
bUseUnity = false
```

Cela empêche les macros/PCH Unreal et les autres unités de compilation de polluer la compilation du source officiel Lua.

`Lua54.cpp` compile l'amalgamation officielle avec :

```cpp
#define MAKE_LIB
#include "onelua.c"
```

L'interpréteur CLI `lua.c` n'est donc pas construit.

## 4. Aucune API Lua dans le module principal

Le C API (`lua_State`, `lua_*`, `luaL_*`) reste entièrement privé à `GrimrockLua`.

Le module `GrimrockPrototype` ne voit que :

```text
FGridLuaScriptSource
FGridLuaVmConfig
FGridLuaVm
```

Cette séparation évite d'introduire l'ABI Lua dans les headers de gameplay et laisse la future API `grid` sous contrôle du projet.

## 5. Scripts multiples par niveau

`UGridLevelAsset` reçoit :

```cpp
TArray<FGridLuaScriptSource> LuaScripts;
```

Une entrée contient :

```text
ScriptId
bEnabled
Source
```

`ScriptId` est obligatoire et unique dans le niveau, y compris pour les entrées désactivées.

Le code source est la seule représentation persistante du script. Aucun bytecode Lua n'est stocké dans le niveau ou le SaveGame.

## 6. VM et reload atomique

`FGridLuaVm::Reload()` construit d'abord un VM candidat.

```text
validation des ScriptId
    -> création du lua_State candidat
    -> construction du sandbox
    -> compilation de tous les scripts enabled
    -> exécution de leurs chunks initiaux
    -> succès global
         -> remplacement de l'ancien VM
       échec
         -> destruction du candidat
         -> ancien VM conservé
```

Une erreur de syntaxe dans un script ne détruit donc pas un état runtime précédemment valide.

Les chunks sont chargés par :

```text
luaL_loadbufferx(..., "t")
```

Le mode `t` refuse le bytecode précompilé.

## 7. Isolation `_ENV`

Tous les scripts d'un niveau partagent le même `lua_State`, mais chacun reçoit sa propre table `_ENV` conservée dans le registry Lua sous un `ScriptId` stable.

Exemple :

```lua
-- PuzzleA
counter = 1

-- PuzzleB
counter = 40
```

Les deux `counter` sont indépendants.

Les tables standard autorisées sont clonées pour chaque environnement afin qu'une écriture telle que :

```lua
math.pi = 1
```

ne modifie pas la table `math` visible par un autre ScriptId.

Les échanges explicites entre scripts devront plus tard utiliser l'API `grid` et les variables de niveau MON19.2, pas des globals Lua partagés.

## 8. Surface standard autorisée

Le sandbox fournit les fonctions de base suivantes :

```text
assert
error
ipairs
next
pairs
pcall
rawequal
rawget
rawlen
rawset
select
tonumber
tostring
type
xpcall
_VERSION
```

ainsi que :

```text
math
string
table
utf8
```

Ne sont volontairement pas exposés :

```text
io
os
package
debug
require
dofile
loadfile
load
collectgarbage
coroutine
```

MON19.3.1 n'expose aucun `UWorld`, `Actor`, `UObject`, reflection, console, fichier ou processus au script.

## 9. Quota mémoire

Le VM utilise `lua_newstate()` avec un allocateur Unreal personnalisé basé sur `FMemory`.

Le quota est comptabilisé par VM :

```text
Default : 8 MiB
Minimum : 1 MiB
```

Toute croissance qui dépasserait la limite retourne `nullptr` à Lua et fait échouer proprement le protected call concerné.

## 10. Budget d'instructions

Chaque exécution de chunk/callback reçoit un budget indépendant :

```text
Default : 100000 instructions
```

Un hook `LUA_MASKCOUNT` interrompt une boucle infinie par une erreur protégée :

```text
Grimrock Lua instruction budget exceeded
```

Le hook est retiré après chaque `lua_pcall`.

MON19.4 devra intégrer ce budget au garde global Event → Command → Lua afin que les deux systèmes partagent une politique de récursion cohérente.

## 11. Persistance

MON19.3.1 ne modifie pas le SaveGame :

```text
SaveVersion = 7
```

Ne seront jamais sérialisés :

```text
lua_State
pile Lua
registry Lua
closures
coroutines
userdata
chunks compilés
```

Les résultats persistants des puzzles continueront de vivre dans les variables Bool/Int32 de MON19.2.

## 12. Tests automatisés ajoutés

Suite :

```text
Grimrock.MON19.3.Lua.Foundation
```

Tests :

```text
VersionAndLifecycle
MultipleScriptsIsolated
SandboxSurface
InvalidDefinitionsAtomicReload
InstructionBudget
MemoryQuota
```

Ils couvrent respectivement :

- création/destruction et version Lua 5.4.8 ;
- plusieurs scripts dans un même VM et isolation des globals/tables ;
- absence des bibliothèques dangereuses ;
- validation des ScriptId et reload atomique ;
- interruption d'une boucle infinie ;
- refus d'une croissance mémoire hors quota.

## 13. Hors périmètre de MON19.3.1

Restent volontairement à venir :

```text
Event -> Lua callback
Lua -> Command
API grid.vars
API grid.command
API grid.log contrôlée
callbacks avec contexte d'événement
validation/editor scripting
packaging de scripts joueurs
budget partagé Event/Command/Lua
```

La prochaine étape fonctionnelle est **MON19.4 — pont Event → Lua → Command**, construit sur cette fondation.
