# GrimrockPrototype — Decisions Log

Ce fichier conserve les décisions validées afin de ne pas perdre la connaissance acquise entre les sessions ChatGPT, Codex et les commits Git.

---

## 2026-05-19 — Organisation du chantier objets / mécanismes / connexions

### Décisions validées

- Les objets placés dans le donjon doivent être simplifiés et rapprochés de l’esprit *Legend of Grimrock*.
- La logique doit reposer sur une séparation entre :
  - événements émis ;
  - commandes reçues ;
  - liens logiques.
- Le modèle cible est :

```text
SourceObjectId + SourceEvent -> TargetObjectId + TargetCommand
```

- Les objets ne doivent pas se connaître directement.
- Un bouton ne doit pas ouvrir une porte directement.
- Un bouton émet un événement.
- Le système de liens exécute une commande sur la porte.
- Les portes doivent être principalement des cibles de commandes.
- Le runtime doit évoluer vers un dispatcher central, probablement `UGridActivationComponent`.

---

## 2026-05-19 — Objets concrets vs comportements factorisés

### Décisions validées

- Les objets doivent rester distincts dans la palette et dans les DataAssets lorsqu’ils sont visuellement différents.
- `Button_Normal`, `Button_Secret` et `Button_Wall` sont trois objets distincts.
- Ces trois boutons peuvent partager la même classe C++ :

```cpp
AGridButtonActor
```

- Ils sont différents visuellement, mais pas nécessairement conceptuellement.
- Les différences doivent être portées par les archétypes, les meshes, les matériaux et les paramètres.

---

## 2026-05-19 — Catégories d’objets

### Décisions validées

- `EGridObjectCategory` doit rester conservateur.
- Il ne faut pas supprimer inutilement les catégories existantes.
- `Spawn` doit être conservé.
- `Readable` doit être conservé.
- Les catégories recommandées sont :

```text
None
Mechanism
Receptacle
Passage
Item
Decoration
Readable
Spawn
Trigger
Light
Hazard
```

- `Light` et `Hazard` peuvent être ajoutés plus tard si nécessaire.
- `Category` sert d’abord à classer l’objet dans l’éditeur et la palette, pas à contenir toute la logique runtime.

---

## 2026-05-19 — WallInscription

### Décisions validées

- `WallInscription` est l’objet existant.
- Il ne doit pas être renommé en `Inscription`.
- Il appartient à la catégorie `Readable`.
- Il peut émettre `OnUse`.
- Il doit conserver son rôle narratif actuel.

---

## 2026-05-19 — Porte secrète

### Décisions validées

- La porte secrète doit être un objet explicite.
- Elle appartient à la catégorie `Passage`.
- Elle doit probablement reposer sur :

```cpp
AGridSecretDoorActor
```

- `Door_Secret` doit être un archétype distinct de `Door_Stone`.
- Le principe de mesh fixe + mesh mobile doit être conservé.
- La partie fixe doit être visible en édition et en runtime.

---

## 2026-05-19 — Réceptacles

### Décisions validées

- Les réceptacles concrets suivants doivent exister :
  - `Alcove`
  - `TorchHolder`
  - `Altar`
  - `OfferingBowl`
  - `CoinSlot`
  - `Lock/Keyhole`

- Ils peuvent partager une classe C++ :

```cpp
AGridReceptacleActor
```

- Le support de torche ne doit pas coder la torche en dur.
- Le support de torche est un réceptacle paramétré.
- La torche est un item.
- La lumière est un effet lié à la présence d’un item valide dans le réceptacle.

---

## 2026-05-19 — Événements

### Décisions validées

Événements recommandés :

```text
OnActivate
OnDeactivate
OnToggle
OnEnter
OnExit
OnInsertItem
OnRemoveItem
OnUse
OnUnlock
OnTimer
OnSpawn
```

---

## 2026-05-19 — Commandes

### Décisions validées

Commandes recommandées :

```text
Activate
Deactivate
Toggle
Open
Close
ToggleOpen
Lock
Unlock
Enable
Disable
StartTimer
StopTimer
ResetTimer
Teleport
Spawn
Destroy
ShowText
PlayAnimation
PlaySound
```

---

## 2026-05-19 — Organisation ChatGPT / Codex / Git

### Décisions validées

- ChatGPT ne doit pas être la mémoire principale du projet.
- Codex ne doit pas être la mémoire principale du projet.
- La mémoire stable doit être dans le dépôt Git, dans `Docs/Design`.
- Les décisions importantes doivent être ajoutées dans ce fichier.
- Codex doit recevoir des tâches courtes, ciblées, avec fichiers autorisés.
- Il ne faut pas demander à Codex de refactoriser tout le système d’un coup.
- Chaque tâche doit produire :
  - un diff minimal ;
  - une compilation ;
  - un test UE5 ;
  - un commit Git ;
  - une mise à jour éventuelle des documents.

---

## 2026-05-19 — Feuille de route validée

### Étapes validées

1. Créer la documentation de référence.
2. Ajouter les types C++ `EGridObjectEvent`, `EGridObjectCommand`, `FGridObjectLink`.
3. Adapter progressivement `UGridObjectArchetypeAsset`.
4. Créer ou corriger les archétypes concrets.
5. Créer ou consolider un dispatcher runtime.
6. Brancher d’abord Button -> Door.
7. Brancher ensuite Lever, PressurePlate, Receptacle.
8. Adapter l’inspecteur éditeur.
9. Créer une carte de test.
10. Nettoyer l’ancien code après validation.

---

## 2026-05-19 — Clôture du chantier Event -> Command / Archétypes

### Synthèse des patchs réalisés

- Patch A : `EGridObjectEvent`, `EGridObjectCommand` et `FGridObjectLink` ont été finalisés.
- Patch B : la palette éditeur est compatible avec plusieurs `ArchetypeId` pour un même type conceptuel.
- Patch C : les archétypes concrets attendus ont été normalisés.
- Patch D : `Door_Secret` a été validée comme archétype `Door`.
- Patch E : `Receptacle_Alcove`, `Receptacle_TorchHolder`, `Receptacle_Altar` et `Receptacle_OfferingBowl` ont été validés comme archétypes `Receptacle`.
- Patch F : les règles génériques d’acceptation/refus des items par réceptacle ont été ajoutées.
- Patch G : la validation éditeur signale les liens incomplets de réceptacle.

### Décision

Le chantier Event -> Command / Archétypes est clos.

Les variantes visuelles passent par `ArchetypeId` et par les assets d’archétype, pas par multiplication de `EGridLevelObjectType`.

---

## Suites possibles, hors chantier clos

- Améliorer le preview éditeur composite des portes secrètes.
- Ajouter la logique `TorchHolder` -> lumière / effet visuel.
- Implémenter les commandes avancées `Teleport`, `Spawn`, `ShowMessage`, `Lock` et `Unlock`.
- Créer une carte de test dédiée aux mécanismes.
- Nettoyer les anciens comportements legacy après sauvegarde/migration des assets.

## 2026-05-20 — Phase 1 UX Grid Editor

- Suppression des boutons d’action inutiles de l’inspecteur.
- Remplacement visuel de Links par Connectors.
- Suppression du bouton APPLY BEHAVIOR.
- Application automatique des changements de comportement.
- Suppression du bloc danger Clear Links.
- Conservation de Move To Current Cell et Rotate 90°.
- Aucun changement apporté à EGridObjectEvent ni EGridObjectCommand.

## 2026-05-20 — Phase 2B UX Grid Editor

- L’inspecteur contextualisé affiche désormais les informations utiles selon le type d’objet sélectionné.
- Le header utilise `Archetype->DisplayName` lorsque disponible.
- La section `Game Object` affiche les informations essentielles de placement, catégorie, interaction et blocage.
- Les sections `Door`, `Lever`, `Button`, `Pressure Plate`, `Teleporter`, `Light` et `Receptacle` exposent les champs déjà existants.
- `Advanced / Debug` conserve les champs techniques : `ObjectId`, `ArchetypeId`, `Tag`, `Notes`, `RuntimeActorClass`, `PreviewMesh`, `FixedMesh`, `MovingMesh`.
- Aucun champ sérialisé n’a été ajouté.
- Aucun changement apporté à `EGridObjectEvent`, `EGridObjectCommand`, `FGridObjectLink`, au runtime dispatcher ou aux assets.

## 2026-05-20 — Phase 2C UX Grid Editor

- L’inspecteur affiche désormais une section `Connectors` directement dans l’objet sélectionné.
- Les connecteurs sortants sont regroupés par événement source.
- Les connecteurs entrants sont affichés séparément avec la source, l’événement et la commande.
- Les objets liés utilisent `Archetype->DisplayName` lorsque disponible.
- Les liens cassés affichent `Missing object` en warning.
- Aucun workflow de création de connecteur n’a été ajouté à ce stade.
- Aucun changement apporté à `EGridObjectEvent`, `EGridObjectCommand`, `FGridObjectLink`, au runtime dispatcher ou à la sérialisation.

## 2026-05-20 — Phase 3A Fix UX Grid Editor

- Les flèches de connecteurs utilisent désormais le même calcul de placement que les previews éditeur via `GetObjectPlacementTransform()` lorsque disponible.
- Le fallback tient compte de `PlacementZOffset`, `WallInset`, `LocalOffsetAlongWall`, `LocalOffsetVertical`, `PlacementKind` et `Edge`.
- Les portes sont ciblées au milieu vertical du passage plutôt qu’au sol.
- Les flèches sont plus fines, plus discrètes, avec des dashes et une tête de flèche réduits.
- Aucun changement apporté au runtime, à la sérialisation, aux enums ou au modèle de liens.

## 2026-05-20 — Phase 3B UX Grid Editor

- Le viewport affiche désormais les connecteurs sortants et entrants de l’objet sélectionné.
- Les connecteurs conservent toujours leur direction logique réelle `Source -> Target`.
- Deux options éditeur ont été ajoutées : `bShowOutgoingConnectors` et `bShowIncomingConnectors`.
- Deux checkboxes ont été ajoutées dans le toolkit : `Show Outgoing Connectors` et `Show Incoming Connectors`.
- Les connecteurs entrants utilisent une couleur distincte, violet/bleu, plus discrète.
- Le rendu réutilise `GetObjectEditorWorldCenter()` pour garantir un départ et une arrivée cohérents avec les previews.
- Aucun changement apporté au runtime, à la sérialisation, aux enums ou au modèle de liens.

## 2026-05-20 — Phase 3C UX Grid Editor

- Le viewport affiche désormais des labels légers sur les flèches de connecteurs.
- Les labels utilisent les display names des enums au format `Event / Command`.
- Les labels sont dessinés via `FCanvas`, sans actor, composant ni `UTextRenderComponent`.
- Les labels ne sont affichés que pour les connecteurs dont la flèche est visible.
- Une option `bShowConnectorLabels` a été ajoutée à l’acteur éditeur.
- Une checkbox `Show Connector Labels` a été ajoutée au toolkit.
- Aucun changement apporté au runtime, à la sérialisation, aux enums ou au modèle de liens.

## 2026-05-20 — Phase 4B Clarifications UX GridObjectArchetypeAsset

- Clarification des labels et tooltips éditeur pour les champs ambigus d'archétype.
- `PreviewMesh` / `PreviewMaterial` sont affichés comme mesh/matériau principaux ainsi que mesh/matériau de preview.
- `SupportedType`, `Category` et `ObjectCategory` sont présentés comme type gameplay, catégorie de palette et catégorie fonctionnelle.
- Les flags legacy de placement restent advanced et documentés comme champs de compatibilité.
- `bBlocksMovement`, `bIsReadable`, `bIsLightSource` et `bUseLightFlicker` ont un wording éditeur plus clair.
- Aucun nom de champ sérialisé, comportement runtime, enum, DataAsset ou modèle de liens n'a été modifié.

## 2026-05-20 — Phase 5A DataAsset Naming Normalization

- Les DataAssets principaux ont été renommés directement dans UE5, testés et validés.
- Les anciens noms orientés puzzle/test ont été remplacés par des noms génériques.
- Les noms canoniques actuels sont :
  - `Button_Normal`
  - `Button_Secret`
  - `Button_Wall` à créer
  - `Lever`
  - `PressurePlate`
  - `Trigger`
  - `Door_Stone`
  - `Door_Secret`
  - `Receptacle_TorchHolder`
  - `Item_Torch`
  - `ItemSpawn_Torch`
  - `WallInscription`
- Le choix a été fait de préférer `Lever`, `PressurePlate` et `Trigger` aux formes longues tant qu’il n’existe pas plusieurs variantes.
- Aucun changement C++ ou runtime n’a été nécessaire.

## 2026-05-21 — CONNECTORS UI Filtering and Explicit Event Semantics

- Le panneau CONNECTORS utilise désormais un formulaire `Source / Event / Target / Command`.
- Les sources sont filtrées pour n’afficher que les objets capables d’émettre des événements.
- Les targets sont filtrées pour n’afficher que les objets capables de recevoir des commandes.
- Les readable-only, décorations et objets purement visuels sont exclus des sources.
- Les boutons, leviers, plaques, réceptacles et inscriptions sont exclus des targets pour l’instant.
- `ItemSpawn` et `MonsterSpawn` sont exclus temporairement des targets tant que le spawn commandé reste TODO runtime.
- Le rouge est réservé aux connecteurs cassés.
- Décision de design : un connecteur doit s’exécuter uniquement pour son événement explicite ; aucune inversion implicite ne doit être déduite par le runtime.

## 2026-05-21 — Suppression de l’ancien Behavior Editor générique

- Le panneau global `BEHAVIOR EDITOR` a été supprimé.
- Les champs génériques `TriggerMode`, `Delay`, `Duration`, `Invert Connectors`, `Fire On Enter`, `Fire On Exit` et `Item Spawn` ont été supprimés.
- Les structs `FGridActivationBehaviorParams`, `FGridTriggerBehaviorParams` et `FGridItemSpawnBehaviorParams` ont été supprimées.
- `FGridObjectBehaviorParams` conserve uniquement les comportements encore utiles : `Teleporter`, `Receptacle`, `ButtonAnimation`.
- Le runtime reste basé sur l’exécution explicite des connecteurs par `SourceEvent`.
- Aucun fallback implicite ni inversion automatique n’est conservé.

## 2026-05-21 — Item_Torch placé et récupérable

- `Item_Torch` est un item placé manuellement dans le niveau, pas un spawner.
- `ItemSpawn_Torch` n’est pas utilisé pour représenter les torches posées à la main.
- Une torche placée au sol est éteinte : mesh uniquement, sans flamme Niagara ni lumière active.
- La torche peut être placée près d’un edge de cellule pour rester lisible dans le donjon.
- Le joueur peut la ramasser depuis la même cellule avec l’action `Use`.
- Quand `Item_Torch` est ramassée, elle est ajoutée à l’état d’inventaire léger du Pawn et équipée automatiquement en main.
- La torche tenue en main utilise le chemin d’item tenu existant et active sa flamme/lumière via `OnPlacedInWorld`.
---

## 2026-05-22 - Simplification du panneau Selected Object

- La section `Connectors` dupliquee dans `Selected Object` est supprimee. Les connecteurs restent visibles uniquement dans le panneau dedie `CONNECTORS`.
- `Game Object` ne repete plus `Gameplay Type`, `Cell X`, `Cell Y` ni `Edge / Facing`; ces informations restent dans le header de l'objet selectionne.
- `Initially Enabled` et `Initially Active` sont renommes `Enabled at Start` et `Active at Start`.
- `Advanced / Debug` devient principalement read-only : `ObjectId`, `ArchetypeId` et `Tag` sont en lecture seule ; `Notes` reste editable.
- Le bouton `Rotate 90 deg` est remplace par un widget d'orientation `North / East / South / West`.
- L'edition des receptacles utilise des listes d'items pour `Accepted Items` et un dropdown `None + Item archetypes` pour `Initial Content`.
- Les regles avancees de receptacle, comme rejected archetypes et accepted item tags, ne sont pas exposees par defaut.

---

## 2026-05-22 - Orientation et connecteurs non logiques

- `RotationStepYaw` a ete supprime.
- L'orientation est desormais pilotee uniquement par `Edge / Facing` via le widget `North / East / South / West`.
- Le widget d'orientation n'apparait que pour les objets orientables.
- Le panneau `CONNECTORS` n'affiche plus le bouton `+` pour les objets qui ne peuvent ni emettre d'evenements ni recevoir de commandes.
- Les items au sol comme `Item_Torch` sont physiques et recuperables, mais ne participent pas au systeme de connecteurs.
- Correction : les objets visibles au sol, notamment les decorations `Floor`, affichent aussi le widget d'orientation. `RotationStepYaw` reste supprime.

---

## 2026-06-14 — Système de serrures, clés, crochetage et conteneurs verrouillables

### Décisions validées

- Les portes de donjon ne portent pas de serrure interne.
- Une porte reste un objet passif commandable par le système de liens.
- Une clé agit sur une serrure ou un mécanisme séparé, jamais directement sur une porte.
- Une serrure murale est un objet placé indépendant, généralement classé comme mécanisme ou réceptacle selon son implémentation.
- Les coffres, boîtes, reliquaires, sarcophages et autres conteneurs peuvent posséder une serrure interne, car ils sont eux-mêmes l'objet verrouillable.
- Le même modèle de données de lock doit servir aux serrures murales et aux conteneurs verrouillables.
- Le crochetage doit être abstrait comme une résolution de compétence et d'outil dans le jeu, sans simulation physique réelle.
- Les serrures piégées doivent prévoir détection, désamorçage, politique de déclenchement et effets par liens ou scripts.
- Le document de référence est `docs/Design/GRIMROCK_LOCK_SYSTEM.md`.

---

## 2026-06-15 — Création des assets d'items ramassables

- Un item ramassable nécessite deux assets distincts : `DA_Item_XXX` pour son identité d'inventaire et de gameplay, et `DA_Object_XXXPickup` pour son placement dans le niveau et son exposition dans la palette.
- Le contenu initial d'un réceptacle référence `DA_Item_XXX`, jamais l'archétype `DA_Object_XXXPickup`.
- Le guide de production est `docs/Design/ITEM_AND_PICKUP_ASSET_CREATION_GUIDE.md`.
- Ce guide complète `docs/Architecture/ITEM_PICKUP_AND_PLACEMENT_FOUNDATION.md` et ne le remplace pas.
