# Index de l’architecture

## Objet

Cet index référence les contrats d’architecture courants. Les documents de `docs/Design/` décrivent les jalons et décisions ; `docs/Architecture/` décrit la structure durable et les autorités runtime/editor.

**Référence courante : 26 août 2026, après TD04.3 et audit TD05.1.**  
Phase active : **dette technique ciblée — TD05 RuntimeActor**. MON21.2 n’est plus bloqué par TD04 et pourra reprendre après la tranche TD05 décidée utile.

## Ordre de lecture recommandé

1. [Registre autoritaire de dette technique](TECHNICAL_DEBT_REGISTER.md)
2. [Audit TD05.1 des rubriques de dette et RuntimeActor](TECHNICAL_DEBT_DOCUMENTATION_AUDIT.md)
3. [Synthèse globale du projet](PROJECT_SYNTHESIS.md)
4. [Donjon, niveau et grille](CORE_DUNGEON_LEVEL_GRID.md)
5. [Archétypes et objets placés](OBJECT_ARCHETYPES_AND_PLACED_OBJECTS.md)
6. [Event, variables, Logic et Lua](ADVANCED_DUNGEON_LOGIC_FOUNDATION.md)
7. [Combat, monstres et IA](COMBAT_MONSTER_AI_FOUNDATION.md)
8. [Groupe, RPG et recrutement](PARTY_RPG_RECRUITMENT_FOUNDATION.md)
9. [Magie et effets de statut](MAGIC_STATUS_EFFECTS_FOUNDATION.md)
10. [Sauvegarde et persistance](SAVE_PERSISTENCE_FOUNDATION.md)
11. [UI et flux de jeu](UI_GAME_FLOW_FOUNDATION.md)
12. [Tests et validation](TEST_AUTOMATION_FOUNDATION.md)

`ARCHITECTURE_CONSISTENCY_AUDIT.md` et `Maps/GRIMROCK_PROJECT_MAP.md` sont des snapshots datés du 23 août 2026. Ils restent utiles comme historique détaillé, mais **ne sont plus l’autorité du statut courant**. Pour l’état de la dette, `TECHNICAL_DEBT_REGISTER.md` prévaut.

## Fondations courantes

| Document | Portée |
|---|---|
| [CORE_DUNGEON_LEVEL_GRID.md](CORE_DUNGEON_LEVEL_GRID.md) | Donjon, niveaux, cellules, murs et génération runtime. |
| [OBJECT_ARCHETYPES_AND_PLACED_OBJECTS.md](OBJECT_ARCHETYPES_AND_PLACED_OBJECTS.md) | Archétypes, palette, objets placés, preview et actors. |
| [ADVANCED_DUNGEON_LOGIC_FOUNDATION.md](ADVANCED_DUNGEON_LOGIC_FOUNDATION.md) | Variables typées, Logic, Lua, `persistent`, `LogicId`. |
| [COMBAT_MONSTER_AI_FOUNDATION.md](COMBAT_MONSTER_AI_FOUNDATION.md) | Turn manager, actions, monstres, perception, patrouille, planners. |
| [PARTY_RPG_RECRUITMENT_FOUNDATION.md](PARTY_RPG_RECRUITMENT_FOUNDATION.md) | Groupe, XP/progression, CharacterPool et recrutement. |
| [MAGIC_STATUS_EFFECTS_FOUNDATION.md](MAGIC_STATUS_EFFECTS_FOUNDATION.md) | Spellbook, cast pipeline et Status Effects. |
| [SAVE_PERSISTENCE_FOUNDATION.md](SAVE_PERSISTENCE_FOUNDATION.md) | **SaveGame v9**, snapshots, migration et politique combat. |
| [UI_GAME_FLOW_FOUNDATION.md](UI_GAME_FLOW_FOUNDATION.md) | Menus, inventaire, Skills, Spellbook et surfaces futures. |
| [TEST_AUTOMATION_FOUNDATION.md](TEST_AUTOMATION_FOUNDATION.md) | Automation, harness Editor/Shipping, PIE et règles de validation. |
| [TECHNICAL_DEBT_REGISTER.md](TECHNICAL_DEBT_REGISTER.md) | Dette technique active, surveillée et différée. |

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
5. Event -> Command est le bus gameplay ; Logic et Lua reviennent vers ses commandes.
6. Ownership item exclusif.
7. `FGridPartyInventoryState` reste l’autorité groupe/`CharacterPool`.
8. SaveGame courant = **v9** ; toute montée de version exige un nouvel état durable et une migration définie.
9. Blueprint configure/compose ; logique métier validable en C++.
10. Les assets binaires exigent une validation Unreal/PIE lorsqu’ils sont concernés.
11. `Scripts/ValidateUE.ps1` est le harness local Editor + Automation validé.
12. `Scripts/ValidatePackage.ps1` est le harness Win64 Shipping validé.
13. Aucun refactor massif : réduire la dette par frontières stabilisées et caractérisées.
14. Les helpers locaux de nouveaux `.cpp` doivent être nommés de façon Unity-safe.

## Dette technique

Le registre autoritaire est :

```text
docs/Architecture/TECHNICAL_DEBT_REGISTER.md
```

Les rubriques locales `Dette`, `Risques` et `Points futurs` servent de contexte. Leur cohérence avec le registre a été réauditée dans `TECHNICAL_DEBT_DOCUMENTATION_AUDIT.md`.

## Historique

Git est l’unique mécanisme de conservation des versions antérieures. Les documents de jalon datés ne sont pas réécrits pour simuler l’état courant.
