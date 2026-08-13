# 10 — Grid Editor UI Consistency Checklist

Date : 2026-05-23  
Projet : GrimrockPrototype — Grimrock Grid Editor Mode

## 1. Objectif

Ce document sert de checklist de cohérence pour l’interface du `Grimrock Grid Editor Mode` après les nettoyages successifs de l’inspecteur, des connecteurs, de l’orientation et du runtime.

L’objectif est de vérifier que l’éditeur raconte partout la même chose :

```text
Selected Object = identité, état initial et paramètres utiles de l’objet sélectionné
CONNECTORS      = logique Source / Event / Target / Command
Orientation     = orientation visuelle ou edge de placement
Runtime         = exécute exactement ce qui est affiché dans l’UI
```

Cette checklist ne demande pas de nouvelle fonctionnalité. Elle sert à valider, objet par objet, que l’interface est claire, cohérente et non dangereuse.

---

## 2. Règles générales validées

### 2.1 Selected Object

Le panneau `Selected Object` doit rester un inspecteur de level design, pas un dump technique.

Règles :

- le header affiche déjà le nom, le type, la cellule et l’orientation ;
- `Game Object` ne répète pas `Gameplay Type`, `Cell X`, `Cell Y`, `Edge / Facing` ;
- les champs affichés doivent être utiles pour comprendre l’objet sélectionné ;
- `Advanced / Debug` reste principalement en lecture seule ;
- `Notes` reste éditable ;
- aucun bouton dangereux ou obsolète ne doit réapparaître.

À ne pas réintroduire :

```text
Focus Selected Object
Apply Selected Object
Reset Behavior From Archetype
APPLY BEHAVIOR
Clear Links
Rotate 90°
RotationStepYaw
Behavior Editor global
```

### 2.2 CONNECTORS

Un connecteur est strictement défini par :

```text
Source Object + Source Event + Target Object + Command
```

Règles :

- un connecteur ne s’exécute que pour son `SourceEvent` exact ;
- aucune inversion implicite runtime n’est autorisée ;
- `Activated -> Open` ne signifie jamais automatiquement `Deactivated -> Close` ;
- les sources sont filtrées ;
- les targets sont filtrées ;
- les commands sont filtrées selon la target ;
- les items au sol et décorations ne participent pas au système de connecteurs ;
- le rouge signifie uniquement `Broken Connector`.

Code couleur :

```text
Cyan / vert clair = Outgoing connector
Violet / mauve   = Incoming connector
Rouge            = Broken connector uniquement
```

### 2.3 Orientation

Le widget `North / East / South / West` remplace l’ancien bouton `Rotate 90°`.

Règle finale :

```text
Objet visible = orientable
Objet logique / invisible = non orientable
```

Les objets visibles au sol sont orientables : carpet, bones, dust, roots, rubble, runes, items visibles, etc.

### 2.4 Runtime

Le runtime doit correspondre exactement à ce que l’UI affiche.

Exemples :

```text
PressurePlate Activated   -> Door Open   = entrée sur plaque ouvre
PressurePlate Deactivated -> Door Close  = sortie de plaque ferme
Lever Activated           -> Door Open   = activation levier ouvre
Lever Deactivated         -> Door Close  = désactivation levier ferme
Button Activated          -> Door Toggle = chaque pression toggle
Receptacle ItemRemoved    -> Door Open   = retirer l’objet ouvre
Receptacle ItemInserted   -> Door Close  = remettre l’objet ferme
```

---

## 3. Checklist globale UI

À tester sur chaque objet important :

| Point | Attendu | OK |
|---|---|---|
| Header | Nom lisible + type + cellule + orientation | [ ] |
| Game Object | Pas de répétition de Cell X/Y ou Edge/Facing | [ ] |
| Section contextuelle | Champs utiles uniquement pour ce type d’objet | [ ] |
| Advanced / Debug | ObjectId, ArchetypeId, Tag en lecture seule | [ ] |
| Notes | Champ éditable | [ ] |
| Orientation | Affichée seulement si l’objet visible/orientable | [ ] |
| CONNECTORS | Affiché/actif seulement pour les objets logiques | [ ] |
| Bouton + CONNECTORS | Absent pour items/décorations/readable-only | [ ] |
| Dropdown Source | Ne propose que des sources logiques | [ ] |
| Dropdown Target | Ne propose que des cibles commandables | [ ] |
| Dropdown Command | Ne propose que les commandes valides pour la cible | [ ] |
| Viewport arrows | Correspondent aux connecteurs affichés | [ ] |
| Runtime | Correspond exactement aux connecteurs affichés | [ ] |

---

## 4. Checklist par type d’objet

## 4.1 Door / Stone Door / Secret Door

### Selected Object

Attendu :

- header clair, par exemple `Stone Door`, `Door @ (X,Y) North` ;
- pas de répétition inutile de `Gameplay Type`, `Cell X`, `Cell Y`, `Edge / Facing` dans `Game Object` ;
- section `Door` visible ;
- `Supported Commands` cohérent : `Open`, `Close`, `Toggle`, `Lock`, `Unlock` si disponibles ;
- `Advanced / Debug` en lecture seule pour `ObjectId`, `ArchetypeId`, `Tag` ;
- `Notes` éditable.

### Orientation

Attendu :

- widget `North / East / South / West` visible ;
- changer l’orientation change l’edge/facing de la porte ;
- le placement viewport reste cohérent avec l’edge.

### CONNECTORS

Attendu :

- la porte peut apparaître comme `Target Object` ;
- la porte ne doit pas être une source logique par défaut ;
- commandes proposées : uniquement commandes de porte ;
- si la porte est sélectionnée, les incoming connectors sont visibles ;
- les flèches viewport entrantes pointent source -> porte.

### Runtime

Tests :

```text
Button Activated -> Door Toggle
Lever Activated -> Door Open
Lever Deactivated -> Door Close
PressurePlate Activated -> Door Open
PressurePlate Deactivated -> Door Close
```

Validation :

| Test | OK |
|---|---|
| Door apparaît comme target | [ ] |
| Door ne propose pas de source absurde | [ ] |
| Orientation N/E/S/W fonctionne | [ ] |
| Open fonctionne | [ ] |
| Close fonctionne | [ ] |
| Toggle fonctionne | [ ] |
| Incoming viewport arrows correctes | [ ] |

---

## 4.2 Button_Normal / Button_Secret / Button_Wall

### Selected Object

Attendu :

- header lisible : `Button`, `Secret Button`, `Wall Button` ;
- section `Button` visible ;
- type de bouton affiché si disponible ;
- informations d’animation bouton visibles si utiles ;
- pas de `Behavior Editor` global ;
- pas de `Trigger Mode`, `Delay`, `Duration`, `Invert Connectors`.

### Orientation

Attendu :

- widget orientation visible ;
- changer orientation déplace le bouton sur l’edge choisi ;
- preview correcte sur le mur/edge.

### CONNECTORS

Attendu :

- bouton apparaît comme `Source Object` ;
- events proposés : `Activated`, éventuellement `Used` si réellement supporté ;
- bouton ne doit pas apparaître comme `Target Object` ;
- bouton `+` disponible quand le bouton est sélectionné ;
- target dropdown ne propose que des objets commandables.

### Runtime

Tests :

```text
Button Activated -> Door Toggle
Button Activated -> Door Open
```

Validation :

| Test | OK |
|---|---|
| Button source disponible | [ ] |
| Button target absent | [ ] |
| Events filtrés | [ ] |
| Orientation fonctionne | [ ] |
| Activated -> Toggle fonctionne | [ ] |
| Viewport arrow sortante correcte | [ ] |

---

## 4.3 Lever

### Selected Object

Attendu :

- header lisible : `Lever @ (X,Y) East` ;
- section `Lever` visible ;
- état initial lisible : `Active at Start` ;
- pas de champs obsolètes.

### Orientation

Attendu :

- widget orientation visible ;
- changement d’edge/facing correct.

### CONNECTORS

Attendu :

- lever apparaît comme `Source Object` ;
- events proposés : `Activated`, `Deactivated`, éventuellement `Toggled` ;
- lever ne doit pas apparaître comme target par défaut.

### Runtime

Tests :

```text
Lever Activated -> Door Open
Lever Deactivated -> Door Close
```

Validation :

| Test | OK |
|---|---|
| Lever source disponible | [ ] |
| Lever target absent | [ ] |
| Activated exécute uniquement Activated | [ ] |
| Deactivated exécute uniquement Deactivated | [ ] |
| Aucune inversion implicite | [ ] |

---

## 4.4 PressurePlate

### Selected Object

Attendu :

- header lisible : `Pressure Plate @ (X,Y)` ;
- section `Pressure Plate / Floor Trigger` visible ;
- pas de `Fire On Enter`, `Fire On Exit`, `Trigger Mode` obsolètes ;
- `Active at Start` généralement false.

### Orientation

Attendu :

- si l’objet est visible, le widget orientation peut rester visible ;
- si le mesh est symétrique, l’orientation peut n’avoir aucun effet visuel fort, mais cela n’est pas une erreur.

### CONNECTORS

Attendu :

- pressure plate apparaît comme source ;
- events proposés : `Activated`, `Deactivated` ;
- pressure plate ne doit pas apparaître comme target.

### Runtime

Tests :

```text
Activated -> Door Open uniquement : sortir de la plaque ne ferme pas.
Activated -> Door Open + Deactivated -> Door Close : sortir ferme.
```

Validation :

| Test | OK |
|---|---|
| Source disponible | [ ] |
| Target absent | [ ] |
| Activated exact | [ ] |
| Deactivated exact | [ ] |
| Pas d’inversion implicite | [ ] |

---

## 4.5 Trigger

### Selected Object

Attendu :

- si trigger invisible : section concise ;
- pas d’orientation si aucun mesh visible ;
- pas de champ obsolète `Trigger Mode`, `Fire On Enter`, `Fire On Exit`.

### CONNECTORS

Attendu :

- trigger peut être source si c’est un trigger gameplay ;
- trigger ne doit pas être target par défaut sauf futur relay/controller explicite.

### Runtime

Attendu :

```text
Entrée -> Activated
Sortie -> Deactivated
```

Validation :

| Test | OK |
|---|---|
| Trigger source si logique | [ ] |
| Trigger target absent | [ ] |
| Pas d’orientation si invisible | [ ] |
| Activated/Deactivated exacts | [ ] |

---

## 4.6 Receptacle_TorchHolder

### Selected Object

Attendu :

- header lisible : `Torch Holder @ (X,Y) East` ;
- section `Receptacle` visible ;
- pas de saisie manuelle d’ArchetypeId ;
- `Accepted Items` via liste d’items ;
- `Initial Content` via dropdown `None + items` ;
- `Rejected Item Archetypes` et `Accepted Item Tags` absents de l’UI normale ou déplacés en advanced si encore disponibles.

### Orientation

Attendu :

- widget orientation visible ;
- changement d’edge/facing replace le support correctement.

### CONNECTORS

Attendu :

- receptacle apparaît comme source ;
- events proposés : `Item Removed`, `Item Inserted` ;
- receptacle ne doit pas apparaître comme target pour l’instant.

### Runtime

Tests :

```text
ItemRemoved -> Door Open
ItemInserted -> Door Close
```

Validation :

| Test | OK |
|---|---|
| Accepted Items ne propose que des items | [ ] |
| Initial Content propose None + items | [ ] |
| Initial Content compatible | [ ] |
| ItemRemoved exact | [ ] |
| ItemInserted exact | [ ] |
| Torche support attachée ne tombe pas | [ ] |

---

## 4.7 WallInscription

### Selected Object

Attendu :

- section `Readable Text` visible ;
- texte éditable si prévu ;
- pas de connecteurs par défaut ;
- pas de target commandable ;
- pas de source d’événement.

### Orientation

Attendu :

- widget orientation visible si inscription murale visible ;
- changement d’edge/facing replace l’inscription correctement.

### Runtime

Attendu :

- Use ou interaction affiche le texte ;
- aucun connecteur n’est déclenché.

Validation :

| Test | OK |
|---|---|
| Readable Text visible | [ ] |
| Source connector absent | [ ] |
| Target connector absent | [ ] |
| Texte affiché en runtime | [ ] |

---

## 4.8 Item_Torch

### Selected Object

Attendu :

- header lisible : `Torch @ (X,Y) Edge` ;
- pas de connecteurs ;
- message `This object has no connector behavior.` acceptable ;
- Advanced / Debug read-only sauf Notes ;
- item visible, physique et récupérable.

### Orientation

Attendu :

- widget orientation visible ;
- orientation détermine l’edge / position de l’item ;
- l’item reste au sol proche du bord choisi.

### CONNECTORS

Attendu :

- bouton `+` absent ;
- item absent de `Source Object` ;
- item absent de `Target Object`.

### Runtime

Tests :

```text
Item_Torch au sol : physique active, torche éteinte.
Use depuis même cellule : pickup.
Après pickup : torche en main allumée.
Torche support : ne tombe pas.
```

Validation :

| Test | OK |
|---|---|
| Item_Torch placé depuis Paint Object | [ ] |
| Physique naturelle au sol | [ ] |
| Pas de flamme au sol | [ ] |
| Pas de lumière au sol | [ ] |
| Pickup depuis même cellule | [ ] |
| bHasTorchInHand true après pickup | [ ] |
| HeldItemArchetypeId = Item_Torch | [ ] |
| Torche en main allumée | [ ] |

---

## 4.9 Floor Decorations

Objets concernés :

```text
FloorBloodStain
FloorBones
FloorCarpet
FloorDebris
FloorDust
FloorMoss
FloorRoots
FloorRubble
FloorRuneCircle
```

### Selected Object

Attendu :

- header lisible ;
- pas de connecteurs ;
- pas de champs runtime inutiles ;
- pas de comportement interactif sauf cas volontaire futur.

### Orientation

Attendu :

- widget orientation visible ;
- rotation N/E/S/W utile pour varier la pose ;
- pas de `RotationStepYaw`.

### CONNECTORS

Attendu :

- bouton `+` absent ;
- pas de source ;
- pas de target ;
- message `This object has no connector behavior.` acceptable.

### Runtime

Attendu :

- visible ;
- non bloquant ;
- non interactif ;
- pas de logique connector.

Validation :

| Test | OK |
|---|---|
| Orientation visible | [ ] |
| Pas de bouton + connectors | [ ] |
| Pas source/target | [ ] |
| Visible runtime | [ ] |
| Ne bloque pas mouvement | [ ] |

---

## 5. Validation des dropdowns CONNECTORS

## 5.1 Source Object

Doit contenir :

```text
Button_Normal
Button_Secret
Button_Wall
Lever
PressurePlate
Trigger logique
Receptacle_TorchHolder
```

Ne doit pas contenir :

```text
Door_Stone
Door_Secret
WallInscription
Item_Torch
Floor decorations
ItemSpawn
MonsterSpawn
Readable-only
```

## 5.2 Event

Doit dépendre de la source.

Exemples :

```text
Button        -> Activated
Lever         -> Activated, Deactivated
PressurePlate -> Activated, Deactivated
Trigger       -> Activated, Deactivated
Receptacle    -> ItemInserted, ItemRemoved
```

## 5.3 Target Object

Doit contenir actuellement :

```text
Door_Stone
Door_Secret
Teleporter
Light
Receptacle
MonsterSpawn
```

Targets futures possibles, mais non actives tant que le runtime n’est pas implémenté :

```text
ItemSpawner
LogicRelay
```

Ne doit pas contenir :

```text
Button
Lever
PressurePlate
WallInscription
Item_Torch
Floor decorations
ItemSpawn tant que spawn commandé non implémenté
```

## 5.4 Command

Pour `Door` :

```text
Open
Close
Toggle
Lock
Unlock
```

Pour `MonsterSpawn` :

```text
Spawn
Despawn
Teleport
Activate
Deactivate
Enable
Disable
Toggle
```

Aucune commande de porte ne doit être proposée pour un objet qui n’est pas une porte.

---

## 6. Validation viewport

| Point | Attendu | OK |
|---|---|---|
| Outgoing arrows | Cyan / vert clair | [ ] |
| Incoming arrows | Violet / mauve | [ ] |
| Broken arrows | Rouge uniquement | [ ] |
| Direction | Toujours Source -> Target | [ ] |
| Départ flèche | Centre logique réel source | [ ] |
| Arrivée flèche | Centre logique réel cible | [ ] |
| Labels | Event / Command | [ ] |
| Labels toggle | `Show Connector Labels` fonctionne | [ ] |
| Incoming toggle | `Show Incoming Connectors` fonctionne | [ ] |
| Outgoing toggle | `Show Outgoing Connectors` fonctionne | [ ] |

---

## 7. Validation des données / DataAssets

À vérifier pour chaque DataAsset principal :

| Asset | Gameplay Type | Functional Category | Palette Category | Placement Kind | Mesh | Runtime / Item Actor | OK |
|---|---|---|---|---|---|---|---|
| DA_Button_Normal | Button | Mechanism | Mechanisms | Wall/Edge | Oui | Button actor | [ ] |
| DA_Button_Secret | Button | Mechanism | Mechanisms | Wall/Edge | Oui | Button actor | [ ] |
| DA_Lever | Lever | Mechanism | Mechanisms | Wall/Edge | Oui | Lever actor | [ ] |
| DA_PressurePlate | PressurePlate | Mechanism | Mechanisms | Floor/Center | Oui | PressurePlate actor | [ ] |
| DA_Trigger | Trigger | Trigger/Mechanism | Triggers | Center/Floor | Optionnel | Trigger actor ou invisible | [ ] |
| DA_Door_Stone | Door | Mechanism | Doors | Edge | Oui | Door actor | [ ] |
| DA_Door_Secret | Door | Mechanism | Doors | Edge | Oui | Secret door actor | [ ] |
| DA_Receptacle_TorchHolder | Receptacle | Receptacle | Receptacles | Wall/Edge | Oui | Receptacle actor | [ ] |
| DA_Item_Torch | Item | Item | Items | Edge | Oui | Item actor | [ ] |
| DA_WallInscription | Readable | Readable | Readables | Wall/Edge | Oui | Readable actor | [ ] |

---

## 8. Tests runtime de non-régression

À effectuer après chaque gros changement UI ou runtime :

| Test | Résultat attendu | OK |
|---|---|---|
| Button -> Door Toggle | Chaque pression alterne la porte | [ ] |
| Lever Activated -> Door Open | Activation ouvre | [ ] |
| Lever Deactivated -> Door Close | Désactivation ferme | [ ] |
| PressurePlate Activated -> Door Open | Entrée sur plaque ouvre | [ ] |
| PressurePlate Deactivated -> Door Close | Sortie de plaque ferme | [ ] |
| PressurePlate Activated -> Door Open seul | Sortie de plaque ne ferme pas | [ ] |
| Receptacle ItemRemoved -> Door Open | Retirer objet ouvre | [ ] |
| Receptacle ItemInserted -> Door Close | Remettre objet ferme | [ ] |
| Item_Torch pickup | Ramassage depuis cellule fonctionne | [ ] |
| Torche support | Ne tombe pas, reste attachée | [ ] |
| Torche sol | Physique active, ramassable | [ ] |

---

## 9. Critères de clôture de la phase

La phase `UI Consistency` peut être considérée comme validée si :

```text
- Selected Object n’a plus de redondances majeures ;
- CONNECTORS ne propose plus de source/target absurde ;
- Orientation est disponible pour les objets visibles ;
- Items/décorations/readable-only ne participent pas aux connecteurs ;
- Runtime suit strictement SourceEvent ;
- Viewport arrows/labels correspondent aux connecteurs ;
- DataAssets principaux sont cohérents ;
- Les tests runtime de non-régression passent.
```

---

## 10. Corrections futures possibles

À ne pas implémenter dans cette phase, mais à garder comme suite logique :

```text
Validate Level
Validate Selected Object
Warning pour connecteurs contradictoires
Support explicite de LogicRelay / Controller
Vrai système de spawners commandés
Mini-inventaire complet
Dépôt d’items dans les réceptacles
Gestion de plusieurs items dans une même cellule
```
