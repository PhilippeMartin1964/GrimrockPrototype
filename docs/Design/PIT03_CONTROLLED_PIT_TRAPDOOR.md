# PIT03 — Controlled Pit Trapdoor

> **État actuel : PIT03.2 Dual-Leaf Trapdoor.** Le modèle mono-volet PIT03.1 est supprimé.

Date initiale : 01.09.2026  
Mise à jour : 02.09.2026

## Objectif

PIT03 transforme la fosse PIT01/PIT02 en mécanisme contrôlable par les connecteurs existants.

État runtime :

```text
Closed
  = les deux volets sont fermés
  = la cellule se comporte comme un plancher
  = le groupe ne tombe pas
  = les World Items restent dessus

Open
  = les deux volets sont ouverts
  = la cellule se comporte comme une fosse
  = le groupe tombe via PIT01
  = les World Items tombent via PIT02
```

## Commandes

Une Pit reçoit :

- `Open`
- `Close`
- `Toggle`
- `Activate` = Open
- `Deactivate` = Close

Elle émet :

- `Opened`
- `Closed`

## Présentation runtime

`AGridPitTrapdoorActor` porte la présentation.

Le modèle actuel est :

```text
FixedMeshComponent
LeftTrapdoorHinge
  -> LeftTrapdoorLeaf
RightTrapdoorHinge
  -> RightTrapdoorLeaf
```

Le `MovingMeshComponent` générique hérité de `AGridMechanismActor` est neutralisé pour les Pit.

## Géométrie dual-leaf

Valeurs par défaut :

```text
Left Hinge  = (-85, 0, -5) cm
Right Hinge = (+85, 0, -5) cm
Hinge Axis  = local Y
Open Angle  = 80°
Move Duration = 0.75 s
```

À l'ouverture :

```text
Left leaf  Pitch = -80°
Right leaf Pitch = +80°
```

Les deux volets basculent donc en sens opposés vers l'intérieur de la fosse.

## DA_Pit_Stone_01

```text
Gameplay Type            = Pit
Category                 = Hazards
Functional Category      = Mechanism
Placement Kind           = Floor
Blocks Movement          = False
Hide Cell Floor          = True

Visual
├─ Main Mesh / Preview Mesh = SM_Pit_Stone_01
├─ Fixed Mesh               = SM_Pit_Stone_01
└─ Pit Trapdoor
   ├─ Left Leaf Mesh
   ├─ Right Leaf Mesh
   ├─ Left Leaf Material
   └─ Right Leaf Material

Runtime
└─ Runtime Actor Class = GridPitTrapdoorActor

Default Behavior > Pit Animation
├─ Left Hinge Location  = (-85, 0, -5)
├─ Right Hinge Location = (+85, 0, -5)
├─ Open Angle           = 80
└─ Move Duration        = 0.75
```

Les anciens champs Pit à un volet ne sont plus utilisés :

```text
Moving Mesh
Moving Material
Open Relative Rotation
Open Pitch
Open Yaw
Open Roll
```

## Fosse statique

Si aucun couple de volets n'est configuré :

```text
Left Leaf Mesh  = None
Right Leaf Mesh = None
```

la Pit est une fosse statique toujours ouverte.

Un état Closed n'est valide que lorsque les deux volets sont présents.

Un seul volet configuré est une erreur de validation.

## Animation et gameplay

Lors d'une ouverture :

1. la commande Open rend immédiatement la Pit gameplay Open ;
2. les collisions des deux volets sont désactivées immédiatement ;
3. les World Items présents tombent immédiatement via PIT02 ;
4. le groupe présent tombe immédiatement via PIT01 ;
5. `Opened` est émis immédiatement ;
6. les deux volets poursuivent ensuite leur animation visuelle jusqu'à ±OpenAngle.

Lors d'une fermeture, la règle est volontairement asymétrique : la Pit reste gameplay Open et sans collision pendant le mouvement, puis devient Closed et réactive la collision seulement à l'endpoint fermé.

Une inversion reprend depuis l'angle courant sans snap.

## Grid Editor

`Selected Object > Pit` expose :

```text
Open at Start
Use Same Cell Coordinates
Trapdoor Layout = Dual Leaf / Hinge Axis Y
Left Hinge X/Y/Z
Right Hinge X/Y/Z
Open Angle
Move Duration
```

## Tests

Tests de base :

- `Grimrock.Pit.PIT03.ControlledStateAndLinks`
- `Grimrock.Pit.PIT03.PresentationActorState`

Animation dual-leaf :

- filtre `Grimrock.Pit.PIT03_2`

Référence détaillée :

`docs/Design/PIT03_2_DUAL_LEAF_TRAPDOOR.md`
