# GrimrockPrototype — Spécification UX du Grimrock Grid Editor Mode

## Objectif du document

Ce document complète les documents existants du dossier `docs/Design`.

Il ne redéfinit pas le système d’objets, les catégories, les événements, les commandes ou les liens. Ces éléments sont déjà décrits dans :

- `00_PROJECT_OVERVIEW.md` ;
- `01_GRID_OBJECT_SYSTEM.md` ;
- `02_OBJECT_ARCHETYPES.md` ;
- `03_EVENT_COMMAND_LINKS.md` ;
- `04_IMPLEMENTATION_ROADMAP.md` ;
- `05_CODEX_TASKS.md` ;
- `99_DECISIONS_LOG.md`.

Le présent document définit ce qu’il faut faire pour rendre le mode éditeur `Grimrock Grid Editor Mode` plus simple, plus lisible et plus proche de l’esprit de l’éditeur de *Legend of Grimrock 2*.

L’objectif principal est de transformer l’inspecteur actuel, encore trop technique, en un outil orienté level design.

---

## Décisions non négociables

### Ne pas modifier les enums existants

Les enums suivants ne doivent pas être renommés ni restructurés dans le cadre de cette évolution UX :

```cpp
EGridObjectEvent
EGridObjectCommand
```

Le travail porte sur la présentation et l’ergonomie, pas sur le modèle logique.

### Ne pas refaire l’architecture runtime

Ce chantier ne doit pas être un refactor massif du runtime.

Il doit respecter les principes déjà validés :

```text
SourceObjectId + SourceEvent -> TargetObjectId + TargetCommand
```

Les objets ne doivent toujours pas se connaître directement.

### Interface en anglais acceptée

L’interface de l’éditeur peut rester en anglais.

Il faut cependant remplacer les formulations techniques ou ambiguës par des libellés clairs.

Exemples :

```text
Outgoing Links      -> Connectors / Outgoing Connectors
Incoming Links      -> Incoming Connectors
Accepted Archetype  -> Accepted Item
Initial Contained   -> Initial Content
```

---

## Problème actuel

L’inspecteur actuel affiche simultanément :

- l’identité brute de l’objet ;
- sa position ;
- son archétype ;
- son tag ;
- ses états initiaux ;
- ses paramètres de comportement ;
- ses liens ;
- ses commandes ;
- ses paramètres de debug ;
- des boutons d’action peu utilisés.

Cette présentation est utilisable pour du debug, mais elle n’est pas assez intuitive pour construire rapidement un donjon.

Le designer doit comprendre immédiatement :

```text
Quel est cet objet ?
Où est-il placé ?
Que fait-il ?
Quels événements émet-il ?
Quelles commandes reçoit-il ?
À quels autres objets est-il connecté ?
```

---

## Principe UX cible

L’éditeur doit présenter chaque objet comme un `Game Object` composé de `Components`, à la manière de *Legend of Grimrock 2*.

Même si tous ces composants ne correspondent pas forcément à des `UActorComponent` Unreal, l’interface doit exposer l’objet ainsi :

```text
Game Object
Components
  Model
  Clickable
  Door
  Lever
  Receptacle
  Trigger
  Surface
  Light
  Sound
Advanced / Debug
```

L’objectif est de faire comprendre l’objet par ses capacités, pas par ses champs C++ internes.

---

## Nouvelle structure du panneau Selected Object

Structure cible recommandée :

```text
SELECTED OBJECT

[Icon] Human Readable Name
Object Kind @ Cell / Edge / Facing

▾ Game Object
▾ Components
▸ Advanced / Debug
```

Le panneau `CONNECTORS` est un panneau dédié séparé. Il ne doit pas être dupliqué dans `Selected Object`.

L’en-tête doit afficher en priorité :

- nom lisible ;
- type lisible ;
- cellule ;
- edge ou facing ;
- état principal si pertinent.

`Selected Object` ne doit pas répéter dans `Game Object` les champs déjà présents dans l’en-tête, notamment `Cell X`, `Cell Y`, `Edge` ou `Facing`.

Exemple :

```text
[Lever Icon] Lever
Wall Mechanism @ (11,3) East
Initial State: Deactivated
```

ou :

```text
[Door Icon] Portcullis Door
Door @ (3,4) North
Initial State: Closed
```

---

## Boutons à retirer de l’interface principale

Les boutons suivants ne doivent plus apparaître comme actions principales permanentes :

```text
Focus Selected Object
Apply Selected Object
Reset Behavior From Archetype
Select Target
Clear Links
APPLY BEHAVIOR
```

### Remplacement recommandé

| Bouton actuel | Décision | Remplacement recommandé |
|---|---|---|
| `Focus Selected Object` | Retirer | Raccourci viewport `F` ou menu contextuel |
| `Apply Selected Object` | Retirer | Auto-apply lors de la modification des champs |
| `Reset Behavior From Archetype` | Masquer | Menu `Advanced / Debug` ou menu `...` |
| `Select Target` | Remplacer | Mode `Pick Target in Viewport` dans `+ Add Connector` |
| `Clear Links` | Masquer | Action dangereuse dans menu `...`, avec confirmation |
| `APPLY BEHAVIOR` | Retirer | Auto-apply des champs contextuels |

L’interface doit éviter de donner l’impression que l’utilisateur doit valider manuellement chaque modification.

---

## Champs à masquer dans Advanced / Debug

Les champs suivants ne doivent pas être visibles par défaut :

```text
ObjectId
Raw Type
Raw ArchetypeId
Raw Tag
Internal Notes
Raw Accepted Archetype Ids
Raw Rejected Archetype Ids
Raw Accepted Item Tags
Raw Rejected Item Tags
Runtime Actor Class
Editor Preview Mesh
Runtime Spawn Offset
Debug Flags
```

Ils restent utiles, mais uniquement dans une section fermée par défaut :

```text
▸ Advanced / Debug
```

---

## Distinction entre Archetype et Instance

L’interface doit clairement distinguer :

### Archetype

Définition générale de l’objet.

Exemples :

```text
Door_Portcullis
Button_Secret
Receptacle_WallTorchHolder
Receptacle_Alcove
Lock_BrassKey
```

L’archetype définit typiquement :

- nom affiché ;
- catégorie ;
- placement autorisé ;
- classe runtime ;
- mesh de preview ;
- paramètres par défaut ;
- composants affichés ;
- événements émis possibles ;
- commandes acceptées possibles.

### Instance placée

Objet concret placé dans le niveau.

L’instance définit typiquement :

- position ;
- edge ou facing ;
- état initial spécifique ;
- item initial ;
- liens sortants ;
- liens entrants ;
- overrides éventuels.

L’inspecteur doit donc éviter de mélanger les propriétés d’archétype et les propriétés d’instance.

---

## Interface contextuelle par type d’objet

Chaque objet doit afficher uniquement les sections pertinentes.

### Door

Inspiré de `dungeon_door_portcullis`.

```text
SELECTED OBJECT
[Door Icon] Portcullis Door
Door @ (3,4) North

▾ Game Object
Name: dungeon_door_portcullis

▾ Door
Initial State: [Closed ▼]
Pull Chain: [ ]
Blocking: Yes
Opening Mode: Vertical Slide

▸ Advanced / Debug
ObjectId
ArchetypeId
Runtime Actor Class
Raw Door Params
```

Une porte est principalement une cible de commandes.

Ses connecteurs entrants sont affichés dans le panneau dédié `CONNECTORS`.

---

### Pressure Plate

Inspiré de `dungeon_pressure_plate`.

```text
SELECTED OBJECT
[Pressure Plate Icon] Pressure Plate
Floor Trigger @ (12,6)

▾ Game Object
Name: dungeon_pressure_plate

▾ Floor Trigger
Triggered By Party:   [x]
Triggered By Monster: [x]
Triggered By Item:    [x]
Triggered By Digging: [ ]
Disable Self:         [ ]

▸ Advanced / Debug
ObjectId
ArchetypeId
Raw Trigger Params
```

Une plaque de pression est un déclencheur de sol.

Elle doit exposer clairement ce qui peut l’activer.

---

### Lever

Inspiré de `lever_1`.

```text
SELECTED OBJECT
[Lever Icon] Lever
Wall Mechanism @ (11,3) East

▾ Game Object
Name: lever

▾ Lever
Initial State: [Deactivated ▼]
Disable Self: [ ]

▸ Advanced / Debug
ObjectId
ArchetypeId
Raw Lever Params
```

Le levier doit afficher son état initial. Ses connecteurs sont affichés dans le panneau dédié `CONNECTORS`.

---

### Button / Secret Button / Wall Button

Inspiré de `dungeon_secret_button_small`.

```text
SELECTED OBJECT
[Button Icon] Secret Button
Wall Button @ (23,5) East

▾ Game Object
Name: dungeon_secret_button_small

▾ Button
Button Type: Secret
Initial State: Released
Disable Self: [ ]

▸ Advanced / Debug
ObjectId
ArchetypeId
Raw Button Params
```

Les boutons `Button`, `Secret Button` et `Wall Button` restent des objets distincts dans la palette, même s’ils partagent une classe runtime.

---

### Lock

Inspiré de `lock_1`.

```text
SELECTED OBJECT
[Lock Icon] Brass Key Lock
Lock @ (20,7) South

▾ Game Object
Name: lock

▾ Lock
Opened By: [brass_key ▼]
Consumes Key: [ ]

▸ Advanced / Debug
ObjectId
ArchetypeId
Raw Required Item Id
```

Une serrure peut techniquement partager une base avec les réceptacles, mais elle doit être affichée comme une serrure, pas comme un réceptacle générique.

---

### Alcove

Inspiré de `dungeon_alcove`.

```text
SELECTED OBJECT
[Alcove Icon] Alcove
Wall Surface @ (15,11) West

▾ Game Object
Name: dungeon_alcove

▾ Surface / Container
Initial Items:
- iron_key

[+ Add Item]

▸ Advanced / Debug
ObjectId
ArchetypeId
Raw Item List
```

Une alcôve est un conteneur de surface.

Elle ne doit pas être affichée comme un support de torche, une serrure ou un autel.

---

### Torch Holder / Receptacle

Inspiré du support de torche actuel.

```text
SELECTED OBJECT
[Torch Holder Icon] Wall Torch Holder
Receptacle @ (28,27) East

▾ Game Object

▾ Receptacle
Accepted Item: [Torch ▼]
Initial Content: [Torch ▼]
Accept Any Item: [ ]
Lock After Insert: [ ]
Consume Inserted Item: [ ]

▸ Advanced / Debug
ObjectId
ArchetypeId
Raw Accepted Archetype Ids
Raw Accepted Tags
```

Le support de torche est un réceptacle paramétré.

La torche ne doit pas être codée en dur dans le support.

---

## Comportements contextuels

Les comportements doivent être édités uniquement dans les sections contextuelles de l'objet sélectionné : `Door`, `Lever`, `Button`, `Pressure Plate`, `Trigger`, `Receptacle`, `Teleporter`, `Light` ou `Readable Text`. Les connecteurs sont édités dans le panneau dédié `CONNECTORS`.

Il ne doit plus exister de panneau global `BEHAVIOR EDITOR`.

Les anciens champs génériques suivants ne doivent plus apparaître :

```text
Trigger Mode
Delay
Duration
Invert Connectors
Fire On Enter
Fire On Exit
Item Spawn / SpawnedItemArchetypeId
```

Le comportement gameplay doit être exprimé par des connecteurs explicites `Source Object / Event / Target Object / Command`.

---

## Connectors

Le terme `Links` peut rester en interne, mais l’interface doit privilégier le terme `Connectors`, plus proche de l’éditeur de *Legend of Grimrock 2*.

### Présentation cible

Au lieu de :

```text
Item Removed -> Open -> Door (28,27 Edge=North)
```

Afficher :

```text
On Item Removed  -> Door @ (28,27) North : Open
On Item Inserted -> Door @ (28,27) North : Close
```

ou sous forme groupée :

```text
Connectors
  On Item Removed
    -> Door @ (28,27) North : Open

  On Item Inserted
    -> Door @ (28,27) North : Close
```

### Création d’un connector

Le header `CONNECTORS` doit proposer un petit bouton `+`.

Ce bouton ouvre ou ferme un formulaire inline :

```text
Source Object: [filtered dropdown]
Event:         [context dropdown]
Target Object: [filtered dropdown]
Command:       [context dropdown]

[Create] [Cancel]
```

Le formulaire remplace l'ancien workflow :

```text
New Connector Event
Link
```

Ces contrôles ne doivent plus apparaître.

Le compteur résumé `Outgoing N Incoming M` sous le titre `CONNECTORS` ne doit plus apparaître non plus. Les groupes `Outgoing` et `Incoming` restent la source de vérité visuelle.

L’éditeur doit filtrer les choix :

- `Source Object` affiche uniquement les objets capables d'émettre des événements de gameplay ;
- `Event` affiche uniquement les événements pertinents pour la source sélectionnée ;
- `Target Object` affiche uniquement les objets capables de recevoir une commande significative ;
- `Command` affiche uniquement les commandes pertinentes pour la cible sélectionnée.

Sources actuellement attendues :

```text
Button
Lever
PressurePlate
Trigger
Receptacle
MonsterSpawn
```

Targets actuellement attendues :

```text
Door
Secret Door
Teleporter
Light
Receptacle
MonsterSpawn
```

Objets exclus des sources ou des targets selon leur rôle actuel :

```text
WallInscription
readable-only
decorations
items simples
objets purement visuels
Button / Lever / PressurePlate comme targets
ItemSpawn comme target tant que le spawn commandé reste TODO runtime
```

Pour `MonsterSpawn`, MON13.3 expose `MonsterDied`, `MonsterSpawned`,
`MonsterDespawned` et `MonsterTeleported` comme événements, puis `Spawn`,
`Despawn`, `Teleport` et leurs alias comme commandes.

MON13.4 ajoute `EncounterWaveStarted`, `EncounterCompleted` et la commande
`StartEncounter`. La commande exige un `EncounterGroupId`; l'indice de vague
est éditable dans Selected Object via `EncounterWaveIndex`.

Règles de lecture :

- `Readable` ne signifie pas `Event Source` ;
- `Interactable` ne signifie pas `Command Target`.

Ce filtrage est uniquement UI.

Il ne modifie pas les enums.

### Légende couleur

La section `CONNECTORS` doit afficher une légende compacte :

```text
Cyan = Outgoing
Purple = Incoming
Red = Broken
```

Le rouge signifie uniquement `broken connector` : objet manquant, cible introuvable ou lien invalide.

Le rouge ne doit jamais signifier `Close`, `Item Inserted` ou tout autre événement / commande valide.

---

## Flèches de liens dans le viewport

Les flèches doivent être améliorées afin de comprendre rapidement les puzzles.

### Règles de dessin

Quand un objet est sélectionné :

- afficher ses connecteurs sortants ;
- optionnellement afficher ses connecteurs entrants ;
- dessiner une flèche en trait pointillé entre source et cible ;
- la base de la flèche doit partir du centre logique de l’objet source ;
- la pointe doit arriver au centre logique de l’objet cible ;
- afficher une pointe de flèche claire ;
- afficher un label court optionnel.

Exemple :

```text
Lever - - - - - - - - - - > Door
       On Activate / Open
```

### Centre logique selon le type d’objet

| Type d’objet | Centre logique recommandé |
|---|---|
| Cell object | Centre de la cellule |
| Floor object | Centre de cellule, légèrement au-dessus du sol |
| Wall object | Centre de la face murale |
| Door / Edge object | Centre de l’edge entre deux cellules |
| Receptacle mural | Centre de la face murale, hauteur médiane |
| Ceiling object | Centre de cellule, hauteur plafond |

Il ne faut pas utiliser aveuglément le pivot du mesh ou de l’acteur.

### États visuels recommandés

| Type de lien | Affichage recommandé |
|---|---|
| Outgoing selected | Trait pointillé clair |
| Incoming selected | Trait pointillé d’une autre couleur |
| Broken link | Rouge / warning |
| Hovered link | Accentué |
| Disabled link | Atténué |

---

## GridObjectArchetypeAsset — clarification nécessaire

Le `GridObjectArchetypeAsset` contient beaucoup de champs.

Il faut clarifier leur rôle, car tous les champs ne concernent pas tous les objets.

### Problème

Un même asset semble porter plusieurs familles de données :

- identité ;
- catégorie ;
- placement ;
- preview éditeur ;
- classe runtime ;
- meshes ;
- offsets ;
- comportement ;
- interaction ;
- paramètres de porte ;
- paramètres de trigger ;
- paramètres de réceptacle ;
- paramètres de spawn ;
- debug.

Cette richesse est utile, mais elle rend l’éditeur difficile à comprendre si tout est exposé au même endroit.

### Action demandée

Créer un audit de `GridObjectArchetypeAsset` avec le tableau suivant :

| Field Name | Type | Used By | Used In Editor | Used In Runtime | Object Types Concerned | Instance Override | Keep / Hide / Remove | Comment |
|---|---|---|---|---|---|---|---|---|

Ce travail doit permettre de classer chaque champ dans un des niveaux suivants :

### Essential

Visible dans l’inspecteur courant.

Exemples :

```text
Display Name
Initial State
Accepted Item
Initial Content
Triggered By Party
```

### Advanced / Debug

Masqué par défaut.

Exemples :

```text
ObjectId
ArchetypeId
Runtime Actor Class
Raw Tags
Raw Offsets
```

### Archetype Only

Visible uniquement lors de l’édition de l’archétype, pas de l’instance placée.

Exemples :

```text
Preview Mesh
Default Material
Default Placement Type
Default Runtime Actor Class
Default Component List
```

---

## Roadmap UX proposée

### Phase 1 — Nettoyage immédiat de l’inspecteur

Objectif : réduire la confusion sans modifier le runtime.

À faire :

1. Retirer les boutons inutiles de l’interface principale.
2. Ajouter les sections :
   - `Game Object` ;
   - `Components` ;
   - `Connectors` ;
   - `Advanced / Debug`.
3. Masquer les champs techniques par défaut.
4. Remplacer `Links` par `Connectors` dans l’UI.
5. Afficher les liens sous forme lisible.
6. Conserver les enums existants.

### Phase 2 — Inspecteur contextuel par type

Objectif : afficher uniquement les champs utiles.

À faire :

```cpp
BuildDoorDetails(...)
BuildPressurePlateDetails(...)
BuildLeverDetails(...)
BuildButtonDetails(...)
BuildReceptacleDetails(...)
BuildLockDetails(...)
BuildAlcoveDetails(...)
BuildTriggerDetails(...)
BuildTeleporterDetails(...)
BuildSpawnDetails(...)
```

Chaque builder doit afficher les champs métier de l’objet sélectionné.

### Phase 3 — Connectors façon Grimrock 2

Objectif : simplifier la lecture des puzzles.

À faire :

1. Afficher les connecteurs sous le composant concerné.
2. Grouper par événement source.
3. Afficher la cible sous forme lisible.
4. Ajouter `+ Add Connector`.
5. Ajouter `Pick Target in Viewport`.
6. Filtrer les commandes selon la cible lorsque possible.

### Phase 4 — Flèches pointillées dans le viewport

Objectif : rendre les liens visibles spatialement.

À faire :

1. Calculer `GetObjectEditorWorldCenter` selon le type d’objet.
2. Dessiner des traits pointillés source -> cible.
3. Ajouter une pointe de flèche.
4. Ajouter un label court optionnel.
5. Afficher les liens sortants de l’objet sélectionné.
6. Ajouter plus tard `Show All Connectors`.

### Phase 5 — Audit de GridObjectArchetypeAsset

Objectif : comprendre et nettoyer l’asset central.

À faire :

1. Lister tous les champs.
2. Dire quels objets les utilisent.
3. Dire s’ils sont runtime, editor ou les deux.
4. Dire s’ils sont modifiables par instance.
5. Classer chaque champ : `Essential`, `Advanced`, `Archetype Only`, `Remove Later`.

### Phase 6 — Validation de niveau

Objectif : détecter les erreurs avant le PIE.

À faire plus tard :

```text
Validate Level
```

Cette validation devra détecter :

- lien vers objet supprimé ;
- commande invalide pour la cible ;
- événement incohérent pour la source ;
- archetype absent ;
- runtime actor manquant ;
- item initial inconnu ;
- porte sans edge valide ;
- objet mural sans support ;
- doublon d’ObjectId.

---

## Tâches Codex recommandées

### Tâche 1 — Nettoyer les boutons de l’inspecteur

Supprimer de la vue principale :

```text
Focus Selected Object
Apply Selected Object
Reset Behavior From Archetype
Select Target
Clear Links
APPLY BEHAVIOR
```

Les actions dangereuses ou rares doivent être déplacées dans `Advanced / Debug` ou dans un menu contextuel.

### Tâche 2 — Renommer Links en Connectors dans l’UI

Ne pas renommer les structures C++ si cela implique un refactor.

Changer uniquement les labels Slate affichés.

### Tâche 3 — Ajouter les sections repliables

Ajouter :

```text
Game Object
Components
Connectors
Advanced / Debug
```

La section `Advanced / Debug` doit être fermée par défaut.

### Tâche 4 — Ajouter l’inspecteur contextuel Receptacle

Premier cas concret à traiter : `Receptacle_WallTorchHolder`.

Objectif : afficher :

```text
Accepted Item
Initial Content
Accept Any Item
Connectors
```

et masquer les champs bruts.

### Tâche 5 — Améliorer les flèches de liens viewport

Objectif : flèche pointillée source -> cible.

La base doit partir du centre logique de la source.

La pointe doit arriver au centre logique de la cible.

### Tâche 6 — Produire l’audit GridObjectArchetypeAsset

Créer un document séparé :

```text
docs/Design/07_GRID_OBJECT_ARCHETYPE_ASSET_AUDIT.md
```

Ce document devra lister tous les champs de `GridObjectArchetypeAsset` et dire à quoi ils servent.

---

## Résultat attendu

L’éditeur doit permettre de lire un objet ainsi :

```text
Cet objet est un levier.
Il est désactivé au départ.
Quand il est activé, il ouvre cette porte.
Quand il est désactivé, il ferme cette porte.
```

ou :

```text
Cet objet est un support de torche.
Il accepte une torche.
Il contient une torche au départ.
Quand la torche est retirée, il ouvre une porte.
Quand la torche est insérée, il ferme cette porte.
```

L’utilisateur ne doit plus devoir interpréter en permanence des champs techniques tels que :

```text
ObjectId
ArchetypeId
Raw Tags
Outgoing Links
```

Les anciens champs `Trigger Mode`, `Fire On Enter`, `Fire On Exit`, `Delay`, `Duration`, `Invert Connectors` et `Item Spawn` ne sont plus des données disponibles dans l'inspector.

Ces données techniques restantes restent disponibles, mais elles ne doivent plus dominer l’interface.
---

## 2026-05-22 - Selected Object Layout Update

The current `Selected Object` inspector must avoid duplicating information already shown in its header.

`Game Object` shows only:

- `Placement Kind`
- `Palette Category`
- `Functional Category`
- `Runtime Interactable`
- `Runtime Readable`
- `Runtime Light Source`
- `Enabled at Start`
- `Active at Start`

`Gameplay Type`, `Cell X`, `Cell Y`, and `Edge / Facing` are not repeated in `Game Object`; they remain visible in the object header as `DisplayName` and `Type @ (X,Y) Edge/Facing`.

The duplicated `Connectors` section is removed from `Selected Object`. Connectors are shown only in the dedicated `CONNECTORS` panel.

The `Rotate 90 deg` action is replaced by a `North / East / South / West` orientation widget. For edge-placed objects it changes the placement edge. For center/floor objects it changes the facing yaw.

The orientation widget is shown for visible orientable objects using `PlacementKind = Edge`, `Wall`, `Floor`, or `Center`. Visible floor decorations and visible floor items are orientable. The widget is hidden only for purely logical or invisible objects, such as invisible triggers or invisible spawns.

The `CONNECTORS` panel hides the `+` action for selected objects that can neither emit events nor receive commands. Ground items such as `Item_Torch` are placeable, physical and pickupable, but they are not connector sources or connector targets.

`Advanced / Debug` is primarily read-only: `ObjectId`, `ArchetypeId`, and `Tag` are read-only, while `Notes` remains editable.

Receptacles use safe item selectors:

- `Accept Any Item`
- `Accepted Items`, as a selectable list of `Item` archetypes
- `Initial Content`, as a `None + Item archetypes` dropdown

Advanced receptacle rules such as rejected archetypes and accepted item tags are not exposed by default. If `Initial Content` is not compatible with `Accepted Items`, the inspector shows a warning without blocking save.
