# GrimrockPrototype — Synthèse globale du projet

> Point d’entrée transversal de l’architecture et de l’état fonctionnel actuel.  
> État : **26 août 2026, après TD06.9 et MON21.3.**

## 1. Référence

| Élément | Valeur |
|---|---|
| Projet | `GrimrockPrototype` |
| Moteur | Unreal Engine 5.5.4 |
| Branche | `master` |
| Modules C++ | `GrimrockPrototype`, `GrimrockPrototypeEditor`, `GrimrockLua` |
| SaveGame | version courante **9**, compatibilité minimale `1` |
| Dernier jalon fonctionnel | `MON21.3 — Quest Event -> Command Integration` |
| Dette structurelle ciblée | TD05 et TD06 en **stop condition atteinte** |
| Validation locale | Editor + Automation + Win64 Shipping via les harness TD04 |
| Prochaine tranche | `MON21.4 — Quest Persistence / Migration` |

La dette technique courante est autoritairement suivie dans `docs/Architecture/TECHNICAL_DEBT_REGISTER.md`. La roadmap produit est `docs/Design/PROJECT_COMPLETION_ROADMAP.md`.

## 2. Lecture en cinq minutes

GrimrockPrototype est un dungeon crawler case par case avancé : édition de donjons, exploration, mécanismes, Event -> Command enrichi de variables/Logic/Lua, groupe RPG persistant, inventaire/équipement, combat tactique, IA de monstres, XP/niveaux, Status Effects, magie/Spellbook, recrutement, Skills, Talents et persistance associée.

MON13 à MON20 sont clos. MON21 a maintenant une fondation Quest data-driven : `UGridQuestDefinitionAsset`, `UGridQuestSubsystem`, état runtime de campagne et intégration au bus Event -> Command. La prochaine étape est de persister cet état de quête avant de construire le Journal.

Les campagnes TD05 et TD06 ont atteint leur stop condition : `AGridLevelRuntimeActor` et `UGridPartyInventoryComponent` restent des façades/orchestrateurs, mais leurs frontières à forte cohésion sont désormais séparées sans dupliquer l’autorité.

## 3. Principes autoritaires

1. DataAssets = état de conception persistant ; Actors runtime reconstruits et transitoires.
2. Grille autoritaire pour déplacement, occupation, ligne de mire et ciblage.
3. Event -> Command reste le bus gameplay ; Logic et Lua y reviennent.
4. Identités stables : `ObjectId`, `LogicId`, `CharacterId`, `RuntimeObjectId`, `QuestId`, `ObjectiveId`.
5. État initial distinct de l’état vivant/sauvegardé.
6. Logique déterministe séparée de la présentation.
7. Pas d’abstraction parallèle sans besoin démontré.
8. Editor dépend du Runtime, jamais l’inverse ; `GrimrockLua` est autonome.
9. `FGridPartyInventoryState` reste l’autorité unique du groupe.
10. `UGridQuestSubsystem` reste l’autorité runtime unique des quêtes.
11. Les données dérivées ne sont pas persistées lorsqu’elles peuvent être reconstruites.
12. Refactors structurels uniquement après caractérisation et avec une stop condition explicite.

## 4. Architecture des modules

```text
GrimrockLua
    ↓
GrimrockPrototype
    ↓
GrimrockPrototypeEditor
```

Le module Editor dépend aussi de `GrimrockLua`. Le Runtime ne dépend pas du module Editor.

## 5. Domaines actuels

| Domaine | État |
|---|---|
| Donjon / grille / LevelAsset | ✅ |
| Grid Editor | ✅ avancé ; TD03 legacy Details nettoyé |
| Runtime niveau | ✅ ; TD05.9 stop condition atteinte |
| Interaction / mécanismes | ✅ |
| Event / Command | ✅ ; commandes Quest intégrées MON21.3 |
| Variables / Logic / Lua | ✅ MON19 |
| Items / inventaire / équipement | ✅ avancé ; TD06.9 stop condition atteinte |
| Combat | ✅ MON12+ |
| Monstres / IA | ✅ ; bestiaire à densifier comme contenu |
| Progression RPG | ✅ MON15 |
| Status Effects | ✅ MON16 |
| Magic / Spellbook | ✅ MON18 |
| Recrutement / réserve | ✅ MON20 |
| Skills / Talents | ✅ MON20 |
| Save | ✅ **v9** ; état Quest non encore persisté |
| Quêtes runtime | ✅ MON21.2–MON21.3 |
| Journal | ⬜ WBP existant ; read model prévu MON21.5 |
| Map | ⬜ WBP existant ; exploration prévue MON21.6 |
| Codex | ⬜ WBP existant ; discovery prévu MON21.7 |

## 6. Donjon, grille et éditeur

`UGridDungeonAsset -> UGridLevelAsset` reste la racine du contenu. Un LevelAsset porte cellules, objets, liens, variables typées, scripts Lua et désormais les références de définitions Quest utilisées par le niveau.

Le Grid Editor offre peinture cellule/mur, placement, inspecteur, connecteurs, preview, mini-carte, validation et playtest PIE.

## 7. Event -> Command, Logic, Lua et Quests

```text
Event -> Command
Event -> Logic -> Event -> Command
Event -> Lua -> grid.command(...) -> Command
Event -> QuestStart / QuestCompleteObjective / QuestComplete / QuestFail
```

`TD-EVENT-001` est résolu. MON21.3 adapte le bus existant vers `UGridQuestSubsystem` ; les commandes Quest ne créent aucun pipeline parallèle.

## 8. Groupe, inventaire, recrutement, Skills et Talents

Autorité :

```text
FGridPartyInventoryState
├── ActiveCharacters
├── ActiveEquipment
├── CharacterPool
└── FGridCharacterInventoryState
```

MON20 fournit recrutement actif/réserve, Story Companion, Custom Recruit, Skills, skill checks déterministes, Talents via `ProgressionChoices` et projection vers `RequirementIds`.

TD06 a réparti l’implémentation de `UGridPartyInventoryComponent` entre Hotbar, Cursor Transfer, Equipment Core, World Transfer et Diagnostics. Le fichier principal conserve le lifecycle, l’inventaire générique, Registry/Rehydration, poids, ownership et validations centrales. L’autorité `FGridPartyInventoryState` reste unique.

## 9. Combat, monstres et magie

Combat : initiative globale, PA/PAM, catalogue d’actions, ciblage grille, transactions de ressources, cooldowns, hotbar 0–9.

Monstres : occupation/pathfinding, perception automatique, dormance, patrouille, investigation, alarmes ; Rat géant mêlée et Gobelin lanceur à distance.

MON16 fournit Status Effects ; MON18 fournit Spellbook/cast. Le petit bestiaire est un manque de contenu, pas une dette d’architecture.

## 10. Quêtes

```text
UGridQuestDefinitionAsset
    -> QuestId
    -> objectifs ordonnés / ObjectiveId

UGridQuestSubsystem : UGameInstanceSubsystem
    -> registre transient des définitions
    -> FGridCampaignQuestRuntimeState
    -> Start / CompleteObjective / Complete / Fail
```

MON21.2 a établi l’autorité runtime et les transitions séquentielles. MON21.3 a relié les commandes Quest au bus Event -> Command via `FGridObjectLink`.

L’état Quest reste transient en v9 : **MON21.4 doit introduire sa persistance et la migration associée avant MON21.5 Journal**.

## 11. Persistance

`UGrimrockPartySaveGame v9` conserve notamment groupe, inventaire/équipement/hotbar, progression, Status Effects, Spellbooks, Skills, dungeon runtime state, variables, position/facing, monstres et permissions runtime des réceptacles.

Migrations récentes :

```text
v7 -> v8  SkillRanks MON20.9
v8 -> v9  Receptacle removal permission TD01.1
```

Le prochain changement de SaveVersion n’est justifié que si MON21.4 ajoute effectivement un snapshot Quest durable.

## 12. UI

Surfaces fonctionnelles : menu principal/Continue/Load, inventaire/paper doll, sélection du groupe, création/recrutement, Level Up, combat, Spellbook, Skills/Talents.

Journal, Map et Codex existent déjà dans le menu, mais restent des projections futures. Ils ne doivent jamais devenir des autorités gameplay.

## 13. Validation et packaging

```text
Scripts/ValidateUE.ps1
    -> GrimrockPrototypeEditor Win64 Development
    -> Automation explicite

Scripts/ValidatePackage.ps1
    -> GrimrockPrototype Win64 Shipping
    -> Build + Cook + Stage + Package + Pak + Archive
```

La CI distante UE reste différée tant qu’aucun vrai runner UE5.5.4 n’est provisionné.

## 14. Dette technique

Stop conditions atteintes :

```text
TD05.9  AGridLevelRuntimeActor
TD06.9  UGridPartyInventoryComponent
```

Aucune nouvelle tranche de découpage de ces classes n’est recommandée sans signal concret. Les dettes restantes sont suivies comme surveillées, opportunistes ou différées dans le registre autoritaire.

## 15. Roadmap

```text
MON13–MON20  systèmes gameplay majeurs                    CLOS
MON21.1      architecture Quests/Journal/Map/Codex        CLOS
MON21.2      Quest Definition + Campaign Runtime State    VALIDÉ
MON21.3      Quest Event -> Command Integration           VALIDÉ
TD01–TD06    stabilisation / dette ciblée                 STOP CONDITIONS ATTEINTES
MON21.4      Quest Persistence / Migration                PROCHAIN
MON21.5      Journal Read Model / WBP                     À FAIRE
MON21.6      Map Geometry / Exploration                   À FAIRE
MON21.7      Codex Discovery / Projection                 À FAIRE
MON21.8      Cross-System Regression / PIE / Closure      À FAIRE
MON22        vertical slice 45–90 minutes                 À FAIRE
```

## 16. Cartographie

- statut courant : présent document, `TECHNICAL_DEBT_REGISTER.md` et `PROJECT_COMPLETION_ROADMAP.md` ;
- cartes détaillées sous `docs/Architecture/Maps/` : snapshots historiques jusqu’à rafraîchissement global explicite.
