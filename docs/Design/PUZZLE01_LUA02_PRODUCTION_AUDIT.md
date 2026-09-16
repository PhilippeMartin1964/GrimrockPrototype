# PUZZLE01-LUA02 — Audit du contrat de porte du Gardien

Date : 16 septembre 2026  
Baseline : `cf513efdda81ff3c258fea546dbfab0277489e0b`

## Objectif

PUZZLE01-LUA02 ne modifie **aucune ligne** du script Lua de production.

Le ticket doit d'abord établir ce que le niveau de production contient réellement :

```text
DA_GridLevel_00
  puzzle1_lvl1
      |
      +-- binding ItemInserted -> LuaCallback valide ?
      +-- GuardianDoor existe et est unique ?
      +-- GuardianDoor est bien une Door ?
      +-- Compile Lua accepte le script tel quel ?
      +-- le script référence réellement GuardianDoor ?
```

Cette séquence respecte la règle d'architecture :

```text
SCRIPT LUA  = intention gameplay
COMPILATEUR = validation de l'authoring
RUNTIME     = exécution + gardes internes
```

Aucun `must(...)`, `assert(...)`, wrapper `(ok, err)` ou réécriture du Lua n'est introduit par ce ticket.

## Test ajouté

Filtre :

```text
Grimrock.PUZZLE01.LUA02.ProductionAuthoringAudit
```

Le test charge directement :

```text
/Game/GrimrockPrototype/Core/DataAssets/GrimrockLevels/DA_GridLevel_00
/Game/GrimrockPrototype/Core/DataAssets/DA_ObjectPalette_Default
```

Il récupère ensuite le vrai script :

```text
ScriptId = puzzle1_lvl1
```

et son binding de production :

```text
ItemInserted -> LuaCallback
```

## Validation de GuardianDoor

Le test exige qu'un et un seul objet du niveau possède :

```text
LogicId = GuardianDoor
```

et que cet objet soit de type :

```text
Door
```

Cette vérification porte sur les données de production, pas sur un fixture inventé.

## Le compilateur comme preuve du contenu du script

Le test n'utilise volontairement pas :

```cpp
Source.Contains("GuardianDoor")
```

Une recherche de chaîne serait fragile : espaces, guillemets, commentaires ou formatage pourraient produire de faux résultats.

À la place, le test utilise `FGridLuaAuthoringCompiler` comme autorité.

### Étape A — compilation normale

Une copie transitoire de `DA_GridLevel_00` conserve uniquement `puzzle1_lvl1` et ses bindings Lua, tout en gardant les vraies données du niveau.

Le compilateur doit accepter cette copie sans diagnostic.

Cela valide notamment :

- la syntaxe Lua ;
- les variables `persistent` ;
- les callbacks liés ;
- les `LogicId` utilisés ;
- les commandes ;
- les material slots ;
- les material aliases.

### Étape B — preuve de dépendance à GuardianDoor

Une seconde copie transitoire retire uniquement :

```text
GuardianDoor.LogicId
```

Le script Lua reste **strictement inchangé**.

Le compilateur doit alors produire :

```text
E201 unknown LogicId 'GuardianDoor'
```

pour :

```text
ScriptId = puzzle1_lvl1
```

Si ce diagnostic apparaît, cela prouve sémantiquement que le vrai script de production référence déjà `GuardianDoor` par un appel statiquement compilable.

Si la compilation reste valide après retrait du `LogicId`, cela prouve au contraire que `puzzle1_lvl1` ne dépend pas actuellement de `GuardianDoor` et que la complétion fonctionnelle du puzzle n'est pas encore exprimée dans ce script.

## Aucune mutation de production

Le test ne sauvegarde rien et ne modifie aucun asset versionné.

Toutes les altérations utilisées pour l'audit sont réalisées sur des `DuplicateObject<UGridLevelAsset>` transitoires en mémoire.

Donc le ticket ne touche :

- ni `DA_GridLevel_00.uasset` ;
- ni `puzzle1_lvl1` ;
- ni les LogicId réels ;
- ni le runtime du jeu.

## Lecture du résultat

### Test vert

Un résultat vert signifie simultanément :

1. `puzzle1_lvl1` existe et est activé ;
2. son binding `ItemInserted -> LuaCallback` existe ;
3. `GuardianDoor` existe une seule fois ;
4. `GuardianDoor` est une porte ;
5. le compilateur accepte le script réel contre les données réelles ;
6. le compilateur prouve que `puzzle1_lvl1` référence réellement `GuardianDoor`.

Dans ce cas, la prochaine étape de PUZZLE01-LUA02 est uniquement la validation runtime : vérifier que la seconde gemme provoque effectivement l'ouverture de la porte par le chemin réel `grid.command -> ApplyLinkCommand -> OpenDoorOnEdge -> AGridDoorActor::OpenDoor`.

### Test rouge

Le diagnostic devient l'autorité pour la suite :

- `GuardianDoor` absent ou ambigu : correction de donnée / LogicId ;
- mauvais type de cible : correction de donnée ;
- `E201`/`E203`/`E204` sur la compilation normale : correction du contrat d'authoring ou du compilateur selon le cas ;
- absence de `E201` après retrait transitoire de `GuardianDoor` : le script ne référence pas actuellement cette porte.

Aucune correction ne doit être inventée avant d'avoir ce résultat.

## Validation locale

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.PUZZLE01.LUA02"
```

PUZZLE01-LUA02 reste ouvert tant que cette validation locale n'a pas établi le contrat réel de production.
