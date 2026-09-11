# LUA-UX03 — Purge des identités et conditions de lien héritées

## Objectif

LUA-UX03 termine le recentrage de l'authoring des énigmes autour du contrat cible du Grid Editor :

- `LogicId` est l'identité logique lisible et stable utilisée par Lua ;
- les connecteurs natifs restent de simples liaisons **événement -> commande** ;
- les conditions de puzzle générales sont exprimées dans Lua ;
- les conditions natives propres aux réceptacles restent disponibles lorsqu'elles décrivent directement l'état du réceptacle.

Cette étape retire donc de l'authoring actif deux mécanismes historiques qui faisaient double emploi : le `Tag` générique des placements et les conditions de connecteur fondées sur les `LevelVariables`.

## 1. Identité des objets

### Contrat cible

Pour adresser un objet depuis Lua, on utilise :

```text
LogicId
```

Exemple :

```lua
local ok, err = grid.command("GuardianDoor", "Open")
assert(ok, err)
```

Le `LogicId` doit rester unique dans le niveau et respecter le format :

```text
[A-Za-z_][A-Za-z0-9_]*
```

### Suppression du Tag de placement

Le champ `Tag` n'est plus une propriété Unreal des structures de placement suivantes :

```text
FGridWorldObjectInstance
FGridLooseItemInstance
FGridMonsterSpawnInstance
FGridItemSpawnInstance
FGridLogicObjectInstance
```

Il n'est donc plus :

- exposé dans l'authoring ;
- sérialisé dans le LevelAsset ;
- copié lors du placement ;
- modifié par l'inspecteur Selected Object.

`DefaultTag` est également retiré de `UGridWorldObjectDefinitionAsset`.

Pour éviter une migration C++ inutilement brutale dans cette étape, un membre source-only `Tag` peut subsister temporairement dans certaines structures. Il n'est **pas** un `UPROPERTY`, n'est pas sauvegardé et ne fait plus partie du contrat de données. De la même manière, l'ancienne fonction d'édition `SetSelectedObjectTag` reste provisoirement disponible comme pont de compatibilité source mais refuse toute mutation.

Les `ItemTag` appartenant aux définitions d'items ne sont pas concernés : ils servent à classifier les items et restent utiles aux règles de réceptacle et d'inventaire.

## 2. Connecteurs : suppression des conditions LevelVariable

Les conditions historiques suivantes ne sont plus proposées ni exécutées comme conditions de connecteur :

```text
LevelVariableBoolEquals
LevelVariableIntCompare
```

Les payloads associés ne font plus partie des propriétés sérialisées de `FGridObjectLink` :

```text
ConditionVariableId
ConditionBoolValue
ConditionIntComparison
ConditionIntValue
```

Les anciennes valeurs d'enum sont conservées uniquement comme tombstones `Hidden` afin de préserver les ordinaux historiques et de rendre la migration d'anciens assets déterministe. Elles ne constituent plus une API d'authoring ni une fonctionnalité runtime.

## 3. Lua devient l'autorité des conditions de puzzle

Une liaison Lua est volontairement inconditionnelle côté connecteur :

```text
SourceObjectId + SourceEvent
    -> LuaCallback
    -> ScriptId + CallbackName
```

La condition se trouve dans le script :

```lua
persistent = {
    GuardianGemCount = 0
}

function on_gem_inserted(event)
    if persistent.GuardianGemCount >= 2 then
        return
    end

    -- règle de puzzle ici
end
```

Cela évite de répartir la même règle entre l'UI des connecteurs, les LevelVariables et le script Lua.

Les `LevelVariables` ne sont **pas supprimées**. Elles restent l'infrastructure typée utilisée notamment par :

- `persistent` Lua ;
- la persistance SaveGame ;
- le runtime de logique générique existant.

Ce qui disparaît est uniquement leur rôle de condition générale directement attachée à un connecteur.

## 4. Conditions natives conservées

Les conditions intrinsèques à un réceptacle restent supportées :

```text
ReceptacleIsEmpty
ReceptacleHasAnyItem
ReceptacleContainsItemDefinition
ReceptacleContainsItemTag
ReceptacleContainsItemType
ReceptacleItemCountAtLeast
ReceptacleWeightAtLeast
```

Elles décrivent directement l'état du composant cible et ne constituent pas un mini-langage de puzzle parallèle à Lua.

Pour les autres cibles, le connecteur standard expose désormais uniquement :

```text
Condition = None
```

## 5. Migration des niveaux existants

Après mise à jour du code :

1. ouvrir les LevelAssets concernés ;
2. remplacer toute logique dépendant d'un ancien `Tag` d'objet par un `LogicId` explicite ;
3. déplacer toute condition `LevelVariableBoolEquals` / `LevelVariableIntCompare` dans le callback Lua correspondant ;
4. lancer la validation du Grid Editor ;
5. sauvegarder les assets migrés.

Les anciennes données `Tag` et les anciens payloads de condition ne sont plus des propriétés sérialisées actives. Un ancien enum de condition peut encore être lu grâce au tombstone ; la validation doit alors conduire à remplacer cette ancienne liaison plutôt qu'à la conserver silencieusement.

## 6. Validation automatisée

Test dédié :

```text
Grimrock.LUAUX03.LegacyAuthoringPurge
```

Il vérifie notamment :

- l'absence de propriété réfléchie/sérialisée `Tag` sur les cinq structures de placement ;
- l'absence de `DefaultTag` sur la définition d'objet ;
- l'absence des quatre payloads LevelVariable dans les propriétés sérialisées de `FGridObjectLink` ;
- l'absence des deux anciennes conditions dans les choix de connecteur ;
- le maintien des conditions natives de réceptacle ;
- le refus d'une condition de puzzle placée sur un binding Lua.

Validation locale recommandée :

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.LUAUX03"
```

Puis, pour vérifier les contrats MON19 impactés :

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.MON19"
```
