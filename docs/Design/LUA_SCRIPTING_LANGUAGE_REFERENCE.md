# Grimrock Lua — Référence du langage et contrat de compilation

Statut : **CONTRAT CIBLE POUR LE SCRIPTING DE NIVEAU**  
Date : **11 septembre 2026**  
Baseline : `3313fcbc12e0c16623ab2097f46b811f44310b20`

## 1. Décision

Le langage de script du prototype est **Lua 5.4.8**, exécuté dans un environnement sandboxé par `FGridLuaVm`.

Le level designer ne doit pas écrire de plomberie de validation autour de chaque appel gameplay.

Forme cible :

```lua
persistent = { GuardianGemCount = 0 }

function on_gem_inserted()
    if persistent.GuardianGemCount >= 2 then
        return
    end

    persistent.GuardianGemCount = persistent.GuardianGemCount + 1

    if persistent.GuardianGemCount == 1 then
        grid.visual.set_material("Guardian", "EyesLeft", "BlueGem")
    elseif persistent.GuardianGemCount == 2 then
        grid.visual.set_material("Guardian", "EyesRight", "BlueGem")
        grid.command("GuardianDoor", "Open")
    end
end
```

Les formes suivantes ne sont pas la convention d'authoring cible :

```lua
local function must(ok, err)
    assert(ok, err)
end

must(grid.command(...))
assert(grid.command(...))
```

Les erreurs d'authoring doivent être détectées par le **compilateur Grimrock Lua** avant exécution du niveau.

Le runtime conserve uniquement ses gardes internes de sûreté d'exécution. Elles ne doivent pas être reportées dans les scripts du level designer.

---

## 2. Ce qu'est exactement « Grimrock Lua »

Grimrock Lua n'est pas un nouveau langage inventé par le projet.

Il est constitué de :

```text
Lua 5.4.8
+ environnement standard restreint
+ table persistante `persistent`
+ contexte callback `event`
+ API hôte `grid.*`
+ validation sémantique spécifique au UGridLevelAsset
```

Le VM compile les sources texte avec Lua en mode texte uniquement.

Les bibliothèques exposées par le sandbox sont :

```text
math
string
table
utf8
```

Les fonctions de base explicitement exposées sont :

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

Ne sont notamment pas exposés :

```text
io
os
package
require
dofile
loadfile
load
debug
coroutine
print
collectgarbage
```

`string.dump` est retiré du sandbox.

---

## 3. Grammaire Lua utilisée par le prototype

La syntaxe de base est celle de Lua 5.4. La grammaire ci-dessous utilise une notation BNF/EBNF compacte :

- `{ X }` : zéro ou plusieurs `X` ;
- `[ X ]` : `X` optionnel ;
- `|` : alternative ;
- les mots entre guillemets sont des terminaux.

```bnf
<chunk> ::= <block>

<block> ::= { <stat> } [ <retstat> ]

<stat> ::= ";"
         | <varlist> "=" <explist>
         | <functioncall>
         | <label>
         | "break"
         | "goto" <Name>
         | "do" <block> "end"
         | "while" <exp> "do" <block> "end"
         | "repeat" <block> "until" <exp>
         | "if" <exp> "then" <block>
              { "elseif" <exp> "then" <block> }
              [ "else" <block> ] "end"
         | "for" <Name> "=" <exp> "," <exp> [ "," <exp> ]
              "do" <block> "end"
         | "for" <namelist> "in" <explist>
              "do" <block> "end"
         | "function" <funcname> <funcbody>
         | "local" "function" <Name> <funcbody>
         | "local" <attnamelist> [ "=" <explist> ]

<attnamelist> ::= <Name> <attrib> { "," <Name> <attrib> }
<attrib> ::= [ "<" <Name> ">" ]

<retstat> ::= "return" [ <explist> ] [ ";" ]
<label> ::= "::" <Name> "::"

<funcname> ::= <Name> { "." <Name> } [ ":" <Name> ]

<varlist> ::= <var> { "," <var> }
<var> ::= <Name>
        | <prefixexp> "[" <exp> "]"
        | <prefixexp> "." <Name>

<namelist> ::= <Name> { "," <Name> }
<explist> ::= <exp> { "," <exp> }

<exp> ::= "nil"
        | "false"
        | "true"
        | <Numeral>
        | <LiteralString>
        | "..."
        | <functiondef>
        | <prefixexp>
        | <tableconstructor>
        | <exp> <binop> <exp>
        | <unop> <exp>

<prefixexp> ::= <var>
              | <functioncall>
              | "(" <exp> ")"

<functioncall> ::= <prefixexp> <args>
                 | <prefixexp> ":" <Name> <args>

<args> ::= "(" [ <explist> ] ")"
         | <tableconstructor>
         | <LiteralString>

<functiondef> ::= "function" <funcbody>

<funcbody> ::= "(" [ <parlist> ] ")" <block> "end"

<parlist> ::= <namelist> [ "," "..." ]
            | "..."

<tableconstructor> ::= "{" [ <fieldlist> ] "}"

<fieldlist> ::= <field> { <fieldsep> <field> } [ <fieldsep> ]

<field> ::= "[" <exp> "]" "=" <exp>
          | <Name> "=" <exp>
          | <exp>

<fieldsep> ::= "," | ";"

<binop> ::= "+" | "-" | "*" | "/" | "//" | "^" | "%"
          | "&" | "~" | "|" | ">>" | "<<"
          | ".."
          | "<" | "<=" | ">" | ">=" | "==" | "~="
          | "and" | "or"

<unop> ::= "-" | "not" | "#" | "~"
```

### 3.1 Conséquence importante : `++` n'existe pas en Lua

Ceci est invalide :

```lua
persistent.GuardianGemCount++
```

La syntaxe Lua correcte est :

```lua
persistent.GuardianGemCount = persistent.GuardianGemCount + 1
```

Le compilateur Lua doit rejeter immédiatement la première forme.

---

## 4. Grammaire d'authoring Grimrock

La grammaire Lua précédente décrit le langage général. Le projet ajoute un contrat d'authoring statiquement vérifiable.

### 4.1 Déclaration persistante

```bnf
<persistent-declaration> ::= "persistent" "=" "{" [ <persistent-list> ] "}"
<persistent-list> ::= <persistent-entry> { "," <persistent-entry> } [ "," ]
<persistent-entry> ::= <Name> "=" <persistent-literal>
<persistent-literal> ::= "true" | "false" | <Int32Literal>
```

Règles :

- déclaration au niveau supérieur du script ;
- clé = identifiant ;
- type autorisé = Bool ou Int32 ;
- une variable doit conserver le même type ;
- une écriture `persistent.X = ...` est interdite si `X` n'a pas été déclaré.

Exemple :

```lua
persistent = {
    GuardianGemCount = 0,
    GateOpen = false
}
```

---

## 5. Callbacks

Un callback sélectionnable dans **Events & Actions** est une fonction globale du script.

Profil recommandé :

```bnf
<callback> ::= "function" <CallbackName> "(" [ "event" ] ")"
                 <block>
               "end"
```

Exemples valides :

```lua
function on_gem_inserted()
end
```

```lua
function on_gem_inserted(event)
end
```

Le runtime fournit, lorsque `event` est déclaré :

```text
event.source_object_id : string
event.event            : string
```

Le paramètre peut être omis s'il n'est pas utilisé.

---

## 6. API `grid`

Le projet expose actuellement exactement les familles suivantes :

```text
grid.command(...)
grid.visual.set_material(...)
grid.vars.get_bool(...)
grid.vars.set_bool(...)
grid.vars.get_int(...)
grid.vars.set_int(...)
grid.log(...)
```

Pour le nouvel authoring, `persistent` est préféré à `grid.vars.*`.

---

# 7. `grid.command` — contrat détaillé

## 7.1 Syntaxe

```bnf
<grid-command> ::= "grid" "." "command"
                   "(" <LogicIdLiteral> "," <CommandLiteral> ")"

<LogicIdLiteral> ::= <LiteralString>
<CommandLiteral> ::= <LiteralString>
```

Forme normale :

```lua
grid.command("GuardianDoor", "Open")
```

### Premier argument : cible

```text
"GuardianDoor"
```

est le `LogicId` d'un objet placé dans le niveau.

Le contrat d'authoring cible impose un `LogicId` littéral afin que le compilateur puisse résoudre la cible avant le PlayTest.

La forme GUID historique n'est pas la forme d'authoring cible.

### Deuxième argument : commande

```text
"Open"
```

est le nom exact d'une valeur de `EGridObjectCommand` autorisée pour la cible.

Exemples de commandes existantes :

```text
Toggle
Open
Close
Activate
Deactivate
Enable
Disable
Lock
Unlock
Spawn
Despawn
Teleport
ShowMessage
ReceptacleConsumeItem
ReceptacleConsumeAllItems
ReceptacleEnableRemoval
ReceptacleDisableRemoval
StartEncounter
LogicExecute
LogicReset
OfferRecruitment
OpenCustomRecruit
QuestStart
QuestCompleteObjective
QuestComplete
QuestFail
```

`LuaCallback` n'est pas appelable directement par `grid.command`.

Toutes les commandes ne sont pas valides pour tous les types d'objets.

Exemples :

```lua
grid.command("GuardianDoor", "Open")
grid.command("Guardian", "ReceptacleConsumeItem")
grid.command("RatEncounter", "StartEncounter")
```

## 7.2 Ce que fait réellement `grid.command`

`grid.command` n'appelle pas une fonction arbitraire sur un Actor Unreal.

Le chemin est :

```text
Lua
  grid.command("GuardianDoor", "Open")
        |
        v
résolution "GuardianDoor" -> ObjectId interne
        |
        v
résolution "Open" -> EGridObjectCommand::Open
        |
        v
construction d'un FGridObjectLink synthétique
        |
        v
UGridActivationComponent::ApplyLinkCommand()
        |
        v
runtime gameplay existant
```

C'est donc un **pont générique vers le même système Event -> Command** que celui utilisé par l'éditeur.

Il ne crée pas un deuxième système de gameplay.

---

# 8. `grid.visual.set_material`

## 8.1 Syntaxe

```bnf
<set-material> ::= "grid" "." "visual" "." "set_material"
                   "(" <LogicIdLiteral> "," <SlotLiteral> "," <AliasLiteral> ")"

<SlotLiteral> ::= <LiteralString>
<AliasLiteral> ::= <LiteralString>
```

Exemple :

```lua
grid.visual.set_material("Guardian", "EyesLeft", "BlueGem")
```

Signification :

```text
Guardian = LogicId de l'objet cible
EyesLeft = nom du slot matériau du mesh
BlueGem  = alias défini dans RuntimeMaterialAliases du DataAsset de l'objet
```

Le script ne connaît aucun chemin Unreal `/Game/...`.

---

# 9. `grid.vars`

API existante :

```bnf
<get-bool> ::= "grid.vars.get_bool(" <LiteralString> ")"
<set-bool> ::= "grid.vars.set_bool(" <LiteralString> "," <exp> ")"
<get-int>  ::= "grid.vars.get_int(" <LiteralString> ")"
<set-int>  ::= "grid.vars.set_int(" <LiteralString> "," <exp> ")"
```

Pour le nouvel authoring de niveau, utiliser de préférence :

```lua
persistent.Counter = persistent.Counter + 1
```

plutôt que :

```lua
local value = grid.vars.get_int("Counter")
grid.vars.set_int("Counter", value + 1)
```

`persistent` est plus lisible et reste synchronisé avec l'état persistant du niveau.

---

# 10. Contrat du compilateur Grimrock Lua

Le bouton actuel **Validate Lua** doit évoluer en **Compile Lua**.

Le compilateur Grimrock Lua n'est pas un remplaçant de Lua. C'est une chaîne de compilation d'authoring en deux passes :

```text
PASS 1 — compilateur Lua 5.4.8
    syntaxe Lua
    bytecode interdit / source texte seulement
    initialisation sandboxée
    définition de persistent

PASS 2 — compilateur sémantique Grimrock
    callbacks
    persistent
    grid.command
    grid.visual.set_material
    symboles et contrats du niveau
```

Le script n'est considéré compilé que si les deux passes réussissent.

## 10.1 Validation obligatoire de `grid.command`

Pour chaque appel statique :

```lua
grid.command("GuardianDoor", "Open")
```

le compilateur doit vérifier :

1. exactement deux arguments ;
2. deux chaînes littérales ;
3. `GuardianDoor` respecte le format d'un LogicId ;
4. un et un seul objet possède ce LogicId ;
5. `Open` existe dans `EGridObjectCommand` ;
6. la commande est autorisée pour le type de la cible ;
7. `LuaCallback` est interdit ;
8. la cible possède les données requises par la commande.

Exemples d'erreurs de compilation :

```text
LUA-COMPILE E201 line 12: unknown LogicId 'GaurdianDoor'
LUA-COMPILE E202 line 12: LogicId 'GuardianDoor' is ambiguous
LUA-COMPILE E203 line 12: unknown command 'Opeen'
LUA-COMPILE E204 line 12: command 'Open' is not supported by target 'Guardian' (Receptacle)
```

## 10.2 Validation obligatoire de `grid.visual.set_material`

Pour :

```lua
grid.visual.set_material("Guardian", "EyesLeft", "BlueGem")
```

le compilateur doit vérifier :

1. exactement trois chaînes littérales ;
2. `Guardian` résout un objet unique ;
3. l'objet possède une définition visuelle ;
4. `EyesLeft` existe comme slot matériau adressable ;
5. `BlueGem` existe dans `RuntimeMaterialAliases`.

Exemples :

```text
LUA-COMPILE E301 line 8: target 'Guardian' has no visual definition
LUA-COMPILE E302 line 8: material slot 'EyeLeft' does not exist on 'Guardian'
LUA-COMPILE E303 line 8: material alias 'BlueGemm' is not declared on 'Guardian'
```

## 10.3 Validation obligatoire de `persistent`

Le compilateur doit détecter avant PlayTest :

```lua
persistent.Unknown = 1
```

si `Unknown` n'est pas déclaré.

Il doit également refuser :

```lua
persistent.Counter = "two"
```

si `Counter` est Int32.

## 10.4 Appels dynamiques interdits pour l'API gameplay

Afin de garantir la validation à la compilation, les identifiants gameplay doivent être statiques.

À accepter :

```lua
grid.command("GuardianDoor", "Open")
```

À refuser par le profil d'authoring :

```lua
local target = "GuardianDoor"
grid.command(target, "Open")
```

```lua
local command = "Open"
grid.command("GuardianDoor", command)
```

Raison : le compilateur doit pouvoir prouver la validité de l'appel sans exécuter le puzzle.

---

# 11. Rôle du runtime

Le runtime ne doit pas servir de validateur d'authoring.

Il conserve néanmoins des gardes internes parce qu'un moteur ne doit jamais faire confiance aveuglément à des données chargées.

La séparation est :

```text
COMPILATEUR
    "ce script et ses références sont-ils corrects ?"

RUNTIME
    "exécuter le script déjà compilé et protéger l'intégrité du moteur"
```

Le level designer n'a donc pas à écrire :

```lua
assert(...)
must(...)
if not ok then ...
```

pour chaque commande normale du puzzle.

---

# 12. UX cible

Fenêtre **GRIMROCK LUA — SCRIPTS** :

```text
[+ Add Script] [Compile Lua]

SELECTED SCRIPT
    Script Id
    Source

    [Apply] [Revert] [Remove]

COMPILER
    Success
ou
    E203 line 12: unknown command 'Opeen'
    E303 line 8 : material alias 'BlueGemm' is not declared on 'Guardian'

DETECTED CALLBACKS
    on_gem_inserted

PERSISTENT STATE
    GuardianGemCount : Int32 = 0
```

`Apply` doit lancer le même compilateur sur la source candidate. Si la compilation échoue, la source enregistrée dans le LevelAsset ne change pas.

Le PlayTest et la validation générale du niveau doivent refuser un niveau ayant un script non compilable.

---

# 13. Exemple Guardian canonique

```lua
persistent = { GuardianGemCount = 0 }

function on_gem_inserted()
    if persistent.GuardianGemCount >= 2 then
        return
    end

    persistent.GuardianGemCount = persistent.GuardianGemCount + 1

    if persistent.GuardianGemCount == 1 then
        grid.visual.set_material("Guardian", "EyesLeft", "BlueGem")
    elseif persistent.GuardianGemCount == 2 then
        grid.visual.set_material("Guardian", "EyesRight", "BlueGem")
        grid.command("GuardianDoor", "Open")
    end
end
```

Ce script doit être accepté ou rejeté par le compilateur **avant** le runtime.

Aucun `must`, aucun `assert`, aucun GUID, aucune gestion manuelle des retours de `grid.command` n'est requis dans le script de level design.

---

# 14. Implémentation proposée

Jalon proposé : **LUA-COMP01 — Grimrock Lua Authoring Compiler**.

Composants :

```text
FGridLuaVm
    -> reste l'autorité de compilation/exécution Lua 5.4.8

FGridLuaAuthoringCompiler (nouveau, Editor)
    -> analyse lexicale des appels grid.*
    -> diagnostics ligne/colonne
    -> validation sémantique contre UGridLevelAsset

GridEditorLuaService
    -> utilise FGridLuaAuthoringCompiler pour AnalyzeLevel / SetScriptSource

SGridEditorLuaScriptsPanel
    -> Validate Lua devient Compile Lua
    -> affiche diagnostics du compilateur
```

Le compilateur d'authoring doit utiliser les mêmes autorités que le runtime :

```text
LogicId          -> UGridLevelAsset
Command          -> EGridObjectCommand
compatibilité    -> GridEditorLinkPolicy / contrat runtime
Material Alias   -> UGridWorldObjectDefinitionAsset::RuntimeMaterialAliases
Material Slot    -> mesh de la définition
```

Aucune table parallèle de commandes, aucun second modèle d'objet et aucun mini-langage supplémentaire ne doivent être introduits.

---

# 15. Principe final

Le script doit rester du gameplay :

```lua
if condition then
    grid.command("Door", "Open")
end
```

Le compilateur porte la charge de validation.

Le runtime porte la charge d'exécution.

Le level designer ne porte pas la plomberie du moteur.
