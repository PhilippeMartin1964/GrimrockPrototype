# Index de l’architecture

## Objet

Cet index référence les contrats d’architecture courants. Les documents de `docs/Design/` décrivent les jalons et décisions ; `docs/Architecture/` décrit la structure durable et les autorités runtime/editor.

**Référence courante : 27 août 2026, TD07.3.3.7 Spellbook clos ; TD07.3.3.8 Status Effects normalization implémentée — à valider.**  
Phase active : **TD07.3 — Prototype Data Model Reset**. MON21.4 reste suspendu jusqu'à la stop condition TD07.3.

## Ordre de lecture recommandé

1. [Registre autoritaire de dette technique](TECHNICAL_DEBT_REGISTER.md)
2. [Synthèse globale du projet](PROJECT_SYNTHESIS.md)
3. [Roadmap active](../Design/PROJECT_COMPLETION_ROADMAP.md)
4. [Donjon, niveau et grille](CORE_DUNGEON_LEVEL_GRID.md)
5. [Archétypes et objets placés](OBJECT_ARCHETYPES_AND_PLACED_OBJECTS.md)
6. [Event, variables, Logic et Lua](ADVANCED_DUNGEON_LOGIC_FOUNDATION.md)
7. [Combat, monstres et IA](COMBAT_MONSTER_AI_FOUNDATION.md)
8. [Groupe, RPG et recrutement](PARTY_RPG_RECRUITMENT_FOUNDATION.md)
9. [Magie et effets de statut](MAGIC_STATUS_EFFECTS_FOUNDATION.md)
10. [Sauvegarde et persistance](SAVE_PERSISTENCE_FOUNDATION.md)
11. [UI et flux de jeu](UI_GAME_FLOW_FOUNDATION.md)
12. [Tests et validation](TEST_AUTOMATION_FOUNDATION.md)

`TECHNICAL_DEBT_DOCUMENTATION_AUDIT.md`, `ARCHITECTURE_CONSISTENCY_AUDIT.md` et `Maps/GRIMROCK_PROJECT_MAP.md` sont des snapshots historiques. Ils restent utiles comme références datées, mais ne sont pas l’autorité du statut courant.

## Fondations courantes

| Document | Portée |
|---|---|
| [CORE_DUNGEON_LEVEL_GRID.md](CORE_DUNGEON_LEVEL_GRID.md) | Donjon, niveaux, cellules, murs et génération runtime. |
| [OBJECT_ARCHETYPES_AND_PLACED_OBJECTS.md](OBJECT_ARCHETYPES_AND_PLACED_OBJECTS.md) | Archétypes, palette, objets placés, preview et actors. |
| [ADVANCED_DUNGEON_LOGIC_FOUNDATION.md](ADVANCED_DUNGEON_LOGIC_FOUNDATION.md) | Variables typées, Logic, Lua, `persistent`, `LogicId`. |
| [COMBAT_MONSTER_AI_FOUNDATION.md](COMBAT_MONSTER_AI_FOUNDATION.md) | Turn manager, actions, monstres, perception, patrouille, planners. |
| [PARTY_RPG_RECRUITMENT_FOUNDATION.md](PARTY_RPG_RECRUITMENT_FOUNDATION.md) | Groupe, XP/progression, CharacterPool et recrutement. |
| [MAGIC_STATUS_EFFECTS_FOUNDATION.md](MAGIC_STATUS_EFFECTS_FOUNDATION.md) | Spellbook, cast pipeline et Status Effects. |
| [SAVE_PERSISTENCE_FOUNDATION.md](SAVE_PERSISTENCE_FOUNDATION.md) | Save prototype v18 exact-match ; aucune migration arrière. |
| [UI_GAME_FLOW_FOUNDATION.md](UI_GAME_FLOW_FOUNDATION.md) | Menus, inventaire, Skills, Spellbook et surfaces campagne. |
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
6. `UGridQuestSubsystem` est l’autorité runtime de campagne pour les quêtes ; `UGridQuestDefinitionAsset` porte les définitions.
7. Ownership item exclusif.
8. `FGridPartyInventoryState` reste l’autorité groupe/`CharacterPool`.
9. Pendant le prototype, le SaveGame utilise uniquement le schéma courant ; une version ancienne est rejetée et aucune migration arrière n'est exigée.
10. Blueprint configure/compose ; logique métier validable en C++.
11. Les assets binaires exigent une validation Unreal/PIE lorsqu’ils sont concernés.
12. `Scripts/ValidateUE.ps1` est le harness local Editor + Automation validé.
13. `Scripts/ValidatePackage.ps1` est le harness Win64 Shipping validé.
14. Aucun refactor massif : réduire la dette par frontières stabilisées et caractérisées.
15. Les helpers locaux de nouveaux `.cpp` doivent être nommés de façon Unity-safe.

## Dette technique

TD05 et TD06 ont atteint leur stop condition respective :

- `AGridLevelRuntimeActor` : TD05.9 ;
- `UGridPartyInventoryComponent` : TD06.9.

Aucune nouvelle tranche de refactor de ces classes n’est recommandée sans signal concret. Le registre autoritaire reste :

```text
docs/Architecture/TECHNICAL_DEBT_REGISTER.md
```

## Phase courante

Les fonctionnalités sont volontairement suspendues pendant le nettoyage du modèle de données.

```text
TD07.1    Build / dependency reproducibility             VALIDÉ
TD07.2    UE compatibility cleanup                       VALIDÉ
TD07.3.1  Prototype Data Model Policy + Asset Audit      VALIDÉ
TD07.3.2  SaveGame Reset / no backward migration         VALIDÉ
TD07.3.3  Character State Normalization                    ACTIF
TD07.3.3.1 Character State Authority Audit                 VALIDÉ
TD07.3.3.2 Remove Legacy Attribute Bridge                  VALIDÉ
TD07.3.3.3 Normalize Derived Stats / Mutable Resources      VALIDÉ
TD07.3.3.4 Normalize Weight State                             VALIDÉ
TD07.3.3.5 Normalize XP / Level / Class Progression              VALIDÉ
TD07.3.3.6 Normalize Skills                                          VALIDÉ — CLOS
TD07.3.3.7 Normalize Spellbook                                       VALIDÉ — CLOS
TD07.3.3.8 Normalize Status Effects                                  IMPLÉMENTÉ — À VALIDER
TD07.3.4–TD07.3.8                                       À FAIRE
```

## Phase fonctionnelle suspendue

```text
MON21.1  Audit & Architecture Contract                  CLOS
MON21.2  Quest Definition + Campaign Runtime State      VALIDÉ
MON21.3  Quest Event -> Command Integration             VALIDÉ
MON21.4  Quest Persistence                            SUSPENDU JUSQU'À TD07.3
MON21.5  Journal Read Model + Existing WBP Integration  À FAIRE
MON21.6  Map Geometry + Exploration State + Existing WBP À FAIRE
MON21.7  Codex Discovery + Definition Projection        À FAIRE
MON21.8  Cross-System Regression / PIE / Closure        À FAIRE
```

## Historique

Git est l’unique mécanisme de conservation des versions antérieures. Les documents de jalon datés ne sont pas réécrits pour simuler l’état courant.
