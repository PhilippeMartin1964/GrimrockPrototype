# GrimrockPrototype — Carte détaillée du projet

> **Carte textuelle autoritaire — DOC-ARCH01, 9 octobre 2026.**
>
> Baseline C++ auditée : `6f98a0ef5599f37d3d544529aa5a790d647e8251`
> (`UI-RPG-CODE-AUDIT01`). DOC-AUDIT02 et DOC-ARCH01 sont documentaires.
>
> Dernière campagne globale + Shipping : runtime/content `9045ef2d`,
> **1026/1026**, 0 warning, 0 échec, 0 non exécuté ; Shipping Win64 validé.

## Légende

- ✅ implémenté et validé dans le périmètre indiqué ;
- 🟢 autorité/architecture stable ;
- 🟡 implémenté mais validation manuelle/UMG spécifique encore ouverte ;
- 🟠 fondation/shell existant, métier futur ;
- ⬜ futur ;
- ⚠️ limitation ou dette surveillée ;
- 🎯 prochaine étape pertinente.

---

# 00 — Snapshot documentaire

## 00.1 — Corpus au début de DOC-ARCH01

```text
Markdown repository total : 620
docs/Design              : 522
docs/Architecture        : 62
docs/Architecture/Maps   : 3
docs/*.md                : 17

Source files (.h/.cpp/.cs): 850
Runtime test files (.cpp): 275
Editor test files (.cpp) : 69
Lua test files (.cpp)    : 4
```

DOC-ARCH01 ajoute un rapport Architecture ; le total Architecture passe donc à
**63** et le total Markdown à **621** après publication.

## 00.2 — Hiérarchie documentaire

1. code courant ;
2. résultats UE/PIE réellement fournis ;
3. clôtures les plus récentes ;
4. `docs/Design/00_PROJECT_OVERVIEW.md` ;
5. `docs/Design/PROJECT_COMPLETION_ROADMAP.md` ;
6. `docs/Architecture/PROJECT_SYNTHESIS.md` ;
7. présente carte + Mermaid ;
8. `ARCHITECTURE_INDEX.md` ;
9. fondations courantes ;
10. tickets MIG/cleanup/audit historiques.

---

# 01 — Vision et invariants

- ✅ UE 5.5.4, C++, Blueprint/UMG, Lua.
- ✅ Dungeon crawler grille, vue subjective.
- ✅ grille cible 32×32.
- ✅ cellule de référence 200×200×300 cm.
- ✅ North=Y+, East=X+, South=Y−, West=X−.
- 🟢 une autorité par donnée.
- 🟢 Definition ≠ Instance ≠ Runtime State ≠ Save.
- 🟢 C++ décide ; Blueprint/UMG présente.
- 🟢 Lua orchestre les puzzles spécifiques mais revient vers Event→Command.
- 🟢 pas de compatibilité arrière Save/DataAsset exigée pendant le prototype.
- 🎯 long terme : éditeur joueur réutilisant le même modèle.

---

# 02 — Modules

```text
GrimrockLua
    ↓
GrimrockPrototype
    ↓
GrimrockPrototypeEditor
```

## GrimrockLua

- Lua 5.4 embarqué ;
- VM sandboxée ;
- callbacks ;
- persistent variables ;
- tests Lua.

## GrimrockPrototype

- Core data model ;
- Runtime ;
- RPG ;
- Combat ;
- Magic ;
- Quests ;
- Save ;
- UI.

## GrimrockPrototypeEditor

- EdMode / Toolkit ;
- Slate ;
- palette ;
- inspecteurs ;
- validation ;
- preview ;
- Lua authoring ;
- tests Editor.

🟢 Runtime ne dépend jamais du module Editor.

---

# 03 — Dungeon / LevelAsset

```text
UGridDungeonAsset
└── Levels[] : FGridDungeonLevelEntry
    └── UGridLevelAsset
        ├── Cells[]
        ├── WorldObjectInstances[]
        ├── LooseItemInstances[]
        ├── MonsterSpawns[]
        ├── ItemSpawns[]
        ├── LogicObjects[]
        ├── Links[]
        ├── QuestDefinitions[]
        ├── LevelVariables[]
        └── LuaScripts[]
```

- ✅ plusieurs LevelAssets dans un DungeonAsset ;
- ✅ LogicalPosition organise les dalles/niveaux ;
- ✅ un LevelAsset reste l'asset unique d'authoring d'une dalle ;
- ✅ placements typés, pas de collection monolithique legacy.

---

# 04 — Grille et espace

## Cellule

```text
X / Y
CellType
North/East/South/West walls
Ceiling
Occupancy flags
```

## Placement

```text
Surface : Floor / Wall / Ceiling
Facing  : cardinal
Local   : U / V / N
```

## Boundary

Une frontière topologique est partagée par :

- walls ;
- doors ;
- secret doors ;
- passage ;
- collision logique ;
- LOS/acoustique ;
- projectiles ;
- pathfinding.

🟢 Les transforms Unreal sont dérivées depuis la grille et l'authoring local.

---

# 05 — World Object Definition / Instance

## 05.1 Definition

`UGridWorldObjectDefinitionAsset` :

```text
DefinitionId
DisplayName
Description
SupportedType
MapSymbolStyle
DefaultBehavior
AudioEvents
PlacementSurface / DefaultLocalPosition
Spatial behavior
Interaction / Readable
Light
StaticPart
MovingParts[]
RuntimeMaterialAliases
RuntimeActorClass
```

`SupportedType` est la classification gameplay principale.

## 05.2 Instance

`FGridWorldObjectInstance` :

```text
InstanceId
WorldObjectDefinitionId
Type
CellX / CellY / WallSide
LocalTransformOverride
LogicId
Notes
PaletteEntryId
ReadableTextOverride
InstanceConfig
```

`InstanceConfig` porte les états/différences locales pertinentes :

- Door initial state ;
- Relocation enabled + destination ;
- Pit ;
- Receptacle initial content ;
- InteractionOverrides ;
- MovingPartOverrides ;
- Door chain overrides ;
- Lock start state.

## 05.3 Palette

`FGridObjectPaletteEntry` porte :

```text
EntryId
PaletteCategory
Icon
DefaultWorldObjectDefinition
DefaultItemDefinition
DefaultMonsterDefinition
DefaultStoryCompanionDefinition
```

⚠️ **Il n'existe plus de `DisplayNameOverride`.**

Le nom visible vient directement de la définition référencée.
`PaletteCategory` est l'unique groupement Editor.

## 05.4 Moving parts

- ✅ tableau 0..N ;
- ✅ Door anime N parties ;
- ✅ mécanismes utilisent leurs index pertinents ;
- ✅ pas de Part0/Part1 parallèle.

---

# 06 — Grid Editor

```text
FGridLevelEdMode
    -> FGridLevelEdModeToolkit
        -> AGridLevelEditorActor
            -> LevelAsset
```

## Workspace

- Dungeon Levels ;
- Overview Map ;
- Palette ;
- Selected Object ;
- Connectors / Links ;
- Validation ;
- Playtest.

## Tools

- Select ;
- Paint Cell ;
- Paint Wall ;
- Paint Object ;
- Erase one-shot ;
- Link.

## Authoring

- geometry ;
- party start ;
- world objects ;
- items ;
- monsters/spawns ;
- logic ;
- links ;
- variables ;
- Lua ;
- Quest refs ;
- patrol ;
- relocations ;
- pits ;
- sparse instance overrides.

🟢 Editor écrit les DataAssets ; preview/runtime ne deviennent pas une autorité parallèle.

---

# 07 — Runtime niveau

## AGridLevelRuntimeActor

Façade du niveau vivant :

- build/rebuild ;
- geometry ;
- world objects ;
- items ;
- monsters ;
- state restore/capture ;
- transitions ;
- diagnostics ;
- feedback.

TD05 a atteint sa stop condition : conserver la façade, extraire seulement une
nouvelle frontière si une douleur réelle apparaît.

## AGrimrockPartyPawn

- déplacement case par case ;
- strafe ;
- rotations ;
- interpolation/buffering ;
- free look/head bob ;
- pits ;
- transitions ;
- held item ;
- UI orchestration.

---

# 08 — Interaction souris

`AGrimrockPlayerController` :

```text
ResolveLeftMouseInteraction()
ResolveCursorItemHoverCursor()
SetGridInteractionCursor()
```

Priorité :

```text
Readable/UI
    -> Cursor item:
       WallLock / Receptacle / WorldDrop / Throw
    -> World Interactable
    -> no-op
```

- ✅ hover sans mutation ;
- ✅ UI modale bloque le monde ;
- ✅ refus de lock/receptacle ne devient pas un dépôt involontaire ;
- ✅ `IGridInteractableInterface` conserve le contrat local de l'acteur.

---

# 09 — Portes / mécanismes / réceptacles / locks

## Door

- standard + secret ;
- Open/Close/Toggle ;
- moving parts ;
- passabilité seulement à l'état terminal approprié ;
- audio générique via AudioEvents ;
- chaînes optionnelles.

## Mechanisms

- Button ;
- Secret Button ;
- Lever ;
- Pressure Plate ;
- Trigger Enter/Exit.

## Receptacle

Definition :

```text
bAcceptAnyItem
AcceptedItems
MaxContainedItems
VisualPlacementMode
physical placement params
```

Runtime :

```text
ContainedItems
bCanRemoveItem
bCanInsertItems
```

Save :

```text
FGridRuntimeReceptacleState
    ObjectId
    bCanRemoveItem
    bCanInsertItems
    ContainedItems
```

## WallLock

- `bStartsUnlocked` ;
- accepted-key configuration ;
- insertion/consume selon comportement ;
- interaction item au curseur.

---

# 10 — Items / Inventory / Equipment

## Item definition

`UGridItemDefinitionAsset` :

- identity ;
- display/icon ;
- type ;
- weight/stack ;
- world mesh ;
- equipment ;
- stats/resistances ;
- combat actions ;
- light ;
- sparkle ;
- readable data.

## Party authority

```text
UGridPartyInventoryComponent
└── FGridPartyInventoryState
    ├── ActiveCharacters
    ├── ActiveEquipment
    ├── CharacterPool
    ├── SelectedCharacterIndex
    └── CursorItem
```

Chaque personnage porte ses `CombatHotbarSlots`.

## Transfers

`UGridItemTransferService` assure atomicité/rollback entre :

- inventory ;
- equipment ;
- cursor ;
- world ;
- receptacle ;
- autre personnage.

## World item

`AGridItemActor` = représentation générique.
Physics, sparkle, lumière et readable sont data-driven.

---

# 11 — Event → Command / Logic / Lua

```text
Object Event
    -> UGridActivationComponent
        -> FGridObjectLink
            -> Command
            -> Logic
            -> Lua
            -> Quest
```

## Conditions directes

Les conditions d'un Link restent centrées sur les Receptacles :

- empty/has item ;
- definition/tag/type ;
- count ;
- weight ;
- invert.

## Logic

Bool/Int, set/toggle/add/subtract/reset/compare/latch.

## Lua

```text
LevelAsset LuaScripts
    -> FGridLuaVm
    -> persistent / grid.vars
    -> grid.command(...)
    -> dispatcher
```

🟢 Lua n'est pas un second bus.

---

# 12 — Quest / Journal / Codex

## Quest

```text
UGridQuestDefinitionAsset
    -> UGridQuestSubsystem
        -> FGridCampaignQuestRuntimeState (Transient)
```

Event→Command :

- QuestStart ;
- QuestCompleteObjective ;
- QuestComplete ;
- QuestFail.

Status :

```text
MON21.2 Runtime       VALIDÉ
MON21.3 Integration   VALIDÉ
MON21.4 Persistence   EN ATTENTE
```

## Journal

🟠 surface présente, read model métier MON21.5 à faire.

## Codex

🟠 surface présente, discovery/projection MON21.7 à faire.

---

# 13 — Map — VALIDÉE / CLOSE

MON21.6 et MAP-THEME01 sont clos. La carte est une **projection filtrée** du
donjon et de son exploration ; elle ne possède aucune topologie, exploration ou
état de porte autoritaire propre.

## 13.1 — Autorités

```text
UGridDungeonAsset
    -> FGridDungeonLevelEntry::LogicalPosition

UGridLevelAsset
    -> Cells / walls / WorldObjectInstances

FGridLevelRuntimeState::MapExploration
    -> ExploredCells
    -> DiscoveredSecretObjectIds

FGridMapReadModelBuilder
    -> FGridMapTileView
    -> FGridMapFloorView (transient)

UGridMapWidget
    -> navigation / zoom / pan / recenter
UGridMapSurfaceWidget
    -> NativePaint uniquement
UGridMapVisualThemeAsset
    -> présentation uniquement
```

`FGridMapFloorView` et les données de rendu ne sont jamais persistés.

## 13.2 — Exploration et reveal

`FGridMapExplorationState` est fixé au contrat de dalle **32×32 / 1024
cellules**. `ExploredCells` est alloué paresseusement et
`DiscoveredSecretObjectIds` conserve les secrets réellement découverts.

`FGridMapRevealService` porte le fog-of-war topologique :

```text
RevealRadiusCells = 1.25

cellule courante
    + voisins cardinaux visibles
    - diagonales

mur structurel sur l'un ou l'autre côté
    -> bloque le reveal

porte bloquante / fermée
    -> bloque le reveal

occupation/pathfinding
    != autorité de visibilité
```

`AGridLevelRuntimeActor::RevealMapAroundCell()` écrit directement dans le
`MapExploration` du niveau courant.

## 13.3 — Portes secrètes

La découverte d'une porte secrète est distincte de l'exploration de sa cellule.

- une porte secrète authored initialement ouverte est découverte à
  l'enregistrement runtime ;
- une porte secrète fermée est découverte lorsque son animation atteint
  réellement l'état **fully open** ;
- `TryDiscoverMapSecretDoor()` ne marque qu'une vraie porte secrète ;
- une découverte reste dans `DiscoveredSecretObjectIds`.

Projection fail-closed :

```text
secret non découvert
    -> Wall

secret découvert
    -> SecretDoor

métadonnée Door non résolue
    -> Wall
```

Ainsi le read-model ne révèle jamais l'existence d'un secret avant sa
découverte.

## 13.4 — Read-model filtré d'une dalle

`FGridMapReadModelBuilder::BuildTileView()` :

- accepte uniquement une dalle 32×32 avec exploration structurellement valide ;
- expose seulement les cellules **explorées et non Empty** ;
- projette murs, portes et secrets sans exposer d'identité secrète à l'UI ;
- résout une porte depuis sa `UGridWorldObjectDefinitionAsset` et sa
  `RuntimeActorClass` ;
- résout l'état ouvert dans cet ordre :

```text
1. UGridDoorSystemComponent vivant, pour la dalle active
2. FGridRuntimeDoorState persisté
3. InstanceConfig.bDoorInitiallyOpen authored
```

Les symboles de cellule (`StairsUp`, `StairsDown`, `Pit`) et les opt-ins
`UGridWorldObjectDefinitionAsset::MapSymbolStyle` sont également filtrés par
l'exploration.

`MapSymbolStyle` reste strictement une métadonnée de présentation :

```text
None
StairsUp
StairsDown
Relocation
Pit
PointOfInterest
```

Elle ne change ni `SupportedType`, ni le gameplay, ni la persistance.

## 13.5 — Composition multi-dalles / multi-étages

`BuildFloorView()` sélectionne toutes les entrées activées du
`UGridDungeonAsset` qui partagent le même `LogicalPosition.Z`.

Coordonnée globale :

```text
GlobalX = LogicalPosition.X * 32 + LocalX
GlobalY = LogicalPosition.Y * 32 + LocalY
```

Les frontières communes entre dalles sont dédupliquées. Une contradiction
authored sur une couture échoue de façon conservative : `Wall`, ou porte
considérée fermée.

`AvailableFloorZs` est trié depuis les niveaux activés ; la navigation ne
suppose donc pas des étages contigus `Z±1`.

Le marqueur du groupe n'est projeté que sur l'étage réellement occupé par le
groupe.

## 13.6 — Widget et rendu

`UGridMapWidget` est le contrôleur de présentation :

- `RefreshMap()` ;
- `SelectPartyFloor()` ;
- `NavigateFloorUp/Down()` ;
- `CanNavigateFloorUp/Down()` ;
- `AdjustZoom()` ;
- `PanMapByPixels()` ;
- `RecenterMap()`.

Il détient un `FGridMapFloorView` **transient** et délègue le dessin à
`UGridMapSurfaceWidget`.

`UGridMapSurfaceWidget` ne possède aucun état de carte : son
`NativePaint()` lit la projection de son parent.

`UGridMapVisualThemeAsset` est presentation-only : parchemin, sol, murs,
piliers, portes, symboles, marqueur du groupe et paramètres de layout. Une
texture manquante peut utiliser le fallback procédural sans créer une seconde
autorité de carte.

Il n'existe ni `MapActor` permanent, ni grille UMG de 1024 widgets, ni Tick de
gameplay propre à la Map.

## 13.7 — Persistance

```text
FGridLevelRuntimeState
    -> MapExploration
        -> ExploredCells              [SaveGame]
        -> DiscoveredSecretObjectIds  [SaveGame]

FGridDungeonRuntimeState
    -> UGrimrockPartySaveGame v24
```

Le read-model (`TileView`, `FloorView`) et l'état caméra UI
(`SelectedFloorZ`, zoom, pan) restent reconstructibles/transients.

## 13.8 — Orientation écran

Convention gameplay canonique inchangée :

```text
North = Y+
East  = X+
South = Y-
West  = X-
```

Convention de rendu Map/Overview :

```text
North / Y+ -> écran haut
East  / X+ -> écran gauche
```

Le miroir X est **uniquement une convention de présentation** partagée avec
l'Overview Map de l'éditeur. Les coordonnées du read-model et les directions
runtime ne sont jamais inversées.

Références C++ :

- `Runtime/Map/GridMapExplorationState.h`
- `Runtime/Map/GridMapRevealService.h`
- `Runtime/Map/GridMapReadModel.h/.cpp`
- `UI/GridMapWidget.h/.cpp`
- `UI/GridMapSurfaceWidget.h/.cpp`
- `UI/GridMapVisualThemeAsset.h`

# 14 — Groupe / RPG

## Groupe

- MaxActiveCharacters = 6 ;
- CharacterPool ;
- SelectedCharacterIndex unique ;
- recrutement Story + Custom.

## Character State

Durable :

```text
Experience
SelectedClassProgressionChoiceIds
Attributes
Resources
SkillRanks
KnownSpellIds
StatusEffects
InventorySlots
CombatHotbarSlots
identity
```

Transient :

```text
Level
DerivedStats
definition/display caches
```

## Skills

```text
SkillRanks
 -> FRPGSkillService
 -> FRPGSkillPointService
 -> FGridSkillsPageService
```

- ✅ économie Skill Points dérivée ;
- ✅ rank caps ;
- ✅ Safe Undo session-only ;
- ✅ aucun compteur persistent.

## Talents

```text
ProgressionChoices
 -> FRPGClassProgressionService
 -> FRPGClassProgressionTransactionService
 -> FGridSkillsPageService
 -> FGridTalentTreeView
```

- ✅ 6 classes ;
- ✅ 18 branches ;
- ✅ 90 talents conceptuels ;
- ✅ 86 simples ;
- ✅ 4 familles à variantes ;
- ✅ fiche structurée NOM/TYPE/STATUT/PRINCIPE/EFFETS/[UTILISATION]/[VARIANTES]/ACQUISITION.

Supprimés :

```text
FRPGTalentRuntimeService
FRPGSkillRuntimeService
FGridTalentEntryView
FGridSkillsPageView::Talents
CollectAutomaticSatisfiedRequirements
```

## Attributes

`FRPGAttributePointService` dérive le budget ; aucun compteur persistent.

## Level Up

`URPGLevelUpNotificationSubsystem` = toast queue transient.
Pas de `URPGLevelUpWidget`, pas de `LastAcknowledgedLevel`.

---

# 15 — Magic / Status Effects

## Spellbook

`KnownSpellIds` vit directement dans le Character State durable.

## Cast

- catalog/action ;
- PA/mana ;
- targeting ;
- effect resolver ;
- presentation.

## Status

`Character.StatusEffects` est durable.
Les caches de définition sont transient/reconstructed.

🟢 aucun snapshot Spellbook/Status parallèle.

---

# 16 — Combat

`UGridTurnManagerComponent` :

- rounds ;
- initiative globale ;
- tours individuels ;
- PA ;
- PAM ;
- action catalog ;
- targeting ;
- costs ;
- resolution ;
- combat log.

Sources d'action :

- universal ;
- equipment ;
- class/talent ;
- quick item ;
- spell.

`UGridCombatHudWidget` est une projection combat-only.

---

# 17 — Monsters / AI

`UGridMonsterDefinitionAsset -> AGridMonsterActor`

Composants :

- Movement ;
- Behavior ;
- Combat ;
- Death ;
- Audio ;
- VFX ;
- IdleVariation.

Systèmes :

- Occupancy ;
- grid pathfinding ;
- perception LOS/hearing ;
- patrol ;
- investigation ;
- alarm ;
- engagement ;
- encounter groups ;
- persistence.

---

# 18 — UI viewport actuel

```text
Viewport
├── WBP_CharacterSheet
├── WBP_InventoryBag
├── WBP_GridSkills
├── WBP_GridMap
├── WBP_GridCombatHud
└── WBP_GridPersistentHud
```

## Persistent HUD

- global navigation ;
- persistent action bar ;
- progression toast ;
- minimum 12 action slots.

## Combat HUD

- party combat panels ;
- initiative ;
- PAM ;
- end turn ;
- rejection feedback ;
- targeting.

⚠️ UI-COMBAT-UNIFY02 : contrat C++ courant ; validation finale UMG/PIE de la
hiérarchie authored toujours non revendiquée.

## GrimrockMenu shell

Reste temporairement pour :

- Journal ;
- Recipes ;
- Codex ;
- Spellbook.

---

# 19 — Startup / maps Unreal

```text
L_MainMenu
  -> New Game
     -> Character Creation
     -> Dungeon Build Progress
     -> L_Dungeon

  -> Continue / Load
     -> L_Dungeon
     -> restore
```

`L_Dungeon` est le host world canonique runtime/PIE.
Le contenu réel reste data-driven dans DungeonAsset/LevelAssets.

---

# 20 — Save

```text
UGrimrockPartySaveGame v24 exact-match
├── PartyInventoryState
├── DungeonRuntimeState
├── CurrentDungeonLevelId
├── PartyCellX/Y
└── PartyFacing
```

- ✅ Map exploration persistée ;
- ✅ Skills/Talents/Spellbook/Status directement dans Character state ;
- ✅ no backward migration ;
- ⬜ Quest state pas encore persisté.

---

# 21 — Audio / VFX / Light

## Object audio

`UGridWorldObjectDefinitionAsset::AudioEvents` + atténuation unique.
Pas de second schéma Door audio.

## Materials

StaticMesh Material Slots = autorité du rendu principal.

## Item light

`UGridItemDefinitionAsset::LightEmitter` = authoring.
`UGridPartyIlluminationComponent` = proxy ergonomique du groupe.

## VFX

Presentation-only : projectiles, attacks, spells, monster death/dissolve,
sparkle.

---

# 22 — Validation

```text
Scripts/ValidateUE.ps1
  -> Development Editor
  -> AutomationFilter
  -> reports Saved/Automation/TD04

Scripts/ValidatePackage.ps1
  -> Shipping
  -> Build/Cook/Stage/Package/Pak/Archive
```

Dernière globale :

```text
9045ef2d
1026/1026
0 warning
0 failed
Shipping OK
```

Postérieurement : validations ciblées RPG/UI, pas de nouvelle globale annoncée.

---

# 23 — Dette technique

Stop conditions historiques atteintes :

- TD05 RuntimeActor ;
- TD06 PartyInventory ;
- TD07 current-schema.

Surveillées notamment :

- toolchain MSVC warning non bloquant ;
- PlayerController volume ;
- ActivationComponent concentration ;
- Grid Editor/Slate ;
- log hygiene opportuniste ;
- remote UE CI.

🟢 Pas de refactor massif sans signal concret.

---

# 24 — Roadmap courante

```text
DONE
  MON13–MON20
  MON21.1–21.3
  MON21.6
  MAP-THEME01
  RPG-SKILL01
  RPG-LEVELUX01
  RPG-ATTR01
  UI-RPG-DESC01
  UI-RPG-CODE-AUDIT01

OPEN
  MON21.4 Quest Persistence
  MON21.5 Journal
  MON21.7 Codex
  MON21.8 Cross-System Closure
  MON22 Vertical Slice
  Crafting/Recipes
  Player Level Editor
```

---

# 25 — Invariants de non-régression

1. Une autorité par donnée.
2. Placement typé, pas de retour au modèle monolithique.
3. Definition ≠ Instance.
4. Grille autoritaire.
5. Event→Command central.
6. Lua/Logic reviennent vers le dispatcher.
7. PartyInventory autoritaire.
8. TurnManager autoritaire.
9. QuestSubsystem autoritaire mais non persisté.
10. Save v24 exact-match.
11. PersistentHud = navigation/action bar.
12. CombatHud = combat-only.
13. Map = projection.
14. C++ = règles ; UMG = présentation.
15. Aucun ancien service RPG supprimé ne doit redevenir une autorité.
