# GrimrockPrototype Design Docs

Ce dossier est la mémoire de conception stable du projet : décisions acceptées, roadmaps actives, contrats d’implémentation et validations. La description transversale de l’architecture vit sous `docs/Architecture/`.

## Ordre de lecture actif

1. `00_PROJECT_OVERVIEW.md` — orientation et état validé actuel.
2. `PROJECT_COMPLETION_ROADMAP.md` — backlog actif.
3. `../Architecture/PROJECT_SYNTHESIS.md` — bilan architectural transversal.
4. `../Architecture/Maps/GRIMROCK_PROJECT_MAP.md` — carte détaillée autoritaire.
5. `../Architecture/Maps/GRIMROCK_PROJECT_MAP_MERMAID.md` — vues visuelles.
6. `MON19_CLOSURE.md` — dernier jalon majeur clos.
7. `MON20_1_AUDIT_RECRUITMENT_SKILLS_TALENTS.md` — contrat MON20.
8. `MON20_2_ACTIVE_PARTY_RECRUITMENT_FOUNDATION.md` — recrutement actif validé.
9. `MON20_3_STORY_COMPANION_DEFINITION_POOL.md` — compagnon scénarisé validé.
10. `99_DECISIONS_LOG.md` — décisions durables.

## Jalons majeurs

| Famille | Statut | Résumé |
|---|---|---|
| MON1–MON12 | Validé | Fondations monstres, combat, attaques groupe, initiative PA/PAM, HUD/hotbar. |
| MON13 | **Clos** | MonsterSpawn, lifecycle, encounters, vagues, persistance. |
| MON14 | **Clos** | Engagement, perception, dormance, patrouille, investigation, alarmes. |
| MON15 | **Clos** | XP, niveaux, progression de classe, Level Up, save/migration. |
| MON16 | **Clos** | Status Effects. |
| MON17 | **Clos** | Gobelin lanceur, projectile, RangedKeeper, loot/XP. |
| MON18 | **Clos** | Magic & Spellbook. |
| MON19 | **Clos** | Variables, Logic, Lua, persistent, LogicId, authoring Editor. |
| MON20.1 | **Terminé** | Audit Recruitment / Skills / Talents. |
| MON20.2 | **Validé** | CharacterPool → ActiveCharacters, 6/6. |
| MON20.3 | **Validé** | Story Companion data-driven, 6/6. |
| MON20.4 | **Prochain** | Recruitment UI. |

## Roadmap active

```text
MON20.4+ — Recruitment / Skills / Talents
MON21   — Quests / Journal / Map / Codex
MON22   — 45–90 Minute Vertical Slice
```

## Règle de priorité documentaire

1. dernier document de clôture/validation et `99_DECISIONS_LOG.md` ;
2. `PROJECT_COMPLETION_ROADMAP.md` ;
3. `00_PROJECT_OVERVIEW.md` ;
4. `docs/Architecture/PROJECT_SYNTHESIS.md` et fondations courantes.

Git conserve l’historique ; aucun doublon daté n’est nécessaire dans l’arborescence actuelle.
