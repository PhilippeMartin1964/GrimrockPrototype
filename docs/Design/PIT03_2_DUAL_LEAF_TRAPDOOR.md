# PIT03.2 — Dual-Leaf Pit Trapdoor

Date : 02.09.2026

## Statut

PIT03.2 remplace entièrement l'ancien modèle PIT03.1 à un seul Moving Mesh.

Le modèle mono-volet n'est plus supporté pour une Pit.

## Géométrie de référence

Pour une cellule de 200 x 200 cm :

```text
Volet gauche
  charnière locale = (-85, 0, -5) cm

Volet droit
  charnière locale = (+85, 0, -5) cm

Axe de rotation des deux charnières = Y local
Rotation Unreal correspondante      = Pitch
Angle d'ouverture par défaut         = 80 degrés
Durée par défaut                     = 0,75 s
```

Les deux volets tournent en sens opposés vers le bas :

```text
Closed:
----------------|----------------

Open:
       \        |        /
        \       |       /
         \      |      /

Left  Pitch = -OpenAngle
Right Pitch = +OpenAngle
```

## Composants runtime

`AGridPitTrapdoorActor` contient désormais :

```text
SceneRoot
├─ FixedMeshComponent
├─ LeftTrapdoorHinge
│  └─ LeftTrapdoorLeaf
└─ RightTrapdoorHinge
   └─ RightTrapdoorLeaf
```

L'ancien `MovingMeshComponent` hérité de `AGridMechanismActor` est explicitement neutralisé pour les Pit.

Il n'intervient plus dans leur présentation ni leur collision.

## Meshes

`UGridObjectArchetypeAsset` ajoute quatre champs spécifiques :

```text
Visual > Pit Trapdoor
├─ Left Leaf Mesh
├─ Right Leaf Mesh
├─ Left Leaf Material
└─ Right Leaf Material
```

Une trappe contrôlable exige les deux meshes.

Si aucun volet n'est défini, la Pit est une fosse statique toujours ouverte.

Si un seul volet est défini, l'archetype est invalide et le runtime traite la Pit comme ouverte.

Les anciens champs :

```text
Moving Mesh
Moving Material
```

ne sont plus valides pour `SupportedType = Pit`.

## Convention de modélisation des volets

Les deux meshes peuvent être modélisés avec leur origine au centre géométrique.

Le runtime crée lui-même les pivots de charnière.

Avec les valeurs par défaut :

```text
Left Hinge X  = -85
Right Hinge X = +85
```

le centre du volet gauche est automatiquement décalé de +42,5 cm depuis sa charnière.

Le centre du volet droit est automatiquement décalé de -42,5 cm depuis sa charnière.

Ainsi, si chaque volet mesure environ 85 cm selon X, les deux bords intérieurs se rejoignent autour de X=0 lorsqu'ils sont fermés.

## Paramètres d'instance

`Selected Object > Pit` expose :

```text
Open at Start
Use Same Cell Coordinates

Trapdoor Layout = Dual Leaf / Hinge Axis Y

Left Hinge X = -85
Left Hinge Y = 0
Left Hinge Z = -5

Right Hinge X = +85
Right Hinge Y = 0
Right Hinge Z = -5

Open Angle = 80
Move Duration = 0.75
```

Les anciens champs :

```text
Open Pitch
Open Yaw
Open Roll
```

sont supprimés.

## Animation

À l'alpha `A` :

```text
LeftPitch  = -OpenAngle * A
RightPitch = +OpenAngle * A
```

Donc avec `OpenAngle = 80` :

```text
A = 0.0 -> Left 0°,   Right 0°
A = 0.4 -> Left -32°, Right +32°
A = 1.0 -> Left -80°, Right +80°
```

Les inversions conservent le comportement PIT03.1 :

- aucune remise à zéro ;
- reprise depuis l'angle courant ;
- durée proportionnelle à la fraction restante ;
- changement gameplay uniquement à l'endpoint.

## Collision

Closed :

- collision des deux volets active.

Opening :

- gameplay encore Closed ;
- collision des deux volets reste active.

Open :

- collision des deux volets désactivée ;
- la cellule devient une vraie fosse ;
- PIT01/PIT02 s'appliquent.

Closing :

- gameplay encore Open ;
- collision reste désactivée jusqu'à l'endpoint Closed.

## DA_Pit_Stone_01

Configuration attendue :

```text
Gameplay Type = Pit

Visual
├─ Main Mesh / Preview Mesh = SM_Pit_Stone_01
├─ Fixed Mesh               = SM_Pit_Stone_01
│
└─ Pit Trapdoor
   ├─ Left Leaf Mesh        = <mesh volet gauche>
   ├─ Right Leaf Mesh       = <mesh volet droit>
   ├─ Left Leaf Material    = optionnel
   └─ Right Leaf Material   = optionnel

Runtime
└─ Runtime Actor Class      = GridPitTrapdoorActor

Defaults > Default Behavior > Pit Animation
├─ Left Hinge Location      = (-85, 0, -5)
├─ Right Hinge Location     = (+85, 0, -5)
├─ Open Angle               = 80
└─ Move Duration            = 0.75
```

## Validation

Une Pit produit une erreur si :

- l'ancien `Moving Mesh` est encore renseigné ;
- l'ancien `Moving Material` est encore renseigné ;
- un seul des deux volets est présent ;
- une trappe complète n'utilise pas `GridPitTrapdoorActor` ;
- Open Angle est hors [0,120] ;
- une position de charnière n'est pas finie.

## Automation

Filtre :

`Grimrock.Pit.PIT03_2`

Le test de runtime vérifie :

- présence des deux volets ;
- pivots exactement à -85/+85, Y=0, Z=-5 ;
- rotation opposée -32/+32 à 40 % ;
- rotation finale -80/+80 ;
- Opened uniquement à l'endpoint ;
- chute PIT02 à l'endpoint ;
- reversal depuis l'angle courant ;
- une Pit sans deux volets reste une fosse statique ouverte.
