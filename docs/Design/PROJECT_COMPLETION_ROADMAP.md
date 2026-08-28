# GrimrockPrototype — Active Completion Roadmap

Statut : **TD07 ACTIF — TD07.5 CHARACTERIZATION + RECOVERY PREPARED — À VALIDER — MON21.4 SUSPENDU**  
Date de référence : **27 août 2026**

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

# 3. MON21 — Quests / Journal / Map / Codex — SUSPENDU PENDANT TD07.3

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
MON21.4 — Quest Persistence                                   SUSPENDU APRÈS CHARACTERIZATION
MON21.5 — Journal Read Model + Existing WBP Integration         À FAIRE
MON21.6 — Map Geometry + Exploration State + Existing WBP       À FAIRE
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

## MON21.4 — Quest Persistence — SUSPENDU APRÈS CHARACTERIZATION

MON21.4 a été caractérisé après TD07.3, puis resuspendu jusqu'à la clôture complète de TD07.

Sa persistance suivra alors le contrat prototype courant :

- snapshot par `QuestId` / `ObjectiveId` ;
- restauration atomique ;
- validation contre les définitions courantes ;
- quêtes/objectifs inconnus -> snapshot invalide ;
- aucune migration depuis les anciennes SaveGames du prototype ;
- Event -> Command inchangé ;
- aucune persistance de `UGridQuestDefinitionAsset*` comme source de vérité.

## MON21.5–MON21.8

```text
MON21.5 — Journal
    projection read-only de UGridQuestSubsystem
    intégration au WBP existant

MON21.6 — Map
    géométrie depuis DataAssets
    exploration / annotations autoritaires
    intégration au WBP existant

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
TD07.3.3.7 — Normalize Spellbook
```

La prochaine tranche autoritaire est TD07.4 — ActivationComponent characterization. MON21.4 reste suspendu jusqu'à TD07.8.


TD07.3.3.9 ouvre **v19 exact-match** : `LastAcknowledgedLevel` devient l'état durable minimal de notification Level-Up et les queues persistantes MON15.6 sont supprimées.


TD07.3.3.10 ouvre **v20 exact-match** : `DerivedStats` devient transient et est reconstruit depuis l'autorité personnage durable après chargement.


## Ordre de clôture TD07 avant reprise fonctionnelle

```text
TD07.4  ActivationComponent characterization                   VALIDÉ — CLOS SANS EXTRACTION
TD07.5  Suspended test infrastructure / branch recovery        À FAIRE
TD07.6  Legacy asset/API cleanup audit                          ABSORBÉ PAR TD07.3
TD07.7  Targeted log / formatting hygiene                      À FAIRE
TD07.8  Future-proofing re-audit / stop condition              À FAIRE
```

MON21.4 ne reprend qu'après TD07.8.
