# MON19.2.4 — Conditions de liens sur variables de niveau

## Statut

Implémentation C++ préparée et intégrée au contrat Event → Condition → Command. Compilation et tests UE5.5.4 à valider par le propriétaire du projet.

## Objectif

MON19.2.4 permet de conditionner directement un `FGridObjectLink` à une variable logique persistante du niveau, sans imposer l'insertion d'un nœud `CompareBool` ou `CompareInt` pour les cas simples.

Exemples :

```text
Trigger.Activated
  → si GateOpen == true
      → Door.Open
```

```text
PressurePlate.Activated
  → si RuneCount >= 3
      → SecretDoor.Open
```

Les comparateurs Logic de MON19.2.3 restent utiles lorsqu'une comparaison doit elle-même produire plusieurs événements ou être réutilisée comme nœud du graphe. MON19.2.4 ajoute simplement une garde locale au connecteur.

## Conditions ajoutées

Deux valeurs sont ajoutées à `EGridObjectCondition` :

```text
LevelVariableBoolEquals
LevelVariableIntCompare
```

### LevelVariableBoolEquals

Paramètres persistants :

```text
ConditionVariableId
ConditionBoolValue
```

La variable doit être déclarée `Bool` dans `UGridLevelAsset::LevelVariables`.

### LevelVariableIntCompare

Paramètres persistants :

```text
ConditionVariableId
ConditionIntComparison
ConditionIntValue
```

La variable doit être déclarée `Int32`.

Comparateurs :

```text
Equal
NotEqual
Less
LessOrEqual
Greater
GreaterOrEqual
```

`bInvertCondition` continue de s'appliquer après l'évaluation, comme pour les conditions de réceptacle.

## Identité exacte

Les quatre nouveaux paramètres font partie de l'identité exacte du `FGridObjectLink` :

```text
ConditionVariableId
ConditionBoolValue
ConditionIntComparison
ConditionIntValue
```

Deux liens ayant le même quadruplet historique `Source/Event/Target/Command` peuvent donc coexister avec des gardes différentes, par exemple :

```text
Gate == true
Gate == false
```

ou :

```text
RuneCount >= 3
RuneCount >= 5
```

La suppression reste une suppression d'une seule variante exacte.

## Normalisation

`GridEditorLinkService::NormalizeLink()` conserve uniquement les paramètres utiles à la condition choisie.

Une condition Bool efface les anciens paramètres Int/receptacle ; une condition Int efface l'ancien paramètre Bool et les paramètres de réceptacle ; `None` remet tous les paramètres conditionnels à leur valeur canonique et interdit l'inversion.

## Validation éditeur

`GridEditorLinkService::IsLinkSupported()` contrôle :

- l'existence de la variable dans `LevelVariables` ;
- son type `Bool` ou `Int32` ;
- la validité du comparateur Int32 ;
- le contrat Event/Command de la source et de la cible ;
- l'absence de doublon exact lors de la création.

`ValidateCurrentLevel()` est aligné sur ce contrat : les conditions de variable ne nécessitent pas une cible `Receptacle`, la variable et son type sont contrôlés, et la clé de doublon inclut les nouveaux paramètres.

## Runtime

`UGridActivationComponent::EvaluateGridObjectLinkCondition()` traite les conditions de variable avant toute tentative de cast de la cible en `AGridReceptacleActor`.

Le flux est :

```text
Event
  ↓
FGridObjectLink
  ↓
lecture FGridLevelRuntimeState courant
  ↓
GridLevelVariableStore::TryGetBool / TryGetInt32
  ↓
comparaison
  ↓
Invert éventuel
  ↓
Command si vrai
```

La condition est donc indépendante de la présence d'un Actor cible. Elle peut protéger un lien vers une porte, un mécanisme, un MonsterSpawn ou un nœud `Logic` data-only, sous réserve que la commande soit déjà supportée par la policy existante.

Une variable absente, d'un mauvais type ou un comparateur Int32 invalide rejette le lien sans exécuter la commande cible.

## CONNECTORS

Le formulaire CONNECTORS propose désormais `Level Variable Bool Equals` et `Level Variable Int Compare` pour toutes les cibles qui acceptent des commandes.

Pour une condition Bool :

```text
Variable       [liste des Bool déclarés]
Expected Value [true/false]
Invert         [ ]
```

Pour une condition Int32 :

```text
Variable       [liste des Int32 déclarés]
Comparison     [Equal / Not Equal / ...]
Compare Value  [int32]
Invert         [ ]
```

La liste de variables est filtrée par type ; aucun identifiant libre ne doit être saisi.

Les `Receptacle` conservent leurs sept conditions spécialisées en plus des deux conditions de variables et de `None`.

## Persistance

Aucun nouvel état runtime n'est introduit. Les valeurs sont celles de `FGridLevelRuntimeState`, déjà persistées par MON19.2.2.

Le `SaveVersion` reste donc :

```text
7
```

Les nouveaux champs conditionnels appartiennent au `UGridLevelAsset` et sont sérialisés avec le niveau, pas au SaveGame.

## Tests ajoutés

Runtime :

```text
Grimrock.MON19.2.Runtime.VariableConditions.BoolAndInvert
Grimrock.MON19.2.Runtime.VariableConditions.IntComparison
Grimrock.MON19.2.Runtime.VariableConditions.InvalidVariable
```

Éditeur :

```text
Grimrock.MON19.2.Editor.VariableConditions.PolicyAndTyping
Grimrock.MON19.2.Editor.VariableConditions.IdentityAndNormalization
```

Les tests runtime utilisent une cible `Logic/AddInt` data-only afin de vérifier explicitement qu'une condition de variable fonctionne sans Actor cible et qu'un échec de condition reste atomique.

## Hors périmètre

MON19.2.4 n'ajoute pas :

- AND/OR/XOR entre plusieurs gardes ;
- expressions arbitraires ;
- conditions entre deux variables ;
- chaînes de caractères ou nombres flottants ;
- timers/delays ;
- Lua ;
- nouveau SaveVersion ;
- nouvel Actor ou Tick.

Les expressions plus riches pourront être construites avec les nœuds Logic de MON19.2.3 puis, plus tard, avec le langage Lua sécurisé de MON19.3/MON19.4.
