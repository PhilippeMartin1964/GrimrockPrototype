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

---

## 2026-06-15 — Fondation des actions contextuelles d'inventaire

- Le clic droit sur un item d'inventaire devient le point d'entrée principal des actions contextuelles.
- Le drag/drop et le Cursor existants restent disponibles pendant la migration.
- Le premier patch calcule et expose les actions aux Blueprints sans les exécuter.
- Le contexte de cible utilise le premier impact `ECC_Visibility` dans la direction du groupe.
- `Examiner` réutilise les données du tooltip existant.
- `Lancer` n'est proposé que pour un item explicitement lançable.
- Une clé compatible peut proposer `Insérer dans la serrure` lorsque la WallLock verrouillée est en face.
- L'ouverture de porte reste exclusivement `WallLock.Activated -> Door.Open`.
- La suppression de la recherche automatique de clé par simple clic sur la serrure est reportée au patch d'exécution explicite des actions.
- Le document de référence est `docs/Design/ITEM_CONTEXT_ACTION_SYSTEM.md`.

---

## 2026-08-01 — Barre d'initiative du combat V2

- Le HUD affiche en haut-centre le combattant actif puis les prochains
  personnages ou monstres dans l'ordre autoritaire du TurnManager.
- Chaque slot contient le portrait, le camp, l'état de tour, un état vital
  compact et les effets majeurs empêchant d'agir.
- Le slot actif est agrandi ; les slots suivants glissent lorsqu'un tour se
  termine.
- Huit slots sont visibles par défaut. MON12.7.1 remplit cette capacité en
  continuant sur les rounds suivants et remplace l'ancien débordement `+ N`.
- Un combattant vaincu est retiré après notification autoritaire ; un
  combattant incapable d'agir reste lisible avec l'état `Incapacitated` avant
  que son tour soit ignoré.
- La barre est informative : elle ne sélectionne pas de cible et ne déclenche
  aucune action.
- Le widget ne calcule jamais l'initiative et n'utilise aucun `Tick` de
  rafraîchissement. Il lit l'ordre global et ses événements depuis le
  TurnManager.
- Le modèle et les événements autoritaires appartiennent à MON12.4 ; la
  représentation UMG appartient à MON12.7.
- Le document de référence reste
  `docs/Design/COMBAT_SYSTEM_V2_ACTION_POINTS_INITIATIVE.md`.

---

## 2026-08-01 — MON12.3 : état de tour et PA des personnages

- Le verrou `PlayerAttackCommittedCharacterIds` n'est plus l'autorité des
  actions du groupe.
- Chaque personnage possède un `FGridPlayerCharacterTurnState` identifié par
  son `CharacterId` stable.
- Les états communs sont `Waiting`, `Active`, `Completed`, `Incapacitated` et
  `Defeated`.
- Chaque personnage vivant reçoit 4 PA au début de la phase joueur.
- Les attaques existantes coûtent 2 PA ; deux attaques peuvent donc être
  acceptées pendant la même phase.
- Une attaque sans PA est refusée avec `InsufficientActionPoints` sans tirage,
  dégâts ni consommation d'objet.
- Les budgets restent indépendants entre personnages et sont restaurés à la
  manche suivante, sans report des PA inutilisés.
- `PlayerPhase / EnemyPhase` est conservé jusqu'à MON12.4 ; tous les héros
  vivants sont provisoirement `Active` pendant la phase joueur.
- Le panneau lit et affiche l'état ainsi que `PA courant / maximum` et se
  rafraîchit via `OnPlayerCharacterTurnStateChanged`, sans `Tick`.
- Le document d'implémentation est
  `docs/Design/MON12_3_CHARACTER_TURN_ACTION_POINTS.md`.

---

## 2026-08-01 — MON12.4 : initiative globale et tours individuels

- L'initiative est lancée une fois par rencontre pour tous les personnages et
  monstres, puis l'ordre est conservé entre les manches.
- Un flux aléatoire d'initiative dérivé du seed de rencontre est séparé du flux
  des attaques afin que les jets de combat ne soient pas déplacés.
- Le tri utilise le total, la base, la Dextérité finale puis l'identifiant
  stable.
- Un seul combattant est `Active`. Les personnages reçoivent leurs 4 PA au
  début de leur propre tour ; les autres restent `Waiting` ou `Completed`.
- Une attaque provenant d'un autre personnage est refusée avec
  `NotActiveCombatant` sans consommation ni résolution.
- `NumPad 2` termine le tour du personnage actif ; atteindre 0 PA le termine
  automatiquement.
- Les tours de monstres réutilisent leurs budgets, planificateurs et
  présentations existants dans le même ordre global.
- `PlayerPhase` et `EnemyPhase` sont conservés temporairement comme indication
  du camp du combattant actif, et non plus comme phases complètes de camp.
- `InitiativeOrder`, `GetUpcomingInitiativeOrder()`, puis la projection
  multi-round `GetInitiativePreview()`,
  `OnTurnOrderChanged`, `OnActiveCombatantChanged` et
  `OnCombatantStateChanged` forment le contrat autoritaire de la barre UMG
  MON12.7.
- Le document d'implémentation est
  `docs/Design/MON12_4_GLOBAL_INITIATIVE_INDIVIDUAL_TURNS.md`.

---

## 2026-08-02 — MON12.5 : déplacement du groupe avec PA et PAM

- Le groupe possède une réserve commune de `2 PAM`, restaurée au début de
  chaque manche sans report.
- Une translation d'une cellule coûte `1 PA` au personnage actif et `1 PAM`
  au groupe.
- Les quatre translations sont autorisées uniquement pendant le tour d'un
  personnage actif ; les rotations gauche/droite de 90 degrés restent
  gratuites.
- Le TurnManager valide le tour, le repos, la direction, la cellule, le
  passage, l'occupation par un monstre, les PA et les PAM avant tout débit.
- Un refus ne consomme aucune ressource et diffuse une raison explicite via
  `OnPartyMovementRejected`.
- `FGridPartyMobilityState` et `OnPartyMobilityStateChanged` constituent le
  contrat autoritaire de la future représentation UMG.
- Le Pawn conserve l'interpolation. Un tour épuisé par une translation ne se
  termine qu'à l'arrivée sur la case, jamais pendant le mouvement visuel.
- Le déplacement d'exploration hors combat reste inchangé.
- Le document d'implémentation est
  `docs/Design/MON12_5_PARTY_MOVEMENT_ACTION_POINTS.md`.

---

## 2026-08-02 — MON12.6 : définitions et catalogue d'actions

- Le HUD représente des actions disponibles, jamais directement les slots
  `MainHand` et `OffHand`.
- `FGridCombatActionDefinition` est la définition commune aux armes,
  capacités, sorts, objets rapides et actions universelles.
- `FGridAvailableCombatAction` conserve la provenance concrète, les coûts
  courants, l'état disponible et une raison localisable de désactivation.
- Le catalogue est une lecture pure : le TurnManager reste l'autorité qui
  revalide puis consomme les ressources.
- Une arme peut fournir plusieurs actions par
  `UGridItemDefinitionAsset::CombatActions`.
- Les capacités et sorts de classe proviennent de
  `URPGClassAsset::CombatActions` ; le personnage persiste la référence vers
  le DataAsset canonique de sa classe.
- Les armes MON11 non migrées conservent automatiquement une action construite
  depuis `OffensiveProfile`. Une torche sans action ne crée aucune entrée.
- L'attaque MON11 est le premier profil exécuté par
  `RequestCharacterCombatAction()`. Les sorts, capacités, défenses et objets
  rapides restent catalogués mais leurs exécuteurs appartiennent à
  MON12.8–MON12.9.
- Le document d'implémentation est
  `docs/Design/MON12_6_COMBAT_ACTION_CATALOG.md`.

---

## 2026-08-02 — MON12.7 : HUD de combat orienté actions

- Le HUD crée jusqu'à quatre panneaux liés aux membres réellement présents.
- Les gros boutons de mains du panneau transitoire sont masqués ; la barre
  centrale provient exclusivement de `FGridAvailableCombatAction`.
- Un clic transmet l'identité et la provenance de l'action à
  `RequestCharacterCombatAction()` ; le widget ne consomme aucune ressource.
- La première version de la barre lisait `GetUpcomingInitiativeOrder()` et
  affichait huit slots au maximum, puis `+ N`. Ce comportement est remplacé
  par la chronologie glissante MON12.7.1.
- Le slot actif est agrandi à `72 x 72`; les suivants utilisent `56 x 56`.
- Les tours terminés et les vaincus sortent de la barre après notification ;
  les panneaux de personnages vaincus restent visibles mais désactivés.
- Les PAM sont affichés depuis `FGridPartyMobilityState` et actualisés par
  `OnPartyMobilityStateChanged`.
- Tous les rafraîchissements sont événementiels. Aucun `Tick` UI n'est ajouté.
- Le panneau historique reste le fallback tant que
  `CombatHudWidgetClass` n'est pas affectée dans le Pawn.
- Le document d'implémentation est
  `docs/Design/MON12_7_ACTION_ORIENTED_COMBAT_HUD.md`.

---

## 2026-08-06 — MON12.7.1 : chronologie d'initiative glissante

- La barre montre huit activations par défaut, configurables de sept à dix, et
  continue sur les rounds suivants au lieu d'afficher un débordement `+ N`.
- Un séparateur `ROUND N` est inséré entre les activations de deux rounds sans
  consommer de slot.
- Le TurnManager produit la projection autoritaire par
  `GetInitiativePreview()` ; le HUD n'effectue aucun tri.
- Les slots et séparateurs sont mis en pool et actualisés sans recréation des
  widgets à chaque événement.
- `InitiativeModifier` permet aux futurs effets de hâte ou ralentissement de
  réordonner immédiatement les seules activations à venir. L'actif et les tours
  déjà joués ne sont jamais déplacés rétroactivement.
- Le jet d'initiative de rencontre reste immuable et l'ordre complet est
  recalculé au début du round suivant.
- Le document d'implémentation est
  `docs/Design/MON12_7_1_SLIDING_DYNAMIC_INITIATIVE.md`.

---

## 2026-08-07 — Nettoyage du panneau de statut MON12.7

- `UGridCombatActionPanelWidget` devient strictement un panneau de statut.
- Les widgets, vues, callbacks et tests directs `MainHand / OffHand` hérités de
  MON12.1 et MON12.2 sont supprimés.
- Le catalogue n'est plus dupliqué dans le panneau : seule la barre d'actions
  du HUD appelle `GetAvailableCombatActions()` et
  `RequestCharacterCombatAction()`.
- Le fallback de Pawn vers le panneau unique MON12.1 est supprimé ;
  `CombatHudWidgetClass` devient obligatoire.
- Les sources `MainHand / OffHand` restent dans le gameplay d'équipement et
  dans la provenance des actions génériques.

---

## 2026-08-07 — MON12.8.1 : modèle persistant de barre de raccourcis

- Chaque `FGridCharacterInventoryState` possède exactement dix raccourcis
  personnels, indexés `0` à `9` et vides au début d'une partie.
- `FGridCombatHotbarBinding` conserve uniquement l'identité stable de l'action
  et de sa source ; aucune disponibilité ni aucun coût runtime n'est
  sérialisé.
- Une arme mémorise son instance runtime préférée. Un consommable s'appuie sur
  son identifiant de définition ; sa règle d'épuisement est précisée par
  MON12.8.9.
- Le modèle est modifié exclusivement par `UGridPartyInventoryComponent`, qui
  expose lecture, affectation et suppression par personnage et par index.
- Les sauvegardes passent en version `3`. Les versions `1` et `2` reçoivent
  automatiquement dix slots vides lors de leur restauration.
- L'UI, le glisser-déposer, les touches numériques et la résolution des
  raccourcis appartiennent aux jalons suivants.
- Le document d'implémentation est
  `docs/Design/MON12_8_1_PERSISTENT_COMBAT_HOTBAR_MODEL.md`.

---

## 2026-08-07 — MON12.8.2 : dix slots fixes et glisser-déposer

- La liste automatique du catalogue est remplacée par dix widgets persistants
  affichés dans l'ordre `1 2 3 4 5 6 7 8 9 0`.
- Déposer une potion ou un parchemin crée une référence par définition sans
  déplacer la pile ; déposer une arme équipée crée une référence par instance.
- Un raccourci suit la même arme si elle change de main. Une arme absente reste
  configurée mais son slot devient indisponible. Depuis MON12.8.9, une même
  instance d'arme ne peut toutefois occuper qu'un slot à la fois.
- Glisser un raccourci vers un slot vide le déplace ; vers un slot occupé, les
  deux bindings sont échangés atomiquement. Le clic droit efface le slot.
- Les widgets sont mis en pool et rafraîchis sur événements, sans `Tick` et
  sans reconstruction complète de la barre.
- Le clic gauche et les touches numériques n'exécutent pas encore l'action ;
  ils sont réservés à MON12.8.3. Les consommables seront exécutés en MON12.8.4.
- Le document d'implémentation est
  `docs/Design/MON12_8_2_FIXED_COMBAT_HOTBAR_DRAG_DROP.md`.

---

## 2026-08-07 — MON12.8.3 : exécution par clic et clavier

- Le clic gauche court et les touches principales `1 2 3 4 5 6 7 8 9 0`
  appellent la même entrée `RequestHotbarSlot()`.
- Le binding est résolu de nouveau contre le catalogue avant chaque demande ;
  le TurnManager reste seul responsable de la validation et des dépenses.
- Le clic n'est exécuté qu'au relâchement si aucun drag n'a été détecté. Le
  déplacement et l'échange MON12.8.2 ne déclenchent donc jamais une attaque.
- Le Pawn bloque les raccourcis numériques lorsque le menu, l'inventaire, la
  création de personnage ou une autre interface modale possède les entrées.
- Les attaques d'équipement et l'attaque universelle à mains nues sont les
  profils actuellement exécutables. Les effets de capacité, sorts, potions et
  parchemins restent grisés jusqu'à leurs exécuteurs dédiés.
- Le document d'implémentation est
  `docs/Design/MON12_8_3_COMBAT_HOTBAR_EXECUTION.md`.

---

## 2026-08-07 — MON12.8.4 : potions et parchemins de combat

- Un `GridItemDefinitionAsset` de type `Potion` ou `Scroll` peut exposer une
  action `QuickItem` orientée données, sans exécuteur spécifique par asset.
- L'identité est toujours normalisée en `Use_<ItemDefinitionId>` et la quantité
  disponible additionne toutes les piles du personnage actif.
- Les profils `Effect/Self` restaurent les PV et/ou le mana. Les profils
  `Attack/FirstAxialTarget` réutilisent le pipeline d'attaque existant.
- Une action refusée ne consomme ni PA, ni mana, ni objet. Une action acceptée
  consomme exactement le coût source déclaré, au minimum une unité.
- Depuis MON12.8.9, chaque consommation acceptée supprime le binding ; tout
  exemplaire restant doit être assigné explicitement.
- Le document d'implémentation est
  `docs/Design/MON12_8_4_QUICK_ITEM_COMBAT_ACTIONS.md`.

---

## 2026-08-07 — MON12.8.5 : palette des capacités et sorts

- Le HUD expose séparément les actions `Universal`, `Ability` et `Spell` du
  personnage actif dans `Panel_ActionPalette` ; la barre fixe reste limitée à
  dix raccourcis.
- `WBP_GridCombatHudAction` sert aussi de visuel de palette. Le numéro est
  masqué et le clic seul n'exécute rien ; seul le drag vers la barre crée un
  binding.
- Une action de classe est identifiée par `ActionId`, `SourcePolicy` et
  `ClassId`. Elle ne possède jamais de source objet, d'identifiant runtime ou
  de coût en quantité.
- Les profils `Attack / FirstAxialTarget` réutilisent le pipeline d'attaque et
  paient PA et mana de manière transactionnelle. Tout refus restaure le mana
  réservé et ne dépense aucun PA.
- Les profils `Effect / Self` restaurent immédiatement PV et/ou mana. Un effet
  inutile est grisé et refusé sans dépense.
- Les profils `Cell` et `Area` sont confiés au mode de ciblage explicite de
  MON12.8.6 ; MON12.8.5 ne tente jamais de les exécuter axialement.
- Le document d'implémentation est
  `docs/Design/MON12_8_5_CLASS_ACTION_PALETTE.md`.

---

## 2026-08-07 — MON12.8.6 : cellule et zone d'effet

- Un raccourci `Cell` ou `Area` ouvre un mode de ciblage sans payer de
  ressource. Le clic suivant confirme une cellule ; `Échap` annule.
- Le contrôleur joueur convertit le point d'impact sous la souris en cellule
  et donne temporairement la priorité au ciblage sur les interactions monde.
- `RangeCells` mesure la portée jusqu'au centre. `AreaRadiusCells` décrit un
  diamant en distance de Manhattan autour de ce centre.
- La prévisualisation expose les cellules couvertes et les monstres vivants de
  la rencontre. Une cellule vide, une zone sans ennemi ou une cible hors portée
  reste invalide et gratuite.
- Une zone résout une attaque par monstre, mais PA, mana et quantité d'objet
  source sont payés une seule fois après validation complète.
- Le document d'implémentation est
  `docs/Design/MON12_8_6_CELL_AREA_TARGETING.md`.

---

## 2026-08-07 — MON12.8.7 : disponibilité et affectations de la barre

- Les dix slots restent visibles hors combat et affichent alors la barre du
  personnage sélectionné dans l'inventaire.
- Initiative, panneaux de combattants, PAM et fin de tour restent masqués en
  exploration ; les raccourcis sont configurables mais non exécutables.
- `Attack_Unarmed` reste toujours dans la palette, indépendamment des armes
  équipées, afin de permettre son affectation manuelle.
- Si le WBP ne fournit pas `Panel_ActionPalette`, le HUD crée un `WrapBox` de
  secours au-dessus de `Panel_Actions`.
- Pendant l'ouverture de l'inventaire, la barre passe temporairement devant le
  menu pour recevoir les dépôts ; le reste du HUD est masqué et toute exécution
  par clic ou clavier reste bloquée.
- Une arme lançable peut être affectée directement depuis l'inventaire. Elle
  devient une action rapide stable par définition, consomme une unité après
  acceptation et produit le projectile récupérable attendu.
- Le document d'implémentation est
  `docs/Design/MON12_8_7_HOTBAR_AVAILABILITY_ASSIGNMENT.md`.

---

## 2026-08-08 — MON12.8.8 : audit de cohérence du HUD et désaffectation

- Le clic droit efface uniquement le binding du raccourci. Une pile reste dans
  l'inventaire avec la même quantité et une arme équipée reste dans sa main ;
  aucun objet n'est déposé au sol.
- Le tooltip des slots attribués rend ce comportement explicite.
- Le panneau de statut ne répète plus `Waiting` ou `Completed`. Il affiche
  uniquement `ACTIF`, `INCAPACITÉ` ou `VAINCU` lorsque cette précision est utile.
- Le HUD racine reste seul propriétaire de la visibilité combat/exploration des
  panneaux. Les copies `bCombatActive` et `bCollapseOutsideCombat` du panneau
  enfant sont supprimées.
- Les abonnements HUD redondants aux événements d'attaque et d'action sont
  supprimés ; inventaire, état du combattant, état de tour, initiative, phase et
  fin de combat restent les sources de rafraîchissement autoritaires.
- Trois WBP doivent être finalisés dans Unreal Editor : `WBP_GridCombatHud`,
  `WBP_GridCombatHudAction` et `WBP_GridCombatHudInitiativeSlot`. Le panneau
  `WBP_GridCombatActionPanel` est déjà débarrassé de ses anciens boutons de main.
- Le document d'audit et de mise à niveau est
  `docs/Design/MON12_8_8_COMBAT_HUD_COHERENCE_AUDIT.md`.

---

## 2026-08-09 — MON12.8.9 : unicité et épuisement des objets de raccourci

- Une instance d'arme identifiée par `PreferredSourceRuntimeId` ne peut être
  affectée qu'à un seul slot par personnage.
- Une définition `QuickItem` identifiée par `SourceDefinitionId` ne peut elle
  aussi occuper qu'un seul slot.
- Déposer de nouveau la même arme déplace son binding vers le slot cible sans
  déplacer, dupliquer ou déséquiper l'objet.
- Chaque consommation acceptée efface le binding `QuickItem`, même si d'autres
  exemplaires restent dans l'inventaire ; une action refusée le conserve.
- Les anciennes sauvegardes sont normalisées au chargement : doublons d'armes
  et raccourcis de consommables épuisés sont supprimés sans changer la version
  sérialisée.
- Le document d'implémentation est
  `docs/Design/MON12_8_9_HOTBAR_ITEM_LIFETIME.md`.

---

## 2026-08-12 — MON13.2 : instanciation native des `MonsterSpawn`

- `ObjectId` reste l'unique `SpawnId` et l'identité persistante de l'Actor ;
  aucun second GUID n'est créé.
- `DA_MonsterSpawn.RuntimeActorClass` reste à `None`. La classe concrète vient
  exclusivement de `MonsterDefinitionAsset.MonsterActorClass`.
- La résolution runtime exige le DataAsset, un identifiant concordant, une
  classe concrète, une cellule praticable et une orientation cardinale.
- L'Actor est créé en différé et initialisé avant `BeginPlay`; un refus ne
  conserve aucun état partiel.
- Les Actors générés sont suivis par `SpawnId`, détruits lors d'un rebuild ou
  d'un changement de niveau, puis recréés avant restauration MON9.
- L'aperçu éditeur utilise un Actor editor-only à mesh squelettique et ne crée
  aucune logique de combat.
- Les placements désactivés et les commandes dynamiques restent hors périmètre
  jusqu'à MON13.3.
- Le document d'implémentation est
  `docs/Design/MON13_2_MONSTER_SPAWN_PIPELINE.md`.

---

## 2026-08-13 — MON13.3 : commandes runtime des `MonsterSpawn`

- Les commandes générales existantes sont réutilisées : `Spawn`, `Despawn`,
  `Teleport`, avec alias `Activate/Enable`, `Deactivate/Disable` et `Toggle`.
- `Despawn` est une absence persistante mais réversible : un `Spawn` ultérieur
  restaure le dernier état connu sous le même `SpawnId`.
- La commande de lien `Teleport` ramène le monstre à la pose de son placement ;
  l'API Blueprint/C++ accepte une cellule et une orientation explicites.
- Les événements `MonsterSpawned`, `MonsterDespawned` et
  `MonsterTeleported` sont ajoutés à la fin de `EGridObjectEvent`.
- Le Grimrock Grid Editor expose `MonsterSpawn` comme cible de connector avec
  `Spawn`, `Despawn`, `Teleport` et leurs alias ; les quatre événements du
  monstre sont également disponibles comme événements sources.
- `FGridLevelRuntimeState::MonsterPlacements` conserve présence et dernier état
  complet, y compris cellule, orientation, PV, mort et `EncounterGroupId`.
- Un refus de spawn ou de téléportation ne modifie ni l'Actor, ni l'occupation,
  ni l'état persistant ; un échec de capture interdit le despawn.
- La gestion globale des rencontres et la téléportation inter-niveaux restent
  hors périmètre.
- Le document d'implémentation est
  `docs/Design/MON13_3_MONSTER_RUNTIME_COMMANDS.md`.

---

## 2026-08-13 — MON13.4 : rencontres et vagues de monstres

- Aucun nouvel objet de grille n'est créé : un `MonsterSpawn` groupé sert
  d'ancre et reçoit la commande `StartEncounter`.
- `EncounterWaveIndex` ordonne les membres d'un même `EncounterGroupId`; une
  vague future doit être désactivée au démarrage.
- Une vague est créée atomiquement. Tout échec détruit les Actors créés par la
  tentative et restaure l'état persistant précédent.
- Seul `CommitDeath` ajoute un `SpawnId` aux membres vaincus. `Despawn` ne fait
  jamais progresser la rencontre.
- L'ancre émet `EncounterWaveStarted` après chaque vague réussie et
  `EncounterCompleted` une seule fois après la dernière vague.
- La vague active, l'ancre, la fin et les membres vaincus sont conservés dans
  `FGridLevelRuntimeState::MonsterEncounters` et sérialisés par SaveGame.
- Le document d'implémentation est
  `docs/Design/MON13_4_MONSTER_ENCOUNTER_WAVES.md`.

---

## 2026-08-13 — MON13.5 : clôture du pipeline `MonsterSpawn`

- La définition de production `DA_MON_RatGiant` doit référencer
  `BP_MON_RatGiant_C`, et non la classe native `AGridMonsterActor` seule.
- Les fixtures runtime MON13 chargent désormais la véritable classe gameplay du
  Rat, son mesh et son Animation Blueprint. Elles vérifient la présence de
  `MonsterMovement`, `MonsterBehavior` et `MonsterCombat`.
- Un test de contrat charge directement `DA_MON_RatGiant` et refuse toute
  régression vers la classe native de base.
- Le test PIE autoritaire suit la carte versionnée : trois Rats désactivés, deux
  vagues, `Trigger.Activated -> StartEncounter`, deux membres en vague 0 et un
  membre en vague 1.
- Le playtest frais ignore toujours l'état sauvegardé ; un Continue normal
  restaure les deux membres et l'état actif de la vague 0 depuis une sauvegarde
  temporaire qui n'est jamais le slot utilisateur.
- `StartEncounter` ne force pas le début du combat. Le démarrage par perception
  demeure une requête distincte du TurnManager.
- Le document de clôture est
  `docs/Design/MON13_5_MONSTER_SPAWN_CLOSURE.md`.
