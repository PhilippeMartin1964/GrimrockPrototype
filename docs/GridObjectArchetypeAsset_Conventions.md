# GridObjectArchetypeAsset — Conventions

Ce document fixe les conventions actuelles autour de `UGridObjectArchetypeAsset` dans **GrimrockPrototype**.

L'objectif est d'éviter les ambiguïtés entre type gameplay, catégorie éditeur, catégorie de palette, placement, visuel, comportement et items.

---

## 1. Rôle général

`UGridObjectArchetypeAsset` définit un archétype d'objet utilisable par l'éditeur ou le runtime.

Un archétype ne représente pas toujours un objet directement placé dans le niveau.

Exemples :

```text
Door_Stone
→ objet placé dans le niveau

Item_Torch
→ définition d'un item manipulable

ItemSpawn_Torch
→ objet placé qui fait apparaître Item_Torch
```

L'archétype sert donc à décrire :

```text
ce qu'est l'objet,
comment il se place,
comment il s'affiche,
comment il se comporte,
et comment il est instancié au runtime.
```

---

## 2. Classifications principales

### 2.1 SupportedType

`SupportedType` est la **vérité gameplay**.

Il indique ce qu'est réellement l'objet.

Exemples :

```text
Door
Button
Lever
PressurePlate
Trigger
Receptacle
Item
ItemSpawn
Decoration
Light
Teleporter
MonsterSpawn
```

Exemples d'assets :

```text
Door_Stone
SupportedType = Door

Item_Torch
SupportedType = Item

ItemSpawn_Torch
SupportedType = ItemSpawn
```

Règle :

```text
SupportedType sert au runtime, à la validation et aux comportements spécialisés.
```

---

### 2.2 ObjectCategory

`ObjectCategory` est une **classification fonctionnelle éditeur / validation**.

Il ne doit pas piloter directement le gameplay runtime.

Exemples :

```text
Door_Stone
ObjectCategory = Mechanism

WallInscription
ObjectCategory = Readable

Item_Torch
ObjectCategory = Item

Receptacle_WallTorchHolder
ObjectCategory = Receptacle
```

Règle :

```text
ObjectCategory sert à classer et valider.
SupportedType reste la vérité gameplay.
```

---

### 2.3 Palette Category

Le champ C++ `Category`, affiché dans l'éditeur comme `Palette Category`, sert uniquement au groupement dans la palette Paint Object.

Il n'a pas d'impact gameplay.

Exemples :

```text
Door_Stone
Palette Category = Doors

Button_ToggleDoor
Palette Category = Mechanisms

Item_Torch
Palette Category = Items

ItemSpawn_Torch
Palette Category = Spawns

FloorRuneCircle
Palette Category = Floor Decorations
```

Règle :

```text
Category / Palette Category = organisation visuelle dans l'éditeur.
```

---

## 3. PlacementKind

`PlacementKind` est la **seule source de vérité** pour le placement.

Les anciens champs legacy ne doivent plus piloter le placement.

Valeurs principales :

```text
Center
Floor
Wall
Edge
Ceiling
```

Conventions :

```text
Door
→ PlacementKind = Edge ou Wall

Button / Lever / Receptacle mural / WallInscription
→ PlacementKind = Wall

PressurePlate / Trigger / ItemSpawn / Item posé au sol
→ PlacementKind = Floor ou Center

Décoration de sol
→ PlacementKind = Floor
```

---

## 4. Champs legacy de placement

Les champs suivants sont conservés uniquement pour compatibilité avec d'anciens assets :

```text
bPlaceOnEdge
bPlaceAtCellCenter
```

Ils ne doivent plus être utilisés pour les nouveaux assets.

Règle :

```text
Si la validation signale une incohérence legacy,
corriger l'asset, sauvegarder, puis ne plus tenir compte de ces champs.
```

À terme, ces champs pourront être supprimés après migration complète des assets.

---

## 5. Visual — Main / Fixed / Moving

### 5.1 Main Mesh / Preview Mesh

Le champ C++ `PreviewMesh`, affiché comme `Main Mesh / Preview Mesh`, est le mesh principal des objets simples.

À utiliser pour :

```text
Décorations simples
Items
ItemSpawn visible en édition
Plaques
Boutons simples
Inscriptions murales
```

Convention :

```text
PreviewMesh = MainMesh conceptuel.
```

Le nom C++ reste `PreviewMesh` pour éviter de casser les DataAssets Unreal existants.

---

### 5.2 FixedMesh

`FixedMesh` représente la partie fixe d'un objet composé.

À utiliser surtout pour :

```text
Porte secrète
Objet composé avec partie fixe + partie mobile
```

Ne pas utiliser `FixedMesh` comme mesh principal d'une décoration simple.

---

### 5.3 MovingMesh

`MovingMesh` représente une partie mobile, animée, ou un visuel spécifique d'item.

À utiliser pour :

```text
Door
SecretDoor
Button animé
Lever
Item visuel spécifique
```

---

## 6. RuntimeActorClass

`RuntimeActorClass` n'est pas redondant avec `SupportedType`.

```text
SupportedType = ce qu'est l'objet.
RuntimeActorClass = comment il est instancié.
```

Exemples :

```text
Door_Stone
SupportedType = Door
RuntimeActorClass = BP_GridDoorActor

Receptacle_WallTorchHolder
SupportedType = Receptacle
RuntimeActorClass = BP_GridReceptacleActor
```

Pour un `Item`, le runtime utilise plutôt `ItemActorClass`.

---

## 7. ItemActorClass

`ItemActorClass` définit quel acteur est spawné pour un item.

Exemple critique :

```text
Item_Torch
ItemActorClass = BP_Item_Torch
```

Ne pas remplacer `BP_Item_Torch` par la classe C++ nue `GridItemActor` si le Blueprint porte les composants visuels spécifiques.

Pour une torche, le Blueprint peut contenir :

```text
flamme
Niagara System
lumière
flicker
offsets spécifiques
```

Règle :

```text
L'archétype définit l'identité de l'item.
Le Blueprint spécialisé définit son rendu vivant.
```

---

## 8. Item vs ItemSpawn

### 8.1 Item

`Item` définit un objet manipulable, transportable ou insérable dans un receptacle.

Exemple :

```text
Item_Torch
SupportedType = Item
ObjectCategory = Item
Palette Category = Items
PlacementKind = Floor
ItemTags = Torch
ItemActorClass = BP_Item_Torch
```

Un `Item` n'est pas nécessairement destiné à être peint directement dans le niveau.

---

### 8.2 ItemSpawn

`ItemSpawn` est un objet placé dans le niveau qui fait apparaître un item.

Exemple :

```text
ItemSpawn_Torch
SupportedType = ItemSpawn
ObjectCategory = Spawn
Palette Category = Spawns
PlacementKind = Floor
DefaultBehavior.ItemSpawn.SpawnedItemArchetypeId = Item_Torch
```

Au runtime :

```text
ItemSpawn_Torch
→ spawn Item_Torch
→ Item_Torch utilise ItemActorClass = BP_Item_Torch
```

Règle :

```text
Item = définition.
ItemSpawn = instance placée qui fait apparaître cette définition.
```

---

## 9. DefaultBehavior

`DefaultBehavior` contient les comportements par défaut de l'archétype.

Il est structuré en sous-blocs.

### Instance vs Archetype authority

`UGridObjectArchetypeAsset` est un template initial.

`FGridLevelObjectData` est la vérité finale pour une instance placée.

Règles :

```text
DefaultBehavior est copié dans ObjectData.Behavior à la création.
Au runtime, ObjectData.Behavior est toujours utilisé.
bOverrideBehavior est legacy et ne doit plus contrôler le runtime.
```

Une instance placée peut donc vider ou modifier un champ de behavior sans devoir cocher un override.

L'inspecteur d'objet fournit une action explicite :

```text
Reset Behavior From Archetype
```

Cette action recopie volontairement `Archetype.DefaultBehavior` vers `ObjectData.Behavior` pour l'instance sélectionnée. Elle ne modifie pas `InitiallyEnabled`, `InitiallyActive`, `Tag`, `Notes` ou `OverrideReadableText`.

---

### 9.1 Activation

```text
DefaultBehavior.Activation.TriggerMode
DefaultBehavior.Activation.Delay
DefaultBehavior.Activation.Duration
DefaultBehavior.Activation.bInvertLinks
```

Utilisé par les mécanismes et objets déclencheurs.

`Delay` et `Duration` sont prévus pour des activations retardées ou temporisées. Leur usage runtime peut être incomplet selon l'état du prototype.

---

### 9.2 Trigger

```text
DefaultBehavior.Trigger.bFireOnEnter
DefaultBehavior.Trigger.bFireOnExit
```

Utilisé par :

```text
Trigger
PressurePlate
```

---

### 9.3 Teleporter

```text
DefaultBehavior.Teleporter.TargetCellX
DefaultBehavior.Teleporter.TargetCellY
```

Réservé aux téléporteurs.

---

### 9.4 Receptacle

```text
DefaultBehavior.Receptacle.bAcceptAnyItem
DefaultBehavior.Receptacle.AcceptedItemTags
DefaultBehavior.Receptacle.AcceptedArchetypeIds
DefaultBehavior.Receptacle.InitialContainedItemArchetypeId
```

Exemple support mural de torche :

```text
Receptacle_WallTorchHolder
bAcceptAnyItem = false
AcceptedItemTags = Torch
InitialContainedItemArchetypeId = Item_Torch si le support commence avec une torche
```

---

### 9.5 ButtonAnimation

```text
DefaultBehavior.ButtonAnimation.ButtonPressDistance
DefaultBehavior.ButtonAnimation.ButtonPressDuration
DefaultBehavior.ButtonAnimation.ButtonReleaseDuration
DefaultBehavior.ButtonAnimation.ButtonHoldTime
```

Utilisé par les boutons.

---

### 9.6 ItemSpawn

```text
DefaultBehavior.ItemSpawn.SpawnedItemArchetypeId
```

Utilisé par les objets `ItemSpawn`.

Exemple :

```text
ItemSpawn_Torch
SpawnedItemArchetypeId = Item_Torch
```

---

## 10. Interaction et readable

### 10.1 bIsInteractable

Indique qu'un objet peut être utilisé/interagi.

Typiquement :

```text
Button
Lever
Receptacle
Readable decoration
```

---

### 10.2 bIsReadable

`bIsReadable` est le flag runtime pour les objets lisibles.

Ne pas confondre avec :

```text
ObjectCategory = Readable
```

`ObjectCategory = Readable` est une classification éditeur.

`bIsReadable = true` active la logique runtime de lecture.

---

### 10.3 ReadableText

Texte par défaut lu par le joueur.

Une instance placée peut éventuellement utiliser un override.

---

## 11. Lumière

### 11.1 bIsLightSource

`bIsLightSource` indique qu'un objet peut utiliser les paramètres lumière génériques.

Mais pour les Blueprints spécialisés comme `BP_Item_Torch`, le Blueprint reste l'autorité pour :

```text
Niagara
flamme
flicker
offsets
composants spécifiques
```

Règle importante :

```text
L'archétype dit que l'objet est lumineux.
Le Blueprint spécialisé définit comment cette lumière/flamme se comporte.
```

Ne pas écraser les paramètres de lumière d'un Blueprint spécialisé depuis le C++ sauf décision explicite.

---

## 12. Conventions par type

### 12.1 Door

```text
SupportedType = Door
ObjectCategory = Mechanism
Palette Category = Doors
PlacementKind = Edge ou Wall
RuntimeActorClass = BP_GridDoorActor ou BP_GridSecretDoorActor
```

Peut utiliser :

```text
FixedMesh
MovingMesh
```

---

### 12.2 Button

```text
SupportedType = Button
ObjectCategory = Mechanism
Palette Category = Mechanisms
PlacementKind = Wall
bIsInteractable = true
RuntimeActorClass = BP_GridButtonActor
```

Utilise :

```text
DefaultBehavior.Activation
DefaultBehavior.ButtonAnimation
```

---

### 12.3 Lever

```text
SupportedType = Lever
ObjectCategory = Mechanism
Palette Category = Mechanisms
PlacementKind = Wall
bIsInteractable = true
RuntimeActorClass = BP_GridLeverActor
```

---

### 12.4 PressurePlate

```text
SupportedType = PressurePlate
ObjectCategory = Mechanism
Palette Category = Mechanisms
PlacementKind = Floor
RuntimeActorClass = BP_GridPressurePlateActor
```

Utilise :

```text
DefaultBehavior.Activation
DefaultBehavior.Trigger
```

---

### 12.5 Trigger

```text
SupportedType = Trigger
ObjectCategory = Mechanism
Palette Category = Logic ou Triggers
PlacementKind = Floor ou Center
```

Peut être invisible.

Utilise :

```text
DefaultBehavior.Activation
DefaultBehavior.Trigger
```

---

### 12.6 Receptacle

```text
SupportedType = Receptacle
ObjectCategory = Receptacle
Palette Category = Receptacles
PlacementKind = Wall ou Floor selon le cas
bIsInteractable = true
RuntimeActorClass = BP_GridReceptacleActor
```

Utilise :

```text
DefaultBehavior.Receptacle
```

---

### 12.7 Item

```text
SupportedType = Item
ObjectCategory = Item
Palette Category = Items
PlacementKind = Floor
ItemTags = ...
ItemActorClass = BP spécifique
```

Exemple :

```text
Item_Torch
ItemTags = Torch
ItemActorClass = BP_Item_Torch
```

---

### 12.8 ItemSpawn

```text
SupportedType = ItemSpawn
ObjectCategory = Spawn
Palette Category = Spawns
PlacementKind = Floor
DefaultBehavior.ItemSpawn.SpawnedItemArchetypeId = Item_Torch
```

---

### 12.9 Decoration

```text
SupportedType = Decoration
ObjectCategory = Decoration
Palette Category = Floor Decorations ou Wall Decorations
PlacementKind = Floor ou Wall
Main Mesh / Preview Mesh = mesh principal
```

Pour une inscription :

```text
SupportedType = Decoration
ObjectCategory = Readable
bIsReadable = true
bIsInteractable = true
ReadableText = ...
```

---

### 12.10 Light

```text
SupportedType = Light
ObjectCategory = Light
Palette Category = Lights
bIsLightSource = true
```

---

## 13. Validation

Le panneau `Validate Level` doit être utilisé régulièrement.

Il vérifie notamment :

```text
SupportedType ↔ ObjectCategory
PlacementKind
Palette Category
Item / ItemSpawn
Receptacle
Teleporter
ButtonAnimation
Champs visuels suspects
Champs legacy
```

Une validation verte signifie :

```text
Les assets sont cohérents avec les conventions actuelles.
```

Elle ne signifie pas forcément que tout le runtime associé est complet.

---

## 14. Règles rapides à retenir

```text
SupportedType = gameplay
ObjectCategory = classification fonctionnelle éditeur
Category / Palette Category = groupe palette
PlacementKind = placement réel
PreviewMesh = MainMesh conceptuel
FixedMesh = partie fixe d'objet composé
MovingMesh = partie mobile / item spécifique
Item = définition d'un objet manipulable
ItemSpawn = objet placé qui fait apparaître un Item
RuntimeActorClass = acteur runtime de l'objet placé
ItemActorClass = acteur runtime de l'item
Blueprint spécialisé = autorité sur effets visuels complexes
```

---

## 15. Note sur la torche

Pour la torche, la configuration correcte est :

```text
Item_Torch
SupportedType = Item
ObjectCategory = Item
Palette Category = Items
PlacementKind = Floor
ItemTags = Torch
ItemActorClass = BP_Item_Torch
```

Et non :

```text
ItemActorClass = GridItemActor
```

`BP_Item_Torch` doit rester responsable de la flamme, du Niagara, du flickering, de la lumière et des offsets visuels.

`ItemSpawn_Torch` doit seulement référencer `Item_Torch` :

```text
ItemSpawn_Torch
DefaultBehavior.ItemSpawn.SpawnedItemArchetypeId = Item_Torch
```
