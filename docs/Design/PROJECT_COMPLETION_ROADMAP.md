# GrimrockPrototype — Active Completion Roadmap

Statut : **MON20 EN COURS — MON20.6 CLOS — MON20.7 EN COURS**  
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
MON20.1 — Audit & Architecture Contract                  TERMINÉ
MON20.2 — Active Party Recruitment Foundation           VALIDÉ UE5.5.4 — 6/6
MON20.3 — Story Companion Definition / Pool             VALIDÉ UE5.5.4 — 6/6
MON20.4 — Story Companion Recruitment UI                VALIDÉ UE5.5.4 — CLOS — 18/18
MON20.5 — Custom Recruit / Wizard Context Reuse          VALIDÉ UE5.5.4 — CLOS — 23/23 + PIE
MON20.6 — Skills Data Model & Runtime                    VALIDÉ UE5.5.4 — CLOS — 24/24
MON20.7 — Talents / Progression Choice Integration       EN COURS
  MON20.7.1 — Audit & Architecture Contract             TERMINÉ
  MON20.7.2 — Talent Runtime Read Model / Selection     IMPLÉMENTÉ — VALIDATION EN ATTENTE
  MON20.7.3 — Level Up Talent Presentation Contract
  MON20.7.4 — Requirement Projection / Persistence Regression
  MON20.7.5 — Automation Regression / Closure
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

MON20.6 fournit :

```text
URPGSkillAsset
FRPGSkillRank dans FGridCharacterInventoryState
FRPGSkillService
FRPGSkillCheckService
FRPGSkillRuntimeService
```

Formule :

```text
Total = d20 + SkillRank + AttributeModifier
Success = Total >= Difficulty
```

Validation finale :

```text
Grimrock.MON20.6.Skills   24/24 Success
```

`SkillRanks` reste runtime/transient jusqu'à MON20.9.

## MON20.7 — Talents / Progression Choice Integration — EN COURS

Contrat :

```text
ProgressionChoice == Talent de classe
ChoiceId          == identité sémantique du talent
ChoicePoints      == points de talent/progression
PrerequisiteChoiceIds == dépendances
GrantedRequirementIds == projection générique
```

Aucun `TalentId`, `TalentPoints` ou SaveState parallèle n'est introduit.

Découpage :

```text
MON20.7.1 — Audit & Architecture Contract                    TERMINÉ
MON20.7.2 — Talent Runtime Read Model / Selected Character   IMPLÉMENTÉ — VALIDATION EN ATTENTE
MON20.7.3 — Level Up Talent Presentation Contract
MON20.7.4 — Requirement Projection / Persistence Regression
MON20.7.5 — Automation Regression / Closure
```

MON20.7.2 ajoute une façade read-only sans état au-dessus de MON15 pour :

- talents acquis ;
- `HasTalent` ;
- solde de ChoicePoints ;
- talents actuellement disponibles ;
- personnage explicite ou sélectionné.

## Suite MON20

```text
MON20.8 — Cross-System Requirements / Actions / UI
MON20.9 — Reserve / Persistence / Migration
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
MON20.7.2 — Talent Runtime Read Model / Selected Character — VALIDATION UE5.5.4
```
