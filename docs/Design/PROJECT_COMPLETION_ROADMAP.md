# GrimrockPrototype — Active Completion Roadmap

Statut : **MON18.9.1 COMBAT SAVE POLICY VALIDÉ ET CLOS — MON18.9 EN COURS**  
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
MON18.8 — Persistence / Migration               CLOS
MON18.9 — Balance / Regression / Closure        EN COURS
  MON18.9.1 — Combat Save Policy / Checkpoint   CLOS
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

### MON18.8 — CLOS

`MON18.8 — Spellbook Persistence / Migration` est **VALIDÉ ET CLOS sous UE5.5.4**.

Le contrat final est :

```text
UGridPartySpellbookComponent runtime/transient
    -> capture sparse CharacterId + KnownSpellIds
    -> UGrimrockPartySaveGame version 6
    -> migration explicite v5 -> v6
    -> restore atomique par CharacterId
    -> hotbar Spell conservée comme simple référence
```

Principes validés :

- aucun sort n'est inventé lors de la migration d'une sauvegarde v5 ;
- un SpellId valide dont la définition a disparu reste conservé ;
- un binding Spell de hotbar ne peut pas enseigner un sort ;
- `SourceDefinitionId` d'un binding Spell ne redevient jamais un `ItemDefinitionId` ;
- SAVEFIX.1 et SAVEFIX.2 restent protégés ;
- aucune modification `.uasset/.umap` n'a été nécessaire.

Validation UE5.5.4 :

```text
Grimrock.Magic.MON18.8                 12/12 Success
Grimrock.RPG.MON16.5                   9/9 Success après correctif d25cf26e
Grimrock.UI.UI01.4.3e.2                6/6 Success
PIE Save -> arrêt PIE -> Continue      VALIDÉ
Seed après Continue                    Added=0 AlreadyKnown=4
```

Le second seed prouve la restauration réelle des quatre sorts de production depuis le SaveGame v6.

Références :

- `docs/Design/MON18_8_SPELLBOOK_PERSISTENCE_MIGRATION.md` ;
- `docs/Design/MON18_8_VALIDATION.md`.

### MON18.9 — EN COURS

`MON18.9 — Balance / Regression / Closure` consolide MON18 avant clôture du jalon majeur.

#### MON18.9.1 — Combat Save Policy / Pre-Combat Checkpoint — CLOS

Politique validée :

```text
Exploration / Victory      -> sauvegarde autorisée
Combat / Defeat            -> sauvegarde refusée
engagement automatique     -> checkpoint <slot>_AutoCombat avant StartCombat
échec checkpoint           -> combat automatique refusé
```

Le format `UGrimrockPartySaveGame` reste en version 6 : aucun état transitoire de combat (initiative, round, PA/PAM, cooldowns, actions en cours) n'est ajouté à la persistance.

Le verrou autoritaire est appliqué dans `AGrimrockPartyPawn::SaveCurrentGame()` avant toute capture/écriture disque. `UGrimrockPartySaveGame::Serialize()` reste une seconde barrière défensive.

Validation UE5.5.4 fournie le 22 août 2026 :

```text
Grimrock.Save.MON18.9.1                6/6 Success
Grimrock.Save.SAVEFIX.2                1/1 Success
Grimrock.Magic.MON18.8                 12/12 Success
Grimrock.Monsters.MON14.1              7/7 Success
```

Les logs confirment que les sauvegardes régulières sont réellement refusées en combat/défaite, que le slot principal n'est pas écrasé et que le checkpoint `_AutoCombat` est préservé.

Référence : `docs/Design/MON18_9_1_COMBAT_SAVE_POLICY.md`.

MON18.9 poursuit maintenant :

- la vérification des coûts PA/mana, portées, cooldowns et comportements des quatre sorts de production ;
- les interactions Status Effects / combat / hotbar / SaveGame ;
- les diagnostics résiduels utiles, notamment les vieux slots auxiliaires incompatibles s'ils polluent encore les logs ;
- la campagne de non-régression MON18 complète ;
- la mise à jour de la documentation de synthèse ;
- la clôture du jalon majeur MON18.

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
Poursuivre MON18.9 — balance des quatre sorts de production, interactions cross-system et campagne de non-régression avant clôture de MON18
```
