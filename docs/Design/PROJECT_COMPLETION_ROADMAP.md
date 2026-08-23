# GrimrockPrototype — Active Completion Roadmap

Statut : **MON19 ADVANCED DUNGEON LOGIC / SCRIPTING VALIDÉ ET CLOS — MON20 PROCHAIN**  
Date de référence : **23 août 2026**

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

Le prochain jalon autoritaire est :

```text
MON20 — Recruitment / Skills / Talents
```

Référence de clôture du jalon précédent :

```text
docs/Design/MON19_CLOSURE.md
```

---

## 2. Ordre des grands jalons

```text
MON19 — Advanced Dungeon Logic / Scripting        CLOS
        ↓
MON20 — Recruitment / Skills / Talents            PROCHAIN
        ↓
MON21 — Quests / Journal / Map / Codex
        ↓
MON22 — 45–90 Minute Vertical Slice
```

---

# MON19 — Advanced Dungeon Logic / Scripting — CLOS

Objectif atteint : permettre la création d’énigmes riches sans C++ spécifique à chaque puzzle, en conservant l’architecture Event -> Command existante et en ajoutant seulement les briques nécessaires.

Hiérarchie finale :

```text
cas simple
    -> Event -> Command direct

état / compteur / comparaison
    -> variables persistantes + nœuds Logic

orchestration complexe
    -> Lua sandboxé
       -> commandes runtime normales
```

Capacités validées :

- conditions de liens typées ;
- variables de niveau persistantes `Bool` / `Int32` ;
- primitives Logic data-driven ;
- SaveGame / migration des variables de niveau ;
- VM Lua embarquée avec sandbox, quotas et isolation ;
- pont Event -> Lua -> Command ;
- authoring Lua dans le Grid Editor ;
- durcissement sandbox / packaging ;
- table `persistent = { ... }` synchronisée avec le Data Asset ;
- `LogicId` lisible pour cibler les objets sans GUID dans les scripts ;
- quatre puzzles de production de clôture.

Validation finale :

```text
Development Editor / Win64    OK
Grimrock.MON19.8               4/4 Success
Grimrock.MON19                55/55 Success
Fail                           0
Error                          0
PIE puzzle représentatif       VALIDÉ
```

Références :

```text
docs/Design/MON19_CLOSURE.md
docs/Design/MON19_8_PRODUCTION_PUZZLES_CLOSURE.md
```

---

# MON20 — Recruitment / Skills / Talents — PROCHAIN

## Objectif

Enrichir le groupe avec recrutement, compétences, talents et spécialisations.

Le jalon doit réutiliser en priorité :

```text
création de personnage / identité des membres
+ classes / statistiques
+ progression MON15
+ requirements / catalogue d’actions MON12
+ inventaire / équipement
+ Status Effects MON16
+ Spellbook MON18 si pertinent
+ SaveGame / migrations
+ UI de groupe / GrimrockMenu
```

Le premier sous-jalon n’est pas figé avant audit de cet existant.

Référence de démarrage :

```text
docs/Design/MON20_START.md
```

---

# MON21 — Quests / Journal / Map / Codex

Objectif : structurer la campagne avec quêtes, journal, carte et codex.

---

# MON22 — 45–90 Minute Vertical Slice

Objectif : construire un jeu testable de bout en bout avant la phase de production étendue.

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
3. Un commit logique par sous-jalon ou étape de clôture si possible.
4. Pousser sur `origin/master` après chaque étape validée.
5. Aucun refactor massif préventif.
6. Réutiliser les systèmes existants avant d'ajouter une abstraction parallèle.
7. Les tests C++ valident la logique ; assets/WBP/maps exigent une validation UE/PIE lorsqu'ils sont impliqués.
8. À la clôture d'un jalon majeur, mettre à jour overview, roadmap et document de clôture.

---

## Prochain travail autoritaire

```text
MON20 — Recruitment / Skills / Talents
Audit de l’existant avant définition du contrat MON20.1
```
