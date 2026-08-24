# GrimrockPrototype — Active Completion Roadmap

Statut : **MON20 EN COURS — MON20.5 VALIDÉ FONCTIONNELLEMENT — CLÔTURE EN ATTENTE DES ASSETS**  
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
MON20.5 — Custom Recruit / Wizard Context Reuse          VALIDÉ UE5.5.4 — ASSETS À VERSIONNER
MON20.6 — Skills Data Model & Runtime                    PROCHAIN APRÈS CLÔTURE MON20.5
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

Pas de SaveGame v8 ni `PartyMemberKind` persistant à ce stade.

## MON20.4 — Story Companion Recruitment UI — VALIDÉ ET CLOS

Clôturé le **24 août 2026**.

MON20.4 fournit le recrutement scénarisé complet, sans logique métier Blueprint parallèle :

- `WBP_RPGStoryCompanionRecruitment` pour `Recruter / Refuser / Voir la fiche` ;
- intégration modale dans `AGrimrockPartyPawn` ;
- commande data-driven `OfferRecruitment` dans le pipeline Event -> Command / Lua ;
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

## MON20.5 — Custom Recruit / Wizard Context Reuse — VALIDÉ FONCTIONNELLEMENT

MON20.5 réutilise le **même wizard de création de personnage** pour créer une recrue personnalisée pendant l'exploration, sans introduire un second système de groupe ou un second WBP métier.

Architecture finale :

```text
Trigger.Activated
    -> CustomRecruiter.OpenCustomRecruit
    -> AGrimrockPartyPawn
    -> WBP_CharacterCreationWizard
       Context=CustomRecruit
    -> FRPGCustomRecruitService
    -> CharacterPool temporaire
    -> MON20.2 TryRecruitFromPool
    -> ActiveCharacters
```

Principaux acquis :

- contexte transient `NewGameMainHero / CustomRecruit` ;
- même `CharacterCreationWidgetClass`, même instance et même garde modale ;
- transaction custom recruit atomique ;
- identité `CharacterId` unique ;
- attributs, race, classe et portrait préservés ;
- spellbook runtime garanti après recrutement ;
- `Annuler` retourne au jeu sans mutation ;
- target data-only `CustomRecruiter` ;
- commande `OpenCustomRecruit = 24` dans le pipeline Event -> Command existant ;
- policy CONNECTORS dédiée ;
- bridge Lua générique réutilisé ;
- recrutement refusé pendant un combat actif afin de préserver initiative, AP et états de tour ;
- correction de la double suppression `RemoveFromParent()` ;
- aucune migration SaveGame ni `PartyMemberKind` persistant introduit prématurément.

Validation automatisée finale :

```text
Grimrock.MON20.5.CustomRecruit   23/23 Success
```

Validation PIE finale sous UE5.5.4 :

```text
[OK] Hors combat -> Annuler -> retour au jeu, aucune mutation, aucun warning RemoveFromParent
[OK] Hors combat -> Engager -> nouvelle recrue active, retour au jeu
[OK] En combat   -> OpenCustomRecruit refusé, aucun wizard, combat inchangé
```

État de clôture :

```text
[OK] C++ / architecture
[OK] Automation 23/23
[OK] Grid authoring / connector
[OK] PIE post-hardening
[ ] Assets d'authoring MON20.5.6 versionnés dans Git
```

MON20.5 sera marqué **CLOS** dès que l'archetype Custom Recruiter, la palette modifiée et le `GridLevelAsset` concerné auront été commités/poussés.

## Suite MON20

```text
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
1. Versionner les assets MON20.5.6 dans Git
2. Clore MON20.5
3. Ouvrir MON20.6 — Skills Data Model & Runtime
```

MON20.5 est fonctionnellement validé sous UE5.5.4 ; seul le versioning des assets d'authoring reste requis pour sa clôture formelle.
