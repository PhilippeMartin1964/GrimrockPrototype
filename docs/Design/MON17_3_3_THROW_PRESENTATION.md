# MON17.3.3 — Gobelin lanceur — Présentation de lancer

Statut : **EN COURS — contrat socket/source C++ posé, compilation et validation UE5.5.4 à faire**

## Objectif

Améliorer la présentation de `Attack_ThrowKnife` sans modifier son gameplay : point de départ du projectile depuis un socket/offset authoré, synchronisation visuelle avec le geste de lancer, puis validation avec un vrai mesh de couteau.

## Frontière

MON17.3.3 ne modifie ni portée, LOS, dégâts, coût PA, choix tactique de case, recul ou kiting. Ces règles restent respectivement dans les contrats MON17.3.1/17.3.2 et dans MON17.4 pour `RangedKeeper`.

## État de départ validé

MON17.3.2 est clos : le projectile générique est visible en PIE, le placeholder `SM_Bomb` est lancé avec `Attack_ThrowKnife`, et Hit/Miss/dégâts restent autoritaires côté combat.

L'observation « bombe brièvement visible au centre de l'écran » vient du contrat provisoire MON17.3.2 :

```text
Source = centre des bounds du SkeletalMesh du monstre
Target = position monde du PartyPawn
Travel = 0.20 s
```

MON17.3.3 doit améliorer cette présentation sans changer la résolution combat.

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

Nouveau filtre :

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

Résultat : **à valider localement sous UE5.5.4**.

## Étape UE5.5.4 après validation C++

Une fois le test vert :

1. ouvrir `SKEL_GoblinThrower` ou `SK_GoblinThrower` ;
2. identifier le bone de la main qui réalise le lancer ;
3. créer un socket nommé par exemple :

```text
ProjectileSource
```

4. placer le socket au niveau du point où le couteau doit quitter la main ;
5. dans `DA_MON_GoblinThrower > Attack_ThrowKnife`, renseigner :

```text
Projectile Source Socket Name = ProjectileSource
Projectile Source Offset      = (0,0,0) au départ
```

6. conserver temporairement `SM_Bomb` si nécessaire pour valider visuellement la nouvelle origine ;
7. vérifier dans le log que `SourceSocket=ProjectileSource` apparaît et que les coordonnées source ont changé par rapport au centre des bounds.

## Animation / montage

Le dépôt ne contient actuellement sous `GoblinThrower/Animation` que :

```text
ABP_MON_GoblinThrower
A_GoblinThrower_Idle
A_GoblinThrower_Walk
```

Le montage/animation de lancer final devra donc être ajouté depuis les assets source dans UE5.5.4, puis référencé par `AttackMontage`.

La synchronisation fine se fera ensuite avec les données déjà existantes :

```text
ExpectedDuration
ImpactTimeSeconds
ProjectileTravelDuration
AttackMontage
```

Le projectile doit quitter la main avant l'impact autoritaire, sans déplacer ce dernier hors du TurnManager.

## Hors périmètre

- modification de portée ou LOS ;
- dégâts / critique ;
- dépense PA ;
- `RangedKeeper`, recul ou kiting ;
- cooldown runtime, traité en MON17.3.4 ;
- arc balistique ou spin avancé, sauf besoin visuel constaté après le vrai couteau.
