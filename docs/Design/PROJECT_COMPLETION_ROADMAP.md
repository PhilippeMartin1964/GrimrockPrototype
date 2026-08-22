# GrimrockPrototype — Active Completion Roadmap

Statut : **MON18 MAGIC & SPELLBOOK VALIDÉ ET CLOS — MON19 PROCHAIN**  
Date de référence : **22 août 2026**

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
```

Le prochain jalon autoritaire est :

```text
MON19 — Advanced Dungeon Logic / Scripting
```

Référence de clôture du jalon précédent :

```text
docs/Design/MON18_CLOSURE.md
```

---

## 2. Ordre des grands jalons

```text
MON18 — Magic & Spellbook                         CLOS
        ↓
MON19 — Advanced Dungeon Logic / Scripting        PROCHAIN
        ↓
MON20 — Recruitment / Skills / Talents
        ↓
MON21 — Quests / Journal / Map / Codex
        ↓
MON22 — 45–90 Minute Vertical Slice
```

---

# MON18 — Magic & Spellbook — CLOS

Objectif atteint : construire un système RPG complet de magie en réutilisant combat, PA/mana, ciblage, hotbar, inventaire, Status Effects, projectiles, UI et sauvegarde existants.

Sous-jalons :

```text
MON18.1 — Spell Data Model & Cast Contract      CLOS
MON18.2 — Spell Knowledge / Spellbook           CLOS
MON18.3 — Runtime Casting / Cost Transaction    CLOS
MON18.4 — Targeting Integration                 CLOS
MON18.5 — First Production Spells               CLOS
MON18.6 — Spell Presentation                    CLOS
MON18.7 — Spellbook / Hotbar UI                 CLOS
MON18.8 — Persistence / Migration               CLOS
MON18.9 — Balance / Regression / Closure        CLOS
  MON18.9.1 — Combat Save Policy / Checkpoint   CLOS
  MON18.9.2 — Spell Balance / Cross-System      CLOS
  MON18.9.3 — Final Diagnostics / Global Tests  CLOS
```

### MON18.1 — Spell Data Model & Cast Contract

Validation : **4/4 Success**.

Référence : `MON18_1_SPELL_DATA_MODEL_CAST_CONTRACT.md`.

### MON18.2 — Spell Knowledge / Spellbook

Validation : **5/5 Success**.

Références : `MON18_2_SPELL_KNOWLEDGE_SPELLBOOK.md`, `MON18_2_VALIDATION.md`.

### MON18.3 — Runtime Casting / Cost Transaction

Validation : **6/6 Success**.

Références : `MON18_3_RUNTIME_CASTING_COST_TRANSACTION.md`, `MON18_3_VALIDATION.md`.

### MON18.4 — Targeting Integration

Validation : **8/8 Success**.

Références : `MON18_4_TARGETING_INTEGRATION.md`, `MON18_4_VALIDATION.md`.

### MON18.5 — First Production Spells

Validation : **6/6 Success**.

Sorts de production :

```text
Spell_ArcaneBolt
Spell_LesserHeal
Spell_Haste
Spell_CurePoison
```

Références : `MON18_5_FIRST_PRODUCTION_SPELLS.md`, `MON18_5_VALIDATION.md`.

### MON18.6 — Spell Presentation

Validation : **7/7 Success**.

Présentation data-driven séparée du gameplay, réutilisant les infrastructures audio/VFX/projectile existantes.

Références : `MON18_6_SPELL_PRESENTATION.md`, `MON18_6_VALIDATION.md`.

### MON18.7 — Spellbook / Hotbar UI

L'onglet Sorts est fonctionnel. Les sorts connus peuvent être glissés vers les dix raccourcis MON12, déplacés/échangés/désassignés puis exécutés depuis la hotbar.

Pipeline validé :

```text
Spellbook
    -> Hotbar MON12
    -> catalogue
    -> ciblage MON18.4
    -> transaction PA/mana MON18.3
    -> effets MON18.5 / MON16
    -> commit runtime
    -> présentation MON18.6
```

UI01.4.3e.2 : **6/6 Success** + validation PIE réelle.

Références :

- `MON18_7_SPELLBOOK_HOTBAR_UI.md` ;
- `UI_SPELLBOOK_HOTBAR_EXECUTION.md` ;
- `UI_GRIMROCK_MENU_CURRENT.md`.

### MON18.8 — Persistence / Migration

SaveGame courant : **version 6**.

Contrat :

```text
UGridPartySpellbookComponent runtime/transient
    -> capture CharacterId + KnownSpellIds
    -> UGrimrockPartySaveGame v6
    -> migration explicite v5 -> v6
    -> restore atomique par CharacterId
```

Validation :

```text
Grimrock.Magic.MON18.8              12/12 Success
Grimrock.RPG.MON16.5                 9/9 Success après correctif
Grimrock.UI.UI01.4.3e.2              6/6 Success
PIE Save -> Stop PIE -> Continue     VALIDÉ
Seed après Continue                  Added=0 AlreadyKnown=4
```

Références : `MON18_8_SPELLBOOK_PERSISTENCE_MIGRATION.md`, `MON18_8_VALIDATION.md`.

### MON18.9.1 — Combat Save Policy / Pre-Combat Checkpoint

Politique finale :

```text
Exploration / Victory      -> sauvegarde autorisée
Combat / Defeat            -> sauvegarde régulière interdite
engagement automatique     -> checkpoint <slot>_AutoCombat avant StartCombat
échec checkpoint           -> combat automatique refusé
```

Validation :

```text
Grimrock.Save.MON18.9.1       6/6 Success
Grimrock.Save.SAVEFIX.2       1/1 Success
Grimrock.Magic.MON18.8       12/12 Success
Grimrock.Monsters.MON14.1     7/7 Success
```

Référence : `MON18_9_1_COMBAT_SAVE_POLICY.md`.

### MON18.9.2 — Spell Balance / Cross-System

Baseline initiale :

```text
Arcane Bolt   Damage 4              Mana 3  PA 2  portée 1..5  cooldown 0
Lesser Heal   Heal 5                Mana 4  PA 2  portée 0..3  cooldown 0
Haste         Apply Status_Haste    Mana 5  PA 2  portée 0..3  cooldown 0
Cure Poison   Remove Status_Poison  Mana 4  PA 2  portée 0..3  cooldown 0
```

Un cast totalement sans effet utile est rejeté avec `NoEffectWouldApply` avant tout commit PA/mana.

Validation :

```text
Grimrock.Magic.MON18.9.2        5/5 Success
Cross-system demandé           51/51 Success
```

Référence : `MON18_9_2_SPELL_BALANCE_CROSS_SYSTEM_REGRESSION.md`.

### MON18.9.3 — Final Diagnostics / Global Regression

Diagnostics SaveGame :

```text
[MON18.9.3] SlotProbe Slot=<nom> UserIndex=<index> Result=Rejected Reason=<raison>
```

Le PIE final a identifié l'ancien slot problématique :

```text
Slot=GrimrockParty_2
Reason=IncompatibleSave
```

`GrimrockParty` reste chargeable.

Validation ciblée :

```text
CheckpointIsolation   Success
SaveSlotDiagnostics   Success
```

Bilan : **2/2 Success**.

Campagne globale finale :

```text
Automation RunTests Grimrock
221/221 Success
0 Fail
```

PIE final :

```text
Continue GrimrockParty                     VALIDÉ
GrimrockParty_AutoCombat créé              VALIDÉ
Automatic combat + Checkpoint=Saved        VALIDÉ
EndPlay combat -> CombatStateNotSaveable   VALIDÉ
```

Référence : `MON18_9_3_FINAL_DIAGNOSTICS_GLOBAL_REGRESSION.md`.

### Clôture MON18

Référence autoritaire :

```text
docs/Design/MON18_CLOSURE.md
```

---

# MON19 — Advanced Dungeon Logic / Scripting — PROCHAIN

## Objectif

Permettre au level designer de construire des énigmes et mécanismes riches sans ajouter du C++ spécifique à chaque puzzle.

Le jalon devra s'appuyer sur l'architecture existante :

```text
UGridLevelAsset
+ objets de grille
+ SourceEvent -> TargetCommand
+ liens persistants
+ runtime data-driven
```

Contraintes :

- ne pas créer un second système de liens parallèle ;
- préserver l'édition directe sur la grille ;
- rester compatible avec la future création de niveaux par les joueurs ;
- définir un langage léger seulement si les événements/commandes existants ne suffisent pas ;
- conserver une séparation stricte données / runtime / présentation.

Le premier sous-jalon de MON19 doit être défini après audit du système Event -> Command existant.

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
MON19 — Advanced Dungeon Logic / Scripting
Audit de l'existant Event -> Command avant définition de MON19.1
```
