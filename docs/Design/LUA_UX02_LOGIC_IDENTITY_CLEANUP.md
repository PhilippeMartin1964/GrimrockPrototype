# LUA-UX02 — Identité logique unique dans le Grid Editor

Date : 11 septembre 2026

## Objectif

LUA-UX02 simplifie l'identité des objets placés dans le niveau. Le level designer ne doit plus choisir entre `Tag`, `LogicId`, `ObjectId` et différents libellés de secours pour référencer un objet.

Le contrat d'authoring devient :

- `LogicId` = **seule identité humaine et scriptable** d'une instance placée ;
- `ObjectId` = GUID interne autoritaire, non destiné à l'écriture des puzzles ;
- `Tag` de placement = donnée historique conservée temporairement uniquement pour la compatibilité des anciens assets ;
- les tags d'items et autres taxonomies de gameplay ne sont pas concernés par cette migration.

## Selected Object

La page `Properties` contient désormais une section `Identity` dédiée :

```text
Identity
  Logic Id    Guardian
  Type        Receptacle
  Definition  Guardian
  Position    (20,18) East
```

`Logic Id` est validé par le service d'authoring existant :

- syntaxe lisible (`A-Za-z`, chiffres et `_`) ;
- unicité dans le niveau ;
- chaîne vide autorisée tant que l'objet n'a pas besoin d'être référencé.

La page anciennement nommée `Connectors` devient `Events & Actions`.

## Events & Actions

Cette page ne gère plus l'identité de l'objet. Elle suppose que l'objet sélectionné a déjà été nommé dans `Properties` lorsque cela est nécessaire.

Elle reste l'unique surface de raccordement :

```text
Event -> Command
Event -> Lua Callback
```

Les conditions, compteurs, comparaisons et séquences sont écrits en Lua.

## Tag historique

Le champ `Tag` reste encore présent dans les structures sérialisées pour éviter une migration destructive des `.uasset` existants. Il n'est cependant plus présenté comme une propriété modifiable dans le Grid Editor.

Un garde de compatibilité dans les helpers Slate masque les anciennes lignes `Tag` qui pourraient encore être construites par du code d'inspection historique. Cette barrière pourra être supprimée lorsque le membre sérialisé `Tag` sera physiquement retiré dans une migration dédiée.

## Overview

Les libellés d'objets de l'Overview utilisent maintenant :

1. `LogicId` s'il existe ;
2. l'identifiant de définition ;
3. l'entrée de palette ;
4. un `ObjectId` court en dernier recours.

`Tag` n'est plus utilisé comme nom d'affichage de l'objet.

## Compatibilité

Cette tranche ne supprime pas :

- les tags d'items ;
- `ConditionItemTag` des anciens liens conditionnels ;
- le membre sérialisé `Tag` des placements existants ;
- les anciennes fonctions C++ de migration qui savent encore lire/écrire ce champ.

Ces éléments sont conservés afin de ne pas casser les assets et tests historiques avant une migration de données explicite.

## Test de contrat LUA-UX02

`Grimrock.LUAUX02.LogicIdentityContract` vérifie que :

- `LogicId` est bien écrit via l'autorité d'authoring existante ;
- deux objets peuvent être référencés de manière unique par `LogicId` ;
- l'ancien `Tag` sérialisé n'est pas écrasé lors de cette transition UX ;
- la résolution `FindTypedPlacementIdsByLogicId` retrouve l'objet attendu.

## Validation attendue

Après récupération du commit :

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.LUAUX02"

.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.MON19.6.Editor"

.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.MON19.7.1.Editor"
```

Puis contrôle visuel du Grid Editor :

- `Properties` affiche `Identity / Logic Id` ;
- la ligne `Tag` n'est plus visible ;
- l'autre onglet s'appelle `Events & Actions` ;
- `Events & Actions` ne répète plus l'éditeur de `Logic Id` ;
- `Lua Scripts` reste strictement centré sur les scripts ;
- l'Overview affiche `LogicId=...` lorsqu'un objet est nommé.
