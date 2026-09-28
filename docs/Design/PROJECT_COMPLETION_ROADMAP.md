# GrimrockPrototype — Active Completion Roadmap

Statut : **TD07 VALIDÉ/CLOS — MON21.4 EN ATTENTE DU FEU VERT UTILISATEUR**  
Date de référence : **28 août 2026**

Ce document est la feuille de route active et autoritaire du projet. `04_IMPLEMENTATION_ROADMAP.md` reste historique.

---

## 1. État de référence

Jalons majeurs validés et clos :

```text
MON13 — Monster Spawn / Encounters / Persistence
MON14 — Automatic Engagement / Patrol / Investigation / Alarm
MON15 — XP & Level Progression
MON16 — Status Effects
MON17 — Second Monster Family / Gobelin lanceur
MON18 — Magic & Spellbook
MON19 — Advanced Dungeon Logic / Scripting
MON20 — Recruitment / Skills / Talents
```

Jalon fonctionnel actif :

```text
MON21 — Quests / Journal / Map / Codex
```

Campagne technique clôturée :

```text
TD07.3 — Prototype Data Model Reset — VALIDÉ / CLOS
```

Puis :

```text
MON22 — 45–90 Minute Vertical Slice
```

Les campagnes TD05 et TD06 restent closes. TD07.1 et TD07.2 sont validées. Une nouvelle campagne volontaire, **TD07.3 — Prototype Data Model Reset**, suspend temporairement MON21 afin de supprimer les compatibilités historiques et schémas legacy avant de poursuivre les nouvelles fonctionnalités.

---

## 2. MON20 — Recruitment / Skills / Talents — CLOS

MON20 a livré recrutement actif/réserve, Story Companions, Custom Recruit, Skills, Talents et persistance associée.

Architecture autoritaire :

```text
FGridPartyInventoryState
├── ActiveCharacters
├── ActiveEquipment
├── CharacterPool
└── FGridCharacterInventoryState
    └── SkillRanks
```

Le SaveGame était v8 à la clôture MON20, puis v9 via TD01.1. TD07.3.2 a ouvert le schéma prototype v10 exact-match. TD07.3.3.2 a ouvert **v11 exact-match** après suppression du bridge legacy des attributs. TD07.3.3.3 a ouvert **v12 exact-match** après séparation des ressources mutables. TD07.3.3.4 a ouvert **v13 exact-match** après suppression des caches de poids. TD07.3.3.5 B1 a ouvert **v14** lorsque `Level` est devenu transient ; B2 a ouvert **v15 exact-match** après suppression du miroir `ClassProgressionStates`. TD07.3.3.6 ouvre **v16 exact-match** avec `SkillRanks` comme autorité durable unique et suppression de `CharacterSkillStates`. TD07.3.3.7 ouvre **v17 exact-match** avec `KnownSpellIds` comme autorité durable unique et suppression du miroir Spellbook ; aucune migration arrière. TD07.3.3.7 Shipping final est validé le 27 août 2026. TD07.3.3.8 ouvre **v18 exact-match** avec `Character.StatusEffects` comme autorité durable directe et suppression du miroir `CharacterStatusEffectStates`.

---

# 3. MON21 — Quests / Journal / Map / Codex — REPRIS AVEC MON21.6

## Objectif

Transformer les surfaces campagne déjà présentes en systèmes data-driven :

- quêtes et objectifs ;
- journal ;
- carte explorée et annotations ;
- codex / bestiaire / lore ;
- liens avec Event -> Command, variables, Logic et Lua ;
- persistance SaveGame ;
- intégration au menu existant.

## État actuel

```text
MON21.1 — Audit & Architecture Contract                         CLOS
MON21.2 — Quest Definition + Campaign Runtime State             VALIDÉ
MON21.3 — Quest Event/Command Integration                       VALIDÉ
MON21.4 — Quest Persistence                                   EN ATTENTE — CHARACTERIZATION VALIDÉE
MON21.5 — Journal Read Model + Existing WBP Integration         À FAIRE
MON21.6 — Map Geometry + Exploration State + Existing WBP       ACTIF — MON21.6.11 C++ IMPLÉMENTÉ / PIE VISUELLE À VALIDER
MON21.7 — Codex Discovery + Existing Definition Projection      À FAIRE
MON21.8 — Cross-System Regression / PIE / Closure               À FAIRE
```

## MON21.1 — Contrat établi

MON21.1 a figé notamment :

1. une autorité Quest globale de campagne ;
2. des identités stables `QuestId` / `ObjectiveId` ;
3. Event -> Command comme voie de mutation ;
4. Journal/Map/Codex comme projections, jamais autorités ;
5. aucune nouvelle Actor manager permanente ;
6. aucune persistance de read model dérivé.

## MON21.2 — Quest Definition + Campaign Runtime State — VALIDÉ

Livré :

```text
UGridQuestDefinitionAsset
    -> QuestId
    -> Objectives[] / ObjectiveId

UGridQuestSubsystem : UGameInstanceSubsystem
    -> registre transient des définitions
    -> FGridCampaignQuestRuntimeState
    -> StartQuest
    -> CompleteObjective
    -> CompleteQuest
    -> FailQuest
    -> OnQuestStateChanged
```

L’état runtime Quest est encore transient.

Validation :

```text
Grimrock.Quests.MON21_2
2 Success / 0 Failed
```

## MON21.3 — Quest Event/Command Integration — VALIDÉ

Le bus existant porte maintenant :

```text
QuestStart              = 25
QuestCompleteObjective  = 26
QuestComplete           = 27
QuestFail               = 28
```

`FGridObjectLink` transporte `QuestId` / `QuestObjectiveId`. `UGridLevelAsset::QuestDefinitions` référence les définitions utilisées par le niveau. `UGridActivationComponent` délègue les mutations au `UGridQuestSubsystem`.

Validation :

```text
Grimrock.Quests.MON21_3.EventCommandIntegration
1 Success / 0 Failed
```

## MON21.4 — Quest Persistence — EN ATTENTE

MON21.4 a été caractérisé après TD07.3. TD07.8 a validé le 28 août 2026 que ses hypothèses restent correctes. L'implémentation ne reprend toutefois qu'après feu vert explicite de l'utilisateur.

Sa persistance suivra alors le contrat prototype courant :

- snapshot par `QuestId` / `ObjectiveId` ;
- restauration atomique ;
- validation contre les définitions courantes ;
- quêtes/objectifs inconnus -> snapshot invalide ;
- aucune migration depuis les anciennes SaveGames du prototype ;
- Event -> Command inchangé ;
- aucune persistance de `UGridQuestDefinitionAsset*` comme source de vérité.

## MON21.6.1 — Map Architecture Contract — VALIDÉ

Le contrat courant est :

```text
docs/Design/MON21_6_1_MAP_ARCHITECTURE_CONTRACT.md
```

Il fige notamment :

- `UGridLevelAsset` = dalle cartographique canonique 32×32 ;
- même `LogicalPosition.Z` = même étage ;
- `LogicalPosition.X/Y` = composition des dalles adjacentes ;
- révélation par cellule, rayon de design 1.25, filtrée par la topologie ;
- secret non découvert = mur normal, sans métadonnée UI révélatrice ;
- exploration durable dans l’état runtime par `LevelId` ;
- Map = read model filtré + `WBP_GridMap`, jamais seconde autorité ;
- aucun `MapActor`, aucun Tick Map permanent, aucune grille UMG de 1024 widgets.

MON21.6.1 est documentaire : il ne change ni C++, ni assets binaires, ni `CurrentSaveVersion`.

MON21.6.2 est **VALIDÉ** :

- `FGridMapExplorationState` = autorité Unknown/Explored sur 1024 cellules ;
- stockage paresseux `TArray<uint8>` ;
- `FGridLevelRuntimeState::MapExploration` isole l’état par `LevelId` ;
- aucun flag `SaveGame` et aucune nouvelle version avant MON21.6.5 ;
- validation locale `Grimrock.Map.MON21_6_2` : **4/4, 0 warning, 0 échec**, rapport `TD04-20260928-082142`.

MON21.6.3 est **VALIDÉ** :

- `FGridMapRevealService::RevealRadiusCells = 1.25` ;
- cellule courante + quatre cardinales, diagonales exclues ;
- murs `Solid` vérifiés sur les deux côtés de l’arête ;
- porte fermée/bloquante = pas de reveal ; porte ouverte = reveal ;
- `bBlocksOccupancy` n’empêche pas la visibilité ;
- reveal initial et après mouvement via `HandlePartyCellChanged()` ;
- validation locale `Grimrock.Map.MON21_6_3` : **4/4, 0 warning, 0 échec**, rapport `TD04-20260928-083313`.

MON21.6.4 est **VALIDÉ** :

- `DiscoveredSecretObjectIds : TSet<FGuid>` dans `FGridMapExplorationState` ;
- découverte uniquement pour une vraie porte secrète ;
- découverte à l’exposition effective `fully open` ;
- secret initialement ouvert = déjà connu ;
- secret découvert puis refermé = reste connu ;
- validation locale `Grimrock.Map.MON21_6_4` : **4/4, 0 warning, 0 échec**, rapport `TD04-20260928-084526`.

MON21.6.5 est **VALIDÉ** :

- `MapExploration`, `ExploredCells` et `DiscoveredSecretObjectIds` portent `SaveGame` ;
- `CurrentSaveVersion` est **v23 exact-match** ;
- aucune migration v22 -> v23 ;
- validation structurelle Map intégrée à `ValidateCurrentState()` ;
- validation locale `Grimrock.Map.MON21_6_5` : **4/4, 0 warning, 0 échec**, rapport `TD04-20260928-090055`.

MON21.6.6 est **VALIDÉ** :

- `FGridMapTileView` expose uniquement cellules explorées + frontières connues ;
- frontière = `Wall | Door | SecretDoor` ;
- aucun `ObjectId`/`DefinitionId` secret n’est exposé à la présentation ;
- secret non découvert normalisé en `Wall` ;
- variante de porte résolue depuis `RuntimeActorClass`, sans hard-code `Door_Secret` ;
- état de porte vivant prioritaire, fallback runtime persistant puis authored ;
- validation locale `Grimrock.Map.MON21_6_6` : **4/4, 0 warning, 0 échec**, rapport `TD04-20260928-091315`.

MON21.6.7 est **VALIDÉ** :

- `FGridMapFloorView` compose les `FGridMapTileView` d’un même `LogicalPosition.Z` ;
- coordonnées globales stride 32, coordonnées négatives conservées ;
- valeurs Z disponibles triées et distinctes ;
- aucune couture dessinée entre LevelAssets adjacents sans vraie frontière ;
- frontières physiques partagées dédupliquées globalement ;
- marqueur du groupe seulement sur l’étage courant ;
- validation locale `Grimrock.Map.MON21_6_7` : **4/4, 0 warning, 0 échec**, rapport `TD04-20260928-092245`.

MON21.6.8 — C++ implémenté, intégration UMG/PIE à valider :

- nouvelle classe `UGridMapWidget : UGrimrockDesignSurfaceWidget` ;
- `RefreshMap()` reconstruit `FGridMapFloorView` depuis les autorités runtime ;
- rendu natif `NativePaint()` : cellules explorées, murs, portes, secrets découverts, marqueur groupe ;
- aucun widget par cellule, aucun Actor Map, aucun refresh Map par Tick ;
- `UGrimrockMenuWidget` initialise et rafraîchit la Map à l’activation ;
- `Page_Map` reste générique `UWidget` pour ne pas casser le WBP avant reparent ;
- reparent manuel de `WBP_GridMap` vers `UGridMapWidget` requis dans UE ;
- Automation C++ ajoutée sous `Grimrock.Map.MON21_6_8`.

Validation MON21.6.8 reçue : **4/4, 0 warning, 0 échec**, rapport `TD04-20260928-093526`.

`WBP_GridMap` est reparenté et le smoke PIE par `M` affiche bien la carte. La première validation visuelle a identifié un défaut de projection Est/Ouest : le renderer n’appliquait pas le miroir X déjà utilisé par l’Overview Map de l’éditeur.

Correctif appliqué uniquement dans la projection écran : coordonnées Map/read model inchangées ; X écran, frontières East/West et flèche East/West sont miroir.

Seconde validation MON21.6.8 après correctif Est/Ouest : **4/4, 0 warning, 0 échec**, rapport `TD04-20260928-095036`.

Le contrôle PIE utilisateur confirme que l’orientation Est/Ouest est correcte.

MON21.6.8 est **VALIDÉ**. La convention visuelle X miroir reste un point différé à clarifier ultérieurement ; les conventions canoniques runtime `East=X+ / West=X-` restent inchangées.

MON21.6.9 — Floor Navigation est maintenant implémenté côté C++ :

- `SelectedFloorZ` transient dans `UGridMapWidget` ;
- `NavigateFloorUp/Down()` parcourt les Z activés disponibles sans supposer `Z±1` ;
- `CanNavigateFloorUp/Down()` fournit l’état Enabled aux contrôles ;
- `RefreshMap()` conserve l’étage consulté ;
- l’ouverture via le shell appelle `SelectPartyFloor()` et revient donc à l’étage courant du groupe ;
- un autre étage conserve le contrat `bHasPartyMarker=false` ;
- bindings UMG optionnels : `Button_LevelUp`, `Button_LevelDown`, `Text_FloorLabel` ;
- aucune persistance supplémentaire, SaveGame v23 inchangé ;
- Automation ajoutée sous `Grimrock.Map.MON21_6_9`.

Validation initiale MON21.6.9 : **4/4, 0 warning, 0 échec**, rapport `TD04-20260928-102621`.

Le smoke PIE utilisateur confirme que Level Up/Down change correctement d’étage.

Le raffinement `DisplayName` est finalement abandonné : un étage peut contenir plusieurs dalles portant des noms distincts.

`Text_FloorLabel` affiche désormais uniquement le Z logique canonique sous la forme `Niveau <Z>`.

MON21.6.9 est **VALIDÉ** par confirmation utilisateur après le correctif final `Niveau <Z>`.

MON21.6.10 — Zoom / Pan / Recenter est maintenant implémenté côté C++ :

- caméra UI transitoire `ZoomScale` + `PanOffsetPixels` ;
- molette -> zoom borné sans reconstruction du read model ;
- clic gauche + glisser -> pan en pixels UI ;
- `RecenterMap()` revient à l’étage du groupe et centre sa cellule ;
- Up/Down conserve le zoom mais remet le pan à zéro ;
- `Button_Recenter` optionnel, sans Graph Blueprint ;
- aucun nouvel état SaveGame, v23 inchangé ;
- Automation ajoutée sous `Grimrock.Map.MON21_6_10`.

MON21.6.10 est **VALIDÉ** :

- `Grimrock.Map.MON21_6_10` : **4/4, 0 warning, 0 échec**, rapport `TD04-20260928-115317` ;
- smoke PIE validé par l’utilisateur.

MON21.6.11 — Hand-Drawn Parchment Artistic Pass est maintenant implémenté côté C++ :

- fond parchemin procédural dans la zone Map ;
- grain discret déterministe, aucun random par frame ;
- lavis + hachures des cellules explorées ;
- feather léger aux frontières d’exploration ;
- murs/portes/marqueur en doubles traits manuscrits déterministes ;
- secret caché toujours normalisé en `Wall` avant rendu ;
- palette encre/parchemin éditable ;
- aucun asset Map/parchemin supplémentaire requis ;
- SaveGame v23 inchangé ;
- Automation ajoutée sous `Grimrock.Map.MON21_6_11`.

Statut MON21.6.11 : **C++ implémenté ; validation Automation + PIE visuelle requises**.

Prochaine tranche après validation : **MON21.6.12 — Map Symbols**.




## MON21.5–MON21.8

```text
MON21.5 — Journal
    projection read-only de UGridQuestSubsystem
    intégration au WBP existant

MON21.6 — Map
    MON21.6.1 contrat architecture VALIDÉ
    géométrie depuis DataAssets
    exploration autoritaire par LevelId
    projection multi-dalles par LogicalPosition X/Y/Z
    secret caché normalisé en mur
    intégration au WBP existant
    annotations joueur hors première tranche

MON21.7 — Codex
    discovery state
    projection Monster / Item / Spell / Skill / lore

MON21.8 — Closure
    persistance croisée
    Event -> Command
    UI
    PIE
    régressions
```

---

# 4. Dette technique — TD07.3 actif

```text
TD01–TD04  stabilisation / outillage                         RÉALISÉ
TD05.9     RuntimeActor stop condition                       ATTEINTE
TD06.9     PartyInventory stop condition                     ATTEINTE
TD07.1     Build / dependency reproducibility                VALIDÉ
TD07.2     UE compatibility warnings                         VALIDÉ
TD07.3.1   Prototype Data Model Policy + Asset Audit         VALIDÉ
TD07.3.2   SaveGame Reset / no backward migration            VALIDÉ
TD07.3.3   Character State Normalization                      ACTIF
TD07.3.3.1 Character State Authority Audit                    VALIDÉ
TD07.3.3.2 Remove Legacy Attribute Bridge                     VALIDÉ
TD07.3.3.3 Normalize Derived Stats / Mutable Resources         VALIDÉ
TD07.3.3.4 Normalize Weight State                              VALIDÉ
TD07.3.3.5 Normalize XP / Level / Class Progression                VALIDÉ
TD07.3.3.6 Normalize Skills                                        VALIDÉ — CLOS
TD07.3.3.7 Normalize Spellbook                                     VALIDÉ — CLOS
TD07.3.3.8 Normalize Status Effects                                 VALIDÉ — CLOS
TD07.3.3.9 Normalize Level-Up Notification State                       VALIDÉ — CLOS
TD07.3.3.10 Current Save Schema / Regressions / Closure                VALIDÉ — CLOS
TD07.3.4 Authoring Identity Normalization                     VALIDÉ — CLOS
TD07.3.5 Combat Data Schema Reset                           VALIDÉ — CLOS
TD07.3.6 Remaining Legacy API/Data Purge                    VALIDÉ — CLOS
TD07.3.7 Current Asset Repair / Recreation                    VALIDÉ — CLOS
TD07.3.8 Strict Current-Schema Validation / stop condition    À FAIRE
```

Politique autoritaire pendant le prototype : **aucune compatibilité arrière Save/DataAsset/Blueprint n'est requise**. Les données incompatibles peuvent être recréées ; Git conserve l'historique.

Le registre autoritaire reste `docs/Architecture/TECHNICAL_DEBT_REGISTER.md`.

---
# 5. MON22 — 45–90 Minute Vertical Slice

Objectif : construire un parcours jouable de bout en bout avant la production étendue.

Le vertical slice devra combiner au minimum :

- exploration grille ;
- portes, clés, passages secrets et puzzles ;
- Event -> Command / Logic / Lua ;
- objets et équipement ;
- recrutement ;
- Skills / Talents ;
- magie ;
- monstres mêlée + distance ;
- combat et progression ;
- quêtes / journal / carte / codex ;
- sauvegarde / chargement ;
- début et fin de slice clairement identifiés.

---

# 6. Horizon MON23+

```text
MON23 — Containers / Lock Traps / Crafting
MON24 — Production Audio / VFX / Atmosphere
MON25 — Menus / Options / Accessibility
MON26 — Performance / Optimization
MON27 — Packaging / Shipping / Installer
MON28 — Standalone Player Level Editor
MON29 — Dungeon Publication / Validation / Sharing
MON30 — Full Campaign
```

---

## Règles de conduite

1. Un sous-jalon doit être petit, compilable et testable.
2. Travail sur `master`, sans branche de fonctionnalité.
3. **Un commit logique par sous-jalon ou passe documentaire.**
4. Aucun refactor massif préventif.
5. Réutiliser les systèmes existants avant d'ajouter une abstraction parallèle.
6. Les tests C++ valident la logique ; assets/WBP/maps exigent UE/PIE lorsqu'ils sont impliqués.
7. À la clôture d'un jalon majeur, mettre à jour overview, roadmap et documentation d'architecture.
8. Ne jamais déclarer une validation UE5.5.4 sans log ou résultat fourni depuis l'environnement utilisateur.

---

## Prochain travail autoritaire

```text
MON21.6.8 — Existing WBP + Native Map Rendering : VALIDÉ
MON21.6.9 — Floor Navigation : VALIDÉ
MON21.6.10 — Zoom / Pan / Recenter : VALIDÉ
MON21.6.11 — Hand-Drawn Parchment Artistic Pass : C++ IMPLÉMENTÉ / PIE VISUELLE À VALIDER
MON21.6.12 — Map Symbols : prochaine tranche après validation
```

TD07 est validé et clos. MON21.4 reste en attente ; le chantier fonctionnel actif est MON21.6 Map.


TD07.3.3.9 ouvre **v19 exact-match** : `LastAcknowledgedLevel` devient l'état durable minimal de notification Level-Up et les queues persistantes MON15.6 sont supprimées.


TD07.3.3.10 ouvre **v20 exact-match** : `DerivedStats` devient transient et est reconstruit depuis l'autorité personnage durable après chargement.


## Ordre de clôture TD07 avant reprise fonctionnelle

```text
TD07.4  ActivationComponent characterization                   VALIDÉ — CLOS SANS EXTRACTION
TD07.5  Suspended test infrastructure / branch recovery        VALIDÉ — CLOS
TD07.6  Legacy asset/API cleanup audit                          ABSORBÉ PAR TD07.3
TD07.7  Targeted log / formatting hygiene                      VALIDÉ — CLOS
TD07.8  Future-proofing re-audit / stop condition              VALIDÉ — STOP CONDITION ATTEINTE
```

MON21.4 reprend après validation de TD07.8.
