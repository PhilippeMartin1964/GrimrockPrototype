# MON17.3.2 — Projectile de présentation générique

Statut : **EN COURS — Actor et tests de contrat posés, compilation UE5.5.4 à valider**

## Objectif

Fournir une représentation visuelle générique pour les attaques monstres `Delivery=Projectile`, sans déplacer l'autorité de combat hors du TurnManager.

Le projectile MON17.3.2 est strictement **présentation-only** :

- aucun calcul Hit/Miss ;
- aucun dégât ;
- aucune collision de gameplay ;
- aucun inventaire ;
- aucun pickup ;
- aucune persistance SaveGame.

Le résultat combat reste produit par `UGridMonsterCombatComponent::ResolveAndApplyPartyAttack()` au moment d'impact autoritaire du TurnManager.

## Actor générique

Nouveau type :

```text
AGridCombatProjectileActor
```

Fichiers :

```text
Source/GrimrockPrototype/Public/Runtime/Combat/GridCombatProjectileActor.h
Source/GrimrockPrototype/Private/Runtime/Combat/GridCombatProjectileActor.cpp
```

L'Actor contient :

- un `USceneComponent` racine ;
- un `UStaticMeshComponent` sans collision ni physique ;
- une source monde ;
- une cible monde ;
- une durée de trajet ;
- une interpolation linéaire déterministe ;
- destruction automatique à l'arrivée.

Il reçoit au lancement :

```text
ProjectileVisualMesh
ProjectileVisualScale
ProjectileRotationOffset
ProjectileTravelDuration
```

issus de `FGridMonsterAttackDefinition`.

## Synchronisation temporelle

La règle générique est :

```text
LaunchDelay = max(0, ImpactTimeSeconds - ProjectileTravelDuration)
```

Ainsi, si :

```text
ImpactTimeSeconds        = 0.25
ProjectileTravelDuration = 0.20
```

le projectile doit apparaître à `t=0.05` et atteindre sa cible à `t=0.25`.

La synchronisation fine avec un montage et des sockets appartient à MON17.3.3.

## Relation avec MON11.4.1

`AGridThrownItemActor` n'est volontairement pas réutilisé directement : il transporte un vrai `FGridItemInstance` et se convertit en pickup récupérable.

MON17.3.2 réutilise en revanche le principe validé par MON11.4.1 :

```text
résolution combat autoritaire
          !=
trajectoire visuelle
```

## Tests

Filtre :

```text
Grimrock.Monsters.MON17.3.2
```

Tests :

```text
ProjectileTiming
ProjectileTrajectory
```

Ils valident :

- le calcul de délai de lancement ;
- le lancement immédiat lorsque le temps de trajet dépasse le temps d'impact ;
- le clamp des valeurs temporelles négatives ;
- les positions source, milieu et cible ;
- le clamp de l'alpha de trajectoire.

## Étape suivante après validation

MON17.3.2b branchera cet Actor au cycle `RangedAttack` du TurnManager. Aucun comportement de recherche de position ne sera ajouté.

## Hors périmètre

- socket de main / point de départ final ;
- montage de lancer final ;
- choix du mesh couteau final ;
- spin ou arc balistique ;
- cooldown runtime ;
- maintien de distance / recul / kiting (`MON17.4`).
