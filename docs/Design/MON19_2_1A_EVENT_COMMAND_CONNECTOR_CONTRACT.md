# MON19.2.1A — Contrat des connecteurs Event → Command

Statut : **implémenté côté source — compilation et tests UE5.5.4 à valider par le propriétaire du projet**  
Date : **22 août 2026**  
Commit de base : `7f24f2628280ff65ecdfbecfa1bfdd08873ad230` (`Finalize GoblinThrower death`)

## 1. Objectif

MON19.2 commence par durcir le contrat Event → Command avant d’ajouter les variables logiques, les compteurs et, plus tard, Lua.

Cette première sous-étape isole volontairement la partie **contrat/politique/test** avant le câblage du panneau Slate CONNECTORS. Le but est d’obtenir une base petite et vérifiable avant de modifier `GridLevelEditorActor.cpp`, qui est un fichier central et volumineux de l’éditeur.

Aucun runtime Lua n’est introduit ici.

---

## 2. Problèmes traités

L’audit MON19.1 a identifié trois ambiguïtés importantes.

### 2.1 Identité d’un lien conditionnel

Le modèle persistant `FGridObjectLink` contient davantage que le quadruplet historique :

```text
SourceObjectId
SourceEvent
TargetObjectId
Command
```

Il contient également :

```text
Condition
ConditionItemDefinitionId
ConditionItemTag
ConditionItemType
ConditionCount
ConditionWeight
bInvertCondition
```

Deux liens ayant le même quadruplet historique peuvent donc être différents et légitimes si leur condition ou leurs paramètres diffèrent.

MON19.2.1A introduit une définition unique et testée de cette **équivalence exacte persistante** via :

```cpp
GridEditorLinkPolicy::AreLinksExactlyEquivalent(...)
```

Tous les champs persistants de condition participent à l’identité.

Le câblage de cette identité dans `CreateLink()`, `RemoveExactLink()` et la validation sera réalisé dans MON19.2.1B.

### 2.2 « commande acceptée » ne signifie pas toujours « gameplay implémenté »

Le runtime possède actuellement une voie générique qui peut enregistrer un état actif sans que l’objet cible ne dispose nécessairement d’un effet de gameplay spécialisé complet.

L’exemple le plus important est le téléporteur : son état peut être modifié, mais le comportement de téléportation générique est encore marqué TODO dans le runtime.

MON19.2.1A formalise donc trois niveaux :

```text
Unsupported
StateOnly
Gameplay
```

avec :

```cpp
EGridEditorCommandRuntimeSupport
GridEditorLinkPolicy::GetCommandRuntimeSupport(...)
```

La classification actuelle est volontairement conservatrice :

- Door : commandes officielles = `Gameplay` ;
- Receptacle : commandes spécialisées = `Gameplay` ;
- MonsterSpawn / Encounter : commandes MON13 = `Gameplay` ;
- Lever et PressurePlate : commandes d’état génériques = `Gameplay`, car elles modifient effectivement leur état runtime et peuvent réémettre un événement ;
- Teleporter et Light : commandes d’activation officielles = `StateOnly` ;
- les autres types ne disposant que du stockage générique d’état sont classés `StateOnly` pour ces commandes lorsqu’elles sont écrites manuellement ;
- les combinaisons réellement inconnues restent `Unsupported`.

MON19.2.1B branchera cette classification dans la validation du niveau afin qu’un connecteur `StateOnly` produise un diagnostic explicite au lieu de donner l’impression qu’un mécanisme de gameplay complet existe.

### 2.3 Conditions autorisées selon la cible

Les conditions existantes sont toutes fondées sur le contenu du **réceptacle cible**.

La politique expose désormais :

```cpp
GridEditorLinkPolicy::GetSupportedConditionsForTarget(...)
```

Pour un réceptacle :

```text
None
ReceptacleIsEmpty
ReceptacleHasAnyItem
ReceptacleContainsItemDefinition
ReceptacleContainsItemTag
ReceptacleContainsItemType
ReceptacleItemCountAtLeast
ReceptacleWeightAtLeast
```

Pour toute autre cible :

```text
None
```

MON19.2.1B utilisera cette liste directement dans le formulaire CONNECTORS au lieu de recréer une deuxième table de capacités dans Slate.

---

## 3. Tests automatisés ajoutés

Nouveau fichier :

```text
Source/GrimrockPrototypeEditor/Private/Tests/GridEditorMON192LinkContractTests.cpp
```

Deux tests sont définis.

### 3.1 `Grimrock.MON19.2.Editor.LinkPolicyMatrix`

Ce test verrouille notamment :

- les événements officiellement émis par Button, Lever, PressurePlate, Trigger, Receptacle et MonsterSpawn ;
- les cinq commandes Door ;
- les quatre commandes spécialisées Receptacle ;
- les neuf commandes MonsterSpawn/Encounter ;
- la classification `Gameplay` des commandes réellement fonctionnelles ;
- la classification `StateOnly` de Light et Teleporter ;
- les huit choix de condition d’un réceptacle ;
- l’absence de condition spécialisée sur une cible non-réceptacle.

### 3.2 `Grimrock.MON19.2.Editor.ConditionalLinkIdentity`

Ce test vérifie qu’un lien n’est identique à un autre que si tous les champs persistants pertinents sont égaux.

Il vérifie notamment que les différences suivantes créent bien deux identités distinctes :

- type de condition ;
- `ConditionItemDefinitionId` ;
- `ConditionCount` ;
- `ConditionWeight` ;
- `bInvertCondition`.

---

## 4. Ce que cette sous-étape ne fait pas encore

MON19.2.1A ne modifie volontairement pas :

- `SGridEditorLinksPanel` ;
- `GridLevelEditorActor::CreateLink()` ;
- `GridLevelEditorActor::RemoveExactLink()` ;
- la validation des liens dans `ValidateCurrentLevel()` ;
- la persistance runtime ;
- les variables logiques ;
- les primitives compteur/latch/relay ;
- Lua.

Ces changements sont reportés à la sous-étape immédiatement suivante, **MON19.2.1B — CONNECTORS conditionnels et mutations exactes**.

Cette séparation permet de compiler et de tester le contrat avant d’introduire des contrôles Slate supplémentaires.

---

## 5. Validation demandée sous UE5.5.4

Aucune compilation Unreal Engine ni aucun test UE n’a été exécuté dans l’environnement ayant préparé ce commit.

À valider sur la machine de développement :

1. compiler `GrimrockPrototypeEditor` en Development Editor ;
2. lancer :

```text
Grimrock.MON19.2.Editor.LinkPolicyMatrix
Grimrock.MON19.2.Editor.ConditionalLinkIdentity
```

ou le filtre :

```text
Grimrock.MON19.2.Editor
```

La réussite de cette compilation et de ces tests constituera le feu vert pour MON19.2.1B.

---

## 6. Rappel concernant Lua

Lua reste **hors périmètre de MON19.2**.

Le contrat arrêté après MON19.1 reste toutefois le suivant pour les étapes ultérieures :

- un niveau pourra déclarer **plusieurs fichiers source Lua** ;
- les scripts pourront être organisés par puzzle, séquence ou rencontre ;
- un connecteur visera à terme un identifiant de script et un callback précis ;
- les scripts déclarés pour un même niveau partageront la VM isolée de ce niveau ;
- aucun `require()` arbitraire ni accès libre au système de fichiers ne sera exposé ;
- les scripts passeront toujours par l’API Grimrock contrôlée et le dispatcher Command existant pour agir sur le gameplay.

---

## 7. Fichiers concernés

```text
Source/GrimrockPrototypeEditor/Public/EditorTools/GridEditorLinkPolicy.h
Source/GrimrockPrototypeEditor/Private/EditorTools/GridEditorLinkPolicy.cpp
Source/GrimrockPrototypeEditor/Private/Tests/GridEditorMON192LinkContractTests.cpp
docs/Design/MON19_2_1A_EVENT_COMMAND_CONNECTOR_CONTRACT.md
```

Aucun `.uasset`, `.umap` ni fichier runtime de production n’est modifié par MON19.2.1A.
