# GrimrockPrototype Design Docs

Ce dossier est la mémoire de conception stable du projet GrimrockPrototype. Il conserve décisions acceptées, roadmaps actives, contrats d'implémentation et validations entre ChatGPT, Unreal Editor, Visual Studio et Git.

## Ordre de lecture actif

Pour reprendre le développement, lire d'abord :

1. `00_PROJECT_OVERVIEW.md` — orientation du projet et état validé actuel.
2. `PROJECT_COMPLETION_ROADMAP.md` — backlog actif et ordre des prochains jalons.
3. `MON18_CLOSURE.md` — clôture de Magic & Spellbook, état technique immédiatement antérieur à MON19.
4. `99_DECISIONS_LOG.md` — décisions durables.
5. `COMBAT_SYSTEM_V2_ACTION_POINTS_INITIATIVE.md` — architecture de combat actuelle.
6. `03_EVENT_COMMAND_LINKS.md` — modèle Event -> Command, central pour MON19.
7. `06_GRID_EDITOR_UX_SPEC.md` — cible UX du Grid Editor.

`04_IMPLEMENTATION_ROADMAP.md` et `05_CODEX_TASKS.md` sont historiques et ne définissent plus le backlog actif.

---

## Jalons majeurs

| Famille | Statut | Résumé |
|---|---|---|
| MON1–MON10 | Validé | Fondations monstres, déplacement, perception, combat, mort, persistance, présentation, balance. |
| MON11 | Validé | Attaques du groupe, équipement offensif, présentation et armes de jet. |
| MON12 | Validé | Initiative globale, PA/PAM, catalogue d'actions, HUD, hotbar 0–9, targeting et cooldowns. |
| MON13 | **Clos** | `MonsterSpawn` persistant, Spawn/Despawn/Teleport, encounters, vagues, Save/Continue. |
| MON14 | **Clos** | Engagement automatique visuel, perception directionnelle, dormance, patrouille, investigation et alarmes. |
| MON15 | **Clos** | XP, niveaux, progression de classe, Level Up, save/migration et balance initiale. |
| MON16 | **Clos** | Status Effects communs groupe/monstres, stacking, durée, DoT, Haste/Slow, contrôle, HUD et persistance. |
| MON17 | **Clos** | Seconde famille de monstres, Gobelin lanceur, projectile, `RangedKeeper`, loot et XP. |
| MON18 | **Clos** | Magic & Spellbook, quatre sorts, hotbar, ciblage, transaction PA/mana, présentation, SaveGame v6 et checkpoint pré-combat. |
| MON19 | **Prochain** | Advanced Dungeon Logic / Scripting. |

Clôture MON18 :

```text
docs/Design/MON18_CLOSURE.md
```

Validation finale MON18 :

```text
Grimrock.Magic.MON18.9.3    2/2 Success
Automation RunTests Grimrock 221/221 Success
PIE final diagnostics / Continue / checkpoint / combat-save VALIDÉ
```

---

## Roadmap active

```text
MON19 — Advanced Dungeon Logic / Scripting
MON20 — Recruitment / Skills / Talents
MON21 — Quests / Journal / Map / Codex
MON22 — 45–90 Minute Vertical Slice
```

Le travail immédiat est l'audit du système Event -> Command existant avant définition de MON19.1.

Voir `PROJECT_COMPLETION_ROADMAP.md` pour les contraintes de sortie.

---

## Documents de clôture majeurs

| Document | Rôle |
|---|---|
| `MON14_CLOSURE.md` | Automatic engagement, patrol, investigation, alarms. |
| `MON15_CLOSURE.md` | XP & Level Progression. |
| `MON16_CLOSURE.md` | Status Effects. |
| `MON17_CLOSURE.md` | Second Monster Family / Gobelin lanceur. |
| `MON18_CLOSURE.md` | Magic & Spellbook. |

---

## Documents système principaux

| Document | Statut / rôle |
|---|---|
| `00_PROJECT_OVERVIEW.md` | Current — orientation et état validé. |
| `PROJECT_COMPLETION_ROADMAP.md` | Current — backlog autoritaire. |
| `01_GRID_OBJECT_SYSTEM.md` | Modèle d'objets de grille. |
| `02_OBJECT_ARCHETYPES.md` | Familles d'archétypes et règles de nommage. |
| `03_EVENT_COMMAND_LINKS.md` | Connecteurs explicites Source Event -> Target Command. |
| `06_GRID_EDITOR_UX_SPEC.md` | UX du Grid Editor. |
| `11_GRID_OBJECT_ARCHETYPE_PARAMETERS_REFERENCE.md` | Référence pratique des paramètres d'archétype. |
| `COMBAT_SYSTEM_V2_ACTION_POINTS_INITIATIVE.md` | Combat actuel. |
| `99_DECISIONS_LOG.md` | Décisions chronologiques durables. |

---

## Documents MON18 principaux

```text
MON18_1_SPELL_DATA_MODEL_CAST_CONTRACT.md
MON18_2_SPELL_KNOWLEDGE_SPELLBOOK.md
MON18_3_RUNTIME_CASTING_COST_TRANSACTION.md
MON18_4_TARGETING_INTEGRATION.md
MON18_5_FIRST_PRODUCTION_SPELLS.md
MON18_6_SPELL_PRESENTATION.md
MON18_7_SPELLBOOK_HOTBAR_UI.md
MON18_8_SPELLBOOK_PERSISTENCE_MIGRATION.md
MON18_8_VALIDATION.md
MON18_9_1_COMBAT_SAVE_POLICY.md
MON18_9_2_SPELL_BALANCE_CROSS_SYSTEM_REGRESSION.md
MON18_9_3_FINAL_DIAGNOSTICS_GLOBAL_REGRESSION.md
MON18_CLOSURE.md
UI_SPELLBOOK_HOTBAR_EXECUTION.md
UI_GRIMROCK_MENU_CURRENT.md
```

---

## Règle de priorité documentaire

En cas de contradiction :

1. dernier document de clôture validé et `99_DECISIONS_LOG.md` ;
2. `PROJECT_COMPLETION_ROADMAP.md` pour le prochain travail ;
3. `00_PROJECT_OVERVIEW.md` pour l'orientation ;
4. documents système/subsystem actuels ;
5. roadmaps historiques uniquement comme contexte.

Un document ancien ne doit jamais rouvrir implicitement un jalon marqué clos par un document ultérieur.

---

## Règle de mise à jour

À la clôture d'un jalon majeur :

1. produire un document de clôture ;
2. mettre à jour `00_PROJECT_OVERVIEW.md` ;
3. mettre à jour `PROJECT_COMPLETION_ROADMAP.md` ;
4. actualiser ce `README.md` si l'ordre de lecture ou le prochain jalon change ;
5. conserver les documents historiques comme preuves, sans réécrire leur scope initial.
