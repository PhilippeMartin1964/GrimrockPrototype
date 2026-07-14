# MON3 — Occupation et déplacement case par case

## 1. Ce que MON3 ajoute

MON3 ajoute :

- `UGridMonsterOccupancySubsystem`, registre unique par monde des cellules occupées et réservées ;
- `UGridMonsterMovementComponent`, responsable de la validation, de la réservation, de l'interpolation et de la rotation ;
- deux tests automatisés du registre d'occupation.

La grille reste l'autorité. Un déplacement suit toujours :

```text
cellule source occupée
→ destination validée
→ destination réservée
→ interpolation visuelle
→ réservation validée
→ source libérée et destination occupée
```

Pendant l'interpolation, la source reste occupée et la destination reste réservée. Une annulation rend le Rat à la source et libère la réservation.

MON3 ne contient ni pathfinding ni IA. Ceux-ci commencent à MON4.

---

## 2. Valeurs de `DA_MON_RatGiant`

Dans `Monster | Movement`, vérifier :

```text
Grid Footprint      = 1, 1
Move Duration       = 0.36
Turn Duration       = 0.12
Blocks Movement     = true
Can Open Doors      = false
Can Use Teleporters = false
```

MON3 accepte uniquement un footprint `1 × 1` et produit un log explicite pour une autre taille.

---

## 3. Ajouter le composant au Blueprint

Ouvrir :

```text
Content/GrimrockPrototype/Monsters/RatGiant/Blueprints/BP_MON_RatGiant
```

Dans le panneau **Components** :

1. cliquer sur `Add` ;
2. rechercher `Grid Monster Movement Component` ;
3. l'ajouter ;
4. le renommer éventuellement `MonsterMovement`.

Conserver :

```text
Auto Initialize                 = true
Snap To Cell On Initialize      = true
Infer Cell From Actor Location  = true
Use Ease In Out                 = true
```

Avec `Infer Cell From Actor Location`, la cellule initiale est calculée depuis la position du Blueprint placé dans la carte. Placez donc le pivot du Rat au centre d'une cellule praticable. Au PIE, le composant le recale exactement avec `GetCellCenterWorld`.

La propriété `Facing` reste configurable sur l'instance placée dans la carte, dans `Monster | Grid`.

La carte doit contenir un seul `BP_GridLevelRuntimeActor` valide avec son `Level Asset` renseigné.

L'initialisation est refusée lorsque :

- le RuntimeActor ou le DataAsset manque ;
- la position ne correspond pas à une cellule valide et praticable ;
- le groupe occupe cette cellule ;
- un autre monstre l'occupe ou la réserve ;
- le footprint n'est pas `1 × 1`.

Chercher les erreurs sous :

```text
[GridMonsterMovement]
```

---

## 4. Développer `ABP_MON_RatGiant`

`ABP_MON_RatGiant` dérive déjà de `GridMonsterAnimInstance`. Les variables suivantes existent déjà :

```text
bIsMoving
bIsTurning
MoveAlpha
TurnDirection
CurrentCell
Facing
```

Ne les recréez pas.

Dans la State Machine, créer trois états :

```text
Idle
Move
Turn
```

### Idle

Conserver :

```text
Local Space Ref Pose → State Result
```

### Move

Utiliser :

```text
A_RatGiant_Walk → State Result
```

Activer la boucle. L'animation doit rester sur place, sans Root Motion.

### Turn

Pour MON3 :

```text
Local Space Ref Pose → State Result
```

La rotation de 90° est interpolée par le composant C++.

### Transitions

```text
Idle → Move : bIsMoving == true
Move → Idle : bIsMoving == false
Idle → Turn : bIsTurning == true
Turn → Idle : bIsTurning == false
```

Fondus conseillés : `0.05 s` pour Move et `0.03 s` pour Turn.

---

## 5. Commandes disponibles

Depuis la référence du composant `MonsterMovement` :

```text
Try Move Forward
Try Move(Direction)
Try Turn Left
Try Turn Right
Cancel Current Action
Teleport To Grid Pose
Log Registry
```

Pour un test temporaire dans `BP_MON_RatGiant`, définir `Auto Receive Input = Player 0`, puis :

```text
I → MonsterMovement → Try Move Forward
J → MonsterMovement → Try Turn Left
L → MonsterMovement → Try Turn Right
K → MonsterMovement → Cancel Current Action
```

Ces entrées sont uniquement destinées à MON3. Elles seront remplacées par le gestionnaire de tours et l'IA.

---

## 6. Vérifications manuelles

### Initialisation

- le Rat est recalé au centre de sa cellule ;
- le composant indique `Is Initialized = true` ;
- deux Rats placés sur la même cellule ne peuvent pas s'enregistrer tous les deux.

### Rotation

- `J` et `L` produisent exactement 90° ;
- `bIsTurning` reste vrai pendant `Turn Duration` ;
- `Facing` est modifié à la fin seulement.

### Déplacement

- `I` réserve immédiatement la cellule devant le Rat ;
- le Rat passe visuellement d'un centre à l'autre pendant `Move Duration` ;
- `CurrentCell` reste la source pendant le mouvement puis devient la destination ;
- l'ancienne cellule est libérée à la fin ;
- `A_RatGiant_Walk` joue uniquement pendant `bIsMoving`.

### Blocages

Le déplacement doit être refusé vers :

- un mur ;
- une porte fermée ;
- une cellule non praticable ;
- la cellule du groupe ;
- une cellule occupée ;
- une cellule déjà réservée.

Une porte ouverte doit autoriser le passage si la destination est libre.

### Annulation

Pendant un déplacement, `Cancel Current Action` doit :

- replacer le Rat au centre de la source ;
- annuler la réservation ;
- remettre `bIsMoving` à faux.

`Release Occupancy` est appelé automatiquement à la destruction du composant. Quand la mort sera raccordée au gameplay, appeler `Handle Owner Death` afin de libérer immédiatement la cellule.

---

## 7. Tests automatisés

Dans :

```text
Tools → Session Frontend → Automation
```

lancer :

```text
Grimrock.Monsters.MON3.OccupancyRegistry
Grimrock.Monsters.MON3.InvalidTransitions
```

Ils vérifient notamment : occupation exclusive, réservation exclusive, validation du déplacement, annulation, désinscription et remise à zéro du registre.

---

## 8. Validation MON3

MON3 est validé lorsque :

1. `GrimrockPrototypeEditor` compile ;
2. le composant est ajouté à `BP_MON_RatGiant` ;
3. Idle reste en `Local Space Ref Pose` ;
4. Move utilise `A_RatGiant_Walk` sans Root Motion ;
5. Turn utilise provisoirement `Local Space Ref Pose` ;
6. déplacement et rotation sont interpolés ;
7. murs, portes, groupe, occupations et réservations sont respectés ;
8. l'annulation restitue correctement la source ;
9. les deux tests MON3 réussissent.
