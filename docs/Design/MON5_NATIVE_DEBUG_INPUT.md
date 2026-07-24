# MON5 - Entrees de diagnostic natives

## Objectif

Les commandes temporaires MON5 ne sont plus portees par `BP_GridLevelRuntimeActor`.

Les raccourcis natifs sont installes directement dans :

```text
AGrimrockPlayerController::SetupInputComponent()
```

Ils raccordent :

```text
NumPad 1 -> Start Combat From Perception
NumPad 2 -> End Player Phase
NumPad 3 -> Abort Combat
NumPad 4 -> Log Current Turn State
NumPad 5 -> Start Combat With All Monsters
NumPad 6 -> Force Victory
```

Aucun composant Blueprint supplementaire n'est necessaire.

Les raccourcis ne sont pas compiles dans une configuration Shipping.

---

## Compilation

Fermer Unreal Engine, puis compiler :

```text
Configuration : Development Editor
Plateforme    : Win64
Cible         : GrimrockPrototypeEditor
```

La logique native se trouve dans :

```text
Source/GrimrockPrototype/Public/Runtime/GrimrockPlayerController.h
Source/GrimrockPrototype/Private/Runtime/GrimrockPlayerController.cpp
```

---

## Nettoyage de BP_GridLevelRuntimeActor

Apres compilation reussie, l'utilisateur peut ouvrir :

```text
Content/GrimrockPrototype/Blueprints/Runtime/BP_GridLevelRuntimeActor
```

Supprimer ou deconnecter :

```text
Event BeginPlay
-> Get Player Controller
-> Enable Input

NumPad 1
NumPad 2
NumPad 3
NumPad 4
NumPad 5
NumPad 6
```

Dans les Class Defaults, remettre :

```text
Auto Receive Input = Disabled
Block Input        = false
```

Le composant suivant reste present et inchange :

```text
TurnManager
```

avec :

```text
Auto Initialize             = true
Combat Start Safety Padding = 0.05
Action Timeout Padding      = 0.50
Log Phase Changes           = true
```

Compiler et sauvegarder le Blueprint.

---

## Nettoyage de BP_MON_RatGiant

Conserver :

```text
Auto Receive Input = Disabled
```

Les anciennes commandes directes MON3/MON4 restent supprimees ou deconnectees.

Les composants suivants restent obligatoires :

```text
MonsterMovement
MonsterBehavior
```

---

## Test PIE

Lancer le PIE et cliquer une fois dans le viewport.

Au demarrage, l'Output Log doit contenir :

```text
[GridTurnManagerInput] Bound NumPad 1-6 PlayerController=...
```

Filtrer avec :

```text
GridTurnManagerInput
GridTurnManager
```

Tester ensuite :

```text
NumPad 1 -> combat selon la perception
NumPad 2 -> phase ennemie
NumPad 3 -> retour Exploration
NumPad 4 -> diagnostic du TurnManager
NumPad 5 -> combat avec tous les monstres vivants
NumPad 6 -> Victory
```

Les commandes qui renvoient un resultat affichent brievement a l'ecran `true`
ou `false`.

Exemples de logs :

```text
[GridTurnManagerInput] StartCombatFromPerception=true
[GridTurnManagerInput] EndPlayerPhase=true
[GridTurnManagerInput] AbortCombat=true
[GridTurnManagerInput] LogCurrentTurnState=true
[GridTurnManagerInput] StartCombatWithAllMonsters=true
[GridTurnManagerInput] ForceVictory=true
```

---

## Validation

La migration est validee lorsque :

1. le projet compile ;
2. le message `Bound NumPad 1-6` apparait ;
3. les six commandes fonctionnent sans evenement clavier dans le RuntimeActor ;
4. `Auto Receive Input` du RuntimeActor est `Disabled` ;
5. `NumPad 3`, `NumPad 4` et `NumPad 6` restent utilisables pendant `EnemyPhase` ;
6. MON5 conserve ses validations precedentes.
