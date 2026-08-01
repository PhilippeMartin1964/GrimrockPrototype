# MON5 — Gestion des tours, phases et points d'action

> **Statut historique.** MON5 décrit le séquenceur actuellement implémenté
> avec `PlayerPhase` puis `EnemyPhase`. La cible de refonte définie avant la
> poursuite de MON12 est documentée dans
> `COMBAT_SYSTEM_V2_ACTION_POINTS_INITIATIVE.md` : initiative globale, tours
> individuels, PA des personnages et mobilité du groupe. Les règles MON5
> restent autoritaires tant que chaque migration V2 n'est pas implémentée et
> validée.

## 1. Objectif

MON5 ajoute le séquenceur central du combat sans encore résoudre les attaques.

Le jalon contient :

- `FGridActionPointBudget`, budget déterministe de points d'action ;
- `FGridTurnPhaseStateMachine`, règles pures des phases ;
- `FGridMonsterTurnPlanner`, conversion d'un chemin MON4 en actions ;
- `UGridTurnManagerComponent`, gestionnaire runtime des manches ;
- verrouillage des entrées pendant les phases non jouables ;
- timeout de sécurité pour chaque action animée ;
- deux Automation Tests.

MON5 exécute uniquement :

```text
Turn
Move
Wait
```

La morsure, les jets et les dégâts commencent dans MON6.

---

## 2. Cycle d'une manche

```text
Exploration
    ↓ Start Combat From Perception
StartingCombat
    ↓ délai de sécurité
PlayerPhase — Round 1
    ↓ End Player Phase
EnemyPhase
    ↓ tous les Rats ont terminé
EndingRound
    ↓
PlayerPhase — Round 2
```

Le joueur termine volontairement sa phase.

Pour le Rat géant :

```text
Action Points Per Turn = 2
Turn                  = 0 PA
Move d'une cellule    = 1 PA
```

Un Rat éloigné peut donc exécuter :

```text
Turn + Move + Move
```

Un Rat déjà adjacent exécute provisoirement :

```text
Wait
```

---

## 3. Ordre des monstres

Au début de chaque `EnemyPhase`, les monstres vivants sont triés par :

1. initiative décroissante ;
2. `SpawnObjectId` pour départager les égalités.

Une seule action visuelle est exécutée à la fois. Le monstre suivant attend la fin de l'action courante.

---

## 4. Compilation

Fermer Unreal Engine, puis compiler :

```text
Configuration : Development Editor
Plateforme    : Win64
Cible         : GrimrockPrototypeEditor
```

Une compilation complète est recommandée pour faire apparaître :

```text
Grid Turn Manager Component
```

---

## 5. Ajouter le gestionnaire au RuntimeActor

Ouvrir :

```text
Content/GrimrockPrototype/Blueprints/Runtime/BP_GridLevelRuntimeActor
```

Dans **Components** :

1. cliquer sur `Add` ;
2. rechercher `Grid Turn Manager Component` ;
3. l'ajouter ;
4. le renommer `TurnManager`.

Conserver :

```text
Auto Initialize             = true
Combat Start Safety Padding = 0.05
Action Timeout Padding      = 0.50
Log Phase Changes           = true
```

Compiler et sauvegarder le Blueprint.

La carte doit contenir un seul RuntimeActor avec son `Level Asset` valide.

---

## 6. Désactiver les anciennes commandes directes du Rat

MON3 et MON4 utilisaient des touches temporaires directement dans `BP_MON_RatGiant`.

Dans :

```text
BP_MON_RatGiant
→ Class Defaults
→ Input
```

régler :

```text
Auto Receive Input = Disabled
```

Les anciens événements `I J K L O P U Y H` doivent être supprimés ou déconnectés.

Le Rat conserve obligatoirement :

```text
MonsterMovement
MonsterBehavior
```

Le TurnManager appelle désormais ces deux composants.

---

## 7. Vérifier le DataAsset

Dans `DA_MON_RatGiant` :

```text
Stats
  Initiative             = 12
  Action Points Per Turn = 2

Movement
  Move Duration = 0.36
  Turn Duration = 0.12
```

Aucune animation supplémentaire n'est demandée. Conserver provisoirement :

```text
Idle → Local Space Ref Pose
Move → Local Space Ref Pose
Turn → Local Space Ref Pose
```

---

## 8. Commandes temporaires MON5

Dans `BP_GridLevelRuntimeActor`, régler temporairement :

```text
Auto Receive Input = Player 0
```

### F5 — démarrer selon la perception

```text
F5 Pressed
→ TurnManager
→ Start Combat From Perception
→ Print String(Return Value)
```

La fonction actualise elle-même la perception.

### F6 — terminer la phase du joueur

```text
F6 Pressed
→ TurnManager
→ End Player Phase
→ Print String(Return Value)
```

La fonction renvoie `false` si le groupe est encore entre deux poses de grille.

### F7 — interrompre le combat

```text
F7 Pressed
→ TurnManager
→ Abort Combat
```

### F8 — afficher le diagnostic

```text
F8 Pressed
→ TurnManager
→ Log Current Turn State
```

Filtre :

```text
GridTurnManager
```

### F9 — inclure tous les monstres vivants

```text
F9 Pressed
→ TurnManager
→ Start Combat With All Monsters
→ Print String(Return Value)
```

Cette commande facilite les tests à plusieurs Rats.

### F10 — victoire temporaire

```text
F10 Pressed
→ TurnManager
→ Force Victory
```

---

## 9. Démarrage du combat

Placer le Rat à portée de vue ou d'ouïe, lancer le PIE, puis appuyer sur `F5`.

Résultat initial :

```text
Current Phase      = StartingCombat
Combat Active      = true
Party Input Locked = true
```

Le délai de démarrage correspond au maximum de :

```text
Party Move Duration
Party Turn Duration
Input Buffer Max Age
```

plus `Combat Start Safety Padding`.

Avec les valeurs actuelles :

```text
environ 0.41 seconde
```

Le buffer existant expire et une transition du groupe peut se terminer sans lancer une commande supplémentaire.

Le résultat suivant est :

```text
Current Phase      = PlayerPhase
Round Number       = 1
Party Input Locked = false
```

---

## 10. Phase du joueur

Pendant `PlayerPhase` :

- le groupe peut se déplacer ;
- le groupe peut tourner ;
- les interactions restent disponibles ;
- aucun Rat n'agit automatiquement.

Après un mouvement, attendre que la caméra soit revenue exactement au centre de la cellule, puis appuyer sur `F6`.

`End Player Phase` est refusé pendant une interpolation de position ou de rotation.

---

## 11. Phase ennemie

Après `F6` :

```text
Current Phase      = EnemyPhase
Party Input Locked = true
```

Pour chaque Rat :

1. la perception est actualisée ;
2. le chemin MON4 est recalculé ;
3. les PA sont restaurés depuis le DataAsset ;
4. le chemin devient une file `Turn/Move` ;
5. les actions sont exécutées séquentiellement ;
6. les PA sont dépensés après réussite ;
7. le Rat suivant commence.

Exemple :

```text
Turn East — 0 PA
Move East — 1 PA
Move East — 1 PA
```

Dans l'instance PIE de `TurnManager`, observer :

```text
Current Phase
Round Number
Current Monster
Current Enemy Index
Current Monster Maximum Action Points
Current Monster Remaining Action Points
Pending Actions
Active Action
Has Active Action
```

Le Rat ne doit jamais dépenser plus de deux PA.

---

## 12. Fin de la phase ennemie

Lorsque tous les Rats ont terminé :

```text
EnemyPhase
→ EndingRound
→ PlayerPhase
```

Résultat attendu :

```text
Round Number       = 2
Party Input Locked = false
```

`EndingRound` est bref mais émet bien `On Phase Changed`.

---

## 13. Rat adjacent

Si le Rat se trouve déjà sur une cellule d'attaque adjacente au groupe, le chemin MON4 est vide.

MON5 prépare :

```text
Wait
```

Le Rat termine son tour sans bouger. MON6 remplacera ce `Wait` par `MeleeAttack / Attack_Bite`.

---

## 14. Verrouillage des entrées

Vérifier :

```text
StartingCombat : verrouillé
PlayerPhase    : déverrouillé
EnemyPhase     : verrouillé
EndingRound    : verrouillé
PlayerPhase N+1: déverrouillé
```

Les commandes temporaires `F6`, `F7` et `F8` restent disponibles car elles appartiennent au RuntimeActor et non au Pawn verrouillé.

---

## 15. Plusieurs Rats

Placer deux Rats sur des cellules distinctes puis utiliser `F9`.

Résultat attendu :

- un seul Rat agit à la fois ;
- chaque Rat reçoit ses propres PA ;
- l'ordre est stable ;
- l'initiative la plus élevée agit d'abord ;
- les occupations et réservations MON3 restent respectées ;
- le second chemin est recalculé après le déplacement du premier Rat.

Pour tester le tri, utiliser temporairement une seconde définition avec :

```text
Initiative = 20
```

Elle doit agir avant le Rat à initiative 12.

---

## 16. Timeout de sécurité

Pour chaque action :

```text
Timeout = durée attendue + Action Timeout Padding
```

Valeurs actuelles :

```text
Move = 0.36 + 0.50 = 0.86 seconde
Turn = 0.12 + 0.50 = 0.62 seconde
```

En cas de timeout :

- l'action visuelle est annulée ;
- la réservation MON3 est libérée ;
- une erreur est journalisée ;
- la file continue ;
- le combat ne reste pas bloqué.

---

## 17. Logs attendus

```text
[GridTurnManager] Phase=StartingCombat Round=0
[GridTurnManager] Phase=PlayerPhase Round=1
[GridTurnManager] Phase=EnemyPhase Round=1
[GridTurnManager] Phase=EndingRound Round=1
[GridTurnManager] Phase=PlayerPhase Round=2
```

`Log Current Turn State` affiche notamment :

```text
Phase
Round
Current Monster
AP restant / AP maximum
Pending Actions
Active Action
Timeout
Input Locked
```

---

## 18. Tests automatisés

Dans :

```text
Tools
→ Session Frontend
→ Automation
```

lancer :

```text
Grimrock.Monsters.MON5.PhaseStateMachine
Grimrock.Monsters.MON5.ActionPointsAndPlanning
```

Le premier vérifie les transitions de phases et le compteur de manches.

Le second vérifie :

- le budget de PA ;
- les coûts invalides ;
- les rotations gratuites ;
- un ou deux déplacements selon les PA ;
- le demi-tour en deux rotations ;
- `Wait` lorsque le chemin est vide.

---

## 19. Validation MON5

MON5 est validé lorsque :

1. le projet compile ;
2. `TurnManager` est ajouté au RuntimeActor ;
3. les commandes directes du Rat sont désactivées ;
4. `F5` démarre le combat avec un Rat qui perçoit le groupe ;
5. `StartingCombat` verrouille les entrées ;
6. la première manche entre dans `PlayerPhase` ;
7. `F6` est refusé pendant une transition du groupe ;
8. `F6` déclenche `EnemyPhase` lorsque le groupe est arrêté ;
9. le Rat suit automatiquement le chemin MON4 ;
10. il ne dépense jamais plus de deux PA ;
11. les rotations sont gratuites ;
12. plusieurs Rats agissent séquentiellement par initiative ;
13. le groupe reste verrouillé pendant toute la phase ennemie ;
14. `EndingRound` ouvre la manche suivante ;
15. un Rat adjacent exécute `Wait` ;
16. `F7` restitue l'exploration et les entrées ;
17. les deux tests automatisés réussissent.

---

## 20. Suite

MON6 ajoutera :

- `FGridCombatResolver` ;
- l'action `MeleeAttack` ;
- la morsure `Attack_Bite` ;
- le choix d'un personnage de première ligne ;
- les jets d'attaque et de dégâts ;
- l'armure physique ;
- l'application unique de l'impact ;
- le raccordement futur aux Montages et Anim Notifies.
