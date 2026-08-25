# GrimrockPrototype — Active Completion Roadmap

Statut : **MON20 CLOS — MON21 PROCHAIN**  
Date de référence : **24 août 2026**

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

Jalon courant autoritaire :

```text
MON21 — Quests / Journal / Map / Codex
```

Puis :

```text
MON22 — 45–90 Minute Vertical Slice
```

---

## 2. MON20 — Recruitment / Skills / Talents — CLOS

Clôturé le **24 août 2026** sous UE5.5.4.

### État final

```text
MON20.1 — Audit & Architecture Contract                   CLOS
MON20.2 — Active Party Recruitment Foundation            VALIDÉ — 6/6
MON20.3 — Story Companion Definition / Pool              VALIDÉ — 6/6
MON20.4 — Story Companion Recruitment UI                 VALIDÉ — 18/18
MON20.5 — Custom Recruit / Wizard Context Reuse          VALIDÉ — 23/23 + PIE
MON20.6 — Skills Data Model & Runtime                     VALIDÉ — 24/24
MON20.7 — Talents / Progression Choice Integration       VALIDÉ — 24/24
MON20.8 — Cross-System Requirements / Actions / UI       VALIDÉ — 24/24 + PIE
MON20.9 — Persistence / Migration                        VALIDÉ — 24/24 + PIE
MON20.10 — Balance / Regression / Closure                VALIDÉ — 151/151 global + PIE
```

### Architecture finale MON20

```text
FGridPartyInventoryState
├── ActiveCharacters
├── ActiveEquipment
├── CharacterPool
└── FGridCharacterInventoryState
    └── SkillRanks

Story Companion / Custom Recruit
    -> CharacterPool
    -> FRPGPartyRecruitmentService
    -> ActiveCharacters

Skills
    -> URPGSkillAsset
    -> FRPGSkillService
    -> FRPGSkillCheckService
    -> FRPGSkillRuntimeService
    -> Requirement projection

Talents
    -> MON15 ProgressionChoices
    -> ChoiceId / ChoicePoints
    -> GrantedRequirementIds

Class + ItemTags + Talents + Skills
    -> FGridCombatActionCatalog
    -> MissingRequirements
    -> HUD / hotbar

SaveGame v8
    -> CharacterSkillStates keyed by CharacterId
    -> active + CharacterPool
    -> consommateurs reconstruits après restore
```

Décisions structurantes :

- `FGridPartyInventoryState` reste l'autorité unique du groupe ;
- maximum actif = 6 ;
- recrutement atomique pool -> actifs avec rollback ;
- aucune abstraction Talent parallèle : réutilisation de MON15 ;
- Skills sparse, data-driven et rattachés au personnage ;
- `RequirementIds` dérivés et non persistés ;
- identité persistante par `CharacterId` ;
- SaveGame version 8 ;
- migration v7 -> v8 sans inventer de Skill rank ;
- restore fail-closed ;
- monstres morts persistants restaurés cachés, sans collision ni occupation.

### Validation finale MON20

```text
Grimrock.MON20
151/151 Success
0 Fail
0 Error
```

Le smoke test PIE final valide :

```text
L_MainMenu
-> Continuer GrimrockParty
-> Save v8 Result=Accepted
-> 2 Gobelins RestoreDead
-> GridRuntimeState Apply DeadMonsters=2
-> PartySave Continued CharacterCount=2
-> menu / SelectedCharacter fonctionnels
-> resauvegarde GrimrockParty v8 réussie
```

Aucun `PartyOccupiesCell` / `MissingActor` ne réapparaît pour les snapshots morts.

Document de clôture :

```text
docs/Design/MON20_10_5_FINAL_PIE_MON20_CLOSURE.md
```

---

# MON21 — Quests / Journal / Map / Codex — PROCHAIN

## Objectif

Transformer les surfaces UI déjà présentes en systèmes de campagne data-driven :

- quêtes et objectifs ;
- journal ;
- carte explorée et annotations ;
- codex / bestiaire / lore ;
- liens avec Event -> Command, variables, Logic et Lua ;
- persistance SaveGame ;
- intégration au menu existant.

## MON21.1 — Audit & Architecture Contract

Première étape obligatoire : auditer l'existant avant tout nouveau modèle.

À relire notamment :

```text
WBP_GridJournal
WBP_GridMap
WBP_GridCodex
UGrimrockMenuWidget / WBP_GrimrockMenu
UGridLevelAsset / runtime state
Event -> Command
Variables / Logic / Lua
SaveGame v8
Monster definitions / Bestiary presentation
```

Questions à verrouiller :

1. Quelle autorité porte une quête ?
2. Quelle identité stable utiliser pour QuestId / ObjectiveId / CodexEntryId ?
3. Qu'est-ce qui est design-time, runtime, dérivé et persistant ?
4. Comment Event -> Command déclenche progression, journal et codex sans pipeline parallèle ?
5. Quelle donnée de carte est globale au donjon, au niveau ou au groupe ?
6. Quels états doivent migrer dans le prochain SaveGame si nécessaire ?

Règle : **ne pas créer de nouveau subsystem ou registre avant d'avoir vérifié les structures déjà présentes.**

## Découpage pressenti MON21

À confirmer par MON21.1 :

```text
MON21.1 — Audit & Architecture Contract
MON21.2 — Quest Definition / Runtime State
MON21.3 — Event -> Command Quest Progression
MON21.4 — Journal Read Model / UI
MON21.5 — Map Exploration / Annotations
MON21.6 — Codex / Bestiary / Lore Unlocks
MON21.7 — Persistence / Migration
MON21.8 — Cross-System Regression / PIE Closure
```

---

# MON22 — 45–90 Minute Vertical Slice

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

# Horizon MON23+

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
MON21.1 — Audit & Architecture Contract
```
