# MON17.3.3 — Gobelin lanceur — Présentation de lancer

Statut : **VALIDÉ / CLOS sous UE5.5.4**

## Objectif

Finaliser la présentation de `Attack_ThrowKnife` sans modifier son gameplay : animation de lancer retargetée, montage runtime, départ du projectile depuis la paume droite et synchronisation visuelle avec l'impact autoritaire du combat.

## Frontière

MON17.3.3 ne modifie ni portée, LOS, dégâts, coût PA, choix tactique de case, recul ou kiting. Ces règles restent respectivement dans MON17.3.1/17.3.2 et MON17.4 pour `RangedKeeper`.

La résolution Hit/Miss/dégâts reste autoritaire côté TurnManager / combat. Le projectile reste strictement presentation-only.

## Contrat C++ validé

`FGridMonsterAttackDefinition` possède :

```text
ProjectileSourceSocketName
ProjectileSourceOffset
```

Valeurs par défaut :

```text
ProjectileSourceSocketName = None
ProjectileSourceOffset     = (0,0,0)
```

Le runtime :

1. utilise le socket authoré s'il existe sur le `USkeletalMeshComponent` ;
2. applique `ProjectileSourceOffset` dans l'espace local du socket ;
3. conserve le fallback centre-des-bounds si aucun socket n'est configuré ;
4. émet un warning puis utilise le fallback si le socket configuré n'existe pas ;
5. ne fait jamais dépendre portée, LOS, Hit/Miss, dégâts ou PA de ce socket.

## Validation automatisée

Filtre :

```text
Grimrock.Monsters.MON17.3.3
```

Test :

```text
ProjectileSourceContract
```

Résultat local UE5.5.4 :

```text
ProjectileSourceContract  Success
```

Soit **1/1 Success**.

Le test couvre :

- compatibilité des anciennes définitions avec `Socket=None` ;
- valeurs par défaut ;
- acceptation d'un socket et d'un offset finis ;
- rejet d'un offset non fini.

## Socket de projectile

Le socket :

```text
ProjectileSource
```

est placé sur la paume de la main droite du Gobelin lanceur.

Configuration validée :

```text
Projectile Source Socket Name = ProjectileSource
Projectile Source Offset      = (0,0,0)
```

Log PIE représentatif :

```text
[GridMonsterProjectile] Launched Monster=BP_MON_GoblinThrower_C_1 Attack=Attack_ThrowKnife Travel=0.200 SourceSocket=ProjectileSource Source=(5727.2,4904.2,72.3) Target=(5700.0,4500.0,110.0)
```

Validation visuelle acquise :

- `SourceSocket=ProjectileSource` est bien utilisé ;
- le projectile part de la paume de la main droite ;
- le projectile ne part plus du centre des bounds ;
- Hit/Miss et dégâts restent indépendants de la présentation.

## Animation de lancer — Mixamo / IK Retargeter

Le pack source `Goblin_Bomber` ne fournissant aucune animation de lancer exploitable, l'animation `Throw` de Mixamo a été utilisée puis retargetée sous UE5.5.4.

Pipeline validé :

```text
Import source Mixamo
        ↓
IK_Mixamo
        ↓
IK_GoblinThrower
        ↓
RTG_Mixamo_To_GoblinThrower
        ↓
Retarget Pose Goblin_Mixamo_TPose
        ↓
A_GoblinThrower_ThrowKnife
        ↓
AM_GoblinThrower_ThrowKnife
        ↓
Slot montage dans ABP_MON_GoblinThrower
        ↓
DA_MON_GoblinThrower.Attack_ThrowKnife.AttackMontage
```

Éléments validés :

- import source Mixamo ;
- `IK_Mixamo` ;
- `IK_GoblinThrower` ;
- `RTG_Mixamo_To_GoblinThrower` ;
- Retarget Pose `Goblin_Mixamo_TPose` ;
- `A_GoblinThrower_ThrowKnife` ;
- `AM_GoblinThrower_ThrowKnife` ;
- Slot montage dans `ABP_MON_GoblinThrower` ;
- `Attack_ThrowKnife.AttackMontage` affecté ;
- animation complète visible en PIE ;
- pas de dépendance de gameplay au squelette Mixamo ;
- déplacement du Gobelin toujours grid-authoritative / in-place.

## Synchronisation finale validée

Réglages :

```text
ExpectedDuration          = 2.20 s
ImpactTimeSeconds         = 1.00 s
ProjectileTravelDuration  = 0.20 s
```

Le runtime calcule :

```text
LaunchDelay = max(0, ImpactTimeSeconds - ProjectileTravelDuration)
            = 1.00 - 0.20
            = 0.80 s
```

Chronologie validée :

```text
t=0.00  début du montage de lancer
t≈0.80  lancement visuel depuis ProjectileSource / paume droite
t≈1.00  arrivée visuelle prévue + impact combat autoritaire
t≈2.20  fin de la présentation / action
```

La synchronisation visuelle a été jugée satisfaisante en PIE.

## Résultat final MON17.3.3

MON17.3.3 est **VALIDÉ / CLOS** avec :

```text
A_GoblinThrower_ThrowKnife
AM_GoblinThrower_ThrowKnife
ProjectileSource
ExpectedDuration          = 2.20 s
ImpactTimeSeconds         = 1.00 s
ProjectileTravelDuration  = 0.20 s
LaunchDelay               ≈ 0.80 s
```

Le Gobelin lanceur dispose maintenant d'une présentation complète et distincte du Rat Géant : geste de lancer, montage, projectile lancé depuis la main droite et résolution combat toujours séparée de la présentation.

## Étape suivante

MON17.3.4 doit traiter uniquement ce qui reste au jalon `Distinct Attack Set` :

- contrat runtime de `CooldownTurns` ;
- tests avec cooldown non nul synthétique ;
- régressions MON17.1 / MON17.2 / MON17.3 ;
- vérification des pipelines de combat antérieurs pertinents ;
- clôture de MON17.3.

Pour `Attack_ThrowKnife`, la valeur actuelle reste :

```text
CooldownTurns = 0
```

Le cooldown ne doit donc pas modifier le comportement actuel du Gobelin tant qu'une valeur non nulle n'est pas authorée.

## Hors périmètre transféré

- `RangedKeeper`, recherche de distance, recul et kiting : MON17.4 ;
- Patrol / Perception / Alarm : MON17.5 ;
- Encounter / Loot / XP : MON17.6 ;
- équilibrage final MON17 : MON17.7.
