# MON12.5 — Déplacement du groupe avec PA et PAM

## Résultat

MON12.5 raccorde les six commandes de déplacement du Pawn à l'autorité de
combat de MON12.4.

Pendant le tour d'un personnage :

- une translation d'une cellule coûte `1 PA` au personnage actif ;
- la même translation coûte `1 PAM` à la réserve commune du groupe ;
- le groupe reçoit `2 PAM` au début de chaque manche ;
- une rotation de 90 degrés reste gratuite ;
- aucune commande de déplacement n'est acceptée pendant un tour de monstre ;
- l'exploration hors combat conserve exactement son déplacement historique.

Aucun asset Unreal ou widget UMG n'est ajouté dans cette étape.

## Autorités

Le Pawn conserve l'interpolation visuelle, le head bob, la cellule et
l'orientation. Il ne décide cependant plus seul qu'un mouvement de combat est
possible.

| Responsabilité | Autorité |
| --- | --- |
| tour actif et PA personnels | `UGridTurnManagerComponent` |
| PAM de la manche | `UGridTurnManagerComponent` |
| murs, portes et limites de grille | `AGridLevelRuntimeActor` |
| occupation d'une cellule par un monstre | `UGridMonsterOccupancySubsystem` |
| interpolation de la caméra et du Pawn | `AGrimrockPartyPawn` |

`FGridPartyMobilityState` expose :

- `RoundNumber` ;
- `MaximumMobilityActionPoints` ;
- `RemainingMobilityActionPoints`.

La valeur initiale est configurable par
`BasePartyMobilityActionPointsPerRound`, bornée entre `0` et `4`, avec une
valeur par défaut de `2`.

## Translation autoritaire

`AGrimrockPartyPawn::TryStartMove()` appelle
`RequestPartyTranslation()` uniquement lorsque le combat est actif.

Le TurnManager vérifie, dans l'ordre :

1. son initialisation et la rencontre active ;
2. un tour de personnage et un combattant actif valide ;
3. le repos du groupe et l'absence d'une autre action en cours ;
4. une direction cardinale et une cellule voisine valide ;
5. le passage autorisé par `CanMove()` ;
6. l'absence de monstre dans la cellule cible ;
7. le PA personnel nécessaire ;
8. le PAM commun nécessaire.

Les deux ressources ne sont débitées qu'après la réussite de toutes les
validations. Une requête refusée ne modifie donc ni le PA, ni le PAM, ni la
cellule du groupe.

Raisons de refus principales :

- `NotPlayerTurn` ;
- `NotActiveCombatant` ;
- `PartyBusy` ;
- `InvalidDirection` ;
- `TargetCellUnavailable` ;
- `PassageBlocked` ;
- `TargetCellOccupied` ;
- `InsufficientActionPoints` ;
- `InsufficientMobilityActionPoints`.

`OnPartyMovementRejected` permet à une future interface de traduire ces
raisons en feedback localisé.

## Synchronisation avec l'interpolation

Une translation acceptée réserve immédiatement `1 PA + 1 PAM`, puis le Pawn
démarre son interpolation.

Le TurnManager conserve un mouvement en attente jusqu'à
`NotifyPartyTranslationCompleted()`. Pendant cet intervalle :

- une seconde commande ne peut pas être acceptée ;
- une attaque est refusée comme groupe occupé ;
- le panneau existant est rafraîchi par l'événement de PA et devient
  temporairement indisponible ;
- `Fin du tour` ne peut pas avancer l'initiative.

Si la translation a dépensé le dernier PA, le personnage reste le combattant
actif pendant l'interpolation. Son tour passe à `Completed` uniquement après
l'arrivée exacte sur la nouvelle case. Le monstre ou personnage suivant ne
commence donc jamais pendant que la caméra se trouve entre deux cellules.
Une commande mise en tampon pendant ce dernier déplacement est supprimée si
le combattant actif change ; elle ne peut pas être reportée automatiquement
sur le tour du personnage suivant.

## Rotation gratuite

`RequestPartyRotation()` accepte seulement la rotation gauche ou droite de
90 degrés pendant le tour du personnage actif et lorsque le groupe est au
repos.

Elle coûte `0 PA` et `0 PAM`, y compris lorsque les deux PAM de la manche ont
déjà été utilisés. Elle emploie le même verrou temporaire que la translation
jusqu'à `NotifyPartyRotationCompleted()`.

## Cycle des PAM

La réserve est restaurée à chaque appel autoritaire de début de manche :

```text
manche 1 : 2 / 2 PAM
translation : 1 / 2 PAM
translation : 0 / 2 PAM
manche 2 : 2 / 2 PAM
```

Les PAM inutilisés ne sont pas reportés. Ils sont remis à zéro à la fin ou à
l'abandon du combat. `OnPartyMobilityStateChanged` diffuse chaque restauration
et chaque dépense sans rafraîchissement par `Tick`.

## Journal runtime

Le filtre suivant isole le cycle :

```text
GridPartyMovement
```

Exemples attendus :

```text
[GridPartyMovement] Round=1 PAM=2/2
[GridPartyMovement] Accepted=true Type=Translation ... AP=3/4 PAM=1/2
[GridPartyMovement] Completed Type=Translation ... AP=3/4 PAM=1/2
[GridPartyMovement] Accepted=false ... Reason=InsufficientMobilityActionPoints
[GridPartyMovement] Accepted=true Type=Rotation ... CostAP=0 CostPAM=0
```

## Test automatisé

Filtre :

```text
Grimrock.Monsters.MON12.PartyMobility
```

Test :

```text
Grimrock.Monsters.MON12.PartyMobility.Lifecycle
```

Il contrôle :

- les `2 / 2 PAM` au début de la manche ;
- le refus d'entrer dans une cellule occupée sans consommation ;
- le coût `1 PA + 1 PAM` de deux translations ;
- le refus de la troisième translation à `0 PAM` ;
- la rotation gratuite à `0 PAM` ;
- l'impossibilité de terminer le tour pendant l'interpolation ;
- la restauration des PAM à la manche suivante ;
- la fin automatique du tour seulement après un déplacement payé avec le
  dernier PA ;
- l'absence de report d'une commande tamponnée vers le personnage suivant.

## Validation PIE

1. démarrer un combat et attendre le tour d'un personnage ;
2. vérifier `[GridPartyMovement] Round=1 PAM=2/2` ;
3. avancer ou effectuer un pas latéral et vérifier `PA 3/4`, `PAM 1/2` ;
4. effectuer une seconde translation et vérifier `PA 2/4`, `PAM 0/2` ;
5. tenter une troisième translation et vérifier le refus sans changement de
   case ni de PA ;
6. tourner à gauche ou à droite et vérifier l'absence de coût ;
7. lors d'une autre manche, attaquer pour atteindre `1 PA`, puis effectuer une
   translation ;
8. vérifier que le combattant suivant ne devient actif qu'à la fin de
   l'interpolation ;
9. terminer la manche et vérifier le retour à `PAM 2/2` ;
10. sortir du combat et vérifier que l'exploration reste libre.

## Hors périmètre

- affichage UMG permanent des PAM près des contrôles : MON12.7 ;
- définition orientée données des coûts d'action : MON12.6 ;
- surcharge, immobilisation et bonus de mobilité du groupe ;
- interaction de combat, sorts, capacités et objets rapides.
