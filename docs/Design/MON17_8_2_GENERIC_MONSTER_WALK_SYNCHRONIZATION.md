# MON17.8.2 — Generic Monster Walk Synchronization

Statut : **IMPLEMENTATION C++ PRETE — validation compilation/UE5/PIE requise**

## 1. Portee

MON17.8 utilise `GoblinThrower` comme creature pilote, mais le contrat introduit ici est **generique pour tout le bestiaire**.

Il doit s'appliquer sans code specifique a :

- `MON_RatGiant` existant ;
- `MON_GoblinThrower` ;
- tous les monstres futurs utilisant `AGridMonsterActor`, `UGridMonsterMovementComponent` et `UGridMonsterAnimInstance`.

Regle d'architecture :

```text
Gameplay / grille
    -> UGridMonsterMovementComponent
    -> AGridMonsterActor animation signals
    -> UGridMonsterAnimInstance
    -> AnimBP propre au squelette du monstre
    -> Animation Sequence propre au monstre
```

Interdit dans ce jalon :

```text
AGoblinThrowerActor
UGoblinThrowerMovementComponent
if (MonsterId == MON_GoblinThrower)
Root Motion autoritaire
second systeme de locomotion
```

## 2. Probleme corrige

Avant MON17.8.2, `UGridMonsterMovementComponent::TickComponent()` calculait :

```text
LinearAlpha = temps ecoule / MoveDuration
VisualAlpha = EaseInOut(LinearAlpha)
```

La position de l'Actor utilisait `VisualAlpha` :

```cpp
SetActorLocation(Lerp(Start, Target, VisualAlpha));
```

mais le signal animation utilisait encore `LinearAlpha` :

```cpp
SetMovementAnimationState(true, LinearAlpha);
```

Avec `bUseEaseInOut=true`, le squelette pouvait donc evaluer sa marche selon une progression differente de la distance reellement parcourue dans la case.

Cette incoherence est generique et peut produire du foot sliding sur n'importe quel monstre utilisant `MoveAlpha` pour piloter sa phase de locomotion.

## 3. Correction C++

`GridMonsterMovementComponent.cpp` transmet maintenant le meme alpha que celui utilise pour la translation :

```cpp
Monster->SetActorLocation(
    FMath::Lerp(MotionStartLocation, MotionTargetLocation, VisualAlpha));

Monster->SetMovementAnimationState(true, VisualAlpha);
```

Le contrat de `MoveAlpha` devient donc :

```text
MoveAlpha = progression spatiale normalisee du mouvement affiche
0.0 = centre de la cellule source
1.0 = centre de la cellule destination
```

Si `bUseEaseInOut=false`, `MoveAlpha` reste naturellement lineaire.

Aucune nouvelle propriete n'est ajoutee. `MoveDuration` reste l'unique duree autoritaire de presentation du deplacement.

## 4. Contrat AnimBP recommande

Pour une animation de marche **in-place**, l'AnimBP peut utiliser `MoveAlpha` comme phase normalisee de la traversee d'une case.

Pour une sequence de longueur `WalkLengthSeconds` :

```text
ExplicitTime = MoveAlpha * WalkLengthSeconds
```

La solution privilegiee dans UE5.5.4 est donc un `Sequence Evaluator` (ou mecanisme equivalent permettant un temps explicite) plutot qu'un Sequence Player libre dont le PlayRate evolue independamment de la translation de grille.

Le C++ ne connait pas la longueur des animations propres aux squelettes. C'est volontaire :

```text
C++ generique     -> fournit 0..1
AnimBP specifique -> convertit 0..1 vers la duree de son animation
```

Ainsi Rat, Gobelin, Araignee, Squelette, etc. peuvent utiliser des animations de durees differentes sans specialisation C++.

## 5. Modification manuelle UE5.5.4 — GoblinThrower

### 5.1 Verifier la sequence

Ouvrir :

```text
A_GoblinThrower_Walk
```

Verifier :

- Root Motion desactive ;
- animation reellement in-place ;
- longueur exacte de la sequence ;
- debut et fin compatibles avec une boucle ;
- absence de translation globale du root qui ferait avancer physiquement le personnage.

Noter la valeur exacte de `Sequence Length`.

### 5.2 Modifier ABP_MON_GoblinThrower

Conserver le State Machine et les transitions existantes :

```text
Idle -> Walk : bIsMoving == true
Walk -> Idle : bIsMoving == false
```

Dans l'etat `Walk` :

1. remplacer le Sequence Player libre de `A_GoblinThrower_Walk` par un `Sequence Evaluator` ;
2. selectionner `A_GoblinThrower_Walk` ;
3. piloter son `Explicit Time` avec :

```text
MoveAlpha * SequenceLength
```

4. conserver la sortie de pose vers le State Result ;
5. ne pas modifier le Slot utilise par `AM_GoblinThrower_ThrowKnife` ;
6. ne pas activer Root Motion.

Si UE expose directement une entree de temps normalise sur le noeud utilise, preferer cette entree a une multiplication manuelle par la duree.

## 6. Verification Rat Geant

Le Rat Geant sert de test de non-regression du pipeline generique.

Dans son AnimBP :

- s'il utilise seulement `bIsMoving` avec un Sequence Player libre, aucune modification binaire n'est obligatoire pour la compilation de MON17.8.2 ;
- verifier toutefois en PIE que son comportement de marche n'a pas change negativement ;
- a terme, il pourra adopter le meme contrat `MoveAlpha`-driven si son animation presente du glissement de pieds.

MON17.8 ne doit pas imposer qu'une meme animation ou une meme duree soit utilisee par tous les monstres. Seul le signal normalise 0..1 est commun.

## 7. Cas a tester en PIE

### GoblinThrower

Verifier successivement :

```text
1 case en avant
plusieurs cases consecutives
poursuite du groupe
patrouille
move apres rotation
move -> attaque ThrowKnife
attaque ThrowKnife -> move suivant
```

Observer en priorite :

- les pieds ne doivent plus visiblement glisser par rapport au sol ;
- la pose de marche doit avancer avec la distance parcourue ;
- aucun snap supplementaire ne doit apparaitre au centre des cellules ;
- le Gobelin doit toujours terminer exactement au centre de sa cellule ;
- le lancer de couteau doit conserver son montage et son timing MON17.3.3.

### RatGiant

Verifier au minimum :

```text
Idle
move d'une case
moves consecutifs
poursuite/patrouille existante
combat existant
```

Aucune regression gameplay ne doit etre acceptee.

## 8. Validation C++ / automation demandee

Apres pull du commit MON17.8.2 :

1. compiler le projet sous UE5.5.4 / Visual Studio ;
2. executer les tests cibles existants :

```text
Grimrock.Monsters.MON3
Grimrock.Monsters.MON10
Grimrock.Monsters.MON17.2
Grimrock.Monsters.MON17.3
```

3. effectuer la validation PIE GoblinThrower + RatGiant ci-dessus.

Cette documentation ne declare **aucune compilation ni aucun test valide** tant que le resultat n'a pas ete fourni depuis l'environnement UE5.5.4 local.

## 9. Criteres de cloture MON17.8.2

MON17.8.2 peut etre considere valide lorsque :

- le C++ compile sous UE5.5.4 ;
- `MoveAlpha` represente bien la progression spatiale affichee ;
- le GoblinThrower utilise sa marche in-place sans glissement notable ;
- le RatGiant ne regresse pas ;
- ThrowKnife ne regresse pas ;
- aucun code specifique au Gobelin n'a ete introduit ;
- Root Motion reste non autoritaire ;
- les tests cibles passent.

La suite est `MON17.8.3 — Generic Monster Presentation State Integration`, avec le GoblinThrower comme premier cas complet et le RatGiant comme regression representative.
