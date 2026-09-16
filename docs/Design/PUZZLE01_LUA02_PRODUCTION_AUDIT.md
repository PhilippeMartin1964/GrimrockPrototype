# PUZZLE01-LUA02 — Audit et complétion runtime de la porte du Gardien

Date : 16 septembre 2026  
Baseline initiale : `cf513efdda81ff3c258fea546dbfab0277489e0b`

## Objectif

PUZZLE01-LUA02 ne modifie **aucune ligne** du script Lua de production.

Le ticket établit deux choses distinctes :

```text
AUTHORING
DA_GridLevel_00
  puzzle1_lvl1
      |
      +-- binding ItemInserted -> LuaCallback valide ?
      +-- GuardianDoor existe et est unique ?
      +-- GuardianDoor est bien une Door ?
      +-- Compile Lua accepte le script tel quel ?
      +-- le script référence réellement GuardianDoor ?

RUNTIME
1re gemme -> puzzle1_lvl1 -> porte reste fermée
2e gemme -> puzzle1_lvl1 -> grid.command("GuardianDoor", ...)
                         -> ApplyLinkCommand
                         -> OpenDoorOnEdge
                         -> AGridDoorActor::OpenDoor
                         -> animation de porte démarre
                         -> état ouvert engagé à l'endpoint
                         -> passage réellement ouvert
```

Cette séquence respecte la règle d'architecture :

```text
SCRIPT LUA  = intention gameplay
COMPILATEUR = validation de l'authoring
RUNTIME     = exécution + gardes internes
```

Aucun `must(...)`, `assert(...)`, wrapper `(ok, err)` ou réécriture du Lua n'est introduit par ce ticket.

## 1. Audit authoring de production

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

### Validation de GuardianDoor

Le test exige qu'un et un seul objet du niveau possède :

```text
LogicId = GuardianDoor
```

et que cet objet soit de type :

```text
Door
```

Cette vérification porte sur les données de production, pas sur un fixture inventé.

### Le compilateur comme preuve du contenu du script

Le test n'utilise volontairement pas :

```cpp
Source.Contains("GuardianDoor")
```

Une recherche de chaîne serait fragile : espaces, guillemets, commentaires ou formatage pourraient produire de faux résultats.

À la place, le test utilise `FGridLuaAuthoringCompiler` comme autorité.

Une copie transitoire de `DA_GridLevel_00` conserve uniquement `puzzle1_lvl1` et ses bindings Lua, tout en gardant les vraies données du niveau. Le compilateur doit l'accepter sans diagnostic.

Une seconde copie transitoire retire uniquement :

```text
GuardianDoor.LogicId
```

Le script Lua reste strictement inchangé. Le compilateur doit alors produire :

```text
E201 unknown LogicId 'GuardianDoor'
```

Cela prouve sémantiquement que le vrai script de production référence `GuardianDoor` par un appel statiquement compilable.

## 2. Complétion runtime GuardianDoor

Filtre ajouté :

```text
Grimrock.PUZZLE01.LUA02.RuntimeGuardianDoorCompletion
```

Le test runtime charge directement le vrai :

```text
DA_GridLevel_00
puzzle1_lvl1
binding ItemInserted -> LuaCallback
Guardian LogicId de production
GuardianDoor LogicId / InstanceId / Type de production
```

Il ne copie ni ne réécrit le source Lua.

Pour garder le test déterministe et indépendant de la présentation complète de la map, la géométrie est normalisée dans un petit niveau transitoire de deux cellules. Les identités de production utiles au contrat Lua restent celles du vrai niveau ; la porte runtime est une vraie `AGridDoorActor` enregistrée dans le vrai `UGridDoorSystemComponent`.

Le Gardien transitoire utilise le runtime générique `AGridReceptacleActor` afin que les commandes déjà présentes dans `puzzle1_lvl1` s'exécutent réellement : consommation de la gemme, changement des yeux, désactivation d'insertion puis commande de porte.

### Contrat vérifié

Le test impose :

1. `GuardianDoor` commence complètement fermée ;
2. la première gemme exécute le callback de production ;
3. `GuardianGemCount` devient `1` ;
4. la première gemme ne démarre aucune animation de porte ;
5. la seconde gemme exécute le même callback de production ;
6. `GuardianGemCount` devient `2` ;
7. la seconde gemme est consommée ;
8. le Gardien refuse toute insertion supplémentaire ;
9. `GuardianDoor.IsAnimating()` devient vrai immédiatement après la seconde gemme ;
10. `GuardianDoor.bIsOpen` reste faux pendant l'animation, car il représente l'état atteint à l'endpoint et non l'intention d'ouverture ;
11. le passage reste bloqué pendant l'animation ;
12. après la durée de mouvement, `GuardianDoor.bIsOpen` devient vrai et `GuardianDoor.IsFullyOpen()` devient vrai ;
13. `Runtime->IsDoorOpenOnEdge(...)` confirme alors que le passage est réellement ouvert.

Les points 9 à 13 valident le chemin runtime réel :

```text
production puzzle1_lvl1
    -> grid.command("GuardianDoor", ...)
    -> UGridActivationComponent::ExecuteLuaIssuedCommand
    -> ApplyLinkCommand
    -> AGridLevelRuntimeActor::OpenDoorOnEdge
    -> UGridDoorSystemComponent::OpenDoorOnEdge
    -> AGridDoorActor::OpenDoor
    -> AGridDoorActor::SetDoorOpenState(true)
    -> animation
    -> UpdateAnimation atteint l'endpoint
    -> bIsOpen = true
```

Le test ne se contente donc pas de vérifier la présence textuelle de `GuardianDoor` ou d'une commande dans le script : il exige le démarrage de l'animation puis l'état ouvert réel de la vraie classe de porte runtime.

## 3. Aucune mutation de production

Les deux tests ne sauvegardent rien et ne modifient aucun asset versionné.

Ils ne touchent :

- ni `DA_GridLevel_00.uasset` ;
- ni le source de `puzzle1_lvl1` ;
- ni les LogicId réels ;
- ni les Blueprints de production.

Les adaptations nécessaires au test runtime sont uniquement transitoires en mémoire.

## 4. Validation locale

Commande unique :

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.PUZZLE01.LUA02"
```

Résultat attendu après ce ticket :

```text
ProductionAuthoringAudit          Succeeded
RuntimeGuardianDoorCompletion     Succeeded
Failed                            0
```

PUZZLE01-LUA02 est **code-complete** après ajout du test runtime, mais reste à clôturer seulement après validation UE5.5.4 locale verte de ces deux tests.
