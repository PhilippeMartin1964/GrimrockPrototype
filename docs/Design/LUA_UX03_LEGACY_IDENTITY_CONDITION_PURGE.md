# LUA-UX03 — Purge dure des identités et conditions héritées

## Objectif

LUA-UX03 fixe le contrat cible du Grid Editor :

- `LogicId` est l'identité logique lisible utilisée par Lua et les événements ;
- un connecteur natif simple reste une liaison **événement -> commande** ;
- les conditions, compteurs, séquences et combinaisons de puzzle sont écrits dans Lua ;
- seules les conditions intrinsèques de réceptacle restent dans le moteur de connecteurs.

## 1. Identité des objets

`LogicId` est l'identité logique de l'instance. Il doit être unique dans le niveau et respecter :

```text
[A-Za-z_][A-Za-z0-9_]*
```

Exemple :

```lua
local ok, err = grid.command("GuardianDoor", "Open")
assert(ok, err)
```

Le `Tag` générique de placement n'est plus une propriété Unreal sérialisée ni un champ d'authoring. Les tags appartenant aux **définitions d'items** sont une notion différente et restent disponibles pour classifier les items.

## 2. Suppression physique des conditions LevelVariable de connecteur

Les deux anciennes conditions générales de connecteur :

```text
LevelVariableBoolEquals
LevelVariableIntCompare
```

sont supprimées de `EGridObjectCondition`. Elles ne sont ni cachées, ni dépréciées, ni conservées comme tombstones.

Leurs anciens paramètres sont également supprimés de `FGridObjectLink` :

```text
ConditionVariableId
ConditionBoolValue
ConditionIntComparison
ConditionIntValue
```

Il n'existe donc plus de chemin runtime, de chemin éditeur ou de contrat de données permettant d'utiliser ce mini-langage de conditions.

## 3. Lua est l'autorité des conditions de puzzle

Une liaison Lua est inconditionnelle côté connecteur :

```text
SourceObjectId + SourceEvent
    -> LuaCallback
    -> ScriptId + CallbackName
```

Toute règle conditionnelle est dans le script :

```lua
persistent = {
    GuardianGemCount = 0
}

function on_gem_inserted(event)
    if persistent.GuardianGemCount >= 2 then
        return
    end

    -- règle de puzzle
end
```

Les `LevelVariables` restent une infrastructure interne typée pour `persistent`, la persistance SaveGame et les primitives génériques qui en ont besoin. Elles ne sont plus un langage d'authoring de conditions de connecteur.

## 4. Conditions natives conservées

Les seules conditions natives de connecteur sont celles qui décrivent directement l'état d'un réceptacle :

```text
ReceptacleIsEmpty
ReceptacleHasAnyItem
ReceptacleContainsItemDefinition
ReceptacleContainsItemTag
ReceptacleContainsItemType
ReceptacleItemCountAtLeast
ReceptacleWeightAtLeast
```

Pour les autres cibles :

```text
Condition = None
```

Règle d'architecture :

```text
Event -> Command                        : connecteur natif
Event -> condition/compteur/séquence   : Lua
```

## 5. Tests

Le filtre dédié est :

```text
Grimrock.LUAUX03
```

Le test `Grimrock.LUAUX03.HardPurge` vérifie le contrat actuel :

- les cibles génériques n'exposent que `Condition=None` ;
- les réceptacles exposent uniquement leurs prédicats natifs ;
- un binding Lua refuse toute condition côté connecteur ;
- les règles de puzzle doivent être placées dans le callback Lua.

Validation locale :

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.LUAUX03"
```

Puis :

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.MON19"
```
