# GrimrockPrototype — Active Completion Roadmap

Statut : **MON20 EN COURS — MON20.1 À MON20.4 VALIDÉS — MON20.5 PROCHAIN**  
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
MON20.5 — Custom Recruit / Wizard Context Reuse          PROCHAIN
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

`FRPGPartyRecruitmentService::TryRecruitFromPool` réalise une transaction atomique pool → groupe actif, aligne l’équipement, normalise l’ownership et rollback en cas d’échec.

```text
Grimrock.MON20.2.Recruitment   6/6 Success
```

## MON20.3 — Story Companion Definition / Pool — VALIDÉ

`URPGStoryCompanionAsset` et `FRPGStoryCompanionService` ajoutent un compagnon data-driven avec `CompanionId` et `CharacterId` stable, enregistrement idempotent dans `CharacterPool` et reconnaissance active/pool.

```text
Grimrock.MON20.3.StoryCompanion   6/6 Success
```

Pas de SaveGame v8 ni `PartyMemberKind` persistant à ce stade.

## MON20.4 — Story Companion Recruitment UI — VALIDÉ ET CLOS

Clôturé le **24 août 2026**.

MON20.4 fournit le recrutement scénarisé complet, sans logique métier Blueprint parallèle :

- `WBP_RPGStoryCompanionRecruitment` pour `Recruter / Refuser / Voir la fiche` ;
- intégration modale dans `AGrimrockPartyPawn` ;
- commande data-driven `OfferRecruitment` dans le pipeline Event → Command / Lua ;
- cible `StoryCompanion` data-only ;
- placement depuis le Grid Editor via `DefaultStoryCompanionDefinition` ;
- appel des services MON20.3 puis MON20.2 comme autorités de transaction ;
- suppression silencieuse d’une nouvelle offre si le compagnon est déjà actif ;
- refus non définitif, mémorisé uniquement pour `SourceObjectId + CharacterId` pendant la session runtime ;
- correction du focus modal UE5.

Validation :

```text
Grimrock.MON20.4.RecruitmentUI   18/18 Success
```

PIE validé :

```text
Refuser  -> repasser sur la même source -> aucune popup
Recruter -> repasser sur le Trigger     -> aucune popup si AlreadyActive
```

## Suite MON20 proposée

```text
MON20.5 — Custom Recruit / Wizard Context Reuse
MON20.6 — Skills Data Model & Runtime
MON20.7 — Talents / Progression Choice Integration
MON20.8 — Cross-System Requirements / Actions / UI
MON20.9 — Reserve / Persistence / Migration
MON20.10 — Balance / Regression / Closure
```

La numérotation peut encore être ajustée si un sous-jalon doit être divisé, mais l’ordre fonctionnel est désormais clair.

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
MON20.5 — Custom Recruit / Wizard Context Reuse
```

MON20.4 — Story Companion Recruitment UI est validé et clos depuis le 24 août 2026.
