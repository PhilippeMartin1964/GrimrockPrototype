# GrimrockPrototype — Synthèse globale du projet

> Point d’entrée transversal de l’architecture et de l’état fonctionnel actuel.
> État : **26 août 2026, après TD04.3 et audit TD05.1.**

## 1. Référence

| Élément | Valeur |
|---|---|
| Projet | `GrimrockPrototype` |
| Moteur | Unreal Engine 5.5.4 |
| Branche | `master` |
| Modules C++ | `GrimrockPrototype`, `GrimrockPrototypeEditor`, `GrimrockLua` |
| SaveGame | version courante **9**, compatibilité minimale `1` |
| Dernier jalon fonctionnel fermé | `MON20 — Recruitment / Skills / Talents` |
| Audit campagne | `MON21.1 — Quests / Journal / Map / Codex` terminé |
| Validation locale | Editor + Automation + Win64 Shipping validés par TD04 |
| Dette ciblée actuelle | `TD05 — AGridLevelRuntimeActor` |
| Prochaine tranche | `TD05.2 — RuntimeActor Diagnostics characterization` |

La dette technique courante est autoritairement suivie dans `docs/Architecture/TECHNICAL_DEBT_REGISTER.md`.

## 2. Lecture en cinq minutes

GrimrockPrototype est un vertical slice technique avancé de dungeon crawler case par case : édition de donjons, exploration, mécanismes, Event -> Command enrichi de variables/Logic/Lua, groupe RPG persistant, inventaire/équipement, combat tactique, IA de monstres, XP/niveaux, Status Effects, magie/Spellbook, recrutement, Skills, Talents et persistance associée.

MON13 à MON20 sont fermés. MON21.1 a défini le contrat futur Quests / Journal / Map / Codex sans lancer l’implémentation. La campagne TD01–TD04 a stabilisé persistance, notifications, Event -> Command, gros orchestrateurs, Grid Editor et validation locale Editor/Shipping.

## 3. Principes autoritaires

1. DataAssets = état de conception persistant ; Actors runtime reconstruits et transitoires.
2. Grille autoritaire pour déplacement, occupation, ligne de mire et ciblage.
3. Event -> Command reste le bus gameplay ; Logic et Lua y reviennent.
4. Identités stables (`ObjectId`, `CharacterId`, RuntimeObjectId/spawn identities).
5. État initial distinct de l’état vivant/sauvegardé.
6. Logique déterministe séparée de la présentation.
7. Pas d’abstraction parallèle sans besoin démontré.
8. Editor dépend du Runtime, jamais l’inverse ; `GrimrockLua` est autonome.
9. `FGridPartyInventoryState` reste l’autorité unique du groupe.
10. Les données dérivées ne sont pas persistées lorsqu’elles peuvent être reconstruites.
11. Refactors structurels uniquement après caractérisation et avec une stop condition explicite.

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
| Runtime niveau | ✅ ; ⚠️ `AGridLevelRuntimeActor` encore centralisé, TD05 actif |
| Interaction / mécanismes | ✅ |
| Event / Command | ✅ contrat TD-EVENT-001 résolu |
| Variables / Logic / Lua | ✅ MON19 |
| Items / inventaire / équipement | ✅ avancé |
| Combat | ✅ MON12+ |
| Monstres / IA | ✅ ; bestiaire à densifier comme contenu |
| Progression RPG | ✅ MON15 |
| Status Effects | ✅ MON16 |
| Magic / Spellbook | ✅ MON18 |
| Recrutement / réserve | ✅ MON20 |
| Skills / Talents | ✅ MON20 |
| Save | ✅ **v9** |
| UI groupe / inventaire / Skills | ✅ |
| Quêtes / Journal / Map / Codex métier | ⬜ contrat MON21.1 défini, implémentation future |

## 6. Donjon, grille et éditeur

`UGridDungeonAsset -> UGridLevelAsset` reste la racine du contenu. Un LevelAsset porte cellules, objets, liens, variables typées et scripts Lua. Le Grid Editor offre peinture cellule/mur, placement, inspecteur, connecteurs, preview, mini-carte, validation et playtest PIE.

TD03 a retiré uniquement les actions Details historiquement redondantes lorsqu’un chemin Slate canonique était prouvé. Les fonctions restent disponibles selon leur API réelle et les actions debug avancées ont été conservées.

## 7. Event -> Command, Logic et Lua

```text
Event -> Command
Event -> Logic -> Event -> Command
Event -> Lua -> grid.command(...) -> Command
```

`TD-EVENT-001` est résolu. `TD-ARCH-005` surveille seulement la concentration interne de `UGridActivationComponent`.

## 8. Groupe, recrutement, Skills et Talents

Autorité :

```text
FGridPartyInventoryState
├── ActiveCharacters
├── ActiveEquipment
├── CharacterPool
└── FGridCharacterInventoryState
```

MON20 fournit recrutement actif/réserve, Story Companion, Custom Recruit, Skills, skill checks déterministes, Talents via `ProgressionChoices` et projection vers `RequirementIds`.

`TD-PARTY-001` est résolu : le changement de personnage sélectionné notifie le Pawn, qui resynchronise le held visual.

## 9. Combat, monstres et magie

Combat : initiative globale, PA/PAM, catalogue d’actions, ciblage grille, transactions de ressources, cooldowns, hotbar 0–9.

Monstres : occupation/pathfinding, perception automatique, dormance, patrouille, investigation, alarmes ; Rat géant mêlée et Gobelin lanceur à distance.

MON16 fournit Status Effects ; MON18 fournit Spellbook/cast. Le petit bestiaire est un manque de contenu, pas une dette d’architecture.

## 10. Persistance

`UGrimrockPartySaveGame v9` conserve notamment :

- groupe actif et réserve ;
- inventaire / équipement / hotbar ;
- progression de classe et Level Up ;
- Status Effects ;
- Spellbooks ;
- Skill states ;
- dungeon runtime state ;
- variables de niveau ;
- niveau courant ;
- position/facing du groupe ;
- état persistant des monstres ;
- permission runtime `bCanRemoveItem` des réceptacles.

Migrations récentes :

```text
v7 -> v8  SkillRanks MON20.9
v8 -> v9  Receptacle removal permission TD01.1
```

`TD-PERSIST-001` est résolu.

## 11. UI

Surfaces fonctionnelles : menu principal/Continue/Load, inventaire/paper doll, sélection du groupe, création/recrutement, Level Up, combat, Spellbook, Skills/Talents. Journal, Map et Codex existent comme surfaces mais leur métier reste à construire dans MON21.

## 12. Validation et packaging

TD04 a établi deux contrats locaux reproductibles et réellement validés :

```text
Scripts/ValidateUE.ps1
    -> GrimrockPrototypeEditor Win64 Development
    -> Automation explicite

Scripts/ValidatePackage.ps1
    -> GrimrockPrototype Win64 Shipping
    -> Build + Cook + Stage + Package + Pak + Archive
```

Dernière validation Shipping :

```text
Pak files     : 1
Archive files : 41
Archive bytes : 905582948
```

La CI distante UE reste différée tant qu’aucun vrai runner UE5.5.4 n’est provisionné.

## 13. Dette technique prioritaire

Le re-baseline TD05.1 montre :

```text
GridLevelRuntimeActor.cpp  3 359 lignes / 107 095 octets
GridLevelRuntimeActor.h    22 161 octets
```

Malgré `GridLevelRuntimeActorPersistence.cpp` et `GridLevelRuntimeActorWorldItems.cpp`, le RuntimeActor concentre encore de nombreuses responsabilités. `TD-ARCH-001` redevient donc la priorité P2 ciblée.

Prochaine frontière : diagnostics runtime/asset/PIE, à caractériser avant extraction.

Les autres dettes P2 restent surveillées et ne justifient pas de refactor transversal immédiat.

## 14. Roadmap

```text
MON13–MON20   systèmes gameplay majeurs                   CLOS
MON21.1       architecture Quests/Journal/Map/Codex       TERMINÉ
TD01–TD04     stabilisation/dette/outillage               RÉALISÉ
TD05          RuntimeActor ciblé                           ACTIF
MON21.2       reprise fonctionnelle                        APRÈS tranche TD05 utile
MON22         vertical slice 45–90 minutes                 À FAIRE
```

## 15. Cartographie

- statut courant : `docs/Architecture/TECHNICAL_DEBT_REGISTER.md` et présent document ;
- audit TD05 : `docs/Architecture/TECHNICAL_DEBT_DOCUMENTATION_AUDIT.md` ;
- carte détaillée `docs/Architecture/Maps/GRIMROCK_PROJECT_MAP.md` : snapshot daté du 23 août 2026 jusqu’à son prochain rafraîchissement global.
