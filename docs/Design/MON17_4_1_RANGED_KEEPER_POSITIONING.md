# MON17.4.1 — RangedKeeper Preferred Firing Position

Statut : **VALIDÉ EN AUTOMATION UE5.5.4 — régressions et PIE tactique restant avant clôture**

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

Le profil actuel reste :

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

## Politique de décision

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

Ainsi un Gobelin placé trop près ne choisit plus systématiquement `Wait` : il essaie d'abord de reculer vers `3..5`. Un Gobelin trop loin essaie de rejoindre une case de tir. Il ne traverse jamais la cellule du groupe.

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

Pour le Gobelin actuel, `CooldownTurns=0`, donc aucune différence de cadence n'est attendue.

## Tests automatisés

Filtre :

```text
Grimrock.Monsters.MON17.4.1
```

Tests :

```text
PreferredFiringCandidates
RepositionThenAttack
MultiTurnApproach
```

### PreferredFiringCandidates

Vérifie :

- 12 candidats pour la bande `3..5` ;
- génération sur les quatre axes ;
- aucun candidat hors bande préférée.

### RepositionThenAttack

Vérifie le contrat Gobelin :

```text
3 AP
1 Move
2 AP ThrowKnife
=> Move -> RangedAttack
```

### MultiTurnApproach

Vérifie :

- un chemin de deux cellules avec seulement 3 AP produit les deux Moves mais pas un tir prématuré ;
- un cooldown peut empêcher l'attaque sans empêcher le repositionnement.

## Validation Automation UE5.5.4 — 20 août 2026

Résultat fourni par l'utilisateur : **3/3 Success**.

```text
MultiTurnApproach          Success
PreferredFiringCandidates  Success
RepositionThenAttack       Success
```

La logique pure de MON17.4.1 est donc validée : génération des positions préférées, approche multi-tour et séquence `Move + RangedAttack` avec budget PA.

Il reste à vérifier que l'intégration runtime n'a pas régressé les contrats MON17.3/MON6, puis à confirmer le comportement tactique en PIE.

## Validation PIE demandée après régressions

Cas A — trop près :

```text
Party
.
Goblin   distance 1 ou 2
```

Le Gobelin doit chercher à augmenter la distance vers `3..5` au lieu de charger au contact.

Cas B — trop loin :

Le Gobelin hors de portée `>6`, mais ayant connaissance du groupe, doit rejoindre une case de tir légale/préférée au lieu d'attendre indéfiniment.

Cas C — bonne distance :

À distance `3..5`, axial, LOS libre : comportement MON17.3 inchangé, `Attack_ThrowKnife` directement.

## Hors périmètre de MON17.4.1

- propagation d'alarme et interactions de patrouille : MON17.5 ;
- `FleeAndCallHelp` ArtBook : non implémenté comme second profil parallèle ;
- choix sophistiqué couverture / cul-de-sac / groupe de plusieurs archers ;
- portes scriptées spécifiques au Gobelin ;
- changement de projectile ou de montage MON17.3.

## Porte de sortie

État au 20 août 2026 :

```text
1. Compilation UE5.5.4                 VALIDÉE
2. Grimrock.Monsters.MON17.4.1         3/3 Success
3. Régressions MON17.3 + MON6          À VALIDER
4. PIE tactique RangedKeeper           À VALIDER
```

MON17.4.1 sera marqué **VALIDÉ / CLOS** après les régressions et le PIE tactique sans régression du lancer MON17.3.
