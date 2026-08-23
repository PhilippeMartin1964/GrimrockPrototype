# MON19.7.1 — Lua Authoring API : variables persistantes et LogicId

Statut : **implémenté — validation UE5.5.4 en attente**  
Date : **23 août 2026**  
Référence de départ : `3ec89ce23872d7d13fdd79f81c4556e4bc9a4d70` (`Documenter la validation Automation MON19.7`)

## 1. Objectif

MON19.7.1 corrige deux problèmes d’authoring révélés pendant le premier puzzle Lua réel :

1. une variable utilisée par Lua devait auparavant être déclarée manuellement dans `UGridLevelAsset::LevelVariables` avant de pouvoir appeler `grid.vars.*` ;
2. `grid.command()` demandait le GUID `ObjectId` complet de la cible, ce qui rendait le script difficile à écrire et à relire.

Le but est de conserver l’architecture persistante et data-driven existante tout en donnant au créateur de niveau une syntaxe Lua directe et lisible.

## 2. Principes conservés

MON19.7.1 ne remplace aucun système existant :

- `ObjectId` reste l’identité interne autoritaire des objets ;
- `FGridLevelRuntimeState` reste l’état mutable du niveau ;
- `UGridLevelAsset::LevelVariables` reste le contrat typé persistant utilisé par les CONNECTORS, les nœuds Logic et le SaveGame ;
- SaveVersion reste inchangée ;
- le VM Lua n’est jamais sérialisé ;
- l’ancienne API `grid.vars.get_bool/set_bool/get_int/set_int` reste supportée ;
- l’ancien `grid.command("<GUID>", "Open")` reste supporté.

MON19.7.1 ajoute uniquement une couche d’authoring au-dessus de ces contrats.

## 3. Déclaration `persistent` dans Lua

Un script peut maintenant déclarer ses variables persistantes directement dans sa source :

```lua
persistent = {
    GateOpen = false,
    RuneCount = 0
}
```

Les types supportés restent volontairement ceux de MON19.2 :

- `Bool` ;
- `Int32`.

Une entrée d’un autre type est rejetée lors du chargement/validation du script.

Exemple rejeté :

```lua
persistent = {
    DoorName = "SecretDoor"
}
```

## 4. Synchronisation avec `LevelVariables`

`persistent` n’est pas un second système de persistance.

Lors de l’authoring, `GridEditorLuaService` construit les déclarations persistantes issues des scripts activés et les synchronise avec `UGridLevelAsset::LevelVariables`.

```text
Lua persistent.*
       │ authoring / validation
       v
UGridLevelAsset::LevelVariables
       │ runtime
       v
FGridLevelRuntimeState
       │ save/load
       v
SaveGame
```

### 4.1 Ajout et modification de script

`AddScript`, `SetScriptSource` et l’activation d’un script :

1. construisent une copie candidate des scripts ;
2. chargent les scripts activés dans un VM temporaire ;
3. extraient les déclarations `persistent` ;
4. vérifient les conflits ;
5. construisent une copie candidate des `LevelVariables` ;
6. ne modifient l’asset qu’après validation complète.

Une déclaration manquante est ajoutée automatiquement.

### 4.2 Déclaration existante

Si une `LevelVariable` du même nom existe déjà, son type et sa valeur par défaut doivent correspondre à la déclaration Lua.

Exemple accepté :

```text
Lua : GateOpen = false
Asset : GateOpen / Bool / false
```

Exemple rejeté :

```text
Lua : GateOpen = true
Asset : GateOpen / Bool / false
```

Le rejet laisse la source actuellement enregistrée inchangée.

### 4.3 Suppression d’une déclaration Lua

Retirer une entrée de `persistent` ne supprime jamais automatiquement la `LevelVariable` correspondante.

Cette règle évite de détruire silencieusement :

- une valeur utilisée par CONNECTORS ;
- une valeur utilisée par un nœud Logic ;
- un état déjà présent dans une sauvegarde ;
- une variable volontairement partagée entre Lua et la logique data-driven.

La suppression explicite d’une variable de niveau reste une opération d’édition séparée.

## 5. Synchronisation runtime de `persistent`

Avant chaque callback Lua hébergé :

1. les variables déclarées dans `persistent` sont lues depuis `FGridLevelRuntimeState` via `FGridLuaHostApi` ;
2. la table Lua reçoit les valeurs courantes ;
3. le callback s’exécute ;
4. après succès du callback, les valeurs réellement modifiées dans `persistent` sont réécrites vers le store typé.

Exemple :

```lua
persistent = {
    GateOpen = false,
    RuneCount = 0
}

function on_secret_button(event)
    persistent.GateOpen = true
    persistent.RuneCount = persistent.RuneCount + 1
end
```

La valeur initiale écrite dans le script sert de **valeur par défaut de déclaration**, pas de valeur réinitialisée à chaque callback.

Si `RuneCount` vaut déjà `7` dans le runtime, le callback voit `7`, puis écrit `8`.

## 6. Compatibilité avec `grid.vars.*`

La compatibilité MON19.4 est conservée.

Un ancien script reste valide :

```lua
function on_secret_button(event)
    local ok, err = grid.vars.set_bool("GateOpen", true)
    assert(ok, err)
end
```

Si un script possède également une table `persistent`, une entrée `persistent` qui n’a pas changé pendant le callback ne doit pas écraser une mutation directe effectuée via `grid.vars.*`.

## 7. Protection contre les fautes de frappe

La table `persistent` est fermée au niveau du contrat de callback.

Exemple :

```lua
persistent = {
    GateOpen = false
}

function on_secret_button(event)
    persistent.GateOepn = true
end
```

`GateOepn` n’était pas déclaré lors du chargement du script. Le callback est donc rejeté au commit de la table et aucune valeur autoritaire `GateOpen` n’est modifiée par cette faute de frappe.

Une entrée déclarée ne peut pas non plus changer de type pendant l’exécution.

## 8. `LogicId` lisible pour les objets

`FGridLevelObjectData` contient désormais :

```cpp
FName LogicId = NAME_None;
```

`ObjectId` reste inchangé et autoritaire. `LogicId` est un alias d’authoring humain optionnel.

Exemples valides :

```text
SecretDoor
CryptDoor01
BossGate
_exitLever
```

Le format est volontairement proche d’un identifiant de programmation :

```text
[A-Za-z_][A-Za-z0-9_]*
```

Un `LogicId` doit être unique dans le niveau.

## 9. Édition du `LogicId`

Le panneau **Grimrock Lua Scripts** expose, pour l’objet Grid actuellement sélectionné :

```text
Source: SecretDoor @ (28,22)
Logic Id   [ SecretDoor ]
```

Le champ reste disponible même si l’objet n’émet aucun événement. Une porte peut donc recevoir son `LogicId` sans devoir être une source de Lua binding.

La mutation :

- vérifie la syntaxe ;
- vérifie l’unicité ;
- utilise `Modify()` / `MarkPackageDirty()` ;
- reconstruit le preview du Grid Editor.

La validation générale détecte également les `LogicId` invalides ou dupliqués, y compris si une duplication d’objet ou une ancienne donnée a contourné l’éditeur Lua.

## 10. `grid.command()` par `LogicId`

Le nouveau code conseillé est :

```lua
function on_secret_button(event)
    local ok, err = grid.command("SecretDoor", "Open")
    assert(ok, err)
end
```

Résolution runtime :

```text
Target string
   │
   ├─ GUID valide ? ── yes ──> ObjectId historique
   │
   └─ no
       │
       v
   recherche LogicId dans UGridLevelAsset::Objects
       │
       ├─ 0 correspondance  -> rejet
       ├─ >1 correspondance -> rejet ambigu
       └─ 1 correspondance  -> ObjectId interne -> dispatcher existant
```

Ainsi `grid.command()` n’introduit pas une seconde voie de commande : après résolution, il fabrique toujours le même `FGridObjectLink` synthétique et passe par `ApplyLinkCommand()`.

## 11. Exemple complet recommandé

Pour une porte ayant :

```text
Logic Id = SecretDoor
```

le script peut être :

```lua
persistent = {
    GateOpen = false
}

function on_secret_button(event)
    persistent.GateOpen = true

    local ok, err = grid.command("SecretDoor", "Open")
    assert(ok, err)
end
```

Le bouton reste relié au callback par le système de Lua bindings MON19.6 :

```text
Secret Button.Activated
    -> Lua Script.on_secret_button
```

Aucun GUID n’est écrit à la main et `GateOpen` n’a pas besoin d’être précréé manuellement dans le DataAsset.

## 12. Fichiers modifiés

Runtime Lua :

- `Source/GrimrockLua/Public/GridLuaVm.h`
- `Source/GrimrockLua/Private/GridLuaVm.cpp`
- `Source/GrimrockLua/Private/Tests/GridLuaMON1971PersistentAuthoringTests.cpp`

Runtime gameplay :

- `Source/GrimrockPrototype/Public/Core/GridTypes.h`
- `Source/GrimrockPrototype/Private/Runtime/GridActivationComponent.cpp`
- `Source/GrimrockPrototype/Private/Tests/GridMON1971LuaLogicIdTests.cpp`

Editor :

- `Source/GrimrockPrototypeEditor/Public/EditorTools/GridEditorLuaService.h`
- `Source/GrimrockPrototypeEditor/Private/EditorTools/GridEditorLuaService.cpp`
- `Source/GrimrockPrototypeEditor/Private/EditorTools/Widgets/SGridEditorLuaScriptsPanel.cpp`
- `Source/GrimrockPrototypeEditor/Private/Tests/GridEditorMON1971LuaAuthoringTests.cpp`

Documentation :

- `docs/Design/MON19_7_1_LUA_AUTHORING_API.md`

Aucun `.uasset` / `.umap` n’est modifié.

## 13. Automation Tests à exécuter

### 13.1 Nouveau périmètre

```text
Grimrock.MON19.7.1.LuaAuthoring
    PersistentTable
    PersistentValidation
    LegacyGridVarsCompatibility
    LogicIdCommand

Grimrock.MON19.7.1.Editor
    PersistentSynchronization
    LogicIdAuthoring
```

Résultat attendu :

```text
6/6 Success
```

### 13.2 Non-régression Lua demandée

Après compilation UE5.5.4, exécuter également :

```text
Grimrock.MON19.3.Lua.Foundation
Grimrock.MON19.4.LuaBridge
Grimrock.MON19.5.LuaPersistence
Grimrock.MON19.6.Editor
Grimrock.MON19.7.LuaSandboxPackaging
```

Aucun résultat UE5.5.4 n’est considéré comme validé tant qu’il n’a pas été fourni depuis l’environnement utilisateur.

## 14. Validation visuelle demandée

Après compilation et Automation Tests :

1. sélectionner la porte secrète dans le Grid Editor ;
2. ouvrir **Window > Grimrock Lua Scripts** ;
3. cliquer `Refresh` ;
4. saisir `SecretDoor` dans **Logic Id** ;
5. sélectionner le `Secret Button` ;
6. appliquer le script suivant :

```lua
persistent = {
    GateOpen = false
}

function on_secret_button(event)
    persistent.GateOpen = true
    local ok, err = grid.command("SecretDoor", "Open")
    assert(ok, err)
end
```

7. conserver/créer le binding `Activated -> Script.on_secret_button` ;
8. lancer PIE ;
9. presser le bouton.

Résultat attendu :

- aucun message `Variable 'GateOpen' is not a declared Bool` ;
- callback Lua exécuté ;
- `SecretDoor` résolu sans GUID saisi dans le script ;
- porte ouverte ;
- événement source terminé avec `AnyApplied=true`.

## 15. Hors périmètre

MON19.7.1 ne cherche pas à :

- exposer des `UObject`, `AActor` ou `UWorld` au Lua ;
- permettre des variables Lua persistantes arbitraires de type table/string/userdata ;
- remplacer CONNECTORS ou les nœuds Logic ;
- supprimer automatiquement des `LevelVariables` anciennes ;
- sérialiser l’état interne du VM Lua ;
- remplacer l’identité interne `ObjectId`.

Le contrat reste volontairement petit, typé et data-driven.