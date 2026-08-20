# MON17.4.1 — RangedKeeper Preferred Firing Position

Statut : **VALIDÉ ET CLOS sous UE5.5.4**

## Objectif

Faire de `RangedKeeper` un profil réellement distinct de `DirectMelee` en lui permettant de chercher une case de tir plutôt que d'attendre ou de charger au contact.

Cette étape réutilise exclusivement les systèmes existants :

- `FGridMonsterPathfinder` ;
- `UGridMonsterOccupancySubsystem` ;
- `FGridMonsterPerception::HasStraightLineOfSight()` ;
- `FGridMonsterRangedAttackPlanner` ;
- Turn / Move / PA du TurnManager ;
- cooldown d'attaque MON17.3.4.

Aucun second moteur d'IA, aucun Tick décisionnel et aucun déplacement hors grille ne sont ajoutés.

## Contrat Gobelin lanceur

Le profil validé est :

```text
PrimaryAIProfile       = RangedKeeper
PreferredMinDistance   = 3
PreferredMaxDistance   = 5
ActionPointsPerTurn    = 3
```

`Attack_ThrowKnife` reste :

```text
MinRangeCells          = 2
RangeCells             = 6
ActionPointCost        = 2
CooldownTurns          = 0
bRequiresLineOfSight   = true
```

La bande tactique préférée est donc `3..5`, entièrement contenue dans la portée légale `2..6`.

## Politique de décision finale

Pour un `RangedKeeper` qui connaît la cellule actuelle du groupe :

```text
1. case actuelle = tir légal + distance préférée
   -> tirer immédiatement

2. sinon chercher une case axiale de tir dans PreferredMinDistance..PreferredMaxDistance
   -> pathfinding BFS déterministe
   -> se repositionner
   -> si la case est atteinte et qu'il reste assez de PA, tirer dans le même tour

3. aucune case préférée atteignable mais tir actuel légal
   -> tirer depuis la case actuelle en fallback

4. tir actuel illégal
   -> chercher une case axiale dans toute la portée légale de l'attaque

5. aucun plan valide
   -> Wait
```

Ainsi le Gobelin ne choisit plus systématiquement `Wait` lorsqu'il est trop près et ne charge pas au contact comme `DirectMelee`. Il tente d'abord de recréer une distance de tir satisfaisante.

## Construction des cases candidates

`FGridMonsterRangedKeeperPlanner::BuildAxialFiringCandidates()` génère uniquement les quatre rayons cardinaux autour du groupe.

Pour `3..5` :

```text
3 distances x 4 directions = 12 candidats maximum
```

Chaque candidat runtime est ensuite filtré par :

- cellule valide ;
- cellule walkable ;
- occupation / réservation monstre ;
- portée de l'attaque ;
- alignement axial ;
- LOS si `bRequiresLineOfSight=true`.

La recherche de chemin réutilise `FGridMonsterPathfinder::FindPath()` avec les candidats valides comme `Goals`.

## PA et attaque dans le même tour

`FGridMonsterRangedKeeperPlanner::BuildRepositionTurn()` transforme le chemin en actions déterministes.

Avec le Gobelin actuel :

```text
AP disponibles = 3
Move            = 1 AP / cellule
ThrowKnife      = 2 AP
```

Donc :

```text
1 Move vers une case de tir + ThrowKnife = possible dans le même tour
2 Moves vers une case de tir             = déplacement seulement, tir au tour suivant
```

Les `Turn` restent gratuits conformément au TurnManager actuel.

Les actions de déplacement/orientation produites pendant la recherche de position sont marquées `bIsRepositioningAction=true`.

## Cooldown

Le planner ne contourne pas MON17.3.4.

Si toutes les attaques à distance sont momentanément en cooldown :

- le monstre peut encore se repositionner vers sa bande préférée ;
- `RangedAttack` n'est pas ajouté tant que l'attaque est indisponible.

Pour le Gobelin actuel, `CooldownTurns=0`, donc aucune différence de cadence n'est introduite.

## Validation Automation UE5.5.4 — 20 août 2026

Filtre :

```text
Grimrock.Monsters.MON17.4.1
```

Résultat fourni par l'utilisateur : **3/3 Success**.

```text
MultiTurnApproach          Success
PreferredFiringCandidates  Success
RepositionThenAttack       Success
```

### PreferredFiringCandidates

Valide :

- 12 candidats pour la bande `3..5` ;
- génération sur les quatre axes ;
- aucun candidat hors bande préférée.

### RepositionThenAttack

Valide le contrat Gobelin :

```text
3 AP
1 Move
2 AP ThrowKnife
=> Move -> RangedAttack
```

### MultiTurnApproach

Valide :

- un chemin de plusieurs cellules est poursuivi sur plusieurs tours ;
- deux déplacements avec seulement 3 AP ne déclenchent pas un tir prématuré ;
- le monstre peut approcher depuis une position trop lointaine vers une case de tir ;
- un cooldown peut empêcher l'attaque sans empêcher le repositionnement.

Le cas « trop loin » est donc couvert par le contrat Automation du planner et n'a pas été artificiellement répété en PIE.

## Régressions MON17.3 + MON6

La campagne fournie par l'utilisateur est entièrement verte :

```text
MON17.3.1–17.3.4   10/10 Success
MON6                3/3 Success
MON17.4.1            3/3 Success
Total exécuté        16/16 Success
```

Aucune régression automatisée n'est observée sur :

- planner mêlée historique ;
- LOS ;
- stationary ranged planner ;
- timing / trajectoire / optionalité du projectile ;
- `ProjectileSource` ;
- cooldown générique et `CooldownTurns=0` du Gobelin ;
- `CombatResolver` ;
- `DirectMeleePlanner` ;
- `PartyTargetSelector`.

## Validation PIE tactique

### Cas A — distance trop courte — VALIDÉ

Configuration :

```text
Party  = (28,23)
Goblin = (28,24)
Distance initiale = 1
```

Le groupe reste immobile au premier tour.

Observation :

```text
Round 1
- MonsterTurnStarted Gobelin lanceur
- aucune Attack_ThrowKnife
- repositionnement de deux cellules

Round 2
- distance obtenue = 3
- MonsterTurnStarted Gobelin lanceur
- Attack_ThrowKnife
- ProjectileSource utilisé
```

Le log d'attaque de la manche 2 situe l'acteur du Gobelin à `Y=5300` et la cible à `Y=4700`, soit 600 unités = 3 cellules dans cette zone runtime.

Le Gobelin recrée donc correctement sa distance minimale préférée avant d'attaquer.

### Cas C — distance préférée — VALIDÉ

Un PIE ultérieur avec groupe mobile confirme également le tir direct une fois la distance favorable retrouvée.

Séquence observée :

- le groupe poursuit le Gobelin à courte portée ;
- le Gobelin ne déclenche pas `Attack_ThrowKnife` lorsqu'il est au contact ;
- une fois une séparation de 3 cellules retrouvée, il lance immédiatement `Attack_ThrowKnife` ;
- le projectile continue de partir de `ProjectileSource` ;
- Hit/Miss/dégâts restent gérés par le pipeline MON17.3.

Le log de cette attaque donne notamment :

```text
Attack audio location = (5700,4900)
Party target location = (5700,5500)
```

soit 600 unités, donc 3 cellules.

Cette validation démontre que le profil conserve dynamiquement son identité de harceleur à distance lorsqu'un groupe mobile tente de le poursuivre.

## Résultat MON17.4

Le comportement cible de MON17.4 est acquis :

```text
perception
-> évaluer distance / LOS
-> tirer depuis une case favorable
-> sinon rechercher une case de tir
-> maintenir PreferredMinDistance..PreferredMaxDistance
-> reculer/repositionner si le groupe est trop proche
-> approcher si aucune case de tir n'est atteignable depuis la position courante
-> ne jamais charger inutilement au contact comme DirectMelee
```

`MON17.4 — Distinct AI Profile — RangedKeeper` est donc **VALIDÉ ET CLOS sous UE5.5.4**.

## Hors périmètre conservé

- propagation d'alarme et interactions de patrouille : MON17.5 ;
- `FleeAndCallHelp` ArtBook : non implémenté comme second profil parallèle ;
- choix sophistiqué couverture / cul-de-sac / groupe de plusieurs archers ;
- portes scriptées spécifiques au Gobelin ;
- changement de projectile ou de montage MON17.3.

## Porte de sortie — ACQUISE

```text
1. Compilation UE5.5.4                 VALIDÉE
2. Grimrock.Monsters.MON17.4.1         3/3 Success
3. Régressions MON17.3 + MON6          13/13 Success
4. Campagne totale exécutée            16/16 Success
5. PIE distance trop courte            VALIDÉ
6. Approche multi-tour depuis loin     VALIDÉE en Automation
7. PIE distance préférée / groupe mobile VALIDÉ
```

Travail autoritaire suivant :

```text
MON17.5 — Patrol / Perception / Alarm Integration
```
