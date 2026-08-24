# GrimrockPrototype — Active Completion Roadmap

Statut : **MON20 EN COURS — MON20.7 CLOS — MON20.8 EN COURS**  
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
MON20.7 — Talents / Progression Choice Integration        VALIDÉ UE5.5.4 — CLOS — 24/24
MON20.8 — Cross-System Requirements / Actions / UI        EN COURS
  MON20.8.1 — Audit & Architecture Contract              TERMINÉ
  MON20.8.2 — Skill Definition Identity & Requirement Projection    PROCHAIN
  MON20.8.3 — Combat Action Requirement Integration & Diagnostics
  MON20.8.4 — Skills/Talents Page Read Model & Menu Integration
  MON20.8.5 — Automation / PIE Regression & Closure
MON20.9 — Reserve / Persistence / Migration               À FAIRE
MON20.10 — Balance / Regression / Closure                 À FAIRE
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

## MON20.1 à MON20.7 — CLOS

Les fondations de recrutement, les Skills runtime et l'intégration Talent sont validées.

Références principales :

```text
MON20.4 — Story Companion Recruitment UI                 18/18
MON20.5 — Custom Recruit / Wizard Context Reuse           23/23 + PIE
MON20.6 — Skills Data Model & Runtime                     24/24
MON20.7 — Talents / Progression Choice Integration        24/24
```

## MON20.8 — Cross-System Requirements / Actions / UI — EN COURS

MON20.8 branche Skills et Talents sur les consommateurs existants sans créer de pipeline parallèle.

Audit MON20.8.1 :

- `FGridCombatActionDefinition::Requirements` est déjà le contrat de gating des actions ;
- `FGridCombatActionCatalogContext::SatisfiedRequirements` reçoit déjà `ClassId` et `ItemTags` ;
- MON15/MON20.7 projette déjà niveau, `ChoiceId` et `GrantedRequirementIds` ;
- le HUD/hotbar sait déjà conserver et présenter une action indisponible ;
- les `SkillRanks` ne produisent encore aucun RequirementId ;
- `URPGSkillAsset` n'a pas encore d'identité PrimaryAsset explicite basée sur `SkillId` ni de résolveur canonique ;
- `WBP_GridSkills` existe, mais `Page_Skills` reste un `UWidget` générique sans init/refresh C++.

Contrat :

```text
Skill rank > 0
    -> SkillId satisfait

FRPGSkillRequirementGrant
    MinimumRank
    GrantedRequirementIds[]

Skill RequirementIds
+ ClassId
+ ItemTags
+ Talent/Level RequirementIds
    -> FGridCombatActionCatalog
    -> MissingRequirement / Enabled
    -> HUD + hotbar existants
```

La page `Compétences` sera read-only pour la progression dans MON20.8 : elle affichera Skills + Talents du personnage sélectionné, mais n'inventera pas une monnaie de points de compétence. Les talents restent acquis via le Level Up MON15/MON20.7.

Découpage :

```text
MON20.8.1 — Audit & Architecture Contract                         TERMINÉ
MON20.8.2 — Skill Definition Identity & Requirement Projection    PROCHAIN
MON20.8.3 — Combat Action Requirement Integration & Diagnostics
MON20.8.4 — Skills/Talents Page Read Model & Menu Integration
MON20.8.5 — Automation / PIE Regression & Closure
```

Document :

```text
docs/Design/MON20_8_1_CROSS_SYSTEM_REQUIREMENTS_ACTIONS_UI_CONTRACT.md
```

## Suite MON20

```text
MON20.9 — Reserve / Persistence / Migration
    -> persister notamment SkillRanks par CharacterId
    -> migration SaveGame si nécessaire
    -> vérifier réserve / pool / recrutement après reload

MON20.10 — Balance / Regression / Closure
    -> équilibre initial Skills/Talents/recrutement
    -> campagne Automation globale MON20
    -> PIE bout en bout
    -> documentation et clôture MON20
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
MON20.8.2 — Skill Definition Identity & Requirement Projection
```
