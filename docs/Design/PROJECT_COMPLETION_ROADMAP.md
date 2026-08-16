# GrimrockPrototype — Active Completion Roadmap

Statut : **backlog actif après clôture de MON15**  
Date de référence : **16 août 2026**

Ce document est la feuille de route active. `04_IMPLEMENTATION_ROADMAP.md` reste historique.

---

## 1. État de départ

Jalons majeurs clos :

```text
MON13 — Monster Spawn / Encounters / Persistence
MON14 — Automatic Engagement / Patrol / Investigation / Alarm
MON15 — XP & Level Progression
```

MON15 a fermé la boucle RPG combat -> XP -> Level Up -> progression de classe -> Save/Continue.

Référence :

```text
docs/Design/MON15_CLOSURE.md
```

Le prochain besoin structurant est une infrastructure commune d'effets d'état, réutilisable par le combat, les monstres et la magie future.

---

## 2. Ordre des grands jalons

```text
MON16 — Status Effects
        ↓
MON17 — Second Monster Family
        ↓
MON18 — Magic & Spellbook
        ↓
MON19 — Advanced Dungeon Logic / Scripting
        ↓
MON20 — Recruitment / Skills / Talents
        ↓
MON21 — Quests / Journal / Map / Codex
        ↓
MON22 — 45–90 Minute Vertical Slice
```

Les jalons MON23+ restent un horizon de production.

---

# MON15 — XP & Level Progression — CLOS

Statut : **VALIDÉ ET CLOS sous UE5.5.4**.

Sous-jalons :

```text
MON15.1 — Modèle XP / niveaux              CLOS
MON15.2 — Attribution XP                   CLOS
MON15.3 — Level-up / recalcul stats        CLOS
MON15.4 — Progression de classe            CLOS
MON15.5 — Choix de progression + UI        CLOS
MON15.6 — Save / migration / restauration  CLOS
MON15.7 — Équilibrage final                CLOS
```

Contrats finaux :

- XP cumulative : `1000 * (L - 1) * L / 2` ;
- niveau maximum 20 ;
- SaveVersion 4 ;
- récompenses monstres data-driven ;
- Rat Géant = 500 XP de pool total ;
- choix de progression persistants par `CharacterId` ;
- Level Up différable pendant combat et restaurable après Continue ;
- 42/42 tests MON15 verts ;
- campagne finale 95/95 Success.

---

# MON16 — Status Effects

## Objectif

Créer une infrastructure commune pour les états temporaires et permanents affectant personnages et monstres.

## Sous-jalons

```text
MON16.1 — Status Effect Definition & Runtime State
MON16.2 — Duration / Turn / Round Lifecycle
MON16.3 — Damage-over-time: Poison / Bleeding / Burning
MON16.4 — Haste / Slow and InitiativeModifier
MON16.5 — Stun / Silence / Immobilize
MON16.6 — HUD and Combat Feedback
MON16.7 — Save / Restore
MON16.8 — Closure and Regression
```

## Principes

- un seul modèle d'effet pour groupe et monstres lorsque possible ;
- durée autoritaire en tours/rounds, pas en secondes de présentation ;
- intégration événementielle avec le TurnManager ;
- `InitiativeModifier` MON12.7.1 doit être réutilisé pour Haste/Slow ;
- les effets ne doivent jamais dépendre d'un Widget Blueprint pour fonctionner ;
- les définitions doivent rester data-driven ;
- les effets appliqués doivent pouvoir être sérialisés en MON16.7 sans dupliquer l'état combat.

## MON16.1 — Status Effect Definition & Runtime State

Périmètre recommandé :

- définir l'identité d'un effet (`StatusEffectId`) ;
- définir catégorie/tags et règles de stacking minimales ;
- définir un état runtime indépendant de la présentation ;
- supporter propriétaire personnage ou monstre ;
- distinguer définition immutable et instance runtime ;
- prévoir source/applier sans créer de dépendance forte entre Actors ;
- ajouter validations et tests purs ;
- ne pas encore appliquer Poison/Haste/Stun dans ce sous-jalon.

Porte de sortie : une instance d'effet valide peut être créée, interrogée, remplacée/stackée selon une règle déterministe et supprimée, sur personnage comme sur monstre, sans UI.

---

# MON17 — Second Monster Family

## Objectif

Prouver que l'architecture MON1–MON14 n'est pas spécifique au Rat Géant.

Le second monstre doit imposer un comportement différent, par exemple :

- squelette `DirectMelee / SlowPressure` ; ou
- archer/gobelin `RangedKeeper`.

Une simple reskin du Rat Géant n'est pas suffisante.

Sous-jalons :

```text
MON17.1 — Definition / Assets / Spawn Contract
MON17.2 — Skeletal Mesh / Skeleton / AnimBP
MON17.3 — Distinct Attack Set
MON17.4 — Distinct AI Profile
MON17.5 — Patrol / Perception / Alarm Integration
MON17.6 — Encounter / Loot / XP Integration
MON17.7 — Balance / Closure
```

---

# MON18 — Magic & Spellbook

## Objectif

Transformer l'infrastructure de sorts MON12 en système RPG complet.

Sous-jalons :

```text
MON18.1 — Spell Definition / Identity
MON18.2 — Known Spells per Character
MON18.3 — Spellbook
MON18.4 — Learning Rules
MON18.5 — Availability / Preparation
MON18.6 — First Production Offensive Spell
MON18.7 — First Buff / Debuff
MON18.8 — Scroll Learning / Casting
MON18.9 — Save / Closure
```

Cas de validation recommandés : `Magic Missile`, `Fireball`, `Heal`, `Haste`.

---

# MON19 — Advanced Dungeon Logic / Scripting

## Objectif

Permettre au level designer de construire des énigmes riches sans ajouter du C++ spécifique.

Sous-jalons :

```text
MON19.1 — Timer
MON19.2 — Counter
MON19.3 — Logic Relay
MON19.4 — Hazards / Traps
MON19.5 — Advanced Connector Conditions
MON19.6 — Sequences
MON19.7 — Lightweight Script Language
MON19.8 — Sandbox / Validation / Limits
```

Le langage léger doit compléter `Event -> Condition -> Command`, pas le remplacer.

---

# MON20 — Recruitment / Skills / Talents

## Objectif

Passer du groupe techniquement multi-personnages à un système RPG de compagnons et de progression avancée.

Sous-jalons :

```text
MON20.1 — Recruitment / Party Roster
MON20.2 — Skills
MON20.3 — Skill Points
MON20.4 — Feats / Talents
MON20.5 — Specializations
MON20.6 — Character Sheet Integration
MON20.7 — Lockpicking Integration
MON20.8 — Save / Closure
```

---

# MON21 — Quests / Journal / Map / Codex

## Objectif

Donner une structure de campagne aux pages UI actuellement surtout décoratives.

Sous-jalons :

```text
MON21.1 — Quest Definition
MON21.2 — Quest Runtime State
MON21.3 — Journal
MON21.4 — Dialogues / Choices
MON21.5 — Map Discovery
MON21.6 — Codex / Bestiary Unlocks
MON21.7 — Save / Closure
```

---

# MON22 — 45–90 Minute Vertical Slice

## Objectif

Suspendre volontairement l'ajout de grandes infrastructures et construire un jeu testable de bout en bout.

Contenu cible :

- 1 donjon cohérent ;
- 3 à 5 niveaux ;
- 45 à 90 minutes de jeu ;
- plusieurs énigmes multi-étapes ;
- secrets ;
- 2 à 4 familles de monstres ;
- au moins un boss ;
- progression XP/niveaux ;
- sorts et effets de statut ;
- objets, butin, clés et serrures ;
- journal minimal ;
- sauvegarde et Continue ;
- conclusion claire du slice.

Porte de sortie : une personne extérieure peut lancer une build autonome, comprendre les règles, terminer le slice sans commande de debug et reprendre une sauvegarde.

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
3. Un commit logique par étape lorsque possible.
4. Pousser sur `origin/master` après chaque étape validée.
5. Aucun refactor massif préventif.
6. Réutiliser les systèmes existants avant d'ajouter une abstraction parallèle.
7. Les tests C++ valident la logique ; assets/WBP/maps exigent une validation UE/PIE lorsqu'ils sont impliqués.
8. À la clôture d'un jalon majeur, mettre à jour overview, roadmap et document de clôture.

---

## Prochain travail autoritaire

```text
MON16.1 — Status Effect Definition & Runtime State
```
