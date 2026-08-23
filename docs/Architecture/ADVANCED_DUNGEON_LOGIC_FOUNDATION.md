# Event, variables, Logic et Lua — Fondation d’architecture

## Autorités

- `UGridLevelAsset` porte objets, liens, variables et scripts de niveau.
- `UGridActivationComponent` reste le dispatcher Event → Command.
- `GridLevelVariableStore` porte la lecture/écriture typée de l’état vivant.
- `GridLogicRuntime` exécute les nœuds Logic.
- `GrimrockLua` héberge Lua 5.4 et `FGridLuaVm`.

## Règle fondamentale

Lua et Logic **n’introduisent pas une deuxième voie d’effet gameplay**. Un effet sur une porte, un spawn, un mécanisme ou un encounter continue à passer par les commandes existantes.

```text
Event -> Command
Event -> Logic -> Event -> Command
Event -> Lua -> grid.command(...) -> Command
```

## Variables

Les variables de niveau supportées sont `Bool` et `Int32`. Elles possèdent une définition initiale dans le LevelAsset et une valeur vivante dans `FGridLevelRuntimeState`; elles sont sauvegardées avec le dungeon runtime state. Les liens peuvent tester ces variables avant d’appliquer une commande.

## Logic nodes

Le contrat courant inclut `Relay`, `SetBool`, `ToggleBool`, `SetInt`, `AddInt`, `SubtractInt`, `ResetVariable`, `CompareBool`, `CompareInt` et `Latch`. Les sorties sont réémises dans le même graphe via `Activated`/`Deactivated`.

## Lua

Le module `GrimrockLua` est isolé du module Editor et du gameplay Unreal lourd. La VM est sandboxée et partage le budget d’actions avec Event/Command afin qu’un script ne contourne pas les protections de cycle.

### `persistent`

Une table déclarative :

```lua
persistent = {
    GateOpen = false,
    RuneCount = 0
}
```

est synchronisée par l’authoring Editor avec les `LevelVariables`. Au runtime, la valeur vivante est injectée avant callback et seules les modifications valides sont engagées après succès.

### `LogicId`

`LogicId` est un alias optionnel `[A-Za-z_][A-Za-z0-9_]*`, unique dans le niveau. `ObjectId` reste l’identité persistante. `grid.command("SecretDoor", "Open")` résout d’abord l’alias vers l’ObjectId puis utilise le dispatcher normal.

## Authoring Editor

`GridEditorLuaService` valide le code dans une VM temporaire avant de muter le DataAsset, détecte les callbacks et synchronise les déclarations persistantes. Le Grid Editor expose également `LogicId`.

## Règle d’usage

- énigme simple : lien direct ;
- compteur/comparaison : variables + Logic ;
- orchestration réellement complexe : Lua ;
- sortie Lua : commandes runtime normales.

## Validation de référence

MON19 est clos : `Grimrock.MON19` 55/55, `MON19.8` 4/4 et PIE représentatif validé.
