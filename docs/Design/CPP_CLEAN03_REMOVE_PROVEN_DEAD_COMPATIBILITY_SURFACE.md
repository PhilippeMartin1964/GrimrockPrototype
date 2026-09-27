# CPP-CLEAN03 — Remove Proven Dead Compatibility Surface

Date : **27 septembre 2026**  
Parent : **CPP-AUDIT01 — Audit général du code C++**

## Objectif

Supprimer deux surfaces de compatibilité dont l'absence d'autorité runtime est désormais prouvée :

- `EGridReceptacleRejectReason::ExplicitlyRejected` ;
- `FGridMonsterPerception::CanHear()`.

Le ticket ne retire aucune API `BlueprintCallable` uniquement parce qu'elle n'a pas d'appelant C++. `AGrimrockPartyPawn::StartNewGame()` et `ShowInventoryWidget()` restent donc hors périmètre tant qu'un audit de références Blueprint dédié n'a pas démontré leur absence.

## 1. Receptacle reject reason

Avant CPP-CLEAN03 :

~~~text
None                     = 0
InvalidItem              = 1
Full                     = 2
ExplicitlyRejected       = 3   <- hidden legacy value, no producer
NoMatchingAcceptanceRule = 4
InsertionDisabled        = 5
~~~

`EvaluateItemAcceptance()` ne produisait jamais `ExplicitlyRejected`. La valeur ne survivait que dans deux switches de diagnostic.

Après CPP-CLEAN03 :

~~~text
None                     = 0
InvalidItem              = 1
Full                     = 2
NoMatchingAcceptanceRule = 4
InsertionDisabled        = 5
~~~

Les valeurs 4 et 5 restent explicites : **aucune renumérotation du contrat courant** n'est introduite.

Les cases mortes sont également supprimées de :

- `AGridReceptacleActor` ;
- `AGrimrockPlayerController`.

### Audit Blueprint

Comme `EGridReceptacleRejectReason` est un `UENUM(BlueprintType)`, le ticket ajoute :

~~~text
Grimrock.CppCleanup.CPP_CLEAN03.BlueprintLegacyReferences
~~~

Ce test charge tous les Blueprints sous `/Game/GrimrockPrototype` et recherche `ExplicitlyRejected` dans :

- les valeurs par défaut des variables Blueprint ;
- les valeurs par défaut et auto-générées des pins de tous les graphs.

Il échoue si une référence binaire authorée subsiste.

Le contrat strict TD07.3.8 vérifie également par réflexion que `ExplicitlyRejected` est absent et que les valeurs actives 4 et 5 restent stables.

## 2. Monster perception

Avant CPP-CLEAN03, `FGridMonsterPerception` exposait deux chemins d'ouïe :

~~~text
CanHear()
    -> ManhattanDistance uniquement
    -> aucune obstruction

CanHearThroughGrid()
    -> BFS acoustique
    -> contrat runtime réel
~~~

Le runtime n'appelait plus `CanHear()`. Seul l'ancien test MON4 le conservait.

CPP-CLEAN03 supprime donc :

~~~cpp
FGridMonsterPerception::CanHear(...)
~~~

Le test MON4 utilise maintenant `CanHearThroughGrid()` avec une grille entièrement acoustiquement ouverte lorsqu'il veut vérifier uniquement portée et contournement d'angle.

L'autorité devient `FGridMonsterPerception::CanHearThroughGrid()`, unique pour runtime et tests.

## 3. Non-objectifs

CPP-CLEAN03 ne modifie pas :

- la logique d'acceptation des Receptacles ;
- les règles `InvalidItem`, `Full`, `NoMatchingAcceptanceRule`, `InsertionDisabled` ;
- la propagation acoustique ;
- `AGridLevelRuntimeActor::CanSoundTraverse()` ;
- les DataAssets ;
- les Blueprints ;
- les SaveGames ;
- Lua ;
- `StartNewGame()` ;
- `ShowInventoryWidget()`.

## 4. Tests à valider

~~~powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.CppCleanup.CPP_CLEAN03"
~~~

~~~powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.TechnicalDebt.TD07_3_8.StrictCurrentSchema.LegacySymbolsAbsent"
~~~

~~~powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.Monsters.MON4.Perception"
~~~

Puis la régression acoustique réelle :

~~~powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.Monsters.Perception.AcousticHearing"
~~~

Après validation ciblée, un run global `Grimrock` reste le critère final de non-régression.

## 5. Résultat architectural

~~~text
Receptacle reject reasons
    -> uniquement des états réellement produits

Monster hearing
    -> une seule primitive : CanHearThroughGrid()
~~~

Le ticket retire donc de la surface publique et des switches sans créer de nouvelle couche.
