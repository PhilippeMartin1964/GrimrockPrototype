# RPG03.5 — Tactical / Forced Movement (C5)

Date : **5 octobre 2026**  
Parent : `RPG_Talents_Mechanics_v0_1.md`  
Dépendance : **RPG03.4 validé 8/8**  
Statut : **VALIDÉ — TD04.2 / Grimrock.RPG.RPG03.5 / 8 réussis / 0 warning / 0 échec / 5 octobre 2026**

## Objectif

RPG03.5 ajoute une primitive unique de déplacement tactique et forcé qui réutilise la grille, la géométrie, l'occupation dynamique et l'interpolation existantes.

Il n'existe pas de second moteur de mouvement.

## Profil C5

`FGridCombatMovementEffectProfile` décrit :

- `Subject = PartyGroup | TargetCombatant` ;
- `Direction = ForwardFromFacing | BackwardFromFacing | LeftFromFacing | RightFromFacing | AwayFromSource` ;
- `DistanceCells` ;
- `MobilityActionPointCost` ;
- `bForced`.

`PartyGroup` représente le groupe entier par son ancre de cellule. La formation 2×3 ne change pas lorsqu'on translate cette ancre.

Un déplacement de cible ne peut pas consommer les PAM du groupe.

## Resolver de grille

`FGridCombatMovementResolver` calcule une direction canonique puis avance cellule par cellule via :

```text
TryGetNeighborCell()
CanMove()
UGridMonsterOccupancySubsystem
```

Même un déplacement forcé ne traverse donc jamais :

- un mur ou passage bloqué ;
- une cellule hors niveau ;
- une cellule occupée par un monstre.

`bForced` signifie uniquement que les restrictions volontaires du sujet, comme `bBlockTranslation`, ne peuvent pas empêcher le déplacement.

## Retraite tactique

La mécanique RPG02 :

```text
Action_Ranger_TacticalRetreat
1 PA + 1 PAM
groupe entier
1 cellule en arrière
Cooldown 3
```

est désormais représentable directement par :

```text
ResolutionProfile = Effect
TargetingPolicy    = Self
ActionPointCost    = 1

MovementEffect:
    Subject                  = PartyGroup
    Direction                = BackwardFromFacing
    DistanceCells            = 1
    MobilityActionPointCost  = 1
    bForced                  = false
```

L'action paie son PA via le pipeline de combat normal et le profil C5 paie uniquement son PAM.

Elle ne passe surtout pas par `RequestPartyTranslation()`, ce qui aurait payé une seconde fois le PA de mouvement.

## Interpolation autorisée

`AGrimrockPartyPawn::BeginAuthorizedGridTranslation()` démarre seulement une translation déjà validée et payée par le TurnManager.

Le Pawn réutilise ensuite les mêmes états :

```text
MoveStartLocation
MoveTargetLocation
MoveElapsed
bIsMoving
CurrentCellX/Y
ActiveMoveDirection
```

La fin du mouvement continue donc de passer par le chemin de translation existant et les callbacks de changement de cellule.

Il ne s'agit pas d'une téléportation instantanée.

## Transaction PA/PAM

Pour une action C5 de groupe :

1. préflight spatial et contrôle ;
2. vérification du PAM ;
3. dépense du PA de l'action ;
4. commit du PAM ;
5. démarrage de l'interpolation ;
6. résolution des autres effets éventuels ;
7. fin de tour seulement après la fin de l'interpolation si les PA sont épuisés.

Si le préflight échoue, aucun PA ni PAM n'est dépensé.

Si le Pawn refuse exceptionnellement le démarrage après paiement du PA, le TurnState est restauré et le PAM n'est pas conservé.

## Immobilize, surcharge et forced movement

Un déplacement volontaire C5 respecte :

- `StatusEffectControl.bBlockTranslation` ;
- la surcharge d'inventaire du groupe.

Un profil `bForced=true` peut contourner ces deux restrictions.

Il ne contourne jamais la géométrie ni l'occupation.

## Catalogue

Le catalogue comprend maintenant le coût `MobilityActionPointCost`.

Une action C5 visible mais impossible faute de PAM reçoit :

```text
InsufficientMobilityActionPoints
```

Le blocage géométrique reste évalué au moment de la requête autoritaire, afin de ne pas dupliquer la géométrie dans le catalogue pur.

## Déplacements forcés futurs

Le resolver sait déjà calculer :

```text
AwayFromSource
```

pour les effets comme :

```text
Turn Undead
→ push 1 cell if possible
```

L'exécution sur une cible hostile attend volontairement C8, qui doit fournir le ciblage/batch unique pour les Effects ciblés.

RPG03.5 n'ajoute donc aucun exécuteur temporaire spécifique à `TurnUndead`.

## Limites volontaires

- Le chemin runtime PartyGroup exécuté par ce jalon accepte une translation adjacente d'une cellule, correspondant à Tactical Retreat.
- Les déplacements de plusieurs cellules / sélection de cellule, comme Short Teleport, restent C5+C8.
- Les déplacements forcés de monstres utilisent déjà le même resolver mais attendent le ciblage C8 pour leur exécution.
- Les pièges utilisent les callbacks de changement de cellule existants ; leur authoring/exécution spécifique reste lié à C4/C5 et au futur authoring RPG03.9.
- Aucun `.uasset` n'est modifié.

## Automation ajoutée

Filtre :

```text
Grimrock.RPG.RPG03.5
```

Tests :

1. `ProfileValidation`
2. `DirectionResolution`
3. `DestinationResolution`
4. `BlockedGeometry`
5. `CatalogMobilityGate`
6. `ForcedControlBypass`
7. `TacticalRetreatExecution`
8. `RejectedRetreatNoSpend`

Validation utilisateur reçue le 5 octobre 2026 : 8 tests réussis, 0 warning, 0 échec, 0 not run, process exit code 0.
