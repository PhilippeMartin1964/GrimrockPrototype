# MON19.7 — Sandbox et packaging Lua

Statut : **Automation UE5.5.4 validée — smoke test package Win64 en attente**  
Date : **23 août 2026**  
Référence de départ : `50d43bf90aa1abc75496f8267fa031928ac730e2` (`Rétablir une entrée Lua unique dans Window MON19.6`)

## 1. Objectif

MON19.7 verrouille le contrat de sécurité et de packaging du scripting Lua avant les scénarios de clôture MON19.8.

Le principe est :

```text
niveau packagé
    -> UGridLevelAsset::LuaScripts
        -> sources texte embarquées dans l'asset
            -> FGridLuaVm
                -> Lua 5.4.8 compilé dans GrimrockLua
```

Le runtime ne dépend d'aucun :

```text
fichier .lua externe
DLL Lua
require
package.path
filesystem
loader dynamique
```

## 2. Packaging actuel conservé

`GrimrockLua` reste un module Unreal de type `Runtime`.

Lua 5.4.8 est compilé directement dans :

```text
Source/GrimrockLua/Private/Lua54.cpp
```

à partir du source officiel épinglé dans :

```text
ThirdParty/Lua54
```

Le sous-module est une dépendance de **compilation du projet source**. Il n'est pas une dépendance runtime du jeu packagé : le code Lua est lié au module `GrimrockLua` lors du build.

Les scripts de niveau sont des `FString` dans :

```text
FGridLuaScriptSource
UGridLevelAsset::LuaScripts
```

Ils sont donc cuisinés avec l'asset de niveau et non chargés depuis le disque par Lua.

## 3. Hard caps de contenu

MON19.7 ajoute trois limites non configurables par les données d'un niveau :

```text
HardMaxScriptCount             = 64
HardMaxSourceBytesPerScript    = 256 KiB UTF-8
HardMaxTotalSourceBytes        = 1 MiB UTF-8 par niveau
```

Ces limites sont portées par `FGridLuaVm` et appliquées dans :

```text
FGridLuaVm::ValidateScriptDefinitions()
```

Elles s'appliquent aux scripts activés **et désactivés**. Désactiver un script ne permet donc pas de cacher une charge source hors budget dans un niveau distribué.

Les limites existantes restent également actives :

```text
MemoryLimitBytes           = 8 MiB par VM par défaut
InstructionBudgetPerCall   = 100 000 par exécution par défaut
Event/Command/Lua budget   = 128 actions par chaîne runtime
```

## 4. Source texte uniquement

Le chargement conserve :

```cpp
luaL_loadbufferx(..., "t")
```

Le mode `t` refuse les chunks binaires Lua.

MON19.7 ajoute un test explicite qui fournit une signature de chunk binaire et vérifie qu'aucun VM n'est accepté.

Le SaveGame ne sérialise toujours aucun bytecode ni état interne du VM.

## 5. Surface sandbox durcie

Les bibliothèques globales suivantes restent absentes du `_ENV` de script :

```text
io
os
package
debug
coroutine
require
dofile
loadfile
load
collectgarbage
```

Les bibliothèques pures restent disponibles sous forme de tables clonées par script :

```text
math
string
table
utf8
```

MON19.7 retire en plus :

```text
string.dump
```

Un script ne peut donc ni charger un chunk dynamique (`load` absent), ni générer un chunk binaire réutilisable via `string.dump`.

Chaque script conserve son `_ENV` isolé. Une mutation de `math`, `string`, `table` ou `utf8` reste locale au script.

## 6. API hôte

La seule surface gameplay reste :

```text
grid.vars.get_bool
grid.vars.set_bool
grid.vars.get_int
grid.vars.set_int
grid.command
grid.log
```

Elle n'expose aucun :

```text
UObject
AActor
UWorld
réflexion Unreal
console
processus
filesystem
socket
module natif
```

Les fonctions `grid.*` ne sont installées qu'après l'initialisation top-level du script et ne sont effectivement reliées au gameplay que pendant un callback hébergé MON19.4.

## 7. Package sans fichiers Lua externes

Le contrat de distribution est volontairement simple :

```text
éditeur / source du projet
    ThirdParty/Lua54      -> nécessaire au build C++

jeu packagé
    GrimrockLua compilé  -> contient le runtime Lua
    cooked LevelAsset    -> contient les sources de scripts
```

Il n'est pas prévu de copier un dossier `Scripts/` à côté de l'exécutable.

Cette architecture évite notamment :

- les chemins absolus ;
- les différences de répertoire entre Editor et Shipping ;
- `package.path` ;
- le remplacement opportuniste d'un script sur disque ;
- une dépendance DLL Lua séparée.

## 8. Tests MON19.7

Suite :

```text
Grimrock.MON19.7.LuaSandboxPackaging
```

Tests :

```text
SourceHardLimits
TextOnlyBytecodeRejection
ForbiddenSurfaceHardened
EmbeddedSourceRuntime
```

Ils vérifient :

- limite de 64 scripts ;
- limite de 256 KiB UTF-8 par script ;
- limite de 1 MiB UTF-8 par niveau ;
- prise en compte des scripts désactivés dans les hard caps ;
- rejet d'une entrée ressemblant à un chunk binaire ;
- absence de filesystem / module loader / debug ;
- absence de `load` et `string.dump` ;
- présence des bibliothèques pures autorisées ;
- exécution d'une source fournie uniquement en mémoire via `FGridLuaScriptSource`.

## 9. Validation Automation UE5.5.4

Résultats fournis le **23 août 2026** depuis Unreal Engine 5.5.4 :

```text
Grimrock.MON19.7.LuaSandboxPackaging
    EmbeddedSourceRuntime       Success
    ForbiddenSurfaceHardened    Success
    SourceHardLimits            Success
    TextOnlyBytecodeRejection   Success
                                4/4 Success

Grimrock.MON19.3.Lua.Foundation
                                6/6 Success

Grimrock.MON19.4.LuaBridge
                                5/5 Success

Grimrock.MON19.5.LuaPersistence
                                4/4 Success

Grimrock.MON19.6.Editor
                                4/4 Success
```

Bilan du périmètre demandé :

```text
23/23 Success
```

Les avertissements observés ne constituent pas des échecs de MON19.7 :

- `FlushRenderingCommands called recursively` apparaît pendant `EmbeddedSourceRuntime`, qui termine néanmoins en `Success` ;
- `SharedActionBudget` provoque volontairement l'épuisement du budget commun Event/Command/Lua et termine en `Success` ;
- `InvalidCurrentSourceDropsStaleVm` injecte volontairement une source Lua syntaxiquement invalide afin de vérifier qu'un ancien VM ne survit pas au rebuild et termine en `Success`.

La validation automatisée du sandbox, du source-only runtime et des non-régressions Lua est donc **acquise**.

## 10. Validation packaging Win64

La validation finale de MON19.7 doit comprendre, en plus des Automation Tests désormais validés, un build/package Win64 réel.

Critères :

1. le package se construit sans copier manuellement `ThirdParty/Lua54` dans le répertoire de sortie ;
2. le jeu démarre sans DLL Lua externe ;
3. un niveau contenant un binding Lua peut être chargé ;
4. le callback Lua fonctionne depuis les sources embarquées dans le `UGridLevelAsset` ;
5. aucun fichier `.lua` externe n'est nécessaire à côté de l'exécutable.

MON19.7 ne sera marqué **VALIDÉ et clos** qu'après ce smoke test package fourni depuis UE5.5.4.

## 11. Hors périmètre

MON19.7 n'ajoute pas :

```text
éditeur de mods externe
import de fichiers .lua depuis l'OS
require sécurisé
modules Lua entre fichiers
hot reload filesystem
signature cryptographique des niveaux
chiffrement des scripts
réseau
API UObject générique
nouveau SaveVersion
```

L'ouverture complète aux niveaux créés par les joueurs pourra s'appuyer sur ce contrat source-only/sandboxé sans modifier le coeur du runtime Lua.
