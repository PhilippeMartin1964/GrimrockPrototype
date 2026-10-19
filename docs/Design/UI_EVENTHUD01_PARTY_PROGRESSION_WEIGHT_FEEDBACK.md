# EVENTHUD01.15 — Progression et surcharge dans le Flux d'événements

Statut : code publié sur `master`; compilation et tests UE5.5.4 à valider localement.

## Progression des personnages

Lorsque l'un des personnages jouables possède au moins un point de compétence,
d'attribut ou de talent à attribuer, une entrée est publiée dans **Événements**
(`EGridHudEventCategory::Dungeon`, sémantique `Information`,
`EventCode=Party.Progression.PointsAvailable`). Le message indique
les personnages concernés et les surfaces d'attribution.

- Première disponibilité, arrivée d'un nouveau personnage concerné ou nouvelle
  montée de niveau pendant qu'il reste des points : entrée immédiate.
- Tant qu'au moins un personnage possède des points : rappel toutes les
  **300 secondes de jeu actif** (environ cinq minutes réelles non mises en pause).
- Le délai est indépendant de l'horloge narrative accélérée `WorldTimeScale`.
- Lorsque plus aucun personnage n'a de points : arrêt des rappels et remise à
  zéro de la cadence. À la prochaine disponibilité, une nouvelle entrée paraît.
- Aucun `Tick` nouveau : le Pawn en possède déjà un. La consultation du
  résumé canonique `GetCharacterSummary()` est échantillonnée toutes les
  ~1 seconde, pas à chaque image.
- Pas de nouveau compteur sauvegardé ni de doublon de calcul RPG :
  `FGridInventoryCharacterSummary::bHasUnspentProgressionPoints` est
  l'autorité de lecture déjà partagée entre portraits, inventaire et combat.

L'initialisation en PIE sur un personnage de niveau 1 avec des points non
attribués peut produire l'entrée dès le début : c'est intentionnel.

## Refus de déplacement pour surcharge

Le verrou `PARTY-WEIGHT01` est inchangé. **Chaque tentative de translation
traitée et refusée en raison du poids** dans `AGrimrockPartyPawn::TryStartMove()`
publie une nouvelle entrée :
`EventCode=Party.Weight.MovementBlocked`, catégorie `Dungeon`,
sémantique `Danger`, visibilité `Always`.
Le message nomme les personnages surchargés et invite à alléger l'inventaire.

Il s'applique en avant, en arrière et en déplacement latéral, en exploration
comme en combat, avant toute dépense de points d'action. Il **ne concerne pas**
les murs, les rotations, ni les déplacements forcés.

Les sollicitations ignorées pendant une animation `bIsMoving`,
`bIsTurning` ou `bIsBlockedMoveFeedbackActive` ne sont pas des tentatives
traitées par le verrou. La logique de buffer d'entrée préexistante reste inchangée.

## Tests

```powershell
cd D:\Development\GrimrockPrototype
git pull --ff-only
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.UI.EventHUD01.ProgressionReminder"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.Runtime.PartyWeight01"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.UI.EventHUD01"
```

Validation PIE : vérifier que le Flux suit son autoscroll actuel, filtrable
par « Événements », que le premier message apparaît avec des points
restants, que le rappel réapparaît après cinq minutes de jeu actif et
cesse après attribution, et que les quatre tentatives de déplacement
surchargé écrivent bien chacune un message. Les tests locaux et le rendu
PIE sont requis avant de déclarer ce jalon validé.
