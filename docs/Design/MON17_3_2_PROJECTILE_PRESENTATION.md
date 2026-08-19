# MON17.3.2 — Projectile de présentation générique

Statut : **VALIDÉ / CLOS sous UE5.5.4**

## Objectif

Fournir une représentation visuelle générique pour les attaques monstres `Delivery=Projectile`, sans déplacer l'autorité de combat hors du TurnManager.

Le projectile MON17.3.2 est strictement **presentation-only** :

- aucun calcul Hit/Miss ;
- aucun dégât ;
- aucune collision de gameplay ;
- aucun inventaire ;
- aucun pickup ;
- aucune persistance SaveGame.

Le résultat combat reste produit par `UGridMonsterCombatComponent::ResolveAndApplyPartyAttack()` au moment d'impact autoritaire du TurnManager.

## Actor générique

Type :

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

Il reçoit :

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
ImpactTimeSeconds         = 0.25
ProjectileTravelDuration = 0.20
```

le projectile apparaît à `t=0.05` et vise une arrivée à `t=0.25`.

La synchronisation fine avec un montage et un socket de main appartient à MON17.3.3.

## MON17.3.2a — contrat validé

Filtre :

```text
Grimrock.Monsters.MON17.3.2
```

Le contrat initial valide :

```text
ProjectileTiming      Success
ProjectileTrajectory  Success
```

Ces tests couvrent :

- le calcul du délai de lancement ;
- le lancement immédiat lorsque le temps de trajet dépasse le temps d'impact ;
- le clamp des valeurs temporelles négatives ;
- les positions source, milieu et cible ;
- le clamp de l'alpha de trajectoire.

## MON17.3.2b — branchement au RangedAttack

Le point d'intégration est `UGridMonsterCombatComponent::StartAttackPresentation()`, déjà responsable de la présentation générique des attaques monstres.

Pour une action :

```text
Action.Type = RangedAttack
Attack.Delivery = Projectile
ProjectileVisualMesh != None
```

le composant :

1. démarre la présentation d'attaque normale (état, audio, VFX, montage éventuel) ;
2. calcule `LaunchDelay` ;
3. programme le projectile visuel ;
4. charge `ProjectileVisualMesh` au moment du lancement ;
5. prend, pour MON17.3.2, le centre des bounds du SkeletalMesh du monstre comme source provisoire ;
6. prend comme cible la position monde du PartyPawn ;
7. crée `AGridCombatProjectileActor` ;
8. initialise sa trajectoire avec scale, rotation et durée authorés.

Le callback différé vérifie encore que la présentation d'attaque est active et que `LastAttackId` correspond. Une attaque annulée avant le lancement ne produit donc pas de projectile différé.

### Présentation optionnelle

L'absence de `ProjectileVisualMesh` n'est jamais une erreur de gameplay :

```text
ProjectileVisualMesh = None
    -> aucun Actor visuel
    -> RangedAttack continue normalement
    -> Hit/Miss/dégâts/PA inchangés
```

De même, un échec de chargement, de spawn ou d'initialisation du projectile ne fait pas échouer l'action de combat ; seul un warning de présentation est émis.

Le test :

```text
ProjectileVisualOptionality
```

vérifie qu'une `FGridMonsterAttackDefinition` `Delivery=Projectile` reste valide et ranged sans mesh visuel.

## Validation automatisée UE5.5.4

Exécution locale du 19 août 2026 fournie par l'utilisateur : **3/3 Success**.

```text
ProjectileTiming             Success
ProjectileTrajectory         Success
ProjectileVisualOptionality  Success
```

## Validation PIE UE5.5.4

Validation réelle effectuée avec `SM_Bomb` comme placeholder de `ProjectileVisualMesh` sur `Attack_ThrowKnife`.

Le Gobelin était en position légale de tir, avec LOS ouverte. Le log confirme six lancements successifs :

```text
[GridMonsterProjectile] Launched Monster=BP_MON_GoblinThrower_C_1 Attack=Attack_ThrowKnife Travel=0.200 Source=(5698.6,4904.4,78.0) Target=(5700.0,4500.0,110.0)
```

Observations validées :

- le projectile est effectivement visible en PIE ;
- il apparaît brièvement au centre de l'écran, ce qui est attendu avec la source provisoire au centre des bounds, la cible au PartyPawn et un trajet de 0.20 s ;
- le lancement se produit indépendamment du résultat combat ;
- un miss est correctement résolu sans dégâts ;
- les hits appliquent leurs dégâts normalement ;
- les coups critiques fonctionnent ;
- le combat progresse jusqu'à la défaite du personnage sans erreur de projectile ;
- aucune logique d'inventaire/pickup n'intervient.

Extrait représentatif :

```text
Round 1 : Attack_ThrowKnife -> miss, HP 20 -> 20
Round 2 : hit 2, HP 20 -> 18
Round 3 : critical 8, HP 18 -> 10
Round 4 : hit 5, HP 10 -> 5
Round 5 : hit 3, HP 5 -> 2
Round 6 : critical, HP 2 -> 0 -> CharacterDefeated -> Defeat
```

MON17.3.2 est donc **VALIDÉ / CLOS**. L'amélioration du point de départ visuel, du vrai mesh couteau et de la synchronisation avec le geste de lancer passe à MON17.3.3.

## Relation avec MON11.4.1

`AGridThrownItemActor` n'est volontairement pas réutilisé directement : il transporte un vrai `FGridItemInstance` et se convertit en pickup récupérable.

MON17.3.2 réutilise en revanche le principe validé par MON11.4.1 :

```text
résolution combat autoritaire
          !=
trajectoire visuelle
```

## Hors périmètre transféré à MON17.3.3+

- socket de main / point de départ final ;
- montage de lancer final ;
- choix du mesh couteau final ;
- synchronisation fine du lancer ;
- spin ou arc balistique éventuel ;
- cooldown runtime (MON17.3.4) ;
- maintien de distance / recul / kiting (`MON17.4`).
