# MON5 — Entrées de diagnostic natives

## Objectif

Les commandes temporaires MON5 ne sont plus portées par `BP_GridLevelRuntimeActor`.

Le sous-système natif suivant est créé automatiquement dans les mondes Game, PIE et Game Preview :

```text
UGridTurnDebugInputSubsystem
```

Il attend que le `PlayerController` et son `InputComponent` existent, puis raccorde directement :

```text
NumPad 1 → Start Combat From Perception
NumPad 2 → End Player Phase
NumPad 3 → Abort Combat
NumPad 4 → Log Current Turn State
NumPad 5 → Start Combat With All Monsters
NumPad 6 → Force Victory
```

Aucun composant Blueprint supplémentaire n'est nécessaire.

Les raccourcis ne sont pas compilés dans une configuration Shipping.

---

## Compilation

Fermer Unreal Engine, puis compiler :

```text
Configuration : Development Editor
Plateforme    : Win64
Cible         : GrimrockPrototypeEditor
```

Les nouveaux fichiers sont :

```text
Source/GrimrockPrototype/Public/Runtime/Combat/GridTurnDebugInputSubsystem.h
Source/GrimrockPrototype/Private/Runtime/Combat/GridTurnDebugInputSubsystem.cpp
```

---

## Nettoyage de BP_GridLevelRuntimeActor

Après compilation réussie, ouvrir :

```text
Content/GrimrockPrototype/Blueprints/Runtime/BP_GridLevelRuntimeActor
```

Supprimer ou déconnecter :

```text
Event BeginPlay
→ Get Player Controller
→ Enable Input

NumPad 1
NumPad 2
NumPad 3
NumPad 4
NumPad 5
NumPad 6
```

Dans les Class Defaults :

```text
Auto Receive Input = Disabled
Block Input        = false
```

Le composant suivant reste présent et inchangé :

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

Les anciennes commandes directes MON3/MON4 restent supprimées ou déconnectées.

Les composants suivants restent obligatoires :

```text
MonsterMovement
MonsterBehavior
```

---

## Test

Lancer le PIE et cliquer une fois dans le viewport.

Au démarrage, l'Output Log doit contenir :

```text
[GridTurnDebugInput] Bound NumPad 1-6 to PlayerController=...
```

Filtrer avec :

```text
GridTurnDebugInput
GridTurnManager
```

Tester ensuite :

```text
NumPad 1 → combat selon la perception
NumPad 2 → phase ennemie
NumPad 3 → retour Exploration
NumPad 4 → diagnostic du TurnManager
NumPad 5 → combat avec tous les monstres vivants
NumPad 6 → Victory
```

Les commandes qui renvoient un résultat affichent brièvement à l'écran :

```text
true
false
```

Exemples de logs :

```text
[GridTurnDebugInput] StartCombatFromPerception=true
[GridTurnDebugInput] EndPlayerPhase=true
[GridTurnDebugInput] AbortCombat=true
[GridTurnDebugInput] LogCurrentTurnState=true
[GridTurnDebugInput] StartCombatWithAllMonsters=true
[GridTurnDebugInput] ForceVictory=true
```

---

## Validation

La migration est validée lorsque :

1. le projet compile ;
2. le message `Bound NumPad 1-6` apparaît ;
3. les six commandes fonctionnent sans événement clavier dans le RuntimeActor ;
4. `Auto Receive Input` du RuntimeActor est `Disabled` ;
5. `NumPad 3`, `NumPad 4` et `NumPad 6` restent utilisables pendant `EnemyPhase` ;
6. MON5 conserve ses 18 points de validation précédents.
