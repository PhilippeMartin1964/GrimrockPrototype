# GrimrockPrototype — Active Completion Roadmap

Statut : **MON18.8 IMPLÉMENTÉ — VALIDATION UE5.5.4 EN ATTENTE**  
Date de référence : **22 août 2026**

Ce document est la feuille de route active et autoritaire du projet. `04_IMPLEMENTATION_ROADMAP.md` reste historique.

---

## 1. État de référence

Jalons majeurs clos :

```text
MON13 — Monster Spawn / Encounters / Persistence
MON14 — Automatic Engagement / Patrol / Investigation / Alarm
MON15 — XP & Level Progression
MON16 — Status Effects
MON17 — Second Monster Family / Gobelin lanceur
```

MON17.7 est **VALIDÉ ET CLOS sous UE5.5.4**.

MON18 est le jalon actif : **Magic & Spellbook**.

---

## 2. Ordre des grands jalons

```text
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

---

# MON18 — Magic & Spellbook — EN COURS

## Objectif

Construire un système RPG complet de magie en réutilisant les systèmes existants de combat, PA/mana, ciblage, hotbar, inventaire, effets de statut, projectiles, progression et sauvegarde.

Sous-jalons :

```text
MON18.1 — Spell Data Model & Cast Contract      CLOS
MON18.2 — Spell Knowledge / Spellbook           CLOS
MON18.3 — Runtime Casting / Cost Transaction    CLOS
MON18.4 — Targeting Integration                 CLOS
MON18.5 — First Production Spells               CLOS
MON18.6 — Spell Presentation                    CLOS
MON18.7 — Spellbook / Hotbar UI                 CLOS
MON18.8 — Persistence / Migration               EN VALIDATION
MON18.9 — Balance / Regression / Closure        APRÈS MON18.8
```

### MON18.1 — CLOS

`MON18.1 — Spell Data Model & Cast Contract` est **VALIDÉ ET CLOS sous UE5.5.4**. Validation : **4/4 Success**.

Référence : `docs/Design/MON18_1_SPELL_DATA_MODEL_CAST_CONTRACT.md`.

### MON18.2 — CLOS

`MON18.2 — Spell Knowledge / Spellbook` est **VALIDÉ ET CLOS sous UE5.5.4**. Validation : **5/5 Success**.

Références : `MON18_2_SPELL_KNOWLEDGE_SPELLBOOK.md`, `MON18_2_VALIDATION.md`.

### MON18.3 — CLOS

`MON18.3 — Runtime Casting / Cost Transaction` est **VALIDÉ ET CLOS sous UE5.5.4**. Validation : **6/6 Success**.

Références : `MON18_3_RUNTIME_CASTING_COST_TRANSACTION.md`, `MON18_3_VALIDATION.md`.

### MON18.4 — CLOS

`MON18.4 — Targeting Integration` est **VALIDÉ ET CLOS sous UE5.5.4**. Validation : **8/8 Success**.

Références : `MON18_4_TARGETING_INTEGRATION.md`, `MON18_4_VALIDATION.md`.

### MON18.5 — CLOS

`MON18.5 — First Production Spells` est **VALIDÉ ET CLOS sous UE5.5.4**. Validation : **6/6 Success**.

Premiers sorts : `Spell_ArcaneBolt`, `Spell_LesserHeal`, `Spell_Haste`, `Spell_CurePoison`.

Références : `MON18_5_FIRST_PRODUCTION_SPELLS.md`, `MON18_5_VALIDATION.md`.

### MON18.6 — CLOS

`MON18.6 — Spell Presentation` est **VALIDÉ ET CLOS sous UE5.5.4**. Validation : **7/7 Success**.

Présentation data-driven séparée du gameplay, réutilisation audio/VFX MON11 et projectile visuel MON17.3.2.

Références : `MON18_6_SPELL_PRESENTATION.md`, `MON18_6_VALIDATION.md`.

### MON18.7 — CLOS

`MON18.7 — Spellbook / Hotbar UI` est **VALIDÉ ET CLOS sous UE5.5.4**.

Le menu joueur expose maintenant un onglet Sorts fonctionnel, les sorts connus sont projetés dans `WBP_GridSpellbook`, peuvent être glissés vers les dix slots MON12, déplacés/échangés/désassignés puis exécutés depuis la hotbar.

Le chemin d'exécution validé est :

```text
Spellbook
    -> Hotbar MON12
    -> catalogue
    -> ciblage MON18.4
    -> transaction PA/mana MON18.3
    -> effets MON18.5
    -> commit runtime
    -> présentation MON18.6
```

UI01.4.3e.2 a été validé avec **6/6 Automation Success** et en PIE réel :

- `Lesser Heal` : 2 PA, 4 mana, +5 PV ;
- `Arcane Bolt` : 2 PA, 3 mana, 4 dégâts ;
- mort par `Arcane Bolt` correctement propagée vers loot, XP, événement de mort et occupation ;
- refus correct pour mana insuffisant ;
- régression `ExecutionNotImplemented` couverte par tests dédiés.

Références :

- `docs/Design/MON18_7_SPELLBOOK_HOTBAR_UI.md` ;
- `docs/Design/UI_SPELLBOOK_HOTBAR_EXECUTION.md` ;
- `docs/Design/UI_GRIMROCK_MENU_CURRENT.md`.

### MON18.8 — EN VALIDATION

`MON18.8 — Spellbook Persistence / Migration` est implémenté en C++ ; la validation UE5.5.4 reste à effectuer.

Le contrat implémenté est :

```text
UGridPartySpellbookComponent runtime/transient
    -> capture sparse CharacterId + KnownSpellIds
    -> UGrimrockPartySaveGame version 6
    -> migration explicite v5 -> v6
    -> restore atomique par CharacterId
    -> hotbar Spell conservée comme simple référence
```

Principes :

- aucun sort n'est inventé lors de la migration d'une sauvegarde v5 ;
- un SpellId valide dont la définition a disparu reste conservé ;
- un binding Spell de hotbar ne peut pas enseigner un sort ;
- `SourceDefinitionId` d'un binding Spell ne redevient jamais un `ItemDefinitionId` ;
- SAVEFIX.1 et SAVEFIX.2 restent des contrats de non-régression obligatoires ;
- aucune modification `.uasset/.umap` n'est nécessaire.

Référence : `docs/Design/MON18_8_SPELLBOOK_PERSISTENCE_MIGRATION.md`.

Validation attendue :

```text
Grimrock.Magic.MON18.8                 12/12 attendu
+ régressions SAVEFIX.2 / MON15.6 / MON16.7 / MON16.8 / MON12.8.1
+ Save -> Quit -> Continue en PIE réel
```

Après validation et clôture de MON18.8, le prochain sous-jalon sera `MON18.9 — Balance / Regression / Closure`.

---

# MON19 — Advanced Dungeon Logic / Scripting

Objectif : permettre au level designer de construire des énigmes riches sans ajouter du C++ spécifique.

---

# MON20 — Recruitment / Skills / Talents

Objectif : enrichir le groupe avec recrutement, compétences, talents et spécialisations.

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
3. Un commit logique par sous-jalon ou étape de clôture.
4. Pousser sur `origin/master` après chaque étape validée.
5. Aucun refactor massif préventif.
6. Réutiliser les systèmes existants avant d'ajouter une abstraction parallèle.
7. Les tests C++ valident la logique ; assets/WBP/maps exigent une validation UE/PIE lorsqu'ils sont impliqués.
8. À la clôture d'un jalon majeur, mettre à jour overview, roadmap et document de clôture.

---

## Prochain travail autoritaire

```text
Valider MON18.8 — Persistence / Migration du Spellbook sous UE5.5.4
```
