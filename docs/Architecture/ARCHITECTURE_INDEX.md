# Index de l’architecture

## Objet

Cet index référence les contrats d’architecture vérifiés contre le C++ actuel. Les documents de `docs/Design/` décrivent les jalons et décisions ; `docs/Architecture/` décrit la structure durable et les autorités runtime/editor.

**Audit courant :** 23 août 2026, après validation de MON20.3 et avant MON20.4.

## Ordre de lecture recommandé

1. [Synthèse globale du projet](PROJECT_SYNTHESIS.md)
2. [Carte détaillée autoritaire](Maps/GRIMROCK_PROJECT_MAP.md)
3. [Cartes visuelles Mermaid](Maps/GRIMROCK_PROJECT_MAP_MERMAID.md)
4. [Audit transversal de cohérence](ARCHITECTURE_CONSISTENCY_AUDIT.md)
5. [Donjon, niveau et grille](CORE_DUNGEON_LEVEL_GRID.md)
6. [Archétypes et objets placés](OBJECT_ARCHETYPES_AND_PLACED_OBJECTS.md)
7. [Event, variables, Logic et Lua](ADVANCED_DUNGEON_LOGIC_FOUNDATION.md)
8. [Combat, monstres et IA](COMBAT_MONSTER_AI_FOUNDATION.md)
9. [Groupe, RPG et recrutement](PARTY_RPG_RECRUITMENT_FOUNDATION.md)
10. [Magie et effets de statut](MAGIC_STATUS_EFFECTS_FOUNDATION.md)
11. [Sauvegarde et persistance](SAVE_PERSISTENCE_FOUNDATION.md)
12. [UI et flux de jeu](UI_GAME_FLOW_FOUNDATION.md)
13. [Tests et validation](TEST_AUTOMATION_FOUNDATION.md)

## Fondations historiques toujours actives

| Document | Portée |
|---|---|
| [CORE_DUNGEON_LEVEL_GRID.md](CORE_DUNGEON_LEVEL_GRID.md) | Donjon, niveaux, cellules, murs et génération runtime. |
| [OBJECT_ARCHETYPES_AND_PLACED_OBJECTS.md](OBJECT_ARCHETYPES_AND_PLACED_OBJECTS.md) | Archétypes, palette, objets placés, preview et actors. |
| [LEVEL_VALIDATION_PANEL_FOUNDATION.md](LEVEL_VALIDATION_PANEL_FOUNDATION.md) | Validation éditeur et navigation vers les problèmes. |
| [MOUSE_INTERACTION_FOUNDATION.md](MOUSE_INTERACTION_FOUNDATION.md) | Interaction souris, traces, portée et priorité. |
| [LINK_EVENT_COMMAND_FOUNDATION.md](LINK_EVENT_COMMAND_FOUNDATION.md) | Socle Event → Command, étendu par MON19. |
| [DOOR_MECHANISM_FOUNDATION.md](DOOR_MECHANISM_FOUNDATION.md) | Portes, passabilité et commandes. |
| [RECEPTACLE_SYSTEM_FOUNDATION.md](RECEPTACLE_SYSTEM_FOUNDATION.md) | Réceptacles, contenu et événements. |
| [ITEM_PICKUP_AND_PLACEMENT_FOUNDATION.md](ITEM_PICKUP_AND_PLACEMENT_FOUNDATION.md) | Items, ownership, curseur et transferts. |
| [READABLE_OBJECTS_AND_FEEDBACK_FOUNDATION.md](READABLE_OBJECTS_AND_FEEDBACK_FOUNDATION.md) | Objets lisibles et feedback. |

## Fondations transversales de l’audit 2026-08-23

| Document | Portée |
|---|---|
| [ADVANCED_DUNGEON_LOGIC_FOUNDATION.md](ADVANCED_DUNGEON_LOGIC_FOUNDATION.md) | Variables typées, Logic, Lua, `persistent`, `LogicId`. |
| [COMBAT_MONSTER_AI_FOUNDATION.md](COMBAT_MONSTER_AI_FOUNDATION.md) | Turn manager, actions, monstres, perception, patrouille, planners. |
| [PARTY_RPG_RECRUITMENT_FOUNDATION.md](PARTY_RPG_RECRUITMENT_FOUNDATION.md) | Groupe, XP/progression, CharacterPool et recrutement. |
| [MAGIC_STATUS_EFFECTS_FOUNDATION.md](MAGIC_STATUS_EFFECTS_FOUNDATION.md) | Spellbook, cast pipeline et Status Effects. |
| [SAVE_PERSISTENCE_FOUNDATION.md](SAVE_PERSISTENCE_FOUNDATION.md) | SaveGame v7, snapshots, migration et politique combat. |
| [UI_GAME_FLOW_FOUNDATION.md](UI_GAME_FLOW_FOUNDATION.md) | Menus, inventaire, combat, création, progression, spellbook, surfaces futures. |
| [TEST_AUTOMATION_FOUNDATION.md](TEST_AUTOMATION_FOUNDATION.md) | Automation Tests, PIE et règles de validation. |

## Modules

```text
GrimrockLua
    ↓
GrimrockPrototype
    ↓
GrimrockPrototypeEditor
```

L’Editor dépend aussi directement de `GrimrockLua`. Le Runtime ne dépend jamais du module Editor.

## Règles transversales

1. DataAssets = sources persistantes de conception.
2. Actors runtime reconstruits depuis ces données.
3. Grille autoritaire pour déplacement, occupation, ligne de mire et combat.
4. `ObjectId` reste l’identité d’objet ; `LogicId` est un alias authoring.
5. Event → Command est le bus gameplay ; Logic et Lua reviennent vers ses commandes.
6. Ownership item exclusif.
7. `FGridPartyInventoryState` reste l’autorité groupe/`CharacterPool`.
8. Les talents réutilisent d’abord la progression de classe existante.
9. SaveGame reste v7 tant qu’une nouvelle persistance ne justifie pas une migration.
10. Blueprint configure/compose ; logique métier validable en C++.
11. Les assets binaires ne sont validés qu’après UE/PIE.
12. Un sous-jalon = un commit logique.

## Historique

Git est l’unique mécanisme de conservation des versions antérieures de ces documents. Aucun doublon daté n’est maintenu dans l’arborescence courante.
