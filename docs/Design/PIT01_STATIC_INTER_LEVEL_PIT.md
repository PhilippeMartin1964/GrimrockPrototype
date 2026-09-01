# PIT01 — Static Inter-Level Pit

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
   - définir `Target Level Id` ;
   - définir `Target Facing` ;
   - laisser Require Use Action désactivé ;
6. lancer Validation.

Si Same Cell Coordinates est désactivé, saisir aussi Target Cell X/Y.

## Validation

Le Grid Editor vérifie notamment :

- présence d'un TargetLevelId valide ;
- TargetFacing différent de None ;
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
