# PIT01 — Static Inter-Level Pit

> **Ergonomie Grid Editor (02.09.2026).** Dans `Selected Object > Transition`, une Pit affiche toujours sa destination. Lorsque `Use Same Cell Coordinates = True`, `Target Cell X` et `Target Cell Y` sont désactivés car ils sont ignorés par le runtime ; l'inspecteur affiche explicitement la cellule effective `(Pit.CellX, Pit.CellY)`. Dès que l'option est décochée, X/Y redeviennent éditables. Pour une Pit, l'ancien flag générique `Is Transition` est remplacé visuellement par `Transition Mode = Intrinsic Pit Fall`.


> **Correction d’atterrissage (01.09.2026).** Si la cellule exactement sous la Pit est hors limites, `Empty` ou bloque l’occupation, la chute n’est plus annulée. Le runtime cherche automatiquement la cellule praticable la plus proche sur le niveau inférieur, de façon déterministe, et utilise cette cellule comme point d’atterrissage. Une destination contenant directement une autre Pit ouverte reste refusée tant que les chutes en cascade ne sont pas implémentées.


> **Correction runtime (01.09.2026).** Une fosse statique sans `Moving Mesh` est toujours physiquement ouverte. Le runtime reconnaît aussi une Pit par son archetype si le `Type` stocké dans un ancien objet placé est obsolète, et un GUID invalide n'empêche plus la chute statique. À la fin d'un déplacement, la détection Pit est prioritaire sur les triggers, plaques, TurnManager et transitions ordinaires.


> **Correction de contrat (01.09.2026).** Une Pit ouverte est intrinsèquement une cellule de chute. Elle ne dépend plus de `Transition.bIsTransition`, de `bRequireUseAction`, ni d'un `Target Level Id` manuel pour fonctionner. Avec `Target Level Id = None`, le niveau inférieur est résolu automatiquement.


> **Évolution PIT03 (01.09.2026).** Le booléen `bInitiallyOpen` n'est plus l'autorité permanente après le démarrage. L'état Open/Closed est désormais persisté par `FGridRuntimePitState` et peut être commandé par les connecteurs. Le reste du contrat PIT01 (destination et chute du groupe) reste valide.


Date : 01.09.2026

## Objectif

PIT01 introduit une fosse statique occupant une cellule entière. Lorsqu'un groupe termine un déplacement sur une fosse ouverte, une courte présentation de chute est jouée, puis le groupe est transféré vers un autre niveau du même `UGridDungeonAsset`.

PIT01 réutilise l'infrastructure multi-niveaux existante (`TravelToDungeonLevel()`) et ne simule pas physiquement le Pawn.

## Données

Un nouveau type de GridObject `Pit` est ajouté à la fin de `EGridLevelObjectType` afin de ne pas renuméroter les valeurs sérialisées existantes.

`FGridPitBehaviorParams` porte :

- `bInitiallyOpen` : état initial de la fosse ;
- `bUseSameCellCoordinates` : utilise les X/Y de la fosse comme destination dans le niveau cible.

La destination inter-niveaux reste portée par `FGridObjectTransitionParams` :

- `TargetLevelId` ;
- `TargetCellX/Y` lorsque Same Cell Coordinates est désactivé ;
- `TargetFacing` ;
- `bRequireUseAction=false` obligatoire.

## Archétype Stone Pit

Le Grid Editor provisionne automatiquement, si `SM_Pit_Stone_01` existe :

`/Game/GrimrockPrototype/Core/DataAssets/GridObjectArchetypeAsset/DA_Pit_Stone_01`

avec :

- ArchetypeId : `Pit_Stone_01` ;
- SupportedType : `Pit` ;
- Palette Category : `Hazards` ;
- PlacementKind : `Floor` ;
- PreviewMesh : `SM_Pit_Stone_01` ;
- `bHideCellFloor=true` ;
- `bBlocksMovement=false` ;
- `bInitiallyOpen=true` ;
- `bUseSameCellCoordinates=true`.

La cellule reste logiquement walkable : le groupe entre réellement dessus avant que la chute ne se déclenche.

## Mesh `SM_Pit_Stone_01`

Le mesh suit le pipeline Static Mesh standard du projet :

```text
docs/05_Static_Mesh_Blender_UE5_5_4_Pipeline.md
```

Contrôles spécifiques :

- empreinte X/Y d'environ 200 x 200 cm ;
- pivot cohérent avec les meshes de sol ;
- surface supérieure alignée avec les sols voisins ;
- scale 1 / 1 / 1 dans UE ;
- pas de collision convexe automatique qui boucherait l'ouverture.

Sans UCX explicites conçus autour du trou, ne pas importer de collision de fallback pour cette fosse. La chute PIT01 est une mécanique logique de cellule et ne dépend pas d'une simulation physique du trou.

## Flux runtime

```text
déplacement accepté
  -> interpolation jusqu'au centre de la cellule
  -> HandlePartyCellChanged()
  -> TryBeginPitFallAtCell()
  -> validation destination
  -> BeginPitFall()
  -> cri de terreur + descente visuelle contrôlée
  -> TravelToDungeonLevel()
  -> SetGridStart() sur le niveau cible
```

Les transitions génériques d'escalier ignorent explicitement les objets `Pit`, afin qu'une fosse ne puisse jamais contourner la présentation de chute.

PIT01 refuse une destination contenant déjà une fosse ouverte. Les chutes en cascade ne sont pas supportées dans ce jalon.

## Présentation du groupe

`AGrimrockPartyPawn` expose :

- `PitFallDuration` (défaut 0.65 s) ;
- `PitFallDistance` (défaut 220 cm) ;
- `PitFallScreamSounds` ;
- `PitFallScreamVolume`.

Pendant la chute, déplacement, rotation, free-look, hotbar, inventaire, clics monde et lancement de projectile sont bloqués.

Dans `BP_GrimrockPartyPawn`, renseigner un ou plusieurs sons dans :

`Movement > Pit Fall > Pit Fall Scream Sounds`

Le choix utilise la même infrastructure audio déterministe que les sons de déplacement.

## Authoring dans le Grid Editor

1. ouvrir la palette ;
2. choisir `Hazards > Stone Pit` ;
3. placer la fosse sur une cellule Floor ;
4. dans Selected Object > Pit :
   - Open at Start = true ;
   - Use Same Cell Coordinates = true dans le cas standard ;
5. dans Transition :
   - laisser `Target Level Id = None` pour une fosse standard : le runtime résout automatiquement le niveau inférieur ;
   - un `Target Level Id` explicite valide reste possible pour une destination spéciale ;
   - `Target Facing = None` conserve désormais l'orientation actuelle du groupe ;
6. lancer Validation.

La résolution automatique cherche d'abord le niveau compatible immédiatement plus bas selon `LogicalPosition.Z`. Si les anciennes données n'ont pas encore de Z exploitable, le niveau activé suivant dans la liste du `DungeonAsset` sert de fallback prototype.

Si Same Cell Coordinates est désactivé, saisir aussi Target Cell X/Y.

## Validation

Le Grid Editor vérifie notamment :

- destination inférieure automatique résoluble, ou TargetLevelId explicite valide ;
- destination dans les limites ;
- transition Pit automatique ;
- destination ne contenant pas une autre fosse initialement ouverte.

Filtre automatisé :

`Grimrock.Pit.PIT01`

## Hors périmètre

PIT01 ne gère pas encore :

- objets/monstres tombant dans une fosse ;
- dégâts de chute ;
- chaîne de plusieurs fosses ;
- trappe ouvrable/fermable ;
- activation par bouton, levier, plaque ou script.

Jalons prévus :

- **PIT02 — World Items Falling Through Pits** ;
- **PIT03 — Controlled Pit Trapdoor**.
