# GrimrockPrototype — Vue d’ensemble du projet

## Objectif

GrimrockPrototype est un dungeon crawler Unreal Engine 5.5.4 en C++ inspiré de *Legend of Grimrock 2* : vue subjective case par case, Grid Editor, mécanismes data-driven, IA de monstres, combat tactique, groupe RPG, progression, magie et, à terme, création de niveaux par les joueurs.

## État actuel — 23 août 2026

```text
MON13 — Monster Spawn / Encounter / Persistence                 CLOS
MON14 — Automatic Engagement / Patrol / Investigation / Alarm  CLOS
MON15 — XP & Level Progression                                  CLOS
MON16 — Status Effects                                          CLOS
MON17 — Gobelin lanceur / ranged combat                         CLOS
MON18 — Magic & Spellbook                                       CLOS
MON19 — Advanced Dungeon Logic / Scripting                      CLOS
MON20.1 — Audit & Architecture Contract                         TERMINÉ
MON20.2 — Active Party Recruitment Foundation                  VALIDÉ — 6/6
MON20.3 — Story Companion Definition / Pool                    VALIDÉ — 6/6
MON20.4 — Story Companion Recruitment UI                       PROCHAIN
```

## Bilan architectural de référence

```text
docs/Architecture/PROJECT_SYNTHESIS.md
docs/Architecture/ARCHITECTURE_INDEX.md
docs/Architecture/Maps/GRIMROCK_PROJECT_MAP.md
docs/Architecture/Maps/GRIMROCK_PROJECT_MAP_MERMAID.md
```

La carte détaillée courante est maintenue en Markdown et les vues visuelles en Mermaid. Git conserve les versions historiques.

## Architecture

```text
GrimrockLua
    ↓
GrimrockPrototype
    ↓
GrimrockPrototypeEditor
```

DataAssets et grille restent les autorités logiques. Actors, animations, VFX et widgets sont runtime/présentation. Event → Command reste le bus gameplay ; Logic et Lua orchestrent sans créer une voie parallèle.

## Systèmes fermés récemment

- MON15 : XP, niveaux, progression de classe, Level Up et persistance.
- MON16 : Status Effects groupe/monstres, durée, stacking, DoT, initiative, contrôle, save/restore.
- MON17 : Gobelin lanceur, projectiles et `RangedKeeper`.
- MON18 : Spellbook, cast pipeline et quatre sorts de production.
- MON19 : variables Bool/Int32, Logic nodes, Lua sandboxé, `persistent`, `LogicId`, authoring Editor.
- MON20.2/20.3 : recrutement atomique depuis `CharacterPool` et compagnon scénarisé data-driven.

## Persistance

`UGrimrockPartySaveGame` est au contrat v7. Le recrutement MON20.3 réutilise le `CharacterId` stable et ne nécessite ni SaveGame v8 ni `PartyMemberKind` prématuré.

## Prochaine phase

```text
MON20.4 — Story Companion Recruitment UI
...     — Custom Recruit / Skills / Talents / Reserve / Regression
MON21   — Quests / Journal / Map / Codex
MON22   — 45–90 Minute Vertical Slice
```

## Règle de travail

Chaque sous-jalon : audit ciblé, contrat, modification minimale, compilation/tests UE5.5.4 fournis par l’utilisateur, validation PIE si nécessaire, **un commit logique**, puis mise à jour de la documentation durable.
