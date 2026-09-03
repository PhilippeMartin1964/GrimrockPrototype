# ITEM-PHYSICS01 — Stabilisation physique des pickups monde

Date : 03.09.2026

## Objectif

Permettre aux petits items symétriques, notamment une gemme taillée, de se stabiliser naturellement sous Chaos lorsqu'ils sont placés dans le donjon. Une gemme parfaitement verticale sur sa pointe peut rester dans un équilibre numérique artificiel si aucun couple ne vient rompre la symétrie.

ITEM-PHYSICS01 ne remplace pas la physique existante : `AGridItemActor::ConfigureAsWorldPickup()` continue d'activer la collision `PhysicsActor`, la gravité et `SimulatePhysics`.

## Données

`UGridItemDefinitionAsset` expose :

```text
bUseItemWeightAsWorldPhysicsMass      = false
WorldPhysicsInitialTiltDegrees          = 0.0
WorldPhysicsInitialAngularSpeedDegrees  = 0.0
```

- `bUseItemWeightAsWorldPhysicsMass` : si actif, `Weight` est utilisé comme masse Chaos en kilogrammes pour le pickup libre.
- `WorldPhysicsInitialTiltDegrees` : petite inclinaison initiale, limitée à 0..45 degrés.
- `WorldPhysicsInitialAngularSpeedDegrees` : vitesse angulaire initiale, limitée à 0..720 deg/s, appliquée au rigid body une fois Chaos réellement actif.

Les deux réglages sont opt-in et leurs valeurs par défaut préservent le comportement des assets existants.

## Nudge déterministe

Le tilt ne choisit pas une pose finale. Il applique uniquement une rotation de faible amplitude autour d'un axe horizontal dont l'azimut est dérivé de `RuntimeObjectId` et de l'identité d'item.

Ainsi :

```text
pose verticale parfaite
→ tilt initial très faible
→ gravité + collision + inertie + friction
→ pose de repos naturelle
```

Deux occurrences ayant la même identité runtime reçoivent la même direction de nudge, ce qui évite un aléatoire non déterministe.

Correction ITEM-PHYSICS01.1 : le premier prototype inclinait seulement l'Actor avant l'activation de Chaos. Sur certains meshes, la coque convexe pouvait ensuite stabiliser à nouveau la pierre sur une petite base. Le nudge est désormais appliqué directement au `MeshComponent` après `SetSimulatePhysics(true)`, et peut lui donner une vitesse angulaire initiale afin de garantir que l'équilibre vertical est réellement rompu.

## Points d'application

Le nudge est appliqué :

- dans `AGridLevelRuntimeActor::AddPlacedItemActor()` pour un item authoré dans le niveau ;
- dans `TryDropItemInstanceAtCell()` pour un dépôt frais depuis l'inventaire ou un chemin runtime équivalent.

Il n'est volontairement pas appelé par `GridLevelRuntimeActorPersistence.cpp`. Une sauvegarde restaure donc la transform persistée sans réincliner l'objet.

## Configuration recommandée : gemme bleue

Dans `DA_Item_BlueGem` :

```text
Weight                                      = 0.10
Use Item Weight As World Physics Mass       = true
World Physics Initial Tilt Degrees           = 5.0
World Physics Initial Angular Speed Degrees  = 120.0
```

Le Static Mesh `SM_Gem_Blue` doit disposer d'une collision simple valide. Comme la gemme est convexe, un seul Convex Hull est recommandé. Ne pas utiliser `Use Complex Collision As Simple` pour ce rigid body simulé.

## Automation

Filtre :

```text
Grimrock.Items.ITEM_PHYSICS01.WorldPickupSettling
```

Le test protège :

- la validité du contrat data-driven ;
- l'application exacte du tilt configuré ;
- son caractère déterministe ;
- le maintien du comportement legacy avec tilt nul ;
- le rejet d'une masse opt-in nulle, d'un tilt hors plage et d'une vitesse angulaire hors plage.
