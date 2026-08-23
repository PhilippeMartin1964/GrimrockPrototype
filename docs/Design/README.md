# GrimrockPrototype Design Docs

Ce dossier est la mémoire de conception stable du projet GrimrockPrototype. Il conserve décisions acceptées, roadmaps actives, contrats d'implémentation et validations entre ChatGPT, Unreal Editor, Visual Studio et Git.

## Ordre de lecture actif

Pour reprendre le développement, lire d'abord :

1. `00_PROJECT_OVERVIEW.md` — orientation du projet et état validé actuel.
2. `PROJECT_COMPLETION_ROADMAP.md` — backlog actif et ordre des prochains jalons.
3. `MON19_CLOSURE.md` — clôture d’Advanced Dungeon Logic / Scripting.
4. `MON20_START.md` — point de départ du prochain jalon Recruitment / Skills / Talents.
5. `99_DECISIONS_LOG.md` — décisions durables.
6. `COMBAT_SYSTEM_V2_ACTION_POINTS_INITIATIVE.md` — architecture de combat actuelle.
7. `03_EVENT_COMMAND_LINKS.md` — modèle Event -> Command toujours autoritaire après MON19.
8. `06_GRID_EDITOR_UX_SPEC.md` — cible UX du Grid Editor.

`04_IMPLEMENTATION_ROADMAP.md` et `05_CODEX_TASKS.md` sont historiques et ne définissent plus le backlog actif.

---

## Jalons majeurs

| Famille | Statut | Résumé |
|---|---|---|
| MON1–MON10 | Validé | Fondations monstres, déplacement, perception, combat, mort, persistance, présentation, balance. |
| MON11 | Validé | Attaques du groupe, équipement offensif, présentation et armes de jet. |
| MON12 | Validé | Initiative globale, PA/PAM, catalogue d'actions, HUD, hotbar 0–9, targeting et cooldowns. |
| MON13 | **Clos** | `MonsterSpawn` persistant, lifecycle, encounters, vagues, Save/Continue. |
| MON14 | **Clos** | Engagement automatique, perception directionnelle, dormance, patrouille, investigation et alarmes. |
| MON15 | **Clos** | XP, niveaux, progression de classe, Level Up, save/migration et balance initiale. |
| MON16 | **Clos** | Status Effects groupe/monstres, stacking, durée, DoT, Haste/Slow, contrôle, HUD et persistance. |
| MON17 | **Clos** | Gobelin lanceur, projectile, `RangedKeeper`, loot et XP. |
| MON18 | **Clos** | Magic & Spellbook, quatre sorts, hotbar, ciblage, transaction PA/mana, présentation et checkpoint pré-combat. |
| MON19 | **Clos** | Variables persistantes, conditions, Logic nodes, Lua sandboxé, `persistent`, `LogicId`, authoring Editor et puzzles de production. |
| MON20 | **Prochain** | Recruitment / Skills / Talents. |

Clôture MON19 :

```text
docs/Design/MON19_CLOSURE.md
```

Validation finale MON19 :

```text
Grimrock.MON19.8    4/4 Success
Grimrock.MON19     55/55 Success
PIE final           VALIDÉ
```

---

## Roadmap active

```text
MON20 — Recruitment / Skills / Talents
MON21 — Quests / Journal / Map / Codex
MON22 — 45–90 Minute Vertical Slice
```

Le travail immédiat est l’audit de l’existant personnage/groupe/classes/statistiques/progression/actions/inventaire/UI/SaveGame avant définition de MON20.1.

Voir `PROJECT_COMPLETION_ROADMAP.md` et `MON20_START.md`.

---

## Documents de clôture majeurs

| Document | Rôle |
|---|---|
| `MON14_CLOSURE.md` | Automatic engagement, patrol, investigation, alarms. |
| `MON15_CLOSURE.md` | XP & Level Progression. |
| `MON16_CLOSURE.md` | Status Effects. |
| `MON17_CLOSURE.md` | Second Monster Family / Gobelin lanceur. |
| `MON18_CLOSURE.md` | Magic & Spellbook. |
| `MON19_CLOSURE.md` | Advanced Dungeon Logic / Scripting. |

---

## Documents système principaux

| Document | Statut / rôle |
|---|---|
| `00_PROJECT_OVERVIEW.md` | Current — orientation et état validé. |
| `PROJECT_COMPLETION_ROADMAP.md` | Current — backlog autoritaire. |
| `01_GRID_OBJECT_SYSTEM.md` | Modèle d'objets de grille. |
| `02_OBJECT_ARCHETYPES.md` | Familles d'archétypes et règles de nommage. |
| `03_EVENT_COMMAND_LINKS.md` | Connecteurs Source Event -> Target Command. |
| `06_GRID_EDITOR_UX_SPEC.md` | UX du Grid Editor. |
| `11_GRID_OBJECT_ARCHETYPE_PARAMETERS_REFERENCE.md` | Référence pratique des paramètres d'archétype. |
| `COMBAT_SYSTEM_V2_ACTION_POINTS_INITIATIVE.md` | Combat actuel. |
| `99_DECISIONS_LOG.md` | Décisions chronologiques durables. |

---

## Documents MON19 principaux

```text
MON19_1_EVENT_COMMAND_AUDIT_LUA_FEASIBILITY.md
MON19_2_1A_EVENT_COMMAND_CONNECTOR_CONTRACT.md
MON19_2_3_LOGIC_PRIMITIVES.md
MON19_4_EVENT_LUA_COMMAND_BRIDGE.md
MON19_6_LUA_EDITOR_VALIDATION.md
MON19_7_1_LUA_AUTHORING_API.md
MON19_8_PRODUCTION_PUZZLES_CLOSURE.md
MON19_8_FINAL_VALIDATION.md
MON19_CLOSURE.md
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
