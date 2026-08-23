# MON19.6 — Éditeur / validation Lua

Statut : **VALIDÉ sous Unreal Engine 5.5.4 — clos**  
Date : **23 août 2026**  
Référence de départ : `9e22108f66fb7479c56f332a6a33966aa8c28152` (`Valider MON19.5 persistance restauration Lua`)

## 1. Objectif

MON19.6 rend le scripting Lua utilisable par un level designer sans exposer `lua_State`, sans ajouter un second bus d'événements et sans alourdir davantage le panneau CONNECTORS.

Le contrat reste :

```text
FGridObjectLink
    SourceObjectId + SourceEvent
        -> Command = LuaCallback
        -> LuaScriptId + LuaCallbackName
```

Le runtime MON19.4 reste l'unique exécuteur.

## 2. Choix UX : onglet Lua dédié

Le panneau CONNECTORS est déjà dense. MON19.6 n'y ajoute donc pas deux nouveaux champs permanents.

Un onglet Nomad Editor dédié est enregistré :

```text
Window
    -> Grimrock Lua Scripts
```

Il affiche :

```text
GRIMROCK LUA — SCRIPTS & BINDINGS

LEVEL LUA SCRIPTS
    + Add Script
    Validate Lua
    [Enabled] ScriptId    N binding(s)

SELECTED SCRIPT
    ScriptId
    Source multiligne
    Apply / Revert / Remove
    callbacks globaux détectés

LUA BINDINGS — SELECTED GRID OBJECT
    Event
    Script
    Callback
    Condition
    Create Lua Binding
    bindings existants / Remove
```

Pour créer un binding, le level designer sélectionne d'abord l'objet source dans Grimrock Grid Editor puis rafraîchit l'onglet Lua.

L'intégration finale du menu utilise une seule entrée explicite `ToolMenus`, adossée à l'action native du tab spawner. La validation visuelle fournie sous UE5.5.4 confirme qu'une seule entrée `Grimrock Lua Scripts` est visible dans `Window`.

## 3. Édition des scripts

Service :

```text
GridEditorLuaService
```

Opérations :

```text
AddScript
RenameScript
SetScriptEnabled
SetScriptSource
RemoveScript
CountScriptReferences
```

Les mutations utilisent le `UGridLevelAsset` autoritaire et marquent son package dirty.

### 3.1 Rename atomique

Renommer un ScriptId met également à jour tous les :

```text
FGridObjectLink::LuaScriptId
```

qui le référencent.

### 3.2 Suppression / désactivation protégées

Un script référencé ne peut pas être supprimé ni désactivé depuis ce service.

Le level designer doit d'abord supprimer les bindings concernés.

Cela évite de créer volontairement des liens cassés depuis l'éditeur.

Le comptage final utilise un parcours explicite des bindings afin de retourner le nombre réel de références. Le test de régression couvre plusieurs bindings vers un même script.

## 4. Détection des callbacks

Les callbacks accessibles au bridge runtime sont des fonctions globales du `_ENV` du script.

MON19.6 détecte les formes d'authoring usuelles :

```lua
function on_trigger(event)
end
```

et :

```lua
on_trigger = function(event)
end
```

Les fonctions locales et membres de table ne sont pas proposées comme callbacks directs, car le contrat runtime reste :

```text
ScriptId + CallbackName global
```

La source est toujours compilée avec `FGridLuaVm` avant qu'un binding soit considéré supporté.

## 5. Bindings Lua targetless

Un binding Lua canonique est volontairement sans objet cible :

```text
SourceObjectId = valid
TargetObjectId = invalid
SourceEvent    = événement supporté
Command        = LuaCallback
LuaScriptId    = script activé
LuaCallbackName= callback global détecté
```

Le nouvel éditeur crée directement cette forme canonique.

Il ne crée aucun Actor, objet intermédiaire ou tableau parallèle de bindings.

Les liens restent stockés dans :

```text
UGridLevelAsset::Links
```

Le panneau CONNECTORS a été aligné sur ce contrat : un `LuaCallback` targetless est affiché comme `Lua Script.callback`, sans faux `Missing object` ni bouton `Go To Target`. Ce comportement a été validé visuellement sous UE5.5.4.

## 6. Conditions de binding Lua

Les conditions autorisées correspondent à ce que le runtime MON19.4 sait évaluer sans TargetActor :

```text
None
LevelVariableBoolEquals
LevelVariableIntCompare
```

Les conditions Receptacle ne sont pas proposées et sont rejetées par le service Lua.

Pour les variables, l'onglet filtre les choix selon le type :

```text
Bool  -> LevelVariableBoolEquals
Int32 -> LevelVariableIntCompare
```

Les six comparateurs MON19.2.3 sont réutilisés pour Int32.

## 7. Analyse Lua éditeur

`GridEditorLuaService::AnalyzeLevel()` vérifie :

```text
ScriptId non vide
ScriptId unique
compilation/init de chaque script activé
construction du VM complet du niveau
callbacks globaux détectables
```

Le bouton :

```text
Validate Lua
```

présente un résultat immédiat dans l'onglet Lua.

Cela ne remplace pas la validation générale du niveau.

## 8. Validation générale alignée

Le panneau existant :

```text
VALIDATION -> Refresh Validation
```

passe désormais par :

```text
GridEditorLuaService::ValidateCurrentLevelWithLua()
```

Le validateur historique est conservé, puis MON19.6 corrige uniquement les hypothèses devenues fausses et ajoute les diagnostics autoritaires Logic/Lua.

### 8.1 Logic data-only

Depuis MON19.2.3 :

```text
Type = Logic
ArchetypeId = None
```

est la forme correcte.

L'ancien message :

```text
Placed object has no ArchetypeId...
```

est donc supprimé uniquement pour `EGridLevelObjectType::Logic`.

Le nœud est ensuite validé par :

```text
GridLogicRuntime::ValidateNode
```

Un Logic possédant au contraire un ArchetypeId produit maintenant une erreur dédiée.

### 8.2 Lua targetless

L'ancien validateur exigeait un `TargetObjectId` pour tous les liens.

Pour :

```text
Command = LuaCallback
```

ce faux positif est retiré. La validation Lua vérifie ensuite explicitement :

```text
source/event valides
TargetObjectId absent
ScriptId présent
script existant et activé
script compilable
CallbackName présent
callback global détecté
condition autorisée et typée
```

### 8.3 Identité des liens Lua

Le validateur historique construit une clé de doublon antérieure à MON19.4 et n'y inclut pas `LuaScriptId` / `LuaCallbackName`.

MON19.6 filtre donc un faux doublon Lua lorsque :

```text
Puzzle.on_trigger
Puzzle.helper
```

partagent source/event/condition mais ne sont pas exactement équivalents selon :

```text
GridEditorLinkPolicy::AreLinksExactlyEquivalent
```

Un véritable doublon exact reste signalé.

## 9. Fichiers

Nouveaux :

```text
Source/GrimrockPrototypeEditor/Public/EditorTools/GridEditorLuaService.h
Source/GrimrockPrototypeEditor/Private/EditorTools/GridEditorLuaService.cpp
Source/GrimrockPrototypeEditor/Public/EditorTools/Widgets/SGridEditorLuaScriptsPanel.h
Source/GrimrockPrototypeEditor/Private/EditorTools/Widgets/SGridEditorLuaScriptsPanel.cpp
Source/GrimrockPrototypeEditor/Private/Tests/GridEditorMON196LuaEditorTests.cpp
Source/GrimrockPrototypeEditor/Private/Tests/GridEditorMON196ReferenceCountRegressionTest.cpp
docs/Design/MON19_6_LUA_EDITOR_VALIDATION.md
```

Modifiés :

```text
Source/GrimrockPrototypeEditor/GrimrockPrototypeEditor.Build.cs
Source/GrimrockPrototypeEditor/GrimrockPrototypeEditor.cpp
Source/GrimrockPrototypeEditor/Private/EditorTools/Widgets/SGridEditorValidationPanel.cpp
Source/GrimrockPrototypeEditor/Private/EditorTools/Widgets/SGridEditorLinksPanel.cpp
```

Le module éditeur dépend explicitement de `GrimrockLua` ; le runtime `GrimrockPrototype` n'acquiert aucune nouvelle dépendance.

## 10. Tests MON19.6

Suite :

```text
Grimrock.MON19.6.Editor
```

Tests finaux :

```text
Grimrock.MON19.6.Editor.LuaScriptAnalysis
Grimrock.MON19.6.Editor.LuaBindingAndScriptMutations
Grimrock.MON19.6.Editor.LuaReferenceCountRegression
Grimrock.MON19.6.Editor.ValidationAlignment
```

Ils couvrent :

- compilation/analyse de scripts ;
- détection de callbacks globaux ;
- rejet d'une source syntaxiquement invalide ;
- création targetless ;
- condition Bool typée ;
- rejet d'une condition Receptacle sur Lua ;
- rename ScriptId + mise à jour des bindings ;
- protection de disable/remove lorsqu'un script est référencé ;
- comptage de plusieurs références vers un même script ;
- suppression de binding ;
- correction du faux `TargetObjectId` ;
- correction du faux doublon entre callbacks distincts ;
- validation d'un Logic Relay data-only sans ArchetypeId.

## 11. Validation UE5.5.4

Résultat final fourni depuis Unreal Engine 5.5.4 :

```text
LuaBindingAndScriptMutations  -> Success
LuaReferenceCountRegression   -> Success
LuaScriptAnalysis             -> Success
ValidationAlignment           -> Success
```

Soit :

```text
Grimrock.MON19.6.Editor = 4/4 Success
```

Validation visuelle également confirmée :

```text
Window
    -> une seule entrée Grimrock Lua Scripts

CONNECTORS
    -> Lua Script.on_secret_button
    -> aucun faux Missing object
    -> aucun Go To Target pour un LuaCallback targetless
```

Les non-régressions MON19.4 / MON19.3 / MON19.2 demandées pendant le jalon avaient également été fournies avec succès avant les derniers correctifs purement UI.

MON19.6 est donc **VALIDÉ et clos**.

## 12. Hors périmètre

MON19.6 n'ajoute pas :

```text
nouveau SaveVersion
snapshot lua_State
require / filesystem
packaging de scripts joueurs
hot reload automatique des sources pendant PIE
éditeur syntaxique avancé avec coloration Lua
intellisense Lua
nouveau bus d'événements
nouveau tableau de bindings
modification .uasset/.umap
```

Les contraintes de sandbox/package sont traitées par MON19.7.
