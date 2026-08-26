# GrimrockPrototype — Active Completion Roadmap

Statut : **MON21.3 VALIDÉ — MON21.4 PROCHAIN**  
Date de référence : **26 août 2026**

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

Jalon courant :

```text
MON21 — Quests / Journal / Map / Codex
```

Puis :

```text
MON22 — 45–90 Minute Vertical Slice
```

Les campagnes de dette ciblée TD05 et TD06 sont closes par stop condition ; elles ne bloquent plus la roadmap fonctionnelle.

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

Le SaveGame était v8 à la clôture MON20 ; le projet est depuis passé en **v9** via TD01.1.

---

# 3. MON21 — Quests / Journal / Map / Codex — ACTIF

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
MON21.4 — Quest Persistence / Migration                         PROCHAIN
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

## MON21.4 — Quest Persistence / Migration — PROCHAIN

Objectif : rendre durable l’état autoritaire de campagne introduit en MON21.2, sans persister de projection Journal.

À verrouiller :

- snapshot SaveGame par `QuestId` et `ObjectiveId` ;
- restauration atomique du `FGridCampaignQuestRuntimeState` ;
- validation des définitions lors du restore ;
- politique pour quêtes/objectifs retirés ou inconnus ;
- éventuelle montée de SaveVersion **uniquement si nécessaire** ;
- migration depuis v9 ;
- ordre restore : définitions Quest disponibles avant application du snapshot ;
- Event -> Command inchangé ;
- aucune persistance de `UGridQuestDefinitionAsset*` comme source de vérité.

MON21.4 doit être caractérisé et validé sous UE5.5.4 avant MON21.5.

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

# 4. Dette technique — état après TD06

```text
TD01–TD04  stabilisation / outillage                  RÉALISÉ
TD05.9     RuntimeActor stop condition                ATTEINTE
TD06.9     PartyInventory stop condition              ATTEINTE
```

Aucune nouvelle tranche TD05/TD06 n’est recommandée sans signal concret. Le registre autoritaire reste `docs/Architecture/TECHNICAL_DEBT_REGISTER.md`.

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
MON21.4 — Quest Persistence / Migration
```
