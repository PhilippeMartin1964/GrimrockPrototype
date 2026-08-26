# TD02.9 — Party Movement Façade & PartyPawn Stop Condition

Date : 26 août 2026  
Projet : GrimrockPrototype — UE 5.5.4  
Statut : **TERMINÉ / VALIDÉ UE5.5.4**

## Objectif

TD02.9 clôt la séquence de réduction ciblée de `AGrimrockPartyPawn` en isolant la responsabilité cohérente de déplacement case par case et de rotation, puis en appliquant explicitement la stop condition de la campagne de dette technique.

La règle reste la même que pour TD02.4 à TD02.8 : préserver l'API publique, l'état et les autorités existantes ; ne pas créer de nouvelle classe propriétaire ; ne pas modifier SaveGame ni les assets Unreal.

## Caractérisation avant extraction

Deux contrats ont été utilisés comme baseline avant modification structurelle.

```text
Grimrock.TechnicalDebt.TD02_9.PartyMovementFacade.GridStartContract  Success
Grimrock.Monsters.MON12.PartyMobility.Lifecycle                       Success
```

`GridStartContract` verrouille :

- `SetGridStart()` ;
- `SnapToCurrentCell()` ;
- conservation du `LevelRuntimeActor` fourni ;
- coordonnées logiques X/Y ;
- `Facing` ;
- position monde au centre de cellule ;
- yaw issu de `GridDirectionUtils::ToYaw()` ;
- comportement non destructif de `SnapToCurrentCell()` sans runtime.

`PartyMobility.Lifecycle` verrouille notamment :

- refus d'une cellule occupée ;
- coût AP/PAM des translations ;
- interpolation de déplacement ;
- notification de fin de translation ;
- épuisement et restauration des PAM par round ;
- rotations gauche/droite gratuites ;
- convention de facing ;
- blocage de fin de tour pendant une interpolation ;
- changement de personnage actif après consommation du dernier AP ;
- purge d'une commande bufferisée lorsqu'elle ne doit pas traverser un changement de tour.

## Extraction

Commit d'extraction :

```text
af3cc82f4ac3156a820d21911be565c04a3bce14
Extract TD02.9 party movement facade
```

Nouveau fichier :

```text
Source/GrimrockPrototype/Private/Runtime/GrimrockPartyPawnMovement.cpp
```

Méthodes déplacées :

```text
SetGridStart
SnapToCurrentCell
HandleMoveForward
HandleMoveBackward
HandleTurnLeft
HandleTurnRight
HandleStrafeLeft
HandleStrafeRight
TryStartMove
TryStartTurn
UpdateMove
UpdateTurn
```

Diff structurel :

```text
GrimrockPartyPawn.cpp          0 ajout / 287 suppressions
GrimrockPartyPawnMovement.cpp 293 ajouts / 0 suppression
```

Aucun changement de :

- `GrimrockPartyPawn.h` ;
- état runtime ;
- autorité de `UGridTurnManagerComponent` ;
- API Blueprint ;
- SaveGame ;
- `.uasset` / `.umap` ;
- `AGrimrockPlayerController`.

Restent volontairement hors de l'extraction :

```text
HandleUse
TryUseFrontInteraction
HasLevelRuntimeActor
CanMoveOnLevel
TryGetNeighborOnLevel
GetCellCenterOnLevel
TryToggleDoorOnLevel
DismissReadableMessageIfVisible
FindTurnManager
Camera / Head Bob / Free Look
```

## Validation après extraction

Les deux mêmes contrats ont été rejoués sous UE5.5.4 et sont restés verts :

```text
Grimrock.TechnicalDebt.TD02_9.PartyMovementFacade.GridStartContract  Success
Grimrock.Monsters.MON12.PartyMobility.Lifecycle                       Success
```

Les logs de validation confirment notamment :

- refus `TargetCellOccupied` sans dépense ;
- deux translations à 1 AP + 1 PAM ;
- refus `InsufficientMobilityActionPoints` sans dépense ;
- rotations North -> West -> North à coût nul ;
- round 2 restaurant les PAM ;
- translation avec dernier AP puis passage au personnage suivant.

## Stop condition PartyPawn

Après TD02.4 à TD02.9, les responsabilités volumineuses et clairement séparables du Pawn sont désormais réparties ainsi :

```text
GrimrockPartyPawnInputBuffer.cpp   — buffer d'input
GrimrockPartyPawnHeldItem.cpp      — présentation held item / lumière
GrimrockPartyPawnSave.cpp          — façade Save / Load
GrimrockPartyPawnItemTransfer.cpp  — transferts cursor / équipement / monde
GrimrockPartyPawnUI.cpp            — menu / hotbar / création de personnage
GrimrockPartyPawnMovement.cpp      — translation / rotation
```

Les responsabilités principales conservées dans `GrimrockPartyPawn.cpp` sont désormais :

- construction et cycle de vie du Pawn ;
- orchestration de startup ;
- binding des inputs ;
- interaction `Use` ;
- façade légère vers le runtime/inventaire ;
- caméra, head bob et free look ;
- diagnostics/façades mineurs.

Aucune de ces zones ne justifie actuellement une extraction supplémentaire :

- déplacer la caméra dans un autre `.cpp` diminuerait le nombre de lignes sans réduire le couplage ;
- les petites façades niveau/inventaire sont déjà minces ;
- le startup et les bindings appartiennent naturellement au Pawn ;
- l'interaction souris reste portée par le PlayerController et délègue les transactions aux autorités existantes.

**Décision : arrêter ici la décomposition de `AGrimrockPartyPawn`.**

`TD-ARCH-003` reste une dette structurelle surveillée, mais ne justifie plus une tranche immédiate. Toute nouvelle extraction devra être motivée par une douleur concrète rencontrée en développement ou en playtest.

## Audit de frontière Pawn / PlayerController

L'état courant montre une séparation cohérente :

### `AGrimrockPartyPawn`

- état logique du groupe et position/facing ;
- façades de transactions item ;
- déplacement/rotation ;
- UI propre au Pawn ;
- interaction clavier `Use`.

### `AGrimrockPlayerController`

- état du pointeur et du curseur ;
- résolution de l'intention du clic gauche ;
- hover/interactable sous curseur ;
- ciblage de combat à la souris ;
- feedback de pointage ;
- délégation vers Pawn / Inventory / Runtime / HUD.

Le Controller est encore volumineux, mais aucune duplication d'autorité nécessitant une correction immédiate n'a été identifiée. Le découper isolément en unités de traduction serait aujourd'hui principalement cosmétique.

**Décision : ne pas engager de refactor `AGrimrockPlayerController` dans la continuité de TD02.9.**

## Suite recommandée

La prochaine dette P2 avec un coût concret et déjà documenté est :

```text
TD03 — Grid Editor Slate / legacy Details cleanup
       -> TD-EDITOR-001
```

Cette dette est plus rentable parce qu'il existe une duplication réelle d'authoring entre le Grid Editor Mode canonique et des contrôles historiques `CallInEditor` dans les Details. La prochaine étape doit commencer par l'audit existant `docs/Design/GRID_EDITOR_ACTOR_UI_AUDIT.md`, puis caractériser l'équivalence avant de masquer ou supprimer tout contrôle historique.

MON21.2 reste suspendu pendant la campagne de stabilisation.
