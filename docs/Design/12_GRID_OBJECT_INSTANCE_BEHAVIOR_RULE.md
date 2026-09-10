# 12 — Règle Definition / Instance des objets du monde

Statut : **document actif de référence après WORLDOBJ-RECOVERY01**  
Date : 2026-09-10  
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

Une instance ne possède pas une copie de la géométrie d’animation de sa définition. RECOVERY01-C1.1 autorise uniquement des exceptions sparse par `PartIndex` sur `LocalTransform`, `Motion.Amount` ou `Motion.Duration`; le mesh, le type, l’axe, le pivot et `ReverseDuration` restent exclusivement définis par la Definition.

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
        ├── Duration          # Alpha 0 -> 1
        └── ReverseDuration   # Alpha 1 -> 0 ; <= 0 => Duration
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

Une porte verticale, coulissante ou battante doit différer par sa `Motion`, pas par un nouveau champ spécialisé dans `Behavior`. RECOVERY01-C1.1 ajoute seulement une couche d'exception locale sparse ; RECOVERY01-C2 ajoute `ReverseDuration` au contrat générique de `Motion`, ce qui remplace le besoin historique de `ButtonReleaseDuration` sans recréer un schéma spécialisé.

## 4. Behavior restant

`FGridObjectBehaviorParams` porte les règles partagées et le comportement effectif résolu. Les placements ne le sérialisent pas intégralement : `FGridWorldObjectInstanceConfig` conserve les données naturellement locales ainsi que les deux canaux d’exception sparse introduits par RECOVERY01 (parties mobiles et chaîne de porte).

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

RECOVERY01-C1.2 permet à un placement de forcer uniquement la présence de chaîne via `DoorChainMode = Inherit / Enabled / Disabled` et, si nécessaire, sa durée de traction. La distance reste Definition-owned : aucune `ChainPullDistance` d'instance n'existe.

La course du panneau et ses durées forward/reverse sont dans `MovingParts[].Motion`.

## 5. Règle du Grid Editor

L’Inspector ne doit exposer comme paramètres d’instance que les valeurs que le niveau est autorisé à surcharger.

Il ne doit pas réintroduire sur une porte placée les anciens champs spécialisés :

```text
Open Height
DoorAnimation.MoveDuration
```

ni sur les autres mécanismes les anciens pivots, angles, hauteurs ou durées visuelles spécialisés. Les exceptions `MovingPartOverrides` existent comme canal sparse de niveau ; elles ne doivent pas devenir une copie éditable complète de la Motion.

Pour la configuration permanente, l'autorité reste :

```text
Definition > Moving Parts[].Motion
```

Si le designer souhaite une autre animation permanente, il modifie ou crée une définition appropriée. Un override d'instance n'est justifié que lorsqu'un placement précis doit réellement différer.

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
[ ] géométrie/motion partagée des mécanismes dans Definition.MovingParts[].Motion
[ ] overrides de partie mobile strictement sparse : LocalTransform / Amount / Duration seulement
[ ] ReverseDuration générique ; <= 0 retombe sur Duration
[ ] ButtonHoldTime reste une règle logique
[ ] règles de poids de plaque préservées
[ ] chaîne de porte : tri-state local + durée optionnelle, jamais de distance locale
[ ] destinations Teleporter/Transition restent locales
[ ] contenu initial Receptacle reste local
[ ] MonsterSpawn et LogicObject restent typés sans ancien N/LocalOffset world-object
[ ] aucune API spécialisée InitializeButton/InitializeLever/InitializeDoor de production
[ ] aucun ancien champ d’animation spécialisé réintroduit comme autorité
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