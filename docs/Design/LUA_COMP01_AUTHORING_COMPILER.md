# LUA-COMP01 — Grimrock Lua Authoring Compiler

Statut : **implémenté, validation UE5.5.4 utilisateur requise**  
Date : **11 septembre 2026**  
Baseline : `4086e137549aa7fef1c82df693dc96ae44d7995f`

Référence syntaxique canonique : `docs/Design/LUA_SCRIPTING_LANGUAGE_REFERENCE.md`.

## Objectif

LUA-COMP01 déplace la responsabilité de validation du level designer vers l'outil d'authoring.

Le script cible reste du gameplay lisible :

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

Aucun `must()`, `assert(grid.command(...))` ou traitement manuel de `(ok, err)` n'est requis par la convention d'authoring.

## Chaîne de compilation

```text
source Lua
   |
   +-- PASS 1 : FGridLuaVm / Lua 5.4.8
   |      syntaxe Lua réelle
   |      initialisation sandboxée
   |      persistent Bool/Int32
   |      conflits de déclarations
   |
   +-- PASS 2 : FGridLuaAuthoringCompiler
          persistent.*
          grid.command(...)
          grid.visual.set_material(...)
          API grid.* connue
          bindings ScriptId + CallbackName
          symboles du UGridLevelAsset
```

Le compilateur n'introduit aucune table parallèle de commandes. Les autorités restent :

```text
LogicId        -> UGridLevelAsset
Command        -> EGridObjectCommand
Authoring      -> GridEditorLinkPolicy::GetSupportedCommandsForTarget
Definition     -> AGridLevelEditorActor::FindWorldObjectDefinitionById
Material alias -> UGridWorldObjectDefinitionAsset::RuntimeMaterialAliases
Material slot  -> UStaticMesh::StaticMaterials
```

## `grid.command`

Syntaxe statiquement compilable :

```lua
grid.command("GuardianDoor", "Open")
```

Le compilateur exige exactement deux chaînes littérales simples.

Il vérifie :

- syntaxe du `LogicId` : `[A-Za-z_][A-Za-z0-9_]*` ;
- résolution vers exactement un objet placé ;
- existence de la valeur `EGridObjectCommand` ;
- interdiction de `LuaCallback` ;
- support de la commande dans le contrat d'authoring du type de cible.

Les formes dynamiques sont refusées :

```lua
local target = "GuardianDoor"
grid.command(target, "Open")
```

Le runtime continue ensuite à exécuter `grid.command` via son chemin existant : résolution `LogicId -> ObjectId`, création d'un `FGridObjectLink` synthétique, puis `ApplyLinkCommand()`.

## `grid.visual.set_material`

Syntaxe :

```lua
grid.visual.set_material("Guardian", "EyesLeft", "BlueGem")
```

Le compilateur vérifie avant PlayTest :

- trois chaînes littérales ;
- cible unique ;
- placement world-object ;
- définition visuelle disponible ;
- mesh statique principal disponible ;
- slot `EyesLeft` réellement présent sur le mesh ;
- alias `BlueGem` réellement déclaré et non nul.

## `persistent`

`FGridLuaVm` reste l'autorité de déclaration :

```lua
persistent = {
    GateOpen = false,
    RuneCount = 0
}
```

LUA-COMP01 ajoute la vérification statique des accès :

```lua
persistent.Unknown = 1
```

est rejeté si `Unknown` n'est pas déclaré.

L'indexation dynamique est interdite dans le profil d'authoring :

```lua
persistent[name] = 1
```

Il faut utiliser un membre statique :

```lua
persistent.RuneCount = persistent.RuneCount + 1
```

Les changements de type littéraux évidents sont également refusés avant runtime.

## Diagnostics

Format :

```text
LUA-COMPILE <code> [ScriptId] line <ligne>:<colonne>: <message>
```

Principaux codes :

```text
E001  erreur Lua / déclaration de script
E100  indexation dynamique de persistent
E101  variable persistent non déclarée
E102  changement de type persistent évident
E103  conflit persistent / LevelVariable ou entre scripts
E200  signature statique invalide de grid.command / LogicId invalide
E201  LogicId inconnu
E202  LogicId ambigu
E203  commande inconnue
E204  commande non supportée par la cible
E205  tentative d'appel direct de LuaCallback
E300  signature invalide de grid.visual.set_material
E301  définition visuelle/mesh absent
E302  slot matériau inconnu
E303  alias matériau inconnu
E400  API grid.* inconnue
E501  binding vers script absent/désactivé
E502  callback lié absent
E503  source du binding absente
E504  condition de connecteur sur un binding Lua
E900  erreur de contexte d'authoring
```

## UX

Le bouton :

```text
Validate Lua
```

devient :

```text
Compile Lua
```

Il compile le jeu de scripts activés et affiche les diagnostics `LUA-COMPILE`.

`Apply` compile d'abord une copie candidate contenant :

- le nouveau `ScriptId` ;
- la nouvelle source ;
- les bindings Lua renommés en mémoire.

Aucune source n'est modifiée dans le `LevelAsset` si cette compilation candidate échoue.

Après succès, le chemin existant `RenameScript` / `SetScriptSource` applique la modification et synchronise `persistent` vers `LevelVariables`.

## Runtime

LUA-COMP01 ne retire aucune garde de sûreté du runtime.

La règle est :

```text
COMPILATEUR = erreurs d'authoring
RUNTIME     = exécution + intégrité moteur
SCRIPT LUA  = gameplay uniquement
```

Les retours `(ok, err)` de l'API C restent disponibles techniquement, mais ne constituent plus la convention d'écriture des scripts de niveau.

## Tests

Nouveau filtre :

```text
Grimrock.LUACOMP01
```

Cas couverts :

- script Guardian canonique accepté ;
- `++` rejeté par Lua 5.4 ;
- `persistent.Unknown` rejeté ;
- changement de type littéral rejeté ;
- LogicId inconnu ;
- commande inconnue ;
- commande incompatible avec la cible ;
- cible dynamique de `grid.command` rejetée ;
- slot matériau inconnu ;
- alias matériau inconnu ;
- callback lié absent ;
- compilation d'un draft invalide sans mutation de la source sauvegardée.

Validation locale recommandée :

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.LUACOMP01"
```

Puis non-régression :

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.MON19"
```
