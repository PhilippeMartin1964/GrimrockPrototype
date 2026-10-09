# Index de l'architecture

> **Référence courante — DOC-ARCH01, 9 octobre 2026.**
>
> Baseline C++ auditée : \`6f98a0ef5599f37d3d544529aa5a790d647e8251\`.
> Dernière campagne globale + Shipping : \`9045ef2d\`, 1026/1026,
> 0 warning, 0 échec, Shipping Win64 validé.

## 1. Objet

\`docs/Architecture/\` décrit les **autorités et frontières durables** du projet.
Les documents \`docs/Design/\` décrivent davantage les tickets, décisions,
validations et roadmaps.

DOC-ARCH01 classe le corpus Architecture en deux groupes :

- **références courantes** : à utiliser pour comprendre le code actuel ;
- **snapshots historiques** : migrations, cleanup, audits et clôtures datés.

Un document historique peut contenir une API ou une version Save aujourd'hui
supprimée sans être « faux » : il décrit son jalon. Il ne prime jamais sur les
références courantes.

## 2. Ordre de lecture recommandé

1. [Synthèse globale](PROJECT_SYNTHESIS.md)
2. [Carte détaillée](Maps/GRIMROCK_PROJECT_MAP.md)
3. [Cartographie Mermaid](Maps/GRIMROCK_PROJECT_MAP_MERMAID.md)
4. [Roadmap active](../Design/PROJECT_COMPLETION_ROADMAP.md)
5. [Registre de dette technique](TECHNICAL_DEBT_REGISTER.md)
6. [Donjon / Niveau / Grille](CORE_DUNGEON_LEVEL_GRID.md)
7. [World Object Definition / Instance](WORLD_OBJECT_DEFINITIONS_AND_PLACED_OBJECTS.md)
8. [Event / Logic / Lua](ADVANCED_DUNGEON_LOGIC_FOUNDATION.md)
9. [Interactions souris](MOUSE_INTERACTION_FOUNDATION.md)
10. [Items / transferts](ITEM_PICKUP_AND_PLACEMENT_FOUNDATION.md)
11. [Réceptacles](RECEPTACLE_SYSTEM_FOUNDATION.md)
12. [Combat / IA](COMBAT_MONSTER_AI_FOUNDATION.md)
13. [Groupe / RPG](PARTY_RPG_RECRUITMENT_FOUNDATION.md)
14. [Magic / Status](MAGIC_STATUS_EFFECTS_FOUNDATION.md)
15. [Save](SAVE_PERSISTENCE_FOUNDATION.md)
16. [UI / flux](UI_GAME_FLOW_FOUNDATION.md)
17. [Tests / validation](TEST_AUTOMATION_FOUNDATION.md)

## 3. Références courantes

| Document | Autorité décrite |
|---|---|
| \`PROJECT_SYNTHESIS.md\` | Vue transversale actuelle. |
| \`CORE_DUNGEON_LEVEL_GRID.md\` | DungeonAsset, LevelAsset, grille et placements typés. |
| \`WORLD_OBJECT_DEFINITIONS_AND_PLACED_OBJECTS.md\` | Definition/Instance, palette, runtime/preview. |
| \`DOOR_MECHANISM_FOUNDATION.md\` | Portes, motion, passabilité et mécanismes. |
| \`LINK_EVENT_COMMAND_FOUNDATION.md\` | Events, links, commands, conditions, Quest/Lua. |
| \`ADVANCED_DUNGEON_LOGIC_FOUNDATION.md\` | Variables, Logic, Lua sandboxé. |
| \`MOUSE_INTERACTION_FOUNDATION.md\` | Routage souris et curseur. |
| \`ITEM_PICKUP_AND_PLACEMENT_FOUNDATION.md\` | Items, ownership, transferts monde/inventaire. |
| \`RECEPTACLE_SYSTEM_FOUNDATION.md\` | Contenu, acceptation, insertion/retrait, save. |
| \`READABLE_OBJECTS_AND_FEEDBACK_FOUNDATION.md\` | Readables et feedback interaction. |
| \`ITEM_LIGHT01_DATA_DRIVEN_ITEM_LIGHT.md\` | Lumière data-driven des items. |
| \`LIGHT_CONFIG02_SINGLE_POINT_LIGHT_AUTHORITY.md\` | Autorité unique des bases PointLight. |
| \`PARTY_LIGHT01_EQUIPMENT_DRIVEN_PARTY_ILLUMINATION.md\` | Proxy lumineux du groupe. |
| \`MATERIAL_OWNERSHIP.md\` | Material Slots des StaticMesh. |
| \`COMBAT_MONSTER_AI_FOUNDATION.md\` | TurnManager, actions, IA et encounters. |
| \`PARTY_RPG_RECRUITMENT_FOUNDATION.md\` | Groupe, recrutement, Skills, Talents, Attributes. |
| \`MAGIC_STATUS_EFFECTS_FOUNDATION.md\` | Spellbook, cast et Status Effects. |
| \`SAVE_PERSISTENCE_FOUNDATION.md\` | Save v24 exact-match. |
| \`UI_GAME_FLOW_FOUNDATION.md\` | Surfaces runtime et navigation. |
| \`STARTUP_FLOW01_FRONTEND_DUNGEON.md\` | Main menu → création → L_Dungeon. |
| \`LEVEL_VALIDATION_PANEL_FOUNDATION.md\` | Validation Editor. |
| \`TEST_AUTOMATION_FOUNDATION.md\` | Build, Automation, PIE, Shipping. |
| \`TECHNICAL_DEBT_REGISTER.md\` | Dette active/surveillée. |
| \`Maps/GRIMROCK_PROJECT_MAP.md\` | Carte textuelle autoritaire. |
| \`Maps/GRIMROCK_PROJECT_MAP_MERMAID.md\` | Vues système Mermaid. |
| \`Maps/Grimrock_MindMap_Architecture_Cible_v2_XMind.md\` | Arbre Markdown importable XMind. |

## 4. Snapshots historiques

Les familles suivantes restent dans Git pour expliquer les migrations et les
décisions, mais ne définissent plus le schéma actuel :

- \`ALIGN_*\` ;
- \`*_CLEANUP_NOTES.md\` ;
- \`ARCHITECTURE_CONSISTENCY_AUDIT.md\` ;
- \`TECHNICAL_DEBT_DOCUMENTATION_AUDIT.md\` ;
- \`CPP_GENERAL_AUDIT_2026_09_27.md\` ;
- \`TD07_FINAL_QUANTITATIVE_AUDIT_BASELINE.md\` ;
- \`PAWN_CLEAN02_EDITOR_SURFACE.md\` ;
- \`ITEM_THROW_MIG01_SHURIKEN_AUTHORITY.md\` ;
- \`WORLDOBJ_MIG*.md\` ;
- \`WORLDOBJ_RECOVERY01_FINAL.md\` ;
- \`WORLDOBJ_ITEMCLASS01_ITEM_ACTOR_AUTHORITY.md\` ;
- \`WORLDOBJ_MOVINGPARTS01_GENERIC_ARRAY.md\` ;
- \`WORLDOBJ_MIGRATION_ROADMAP_AND_TARGET_DATA_MODEL.md\`.

DOC-ARCH01 ajoute un bandeau historique à ces fichiers afin qu'un ancien
\`CURRENT\`, \`FINAL\` ou plan de migration ne soit pas interprété comme une
nouvelle autorité.

## 5. Règles transversales courantes

1. \`SupportedType\` est la classification gameplay principale des World Objects.
2. \`FGridObjectPaletteEntry::PaletteCategory\` est le groupement Editor.
3. La palette ne possède plus de \`DisplayNameOverride\`.
4. \`InstanceId\` / SpawnId restent les identités stables de placement.
5. \`LogicId\` est un alias d'authoring, pas l'identité persistante.
6. Event → Command reste le bus de mutation.
7. Lua et Logic ne contournent pas ce bus.
8. \`FGridPartyInventoryState\` reste l'autorité groupe/inventaire.
9. \`UGridTurnManagerComponent\` reste l'autorité combat.
10. \`UGridQuestSubsystem\` est l'autorité Quest runtime, encore non persistée.
11. Le SaveGame courant est **v24 exact-match**.
12. La Map est une projection filtrée de l'état runtime et de l'exploration.
13. \`UGridPersistentHudWidget\` porte navigation + action bar persistante.
14. \`UGridCombatHudWidget\` porte uniquement la présentation combat.
15. C++ décide ; UMG présente.

## 6. Jalons encore ouverts

\`\`\`text
MON21.4 Quest Persistence      EN ATTENTE
MON21.5 Journal               À FAIRE
MON21.7 Codex                 À FAIRE
MON21.8 Cross-System Closure  À FAIRE
MON22 Vertical Slice          À FAIRE
\`\`\`

MON21.6 Map est **clos** et ne doit plus apparaître comme « shell » ou futur.
