# MON17.3.2 — Projectile de présentation générique

Statut : **EN COURS — contrat 17.3.2a validé, branchement runtime 17.3.2b posé et à valider sous UE5.5.4**

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

Résultat local UE5.5.4 du 19 août 2026 : **2/2 Success** sur le contrat initial.

```text
ProjectileTiming      Success
ProjectileTrajectory  Success
```

Ces tests valident :

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
5. prend provisoirement comme source le centre des bounds du SkeletalMesh du monstre ;
6. prend comme cible la position monde du PartyPawn ;
7. crée `AGridCombatProjectileActor` ;
8. initialise sa trajectoire avec scale, rotation et durée authorés.

Le callback différé vérifie encore que la présentation d'attaque est active et que `LastAttackId` correspond. Une attaque annulée avant le lancement ne produit donc pas de projectile différé.

### Présentation optionnelle

L'absence de `ProjectileVisualMesh` n'est **jamais** une erreur de gameplay :

```text
ProjectileVisualMesh = None
    -> aucun Actor visuel
    -> RangedAttack continue normalement
    -> Hit/Miss/dégâts/PA inchangés
```

De même, un échec de chargement, de spawn ou d'initialisation du projectile ne fait pas échouer l'action de combat ; seul un warning de présentation est émis.

Un troisième test est ajouté :

```text
ProjectileVisualOptionality
```

Il vérifie qu'une `FGridMonsterAttackDefinition` `Delivery=Projectile` reste valide et ranged sans mesh visuel.

Résultat de ce troisième test : **à valider localement**.

## Relation avec MON11.4.1

`AGridThrownItemActor` n'est volontairement pas réutilisé directement : il transporte un vrai `FGridItemInstance` et se convertit en pickup récupérable.

MON17.3.2 réutilise en revanche le principe validé par MON11.4.1 :

```text
résolution combat autoritaire
          !=
trajectoire visuelle
```

## Validation PIE attendue

Une fois un mesh temporaire ou final affecté à :

```text
DA_MON_GoblinThrower
  Attacks[Attack_ThrowKnife]
    ProjectileVisualMesh
```

placer le Gobelin sur le même axe que le groupe, entre 2 et 6 cases, avec LOS ouverte.

Log attendu :

```text
[GridMonsterProjectile] Launched Monster=... Attack=Attack_ThrowKnife Travel=...
```

Le projectile doit être visible pendant son trajet, puis disparaître. La résolution de dégâts et la dépense de PA doivent rester identiques avec ou sans mesh projectile.

## Hors périmètre

- socket de main / point de départ final ;
- montage de lancer final ;
- choix du mesh couteau final ;
- spin ou arc balistique ;
- cooldown runtime ;
- maintien de distance / recul / kiting (`MON17.4`).
