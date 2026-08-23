# MON19.4 — Pont Event → Lua → Command

Statut : **implémenté — validation UE5.5.4 en attente**  
Date : **23 août 2026**  
Référence de départ : `57801b87fdba3c03882f71713cf69165fbb5363d` (`Valider MON19.3.1 fondation runtime Lua`)

## 1. Objectif

MON19.4 raccorde la fondation Lua 5.4.8 de MON19.3.1 au dispatcher Event → Command existant sans créer de second bus de gameplay.

Le flux cible est désormais :

```text
Grid Object Event
    -> FGridObjectLink
        -> Lua callback
            -> grid.vars
            -> grid.command
                -> UGridActivationComponent
                    -> commandes gameplay existantes
```

Lua ne reçoit aucun accès direct à `UWorld`, `Actor`, `UObject`, reflection Unreal, filesystem, console ou processus.

## 2. Contrat persistant du lien

`EGridObjectCommand` reçoit une valeur supplémentaire, ajoutée en fin de contrat afin de ne pas renuméroter les valeurs existantes :

```text
LuaCallback = 22
```

`FGridObjectLink` reçoit :

```text
LuaScriptId
LuaCallbackName
```

Pour un lien Lua :

```text
SourceObjectId : objet qui émet l'événement
SourceEvent    : Activated / Deactivated / etc.
Command        : LuaCallback
LuaScriptId    : ScriptId déclaré dans UGridLevelAsset::LuaScripts
LuaCallbackName: fonction Lua appelée
TargetObjectId : non requis par le runtime Lua
```

Les liens ObjectCommand historiques conservent leur fonctionnement inchangé.

L'identité exacte de l'éditeur inclut maintenant `LuaScriptId` et `LuaCallbackName`.

## 3. Contexte d'événement

Le callback reçoit un unique argument table :

```lua
function on_trigger(event)
    -- event.source_object_id
    -- event.event
end
```

Exemple :

```text
event.source_object_id = "00000013-00000004-00000001-00000001"
event.event            = "Activated"
```

Le contexte reste volontairement minimal. Aucun pointeur Unreal n'est transmis.

## 4. API `grid.vars`

Les variables Lua passent exclusivement par `GridLevelVariableStore`, autorité validée en MON19.2.2.

API :

```lua
value, err = grid.vars.get_bool("Gate")
ok, err    = grid.vars.set_bool("Gate", true)

value, err = grid.vars.get_int("RuneCount")
ok, err    = grid.vars.set_int("RuneCount", value + 1)
```

Les erreurs sont retournées comme données. Les fonctions natives n'utilisent pas `luaL_check*` pour transformer une mauvaise entrée en longjmp arbitraire.

Le script peut décider de rendre l'échec fatal :

```lua
local value, err = grid.vars.get_int("RuneCount")
assert(err == nil, err)
```

Les contrôles de type et les variables déclarées restent ceux de `GridLevelVariableStore`.

## 5. API `grid.command`

Syntaxe :

```lua
ok, err = grid.command("OBJECT_GUID", "CommandName")
```

Exemple :

```lua
local ok, err = grid.command(
    "00000013-00000004-00000002-00000002",
    "LogicExecute")
assert(ok, err)
```

`grid.command` ne duplique pas la logique gameplay. Il construit une commande interne sans condition et la repasse dans `UGridActivationComponent::ApplyLinkCommand()`.

Ainsi les implémentations existantes restent autoritaires :

```text
Door
Receptacle
MonsterSpawn
Logic primitives
stateful mechanisms
```

`grid.command(..., "LuaCallback")` est explicitement refusé. Un callback Lua ne peut pas contourner le contrat de liaison pour appeler arbitrairement un autre callback.

## 6. API `grid.log`

Syntaxe :

```lua
grid.log("message")
```

Le runtime journalise avec le `ScriptId` courant :

```text
[GridLua:Puzzle] message
```

## 7. VM par niveau runtime

`UGridActivationComponent` possède un `FGridLuaVm` non sérialisé.

`Initialize()` charge les `LuaScripts` du `LevelAsset` courant. `ReloadLuaRuntime()` est exposé côté C++ pour les tests et les futures fonctions éditeur.

Si le VM n'est pas prêt au premier callback, le runtime tente un reload paresseux avant de rejeter le lien.

Aucun état interne Lua n'est ajouté au SaveGame. `SaveVersion` reste **7**.

Les données persistantes continuent d'être portées par les variables Bool/Int32 MON19.2.

## 8. Conditions de liens

Un lien `LuaCallback` utilise le même évaluateur de conditions que les liens existants.

Sont donc utilisables dès le runtime MON19.4 :

```text
None
Level Variable Bool Equals
Level Variable Int Compare
```

Les conditions Receptacle spécialisées nécessitent toujours un Actor Receptacle cible et ne sont donc pas adaptées à un callback Lua sans cible objet.

## 9. Protection de réentrance

Deux protections coexistent.

### 9.1 Garde historique par SourceObjectId

`DispatchingSourceObjectIds` reste actif et bloque les cycles directs d'événements déjà couverts par MON19.2.3.

### 9.2 Callback Lua imbriqué

Un callback Lua ne peut pas déclencher synchroniquement un second callback Lua pendant que son host est actif.

Cas refusé :

```text
Lua A
  -> grid.command(LogicExecute)
      -> Logic.Activated
          -> Lua B   X rejeté pendant A
```

Les commandes objet et primitives Logic restent autorisées à l'intérieur de A.

Cette règle évite de remplacer le host actif et le hook/budget d'instructions du même `lua_State` pendant un `lua_pcall` en cours.

## 10. Budget partagé Event / Command / Lua

Chaque appel racine à :

```text
ExecuteLinksFromObjectForEvent
```

initialise :

```text
MaxRuntimeActionBudget = 128
```

Chaque `ApplyLinkCommand()` consomme une unité, qu'il s'agisse :

```text
ObjectCommand
LuaCallback
grid.command -> ObjectCommand
```

Les appels imbriqués partagent le même compteur.

Ce garde complète le budget d'instructions Lua de MON19.3.1 :

```text
Lua while true       -> instruction budget du VM
Event/Command chain  -> runtime action budget
```

Une chaîne sans cycle direct mais composée de plus de 128 actions est donc interrompue avant de pouvoir croître indéfiniment.

## 11. Sandbox

MON19.4 ne modifie pas la surface standard validée en MON19.3.1.

Les seuls nouveaux points d'entrée sont :

```text
grid.vars.get_bool
grid.vars.set_bool
grid.vars.get_int
grid.vars.set_int
grid.command
grid.log
```

Ils sont installés dans chaque `_ENV` script-local mais restent inertes hors d'un callback hébergé par le runtime.

## 12. Tests runtime

Suite :

```text
Grimrock.MON19.4.LuaBridge
```

Tests :

```text
EventContextAndVariables
CommandToExistingRuntime
LinkCondition
HostFailureIsProtected
SharedActionBudget
```

Ils couvrent :

- Event → Lua et contexte source/event ;
- lecture/écriture de variables via `GridLevelVariableStore` ;
- Lua → `grid.command` → primitive Logic existante ;
- condition de variable avant callback ;
- erreur de host retournée au script sans casser le VM ;
- arrêt d'une chaîne de plus de 128 actions avant mutation finale.

## 13. Test éditeur

Suite :

```text
Grimrock.MON19.4.Editor
```

Test :

```text
LuaLinkIdentity
```

Il vérifie que `ScriptId` et `CallbackName` participent à l'identité exacte persistante.

## 14. Édition visuelle hors périmètre

MON19.4 ne surcharge volontairement pas le panneau CONNECTORS.

Le runtime et le contrat persistant sont prêts, mais la création visuelle des bindings Lua reste prévue dans **MON19.6 — editor/validation scripting**.

À ce stade, `GridEditorLinkService` et `ValidateCurrentLevel()` restent centrés sur les ObjectCommands authorés visuellement. Ils seront étendus ensemble lors de MON19.6 afin de ne pas ajouter une UX partielle au panneau actuel.

## 15. Règle Git AGENTS

À la demande explicite du propriétaire du dépôt, `AGENTS.md` formalise désormais l'autorisation permanente suivante : un commit parasite/intermédiaire créé accidentellement par ChatGPT ou ses outils peut être retiré immédiatement de `master` par réécriture/force-push sans redemander une confirmation, après vérification qu'aucun commit utilisateur ou changement distant imprévu ne serait supprimé.

## 16. Hors périmètre

Restent à venir :

```text
éditeur de bindings ScriptId / CallbackName
validation complète des callbacks dans ValidateCurrentLevel
inspection/diagnostic script dans le Grid Editor
persistance de nouveaux types de valeurs éventuels
packaging de scripts joueurs
API gameplay supplémentaire strictement nécessaire
```
