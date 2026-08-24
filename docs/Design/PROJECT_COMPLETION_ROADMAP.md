# GrimrockPrototype — Active Completion Roadmap

Statut : **MON20 EN COURS — MON20.8 CLOS — MON20.9 EN COURS**  
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
```

Jalon courant :

```text
MON20 — Recruitment / Skills / Talents
```

État interne :

```text
MON20.1 — Audit & Architecture Contract                   TERMINÉ
MON20.2 — Active Party Recruitment Foundation            VALIDÉ UE5.5.4 — 6/6
MON20.3 — Story Companion Definition / Pool              VALIDÉ UE5.5.4 — 6/6
MON20.4 — Story Companion Recruitment UI                 VALIDÉ UE5.5.4 — CLOS — 18/18
MON20.5 — Custom Recruit / Wizard Context Reuse           VALIDÉ UE5.5.4 — CLOS — 23/23 + PIE
MON20.6 — Skills Data Model & Runtime                     VALIDÉ UE5.5.4 — CLOS — 24/24
  MON20.6.1 — Audit & Architecture Contract              TERMINÉ
  MON20.6.2 — Skill Definition + Character Runtime Ranks VALIDÉ — 8/8
  MON20.6.3 — Deterministic Skill Check Resolution       VALIDÉ — 16/16 cumulés
  MON20.6.4 — Runtime Access / Character Selection API   VALIDÉ — 24/24 cumulés
  MON20.6.5 — Automation Regression / Closure            CLOS
MON20.7 — Talents / Progression Choice Integration        VALIDÉ UE5.5.4 — CLOS — 24/24
  MON20.7.1 — Audit & Architecture Contract              TERMINÉ
  MON20.7.2 — Talent Runtime Read Model / Selection      VALIDÉ UE5.5.4 — 8/8
  MON20.7.3 — Level Up Talent Presentation Contract      VALIDÉ UE5.5.4 — 16/16 cumulés
  MON20.7.4 — Requirement Projection / Persistence       VALIDÉ UE5.5.4 — 24/24 cumulés
  MON20.7.5 — Automation Regression / Closure            CLOS
MON20.8 — Cross-System Requirements / Actions / UI        VALIDÉ UE5.5.4 — CLOS — 24/24 + PIE
  MON20.8.1 — Audit & Architecture Contract              TERMINÉ
  MON20.8.2 — Skill Definition Identity & Requirement Projection    VALIDÉ — 8/8
  MON20.8.3 — Combat Action Requirement Integration & Diagnostics   VALIDÉ — 16/16 cumulés
  MON20.8.4 — Skills/Talents Page Read Model & Menu Integration    VALIDÉ — 24/24 cumulés + PIE
  MON20.8.5 — Automation / PIE Regression & Closure               CLOS
MON20.9 — Persistence / Migration                         EN COURS
  MON20.9.1 — Audit & Architecture Contract              TERMINÉ
  MON20.9.2 — Skill Rank Save Snapshot + v8 Migration   VALIDÉ UE5.5.4 — 8/8
  MON20.9.3 — Active/Pool Character Persistence Regression          VALIDÉ UE5.5.4 — 8/8 — 16/16 cumulés
  MON20.9.4 — Skill Projection / Skills Page Restore Regression     VALIDÉ UE5.5.4 — 8/8 — 24/24 cumulés
  MON20.9.5 — Automation / PIE Regression & Closure                EN VALIDATION — AUTOMATION 24/24 — PIE À FAIRE
MON20.10 — Balance / Regression / Closure
```

---

## 2. Ordre des grands jalons

```text
MON19 — Advanced Dungeon Logic / Scripting        CLOS
        ↓
MON20 — Recruitment / Skills / Talents            EN COURS
        ↓
MON21 — Quests / Journal / Map / Codex
        ↓
MON22 — 45–90 Minute Vertical Slice
```

---

# MON20 — Recruitment / Skills / Talents — EN COURS

## Objectif

Enrichir le groupe avec recrutement, compétences, talents et spécialisations, en réutilisant création de personnage, progression MON15, RequirementIds MON12, inventaire, Status Effects, Spellbook, SaveGame et UI existants.

## MON20.1 — Audit & Architecture Contract — TERMINÉ

Conclusions :

- `FGridPartyInventoryState` reste l’autorité du groupe ;
- `CharacterPool` existe déjà et doit être réutilisé ;
- aucun second `PartyMemberState` ;
- les talents doivent réutiliser les `ProgressionChoices` avant toute abstraction parallèle ;
- un modèle Skill dédié n’est justifié que par rangs/tests hors combat indépendants.

## MON20.2 — Active Party Recruitment Foundation — VALIDÉ

`FRPGPartyRecruitmentService::TryRecruitFromPool` réalise une transaction atomique pool -> groupe actif, aligne l’équipement, normalise l’ownership et rollback en cas d’échec.

```text
Grimrock.MON20.2.Recruitment   6/6 Success
```

## MON20.3 — Story Companion Definition / Pool — VALIDÉ

`URPGStoryCompanionAsset` et `FRPGStoryCompanionService` ajoutent un compagnon data-driven avec `CompanionId` et `CharacterId` stable, enregistrement idempotent dans `CharacterPool` et reconnaissance active/pool.

```text
Grimrock.MON20.3.StoryCompanion   6/6 Success
```

## MON20.4 — Story Companion Recruitment UI — VALIDÉ ET CLOS

Clôturé le **24 août 2026**.

MON20.4 fournit :

- UI `Recruter / Refuser / Voir la fiche` ;
- intégration modale Pawn ;
- `OfferRecruitment` Event -> Command / Lua ;
- cible StoryCompanion data-only ;
- transaction MON20.3 -> MON20.2 ;
- suppression des offres déjà recrutées ;
- refus source-scoped pendant la session.

Validation :

```text
Grimrock.MON20.4.RecruitmentUI   18/18 Success
```

## MON20.5 — Custom Recruit / Wizard Context Reuse — VALIDÉ ET CLOS

Clôturé le **24 août 2026**.

Architecture finale :

```text
Trigger.Activated
    -> CustomRecruiter.OpenCustomRecruit
    -> AGrimrockPartyPawn
    -> WBP_CharacterCreationWizard Context=CustomRecruit
    -> FRPGCustomRecruitService
    -> CharacterPool temporaire
    -> MON20.2 TryRecruitFromPool
    -> ActiveCharacters
```

Principaux acquis :

- réutilisation du même wizard ;
- transaction custom recruit atomique ;
- identité `CharacterId` unique ;
- race, classe, attributs, portrait et spellbook préservés ;
- `Annuler` sans mutation ;
- target `CustomRecruiter` data-only ;
- commande `OpenCustomRecruit` dans Event -> Command / Lua ;
- refus propre pendant un combat actif ;
- correction de la double suppression `RemoveFromParent()` ;
- aucun SaveGame v8 prématuré.

Validation :

```text
Grimrock.MON20.5.CustomRecruit   23/23 Success
```

PIE :

```text
[OK] Hors combat -> Annuler
[OK] Hors combat -> Engager
[OK] En combat   -> recrutement refusé
```

Assets d'authoring versionnés dans :

```text
3a4a7d1cd1133a9536dbde4c964d4b81bfcbc2d4
```

Documentation de clôture :

```text
docs/Design/MON20_5_CLOSURE.md
```

## MON20.6 — Skills Data Model & Runtime — VALIDÉ ET CLOS

Clôturé le **24 août 2026**.

Architecture finale :

```text
URPGSkillAsset
    + FRPGSkillRank dans FGridCharacterInventoryState
    + FRPGSkillService
    + FRPGSkillCheckService
    + FRPGSkillRuntimeService
```

Formule :

```text
Total = d20 + SkillRank + AttributeModifier
Success = Total >= Difficulty
```

Principaux acquis :

- définition Skill data-driven ;
- rang sparse par personnage dans l'autorité existante ;
- mutation atomique des rangs ;
- résolution déterministe via `FRandomStream` ;
- aucune consommation RNG sur rejet ;
- lecture/mutation/check par `CharacterIndex` explicite ou personnage sélectionné ;
- notifications uniquement sur mutation réelle ;
- aucun second registre de personnages ;
- `SkillRanks` reste `Transient` jusqu'à MON20.9 ;
- aucune migration SaveGame dans MON20.6.

Validation cumulative :

```text
Grimrock.MON20.6.Skills   24/24 Success
0 Fail
0 Error
```

Documents :

```text
docs/Design/MON20_6_1_SKILLS_ARCHITECTURE_CONTRACT.md
docs/Design/MON20_6_2_SKILL_DEFINITION_RUNTIME_RANKS.md
docs/Design/MON20_6_3_DETERMINISTIC_SKILL_CHECK_RESOLUTION.md
docs/Design/MON20_6_4_RUNTIME_ACCESS_CHARACTER_SELECTION.md
docs/Design/MON20_6_5_AUTOMATION_REGRESSION_CLOSURE.md
```

## MON20.7 — Talents / Progression Choice Integration — VALIDÉ ET CLOS

Clôturé le **24 août 2026**.

MON20.7 réutilise le système de progression MON15 au lieu d'introduire un domaine Talent parallèle.

Contrat final :

```text
ProgressionChoice == Talent de classe sélectionnable
ChoiceId          == identité stable du talent
ChoicePoints      == monnaie existante
PrerequisiteChoiceIds == arbre / dépendances
GrantedRequirementIds == projection vers consommateurs
```

Autorités réutilisées :

```text
URPGClassAsset.ProgressionChoices
FRPGClassProgressionService
FRPGClassProgressionTransactionService
URPGLevelUpWidget
```

Décisions :

- aucun `TalentId` parallèle ;
- aucun `TalentPoints` parallèle ;
- aucun second arbre ;
- persistance fournie par MON15.6 ;
- mutation toujours via `TryCommitChoices()` ;
- `FRPGTalentRuntimeService` reste une façade read-only sans état ;
- le Level Up existant expose le vocabulaire Talent sans nouveau workflow ;
- les effets transversaux utilisent `RequirementIds` ;
- `CurrentSaveVersion` reste 7 à la clôture de MON20.7.

Découpage final :

```text
MON20.7.1 — Audit & Architecture Contract                    TERMINÉ
MON20.7.2 — Talent Runtime Read Model / Selected Character  VALIDÉ UE5.5.4 — 8/8
MON20.7.3 — Level Up Talent Presentation Contract           VALIDÉ UE5.5.4 — 16/16 cumulés
MON20.7.4 — Requirement Projection / Persistence Regression VALIDÉ UE5.5.4 — 24/24 cumulés
MON20.7.5 — Automation Regression / Closure                 CLOS
```

Principaux acquis :

- lecture des talents acquis, disponibilité et budget par personnage ;
- façade sur le personnage sélectionné sans sélection parallèle ;
- vocabulaire `Talents de classe` / `Points de talent` dans la présentation Level Up ;
- maintien de l'identité `ChoiceId` et des transactions MON15 ;
- projection immédiate de `ChoiceId` et `GrantedRequirementIds` ;
- isolation par `CharacterId` ;
- capture/restore via `ClassProgressionStates` MON15.6 ;
- restore détaché immédiatement consommable ;
- restore invalide atomique ;
- restore indépendant de l'ordre des snapshots.

Validation cumulative :

```text
Grimrock.MON20.7.Talents   24/24 Success
0 Fail
0 Error
```

Documents :

```text
docs/Design/MON20_7_1_TALENTS_PROGRESSION_CHOICE_ARCHITECTURE.md
docs/Design/MON20_7_2_TALENT_RUNTIME_READ_MODEL.md
docs/Design/MON20_7_3_LEVEL_UP_TALENT_PRESENTATION_CONTRACT.md
docs/Design/MON20_7_4_REQUIREMENT_PROJECTION_PERSISTENCE_REGRESSION.md
docs/Design/MON20_7_5_AUTOMATION_REGRESSION_CLOSURE.md
```

## MON20.8 — Cross-System Requirements / Actions / UI — VALIDÉ ET CLOS

Clôturé le **24 août 2026**.

MON20.8 branche Skills et Talents sur les consommateurs existants sans créer de pipeline parallèle.

Architecture finale :

```text
Skill rank > 0
    -> SkillId satisfait

FRPGSkillRequirementGrant
    MinimumRank
    GrantedRequirementIds[]

ClassId + ItemTags + Talent/Level RequirementIds + Skill RequirementIds
    -> FGridCombatActionCatalog
    -> MissingRequirements[] / Enabled
    -> HUD + hotbar existants

SelectedCharacterIndex
    -> FGridSkillsPageService
    -> UGridSkillsWidget
    -> WBP_GridSkills
```

Principaux acquis :

- `URPGSkillAsset::GetPrimaryAssetId()` utilise `RPGSkill:<SkillId>` ;
- `FRPGSkillRequirementProjectionService` projette les requirements par rang de manière atomique ;
- les actions peuvent être déverrouillées par `SkillId` ou seuil de compétence ;
- les anciens requirements classe, équipement et talents restent composables ;
- `FGridAvailableCombatAction::MissingRequirements` expose un diagnostic structuré et déterministe ;
- la page Compétences affiche en lecture seule Skills + Talents du personnage sélectionné ;
- `WBP_GridSkills` est reparenté sur `UGridSkillsWidget` ;
- la présentation native remplace le placeholder au runtime ;
- le changement de personnage a été validé en PIE avec `Elias` puis `Elarion` ;
- les `RequirementIds` restent dérivés et non persistés ;
- `SkillRanks` reste `Transient` jusqu'à MON20.9.

Validation finale :

```text
Grimrock.MON20.8   24/24 Success
0 Fail
0 Error
PIE                OK — page Compétences + changement de personnage
```

Découpage final :

```text
MON20.8.1 — Audit & Architecture Contract                         TERMINÉ
MON20.8.2 — Skill Definition Identity & Requirement Projection    VALIDÉ — 8/8
MON20.8.3 — Combat Action Requirement Integration & Diagnostics   VALIDÉ — 16/16 cumulés
MON20.8.4 — Skills/Talents Page Read Model & Menu Integration    VALIDÉ — 24/24 cumulés + PIE
MON20.8.5 — Automation / PIE Regression & Closure                CLOS
```

Documents :

```text
docs/Design/MON20_8_1_CROSS_SYSTEM_REQUIREMENTS_ACTIONS_UI_CONTRACT.md
docs/Design/MON20_8_2_SKILL_DEFINITION_IDENTITY_REQUIREMENT_PROJECTION.md
docs/Design/MON20_8_3_COMBAT_ACTION_REQUIREMENT_INTEGRATION_DIAGNOSTICS.md
docs/Design/MON20_8_4_SKILLS_TALENTS_PAGE_READ_MODEL_MENU_INTEGRATION.md
docs/Design/MON20_8_4_NATIVE_SKILLS_PRESENTATION.md
docs/Design/MON20_8_4_AUTOMATION_VALIDATION.md
docs/Design/MON20_8_5_AUTOMATION_PIE_REGRESSION_CLOSURE.md
```

## MON20.9 — Persistence / Migration — EN COURS

Objectif : rendre persistantes les données MON20 qui restent runtime-only, en priorité les `SkillRanks`, sans dupliquer la progression de classe/talents déjà persistée par MON15.6.

Contrat MON20.9.1 :

```text
SkillRanks runtime restent Transient
    -> snapshot SaveGame dédié keyed by CharacterId
    -> personnages actifs + CharacterPool
    -> capture/restore atomiques
    -> définition canonique RPGSkill:<SkillId>
    -> SaveGame v8
```

La migration v7 -> v8 initialise un snapshot Skill vide : aucun rang ne peut être inventé puisque les rangs étaient volontairement runtime-only en v7. Les RequirementIds restent dérivés et ne sont jamais sauvegardés.

MON20.9.2 est validé UE5.5.4 :

```text
FRPGSkillRankSaveState
FRPGCharacterSkillSaveState
FRPGSkillPersistence
CharacterSkillStates dans UGrimrockPartySaveGame
CurrentSaveVersion = 8
migration explicite v7 -> v8

Grimrock.MON20.9.SkillPersistence   8/8 Success
```

La capture et le restore couvrent personnages actifs + réserve, utilisent uniquement `CharacterId`, résolvent les définitions canoniques `RPGSkill:<SkillId>` et sont atomiques. Les chemins legacy v1-v7 initialisent le domaine Skill vide lorsqu'il n'existait pas.

MON20.9.3 est validé UE5.5.4 : les Skills suivent `CharacterId` à travers recrutement, réserve, réordonnancement et changement de sélection.

```text
Grimrock.MON20.9.ActivePoolPersistence   8/8 Success
```

MON20.9.4 est validé UE5.5.4 : les rangs restaurés redeviennent immédiatement consommables par la projection des requirements, le catalogue d'actions et la page Compétences, sans persister de données dérivées.

```text
Grimrock.MON20.9.RestoredConsumers   8/8 Success

Validation cumulative MON20.9 :
SkillPersistence        8/8
ActivePoolPersistence   8/8
RestoredConsumers       8/8
---------------------------
TOTAL                  24/24 Success
0 Fail
0 Error
```

MON20.9.5 est en validation finale. Aucun nouveau runtime n'est requis : la campagne logique est complète à 24/24 et le dernier travail est un smoke test PIE de la frontière Save/Continue v8 et de la page Compétences après chargement.

Découpage :

```text
MON20.9.1 — Audit & Architecture Contract                     TERMINÉ
MON20.9.2 — Skill Rank Save Snapshot + v8 Migration           VALIDÉ UE5.5.4 — 8/8
MON20.9.3 — Active/Pool Character Persistence Regression      VALIDÉ UE5.5.4 — 8/8 — 16/16 cumulés
MON20.9.4 — Skill Projection / Skills Page Restore Regression VALIDÉ UE5.5.4 — 8/8 — 24/24 cumulés
MON20.9.5 — Automation / PIE Regression & Closure             EN VALIDATION — AUTOMATION 24/24 — PIE À FAIRE
```

Documents :

```text
docs/Design/MON20_9_1_PERSISTENCE_MIGRATION_ARCHITECTURE_CONTRACT.md
docs/Design/MON20_9_2_SKILL_RANK_SAVE_SNAPSHOT_V8_MIGRATION.md
docs/Design/MON20_9_2_AUTOMATION_VALIDATION.md
docs/Design/MON20_9_3_ACTIVE_POOL_CHARACTER_PERSISTENCE_REGRESSION.md
docs/Design/MON20_9_3_AUTOMATION_VALIDATION.md
docs/Design/MON20_9_4_SKILL_PROJECTION_SKILLS_PAGE_RESTORE_REGRESSION.md
docs/Design/MON20_9_4_AUTOMATION_VALIDATION.md
docs/Design/MON20_9_5_AUTOMATION_PIE_REGRESSION_CLOSURE.md
```

## Suite MON20

```text
MON20.9 — Persistence / Migration                    EN COURS — AUTOMATION 24/24 — PIE FINAL À FAIRE
MON20.10 — Balance / Regression / Closure
```

---

# MON21 — Quests / Journal / Map / Codex

Objectif : structurer la campagne avec quêtes, journal, carte, codex et narration/dialogues associés.

---

# MON22 — 45–90 Minute Vertical Slice

Objectif : construire un jeu testable de bout en bout avant la production étendue.

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
5. Réutiliser les systèmes existants avant d’ajouter une abstraction parallèle.
6. Les tests C++ valident la logique ; assets/WBP/maps exigent UE/PIE lorsqu’ils sont impliqués.
7. À la clôture d’un jalon majeur, mettre à jour overview, roadmap et documentation d’architecture.

---

## Prochain travail autoritaire

```text
MON20.9.5 — Automation / PIE Regression & Closure — PIE FINAL
```
