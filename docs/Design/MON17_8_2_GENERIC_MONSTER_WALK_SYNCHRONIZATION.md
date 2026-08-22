# MON17.8.2 — Generic Monster Walk Synchronization

Statut : **VALIDÉ VISUELLEMENT EN PIE SUR GOBLINTHROWER — compilation C++ / automation / non-régression RatGiant encore à confirmer**

## 1. Portée

MON17.8 utilise `GoblinThrower` comme créature pilote, mais le contrat introduit ici est **générique pour tout le bestiaire**.

Il doit s'appliquer sans code spécifique à :

- `MON_RatGiant` existant ;
- `MON_GoblinThrower` ;
- tous les monstres futurs utilisant `AGridMonsterActor`, `UGridMonsterMovementComponent` et `UGridMonsterAnimInstance`.

Architecture :

```text
Gameplay / grille
    -> UGridMonsterMovementComponent
    -> AGridMonsterActor animation signals
    -> UGridMonsterAnimInstance
    -> AnimBP propre au squelette du monstre
    -> Animation Sequence propre au monstre
```

Interdit :

```text
AGoblinThrowerActor
UGoblinThrowerMovementComponent
if (MonsterId == MON_GoblinThrower)
Root Motion autoritaire
second système de locomotion
```

## 2. Correction C++ générique

Avant MON17.8.2 :

```text
LinearAlpha = temps écoulé / MoveDuration
VisualAlpha = EaseInOut(LinearAlpha)
```

La position monde utilisait `VisualAlpha`, mais `MoveAlpha` transmis à l'animation utilisait encore `LinearAlpha`.

La correction est volontairement minimale :

```cpp
Monster->SetActorLocation(
    FMath::Lerp(MotionStartLocation, MotionTargetLocation, VisualAlpha));

Monster->SetMovementAnimationState(true, VisualAlpha);
```

Le contrat devient :

```text
MoveAlpha = progression spatiale normalisée du mouvement affiché
0.0 = centre de la cellule source
1.0 = centre de la cellule destination
```

`MoveDuration` reste l'unique durée autoritaire de présentation du déplacement. Aucun PlayRate C++ ni connaissance d'asset d'animation n'est ajouté au runtime générique.

## 3. Enseignement important : ne pas mapper automatiquement toute la séquence à une case

L'hypothèse initiale :

```text
ExplicitTime = MoveAlpha * SequenceLength
```

s'est révélée trop simpliste pour un asset contenant plusieurs foulées.

Le contrat générique corrigé est :

```text
ExplicitTime = CycleStart + MoveAlpha * CycleDuration
```

où l'AnimBP spécifique au squelette choisit un **cycle locomoteur propre**, par exemple pied droit -> pied gauche -> pied droit.

Ainsi :

```text
C++ générique
    fournit MoveAlpha 0..1

AnimBP spécifique
    choisit son asset
    choisit CycleStart
    choisit CycleDuration
```

Le C++ ne connaît ni la longueur du clip, ni le nombre de pas, ni la phase des pieds.

## 4. Choix d'asset GoblinThrower

L'ancien `A_GoblinThrower_Walk`, issu de `MM_Walk_InPlace`, a été jugé visuellement trop faible : le Gobelin donnait surtout l'impression de glisser au sol avec peu d'engagement des jambes.

Une meilleure source du même package natif Goblin_Bomber a été retenue :

```text
MM_Walk_Fwd
```

Cette animation :

- provient du même squelette / même hiérarchie d'os que le GoblinThrower réorganisé dans le projet ;
- reste sur place avec le root verrouillé ;
- présente une démarche plus naturelle et plus affirmée ;
- est utilisée sous le nom de production :

```text
A_GoblinThrower_Walk_Fwd
```

Réglages vérifiés :

```text
Skeleton              = SKEL_GoblinThrower
Enable Root Motion    = false
Root Motion Root Lock = Ref Pose
Force Root Lock       = true
```

Aucun IK Retargeter Mixamo n'est requis pour cette animation : `MM_Walk_Fwd` et `MM_Walk_InPlace` proviennent du même package Goblin_Bomber. Le projet a seulement renommé/réorganisé le Skeleton, le Skeletal Mesh et le Physics Asset.

## 5. Cycle locomoteur retenu

Les contacts de pieds observés dans `A_GoblinThrower_Walk_Fwd` sont :

```text
L : 0.413 s  (frame 13)
R : 0.895 s  (frame 27)
L : 1.349 s  (frame 40)
R : 1.800 s  (frame 54)
L : 2.449 s  (frame 67)
R : 2.716 s  (frame 81)
```

Le cycle le plus régulier est :

```text
R 0.895 s
    -> L 1.349 s   (+0.454 s)
    -> R 1.800 s   (+0.451 s)
```

soit :

```text
CycleStart    = 0.895 s
CycleDuration = 0.905 s
```

Le Graphe de l'état `Walk` de `ABP_MON_GoblinThrower` utilise donc :

```text
ExplicitTime = 0.895 + MoveAlpha * 0.905
```

avec :

```text
Sequence                  = A_GoblinThrower_Walk_Fwd
Should Loop               = false
Teleport to Explicit Time = true
Root Motion               = non autoritaire / désactivé
```

Le Slot utilisé par `AM_GoblinThrower_ThrowKnife` reste intact.

## 6. Réglage final GoblinThrower validé visuellement

Après essais PIE, le réglage retenu par validation visuelle est :

```text
DA_MON_GoblinThrower.MoveDuration = 1.00 s
Idle -> Walk Blend Duration       = 0.20 s
Walk -> Idle Blend Duration       = 0.20 s

Walk Sequence                     = A_GoblinThrower_Walk_Fwd
ExplicitTime                      = 0.895 + MoveAlpha * 0.905
```

Le résultat est jugé **presque parfait** visuellement pour le rythme volontairement posé d'un dungeon crawler tactique, et nettement supérieur à l'ancien `MM_Walk_InPlace`.

Un léger à-coup peut encore être perceptible lors de l'enchaînement case -> case, mais il n'est pas considéré bloquant à ce stade. Une éventuelle amélioration future devra rester générique et distinguer la fin logique d'une case de la fin visuelle d'une locomotion continue, sans casser le commit grille case par case.

## 7. Règle data-driven pour les monstres futurs

`MoveDuration` est une caractéristique de présentation/gameplay par espèce, pas une constante universelle.

Exemples conceptuels uniquement :

```text
créature rapide   -> MoveDuration plus court
créature humanoïde posée -> MoveDuration intermédiaire
créature lourde   -> MoveDuration plus long
```

Chaque AnimBP peut utiliser son propre :

```text
WalkSequence
CycleStart
CycleDuration
```

tout en partageant le même `MoveAlpha` générique 0..1.

## 8. RatGiant — non-régression requise

Le Rat Géant reste le test de non-régression représentatif du pipeline existant.

Aucune modification binaire de son AnimBP n'est imposée par MON17.8.2. Il faut toutefois vérifier en PIE :

```text
Idle
move d'une case
moves consécutifs
poursuite / patrouille
combat existant
```

Si son animation présente un foot sliding notable, il pourra adopter le même contrat `MoveAlpha` + sous-cycle propre, sans changement C++ spécifique.

## 9. Validation restant à fournir

La validation visuelle PIE GoblinThrower est acquise dans cette étape.

Il reste à fournir depuis l'environnement UE5.5.4 local :

```text
Compilation C++ du projet
Grimrock.Monsters.MON3
Grimrock.Monsters.MON10
Grimrock.Monsters.MON17.2
Grimrock.Monsters.MON17.3
non-régression PIE RatGiant
non-régression ThrowKnife
```

Cette documentation ne déclare pas ces éléments réussis tant que leur résultat n'a pas été fourni.

## 10. Assets binaires UE concernés localement

Les modifications UE5.5.4 réalisées manuellement concernent notamment :

```text
A_GoblinThrower_Walk_Fwd
ABP_MON_GoblinThrower
DA_MON_GoblinThrower
```

Ces `.uasset` ne sont pas modifiés par le commit documentaire/C++ via GitHub ; ils doivent être sauvegardés et versionnés depuis l'environnement local Unreal/Git.

## 11. Critères de clôture complète MON17.8.2

MON17.8.2 sera entièrement clos lorsque :

- le C++ compile sous UE5.5.4 ;
- `MoveAlpha` représente la progression spatiale affichée ;
- le GoblinThrower utilise le cycle validé ci-dessus ;
- le RatGiant ne régresse pas ;
- ThrowKnife ne régresse pas ;
- aucun code spécifique au Gobelin n'a été introduit ;
- Root Motion reste non autoritaire ;
- les tests cibles passent.

La suite est `MON17.8.3 — Generic Monster Presentation State Integration`, avec GoblinThrower comme premier cas complet et RatGiant comme non-régression représentative.
