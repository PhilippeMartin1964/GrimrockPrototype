# MON17.3.3 — Gobelin lanceur — Présentation de lancer

Statut : **EN COURS — contrat socket/source validé en automation et PIE ; vrai projectile et animation/montage de lancer restants**

## Objectif

Améliorer la présentation de `Attack_ThrowKnife` sans modifier son gameplay : point de départ du projectile depuis un socket/offset authoré, synchronisation visuelle avec le geste de lancer, puis validation avec un vrai mesh de couteau.

## Frontière

MON17.3.3 ne modifie ni portée, LOS, dégâts, coût PA, choix tactique de case, recul ou kiting. Ces règles restent respectivement dans les contrats MON17.3.1/17.3.2 et dans MON17.4 pour `RangedKeeper`.

## État de départ validé

MON17.3.2 est clos : le projectile générique est visible en PIE, le placeholder `SM_Bomb` est lancé avec `Attack_ThrowKnife`, et Hit/Miss/dégâts restent autoritaires côté combat.

L'observation initiale « bombe brièvement visible au centre de l'écran » venait du contrat provisoire MON17.3.2 :

```text
Source = centre des bounds du SkeletalMesh du monstre
Target = position monde du PartyPawn
Travel = 0.20 s
```

MON17.3.3 améliore cette présentation sans changer la résolution combat.

## Contrat C++ ajouté

`FGridMonsterAttackDefinition` possède désormais :

```text
ProjectileSourceSocketName
ProjectileSourceOffset
```

Valeurs par défaut :

```text
ProjectileSourceSocketName = None
ProjectileSourceOffset     = (0,0,0)
```

Ces valeurs préservent le comportement MON17.3.2 des assets existants.

### Résolution runtime

Pour une attaque projectile :

1. si `ProjectileSourceSocketName` est renseigné et existe sur le `USkeletalMeshComponent`, sa transform monde est utilisée ;
2. `ProjectileSourceOffset` est appliqué dans l'espace local de ce socket ;
3. si le socket est `None`, le runtime conserve le centre des bounds comme base et applique l'offset dans l'orientation du mesh ;
4. si un nom de socket est renseigné mais introuvable, un warning est émis et le fallback centre-des-bounds est utilisé ;
5. si aucun SkeletalMeshComponent exploitable n'est disponible, le fallback est la transform de l'Actor.

Log de lancement enrichi :

```text
[GridMonsterProjectile] Launched Monster=... Attack=... Travel=... SourceSocket=... Source=(...) Target=(...)
```

Le gameplay reste inchangé : le socket n'intervient jamais dans portée, LOS, Hit/Miss, dégâts ou PA.

## Test automatisé

Filtre :

```text
Grimrock.Monsters.MON17.3.3
```

Test :

```text
ProjectileSourceContract
```

Il vérifie :

- compatibilité des anciennes définitions avec `Socket=None` ;
- valeurs par défaut ;
- acceptation d'un socket et d'un offset finis ;
- rejet d'un offset non fini.

### Validation locale UE5.5.4

Exécution fournie par l'utilisateur le 19 août 2026 : **1/1 Success**.

```text
ProjectileSourceContract  Success
```

Le contrat C++ MON17.3.3 est donc validé sous UE5.5.4.

## Validation PIE du socket de main

Configuration validée :

```text
Projectile Source Socket Name = ProjectileSource
Projectile Source Offset      = (0,0,0)
Projectile Visual Mesh        = SM_Bomb (placeholder)
Projectile Travel Duration    = 0.20 s
```

Le socket `ProjectileSource` est placé sur la main droite du Gobelin lanceur.

Log PIE représentatif fourni le 19 août 2026 :

```text
[GridMonsterProjectile] Launched Monster=BP_MON_GoblinThrower_C_1 Attack=Attack_ThrowKnife Travel=0.200 SourceSocket=ProjectileSource Source=(5727.2,4904.2,72.3) Target=(5700.0,4500.0,110.0)
```

Validation visuelle fournie par l'utilisateur :

- `SourceSocket=ProjectileSource` est bien utilisé ;
- le projectile ne part plus du centre des bounds ;
- le projectile part visuellement de la paume de la main droite ;
- Hit/Miss et dégâts continuent à être résolus normalement après le lancement.

La partie **source/socket de MON17.3.3 est donc VALIDÉE**.

## Animation / montage — étape restante

Le dépôt ne contient actuellement sous `GoblinThrower/Animation` que :

```text
ABP_MON_GoblinThrower
A_GoblinThrower_Idle
A_GoblinThrower_Walk
```

Le montage/animation de lancer final doit encore être ajouté depuis les assets source dans UE5.5.4, puis référencé par `AttackMontage`.

Le pipeline existant possède déjà les données de temporisation suivantes :

```text
ExpectedDuration
ImpactTimeSeconds
ProjectileTravelDuration
AttackMontage
```

Le projectile visuel est programmé de manière à arriver au moment `ImpactTimeSeconds` :

```text
LaunchDelay = max(0, ImpactTimeSeconds - ProjectileTravelDuration)
```

La prochaine étape doit donc :

1. choisir/importer l'animation de lancer du Gobelin ;
2. créer un montage dédié ;
3. affecter ce montage à `Attack_ThrowKnife.AttackMontage` ;
4. régler `ExpectedDuration`, `ImpactTimeSeconds` et `ProjectileTravelDuration` pour que le projectile quitte la paume au moment visuel du lâcher ;
5. remplacer `SM_Bomb` par le vrai mesh de couteau ;
6. régler `ProjectileVisualScale` et `ProjectileRotationOffset` ;
7. valider en PIE le geste, le départ de la main, le trajet, l'impact et le retour à l'état normal.

MON17.3.3 ne sera clos qu'après cette validation visuelle finale.

## Hors périmètre

- modification de portée ou LOS ;
- dégâts / critique ;
- dépense PA ;
- `RangedKeeper`, recul ou kiting ;
- cooldown runtime, traité en MON17.3.4 ;
- arc balistique ou spin avancé, sauf besoin visuel constaté après le vrai couteau.
