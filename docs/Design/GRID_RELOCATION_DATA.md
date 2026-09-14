# Grid Relocation — Runtime et éditeur

Statut : **contrat actif après WORLDOBJ-CLASS01**, 2026-09-14. Version cible : UE 5.5.4.

## Contrat

`EGridLevelObjectType::Relocation` est le Gameplay Type canonique des escaliers,
portails, passages et autres objets qui déplacent automatiquement le groupe. Le groupe
déclenche une relocation en entrant dans la cellule source. Il n'existe aucun chemin Use,
aucun `IsTransition` et aucun `RequireUseAction`.

Une relocation normale est candidate uniquement lorsque :

```text
Object.Type == Relocation
ET TargetCellX >= 0
ET TargetCellY >= 0
```

Des coordonnées de relocation présentes sur un autre Gameplay Type ne déclenchent rien et
constituent une erreur de validation. Une Relocation sans coordonnées reste authorable dans
l'inspecteur, mais ne s'exécute pas et produit une erreur de validation.

Pit reste un Gameplay Type séparé et utilise son chemin de chute dédié.

| Objet | Destination Level = None | Facing = None |
|---|---|---|
| Relocation normale | niveau courant | conserve l'orientation du groupe |
| Pit | niveau inférieur automatique | fallback Pit existant |

## Données

`FGridRelocationBehaviorParams` est l'unique structure de destination :

- `TargetLevelId` ;
- `TargetCellX` ;
- `TargetCellY` ;
- `TargetFacing`.

Elle est portée par `FGridObjectBehaviorParams.Relocation` pour les defaults de définition et
par `FGridWorldObjectInstanceConfig.Relocation` pour la destination du placement. Les deux
coordonnées valent `INDEX_NONE` quand la destination n'est pas configurée.

L'état initial du placement est `bRelocationInitiallyEnabled`, vrai par défaut. Le composant
`UGridActivationComponent` reste l'autorité runtime : une Relocation inactive ne s'exécute
pas, mais sa destination reste validable.

Aucun alias Teleporter, fallback historique, CoreRedirect ou champ deprecated ne fait partie
de ce contrat.

## Éditeur

Le panneau Selected Object affiche **Relocation** pour `Type=Relocation`, même si la destination
est encore vide, et pour Pit avec sa sémantique dédiée. Il contient seulement :

- Destination Level ;
- Destination Cell X ;
- Destination Cell Y ;
- Facing.

Le Game Object affiche **Initially Enabled** pour une Relocation. La définition expose un seul
groupe `Default Behavior > Relocation`.

Les définitions `Stairs_Up` et `Stairs_Down` utilisent :

- `SupportedType = Relocation` ;
- `PlacementSurface = Floor` ;
- `DefaultBehavior.Relocation = None / INDEX_NONE / INDEX_NONE / None` ;
- `RuntimeActorClass = AGridGenericObjectActor` ;
- `bBlocksMovement = false`.

Leur entrée dans `DA_ObjectPalette_Default` porte `PaletteCategory = Navigation`. La catégorie
de palette appartient exclusivement à `FGridObjectPaletteEntry`.

## Runtime

Après la fin d'un déplacement, l'ordre reste : Pit, `HandlePartyCellChanged`, tour de combat,
puis `TryExecuteRelocationAtCell`.

Une relocation vers le niveau courant valide la cellule, met à jour les coordonnées et
l'orientation, appelle `SnapToCurrentCell`, notifie le changement de cellule et vide le buffer
d'entrée. Elle n'appelle ni `TravelToDungeonLevel`, ni `RebuildLevel`, ni la sauvegarde/restauration
d'état. La garde runtime limite chaque entrée à un seul saut.

Une destination vers un autre LevelId utilise `TravelToDungeonLevel`, avec la persistance et
la reconstruction existantes. La destination doit référencer un niveau activé avec un
LevelAsset et une cellule valide, non Empty et non bloquante.

Pour Pit, `TargetLevelId=None` continue de rechercher le niveau inférieur et
`bUseSameCellCoordinates` conserve la priorité.

## Validation

Les diagnostics et la validation utilisent le terme Relocation. `Destination Level=None` et
`Facing=None` sont valides pour une relocation normale. Une Relocation sans X/Y est une erreur.
Des données Relocation sur un Gameplay Type différent sont une erreur. Pour Pit, l'absence de
niveau inférieur résoluble reste une erreur.

Les tests principaux sont `Grimrock.WorldObjects.CLASS01`, `Grimrock.Relocation.RELOC01` et les
suites `Grimrock.Pit.PIT01`, `PIT02` et `PIT03`.
