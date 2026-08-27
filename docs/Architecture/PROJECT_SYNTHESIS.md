# GrimrockPrototype — Synthèse globale du projet

> Point d’entrée transversal de l’architecture et de l’état fonctionnel actuel.  
> État : **28 août 2026, TD07.3.4 Authoring Identity Normalization validé et clos ; TD07.3.5 à ouvrir.**

## 1. Référence

| Élément | Valeur |
|---|---|
| Projet | `GrimrockPrototype` |
| Moteur | Unreal Engine 5.5.4 |
| Branche | `master` |
| Modules C++ | `GrimrockPrototype`, `GrimrockPrototypeEditor`, `GrimrockLua` |
| SaveGame | **v17 exact-match** ; aucune compatibilité arrière ni migration |
| Dernier jalon fonctionnel | `MON21.3 — Quest Event -> Command Integration` |
| Dette structurelle ciblée | TD05 et TD06 en **stop condition atteinte** |
| Validation locale | Editor + Automation + Win64 Shipping via les harness TD04 |
| Dernière tranche validée | `TD07.3.3.5 — Normalize XP / Level / Class Progression` |
| Tranche active | `TD07.3.5 — Combat Data Schema Reset` — à ouvrir |

La dette technique courante est autoritairement suivie dans `docs/Architecture/TECHNICAL_DEBT_REGISTER.md`. La roadmap produit est `docs/Design/PROJECT_COMPLETION_ROADMAP.md`.

## 2. Lecture en cinq minutes

GrimrockPrototype est un dungeon crawler case par case avancé : édition de donjons, exploration, mécanismes, Event -> Command enrichi de variables/Logic/Lua, groupe RPG persistant, inventaire/équipement, combat tactique, IA de monstres, XP/niveaux, Status Effects, magie/Spellbook, recrutement, Skills, Talents et persistance associée.

MON13 à MON20 sont clos. MON21 possède déjà sa fondation Quest data-driven, mais les nouvelles fonctionnalités sont temporairement suspendues. TD07.3 remet à plat le modèle de données afin d'éliminer les compatibilités historiques, duplications d'autorité et schémas legacy devenus inutiles pendant la phase prototype. TD07.3.1 a scanné 86 DataAssets et produit une baseline de 41 findings. TD07.3.2 est validé : SaveGame v10 exact-match, aucune migration historique, régressions de persistance validées et Shipping vert. TD07.3.3.1 a ensuite cartographié l'autorité du personnage : pont legacy Attributes, DerivedStats mixte, poids dérivés et snapshots parallèles Progression/Skills/Spellbook/Status Effects. TD07.3.3.2 est validé : `Attributes` est l'unique autorité d'attributs, `Strength` legacy et `bRPGAttributesInitialized` sont supprimés, le SaveGame courant est v11 exact-match, les régressions sont vertes et le Shipping Win64 est validé. TD07.3.3.3 est validé : `FRPGDerivedStats` ne porte plus que les valeurs reconstructibles, `FRPGCharacterResources` porte HP/mana/armures courantes, le pipeline Magic consomme les ressources mutables, le SaveGame courant est v12 exact-match et les régressions ciblées ainsi que le Shipping Win64 sont verts. TD07.3.3.4 est validé : `CurrentWeight` et `MaxCarryWeight` ont quitté l'état durable, la charge et la surcharge sont désormais calculées à la demande depuis le contenu réel, `CarryWeightBonus` reste la seule extension de capacité issue de l'équipement, le SaveGame courant est v13 exact-match, les régressions ciblées et le Shipping Win64 sont verts. TD07.3.3.5 est validé : `Experience` est l'autorité durable du niveau, `Level` est une projection Transient reconstruite, `SelectedClassProgressionChoiceIds` vit directement dans le personnage, `RuntimeStates` ne contient plus que les requirements dérivés, le miroir `ClassProgressionStates` est supprimé, le SaveGame courant est v15 exact-match et les régressions ciblées ainsi que le Shipping Win64 sont verts. TD07.3.3.6 est validé et clos : `SkillRanks` est durable et constitue l'autorité unique, `CharacterSkillStates` et les structs de snapshot MON20.9 sont supprimés, `FRPGSkillPersistence` ne conserve que la validation canonique, et le SaveGame courant est v16 exact-match. TD07.3.3.7 est validé et clos : `KnownSpellIds` vit directement dans `FGridCharacterInventoryState`, le composant Spellbook n'est plus qu'une façade, `FGridPartySpellbookState` et `CharacterSpellbookStates` sont supprimés, les 55 régressions sont vertes et le Shipping Win64 est validé. TD07.3.3.8 est implémenté : `Character.StatusEffects` est durable, `DefinitionAsset` reste transient et rehydraté depuis `EffectId`, le miroir `CharacterStatusEffectStates` est supprimé, la persistance monster conserve `FGridStatusEffectSaveState`, et le SaveGame courant passe en v18 exact-match.

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
13. Tant que le projet reste un prototype, aucune compatibilité arrière Save/DataAsset/Blueprint n'est exigée ; Git conserve l'historique.

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
| Save | ✅ v15 exact-match ; aucune migration arrière |
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

L’état Quest reste transient. **MON21.4 est suspendu jusqu’à la stop condition TD07.3** ; sa future persistance suivra le schéma prototype courant sans migration historique.

## 11. Persistance

`UGrimrockPartySaveGame` utilise désormais la génération prototype v15 en exact-match. La v14 et toutes les générations antérieures sont rejetées ; aucune migration n'est exécutée.

TD07.3 impose désormais :

```text
ancienne save incompatible -> rejet
aucune migration arrière pendant le prototype
une seule représentation de chaque donnée durable
données dérivées recalculées
runtime/save fondés sur identités stables, pas sur pointeurs de contenu persistants
```

TD07.3.2 supprime la chaîne de migration v1-v9. TD07.3.3.2 supprime le bridge legacy des attributs et ouvre v11 ; les sous-tranches suivantes normalisent les autres états du personnage. TD07.3.4–TD07.3.7 nettoieront les DataAssets et le contenu courant.

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

Validation TD07.3.2 du 27 août 2026 : 6/6 sur le contrat TD07.3.2, 2/2 MON19.2 Save, 7/7 MON20.9 Skills, 10/10 MON16.7, 11/11 MON18.8 ; TD01.1 termine avec 2 tests `Succeeded with warnings` et 0 échec à cause de warnings du fixture transient. Le Shipping Win64 est validé.

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
TD07.3.1     Prototype Data Model Asset Audit             VALIDÉ
TD07.3.2     SaveGame Reset                              VALIDÉ
TD07.3.3     Character State Normalization                  ACTIF
TD07.3.3.1   Character State Authority Audit                VALIDÉ
TD07.3.3.2   Remove Legacy Attribute Bridge                 VALIDÉ
TD07.3.3.3   Normalize Derived Stats / Mutable Resources     VALIDÉ
TD07.3.3.4   Normalize Weight State                            VALIDÉ
TD07.3.3.5   Normalize XP / Level / Class Progression              VALIDÉ
TD07.3.3.6   Normalize Skills                                          VALIDÉ — CLOS
TD07.3.3.7   Normalize Spellbook                                       VALIDÉ — CLOS
TD07.3.3.8   Normalize Status Effects                                  VALIDÉ — CLOS
TD07.3.3.9   Normalize Level-Up Notification State                    VALIDÉ — CLOS
TD07.3.3.10  Current Save Schema / Regressions / Closure              VALIDÉ — CLOS
MON21.4      Quest Persistence                          SUSPENDU
MON21.5      Journal Read Model / WBP                     À FAIRE
MON21.6      Map Geometry / Exploration                   À FAIRE
MON21.7      Codex Discovery / Projection                 À FAIRE
MON21.8      Cross-System Regression / PIE / Closure      À FAIRE
MON22        vertical slice 45–90 minutes                 À FAIRE
```

## 16. Cartographie

- statut courant : présent document, `TECHNICAL_DEBT_REGISTER.md` et `PROJECT_COMPLETION_ROADMAP.md` ;
- cartes détaillées sous `docs/Architecture/Maps/` : snapshots historiques jusqu’à rafraîchissement global explicite.
