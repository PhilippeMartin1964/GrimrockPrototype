# 12 — Règle Definition / Instance des objets du monde

Statut : **document actif de référence**  
Date : 2026-09-07  
Projet : GrimrockPrototype — WORLDOBJ

## 1. Décision officielle

L’ancienne règle « la définition est copiée intégralement dans chaque objet placé » est obsolète.

La règle actuelle est :

```text
Definition.DefaultBehavior
        +
overrides strictement propres à l’instance
        =
comportement effectif
```

Une valeur permanente ou partagée appartient à la définition. Une valeur n’appartient à l’instance que si le niveau doit réellement pouvoir la différencier pour ce placement précis.

## 2. Autorité des données

### 2.1. Définition

`UGridWorldObjectDefinitionAsset` possède les propriétés permanentes du concept réutilisable :

- présentation ;
- `StaticPart` et `MovingParts` ;
- `MovingParts[].Motion` ;
- comportement spatial ;
- audio/VFX ;
- interaction générique ;
- lumière ;
- classe runtime ;
- règles comportementales partagées.

Modifier une définition doit naturellement modifier le comportement des instances qui n’ont pas d’override local autorisé pour cette donnée.

### 2.2. Instance de niveau

L’instance possède uniquement ce qui est réellement local au niveau, par exemple :

- identifiant d’instance stable ;
- cellule et orientation ;
- état initial ;
- destination de téléporteur ;
- destination de transition ;
- contenu initial d’un réceptacle ;
- état initial de serrure ;
- Tag / Notes ;
- texte ou configuration explicitement locale.

Une instance ne possède pas une copie de la géométrie d’animation de sa définition.

### 2.3. Runtime

Le runtime résout :

```text
Definition
+ Instance Configuration
+ Saved Runtime Delta
```

Il ne réécrit pas la définition et ne maintient pas une seconde source d’authoring.

## 3. Animation des mécanismes

Depuis MIG04 et la purge physique MIG09-C, l’unique autorité géométrique est :

```text
Definition
└── MovingParts[]
    └── Motion
        ├── Type
        ├── Axis
        ├── Pivot
        ├── Amount
        └── Duration
```

Les anciens paramètres spécialisés ne doivent pas revenir :

```text
DoorAnimation.OpenHeight
DoorAnimation.MoveDuration
LeverAnimation.LeverOffPitch
LeverAnimation.LeverOnPitch
LeverAnimation.ToggleDuration
ButtonAnimation.ButtonPressDistance
ButtonAnimation.ButtonPressDuration
ButtonAnimation.ButtonReleaseDuration
PressurePlateAnimation.ReleasedHeightAboveFloor
PressurePlateAnimation.PressedHeightAboveFloor
PressurePlateAnimation.MoveDuration
PitAnimation.LeftHingeLocation
PitAnimation.RightHingeLocation
PitAnimation.OpenAngleDegrees
PitAnimation.MoveDuration
```

Une porte verticale, coulissante ou battante doit différer par sa `Motion`, pas par un nouveau champ spécialisé dans `Behavior`.

## 4. Behavior restant

`FGridObjectBehaviorParams` porte les règles partagées et le comportement effectif résolu. Les placements typés ne le sérialisent pas intégralement : `FGridWorldObjectInstanceConfig` ne conserve que les cinq groupes locaux autorisés.

### Teleporter

La destination est naturellement locale :

```text
Behavior.Teleporter.TargetCellX
Behavior.Teleporter.TargetCellY
```

### Transition / Pit

Données locales possibles :

```text
Behavior.Transition
Behavior.Pit.bInitiallyOpen
Behavior.Pit.bUseSameCellCoordinates
```

La géométrie des volets d’un Pit n’est pas locale ; elle appartient à `MovingParts[].Motion`.

### Receptacle

Le contenu initial peut être local au niveau. Les règles partagées doivent provenir de la définition via le resolver.

### Button

Le bouton conserve :

```text
Behavior.ButtonAnimation.ButtonHoldTime
```

`ButtonHoldTime` est une règle logique. La distance et la durée du déplacement visuel sont dans `Motion`.

### Pressure Plate

Les règles de poids restent du gameplay :

```text
Behavior.PressurePlateWeight
```

La hauteur et la durée d’enfoncement sont dans `Motion`.

### Door

La chaîne reste une capacité/règle de porte :

```text
Behavior.DoorAnimation.bHasChainMechanism
Behavior.DoorAnimation.ChainPullDistance
Behavior.DoorAnimation.ChainPullDuration
```

La course et la durée du panneau de porte sont dans `MovingParts[].Motion`.

## 5. Règle du Grid Editor

L’Inspector ne doit exposer comme paramètres d’instance que les valeurs que le niveau est autorisé à surcharger.

Il ne doit plus permettre d’éditer sur une porte placée :

```text
Open Height
Instance Move Duration
```

ni sur les autres mécanismes les pivots, angles, hauteurs ou durées visuelles spécialisés.

Pour ces valeurs, l’Inspector indique l’autorité :

```text
Definition > Moving Parts[].Motion
```

Si le designer souhaite une autre animation permanente, il modifie ou crée une définition appropriée.

## 6. Règle runtime

Les acteurs de mécanismes reçoivent leurs parties visuelles via la composition générique et leur durée depuis `Motion`.

Un cache runtime calculé est autorisé :

```text
AGridDoorActor::MoveDuration
AGridButtonActor::PressDuration
AGridLeverActor::ToggleDuration
AGridPressurePlateActor::MoveDuration
AGridPitTrapdoorActor::MoveDuration
```

Ces champs sont des états d’exécution résolus depuis la définition ; ils ne sont pas des données d’authoring concurrentes.

## 7. Ce qui est interdit

Ne pas réintroduire :

```text
bOverrideBehavior
copie automatique intégrale de DefaultBehavior comme source permanente d’instance
fallback vers l’ancien ArchetypeId d’item (API supprimée)
paramètre visuel spécialisé qui duplique MovingParts[].Motion
synchronisation implicite ambiguë Definition <-> Instance
```

## 8. Persistance typée finalisée

Depuis MIG09, `UGridLevelAsset` ne stocke que les cinq collections typées. L'ancien `FGridLevelObjectData`, `UGridLevelAsset::Objects` et les projections de compatibilité ont été supprimés. `FGridRuntimeWorldObjectData` reste une frontière d'initialisation native non persistante, spécialisée world-object.

## 9. Checklist de validation

```text
[ ] géométrie/durée des mécanismes uniquement dans MovingParts[].Motion
[ ] ButtonHoldTime reste une règle logique
[ ] règles de poids de plaque préservées
[ ] chaîne de porte préservée
[ ] destinations Teleporter/Transition restent locales
[ ] contenu initial Receptacle reste local
[ ] aucune API spécialisée InitializeButton/InitializeLever/InitializeDoor de production
[ ] aucun ancien champ d’animation spécialisé réfléchi/sérialisé
[ ] Inspector n’édite plus ces champs
[ ] runtime ne modifie jamais un Data Asset pour stocker l’état courant
```

## 10. Formule à retenir

```text
Definition = ce qu’est l’objet
Instance   = où il est et ce qui est particulier à ce placement
Runtime    = ce qui lui arrive pendant la partie
SaveGame   = les deltas nécessaires pour restaurer cet état
```

Cette séparation est le contrat courant après MIG09 et le renommage final MIG10.