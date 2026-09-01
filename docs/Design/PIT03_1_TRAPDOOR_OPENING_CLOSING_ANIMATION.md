# PIT03.1 — Trapdoor Opening / Closing Animation

Date : 01.09.2026

## Objectif

PIT03.1 ajoute une animation réversible au couvercle optionnel d'une Pit contrôlée.

```text
Fixed Mesh  = géométrie permanente de la fosse
Moving Mesh = couvercle / trappe
Closed      = rotation relative (0,0,0)
Open        = Open Relative Rotation
```

## Autorité gameplay

PIT03.1 distingue le target state demandé et le dernier endpoint gameplay stabilisé.

```text
Closed -> Open command -> Opening
    gameplay = Closed
Open endpoint
    gameplay = Open
    World Items fall
    Party falls if present
    Opened event emitted
```

La fermeture applique la règle symétrique : le gameplay reste Open pendant le mouvement puis devient Closed à l'endpoint.

## Reversal

Une commande opposée repart de la fraction courante. La durée effective est proportionnelle à la distance restante.

```text
Closed -> Open -> 40 % -> Close
Close repart de 40 % et prend 40 % de Move Duration.
```

Aucun snap n'est effectué lors d'une inversion.

## Paramètres exacts

Dans `DA_Pit_Stone_01` :

```text
Defaults
└─ Default Behavior
   ├─ Pit
   │  ├─ Initially Open             = selon le niveau
   │  └─ Use Same Cell Coordinates = True généralement
   │
   └─ Pit Animation
      ├─ Open Relative Rotation
      │  ├─ Pitch = -90.0
      │  ├─ Yaw   =   0.0
      │  └─ Roll  =   0.0
      └─ Move Duration = 0.75
```

Dans `Visual` :

```text
Main Mesh / Preview Mesh = SM_Pit_Stone_01
Fixed Mesh               = SM_Pit_Stone_01
Moving Mesh              = <mesh du couvercle>
Fixed Material           = None ou matériau de la fosse
Moving Material          = None ou matériau du couvercle
```

Dans `Runtime` :

```text
Runtime Actor Class = GridPitTrapdoorActor
```

### Pivot du Moving Mesh

`Open Relative Rotation` est appliqué autour du pivot du Static Mesh. Le mesh du couvercle doit donc être exporté depuis Blender avec son origine/pivot placé sur la charnière voulue.

La valeur par défaut `Pitch = -90` suppose que l'orientation locale du mesh utilise cet axe pour l'ouverture. Sinon, régler Yaw ou Roll sans modifier le C++.

## Selected Object

Le panneau `Selected Object > Pit` expose désormais :

```text
Open at Start
Use Same Cell Coordinates
Open Pitch
Open Yaw
Open Roll
Move Duration
```

Ces valeurs sont des données d'instance copiées depuis l'archetype au placement.

## Collision

- gameplay Closed : collision du couvercle active ;
- gameplay Open : collision du couvercle désactivée ;
- pendant l'ouverture : la collision reste Closed jusqu'à l'endpoint ;
- pendant la fermeture : elle reste Open jusqu'à l'endpoint.

## Audio

Les événements génériques `Open` et `Close` restent optionnels. Une inversion arrête le son précédent et redémarre le son opposé au point de timeline correspondant à la fraction mécanique actuelle.

## Niveau déchargé pendant une animation

La fraction intermédiaire n'est pas sérialisée. Le target endpoint est déjà persisté dans `FGridRuntimePitState::bIsOpen`. Si le niveau est quitté en cours de mouvement, le retour restaure directement cet endpoint.

## Absence de Moving Mesh

`Moving Mesh = None` signifie désormais **fosse statique ouverte** : elle fait tomber le groupe et les World Items, et ne peut pas être fermée. Pour tester un état Closed/Open contrôlable, un vrai `Moving Mesh` de couvercle doit être assigné.

## Tests

Filtre : `Grimrock.Pit.PIT03_1`

- `Grimrock.Pit.PIT03_1.AnimationRuntimeContract`
- `Grimrock.Pit.PIT03_1.NoCoverImmediateFallback`

Le contrat vérifie l'ouverture retardée côté gameplay, le délai de l'événement `Opened`, la chute PIT02 seulement à l'endpoint, l'inversion depuis l'alpha courant, Toggle sur le target state et le fallback immédiat sans Moving Mesh.

## Hors périmètre

- mesh artistique final du couvercle ;
- dégâts de chute ;
- monstres tombant ;
- écrasement par fermeture ;
- double battant ;
- puits multi-étages.
