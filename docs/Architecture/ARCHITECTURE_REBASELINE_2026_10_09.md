# DOC-ARCH01 — Rebaseline docs/Architecture et Maps

Date : **9 octobre 2026**  
État : **AUDIT / REBASELINE DOCUMENTAIRE TERMINÉ**  
Baseline C++ auditée : **6f98a0ef5599f37d3d544529aa5a790d647e8251**  
Parent documentaire : **DOC-AUDIT02**

## 1. Périmètre

Inventaire avant DOC-ARCH01 :

```text
docs/Architecture              62 fichiers Markdown
docs/Architecture/Maps          3 fichiers Markdown
docs/Design                   522 fichiers Markdown
Markdown repository total     620
Source files (.h/.cpp/.cs)    850
Runtime test files (.cpp)     275
Editor test files (.cpp)       69
Lua test files (.cpp)           4
```

Les 62 documents Architecture existants ont tous été classés et mis à jour :

```text
Références courantes / revalidées : 27
Snapshots historiques marqués       : 35
Total initial                       : 62
```

Le présent rapport devient le **63e** fichier Architecture ; le repository passe donc à **621** fichiers Markdown.

## 2. Règle documentaire

Trois catégories sont désormais explicites :

1. **Référence courante** : doit refléter le C++ actuel.
2. **Fondation revalidée** : contrat ancien mais toujours conforme au C++ courant.
3. **Historique / Snapshot** : migration, audit ou cleanup conservé pour expliquer l'évolution, sans autorité sur l'API actuelle.

Les documents historiques n'ont pas été réécrits : un bandeau DOC-ARCH01 indique leur statut et renvoie vers les références courantes.

## 3. Ground truth utilisé

### Save

```text
UGrimrockPartySaveGame::CurrentSaveVersion = 24
exact-match
LastAcknowledgedLevel = supprimé
Quest runtime state   = non persisté
```

### RPG

```text
Skills  : SkillRanks -> FRPGSkillService -> FRPGSkillPointService
Talents : ProgressionChoices -> FRPGClassProgressionService
          -> FRPGClassProgressionTransactionService
          -> FGridSkillsPageService -> FGridTalentTreeView
```

Supprimés :

```text
FRPGSkillRuntimeService
FRPGTalentRuntimeService
FGridTalentEntryView
FGridSkillsPageView::Talents
GetTalentEntry*
CollectAutomaticSatisfiedRequirements
```

### UI

```text
WBP_GridPersistentHud = navigation + action bar persistante
WBP_GridCombatHud     = combat-only
WBP_GridSkills        = surface autonome
WBP_GridMap           = surface autonome
WBP_GrimrockMenu      = Journal / Recipes / Codex / Spellbook
```

### Map

```text
MON21.6       VALIDÉ / CLOS
MAP-THEME01   VALIDÉ / CLOS
MapExploration -> FGridMapReadModelBuilder -> FGridMapFloorView
               -> UGridMapWidget / UGridMapSurfaceWidget
               -> UGridMapVisualThemeAsset -> NativePaint
```

### World Objects / Palette

```text
UGridWorldObjectDefinitionAsset
FGridWorldObjectInstance
placements typés
MovingParts 0..N
FGridObjectPaletteEntry::PaletteCategory
DisplayNameOverride = absent
```

## 4. Corrections structurantes

### PROJECT_SYNTHESIS

Réécrit sur l'état du 9 octobre : Save v24, RPG courant, Map close, Quest non persisté, UI autonome, dernière globale réelle distinguée du HEAD documentaire.

### ARCHITECTURE_INDEX

Transformé en index d'autorité : ordre de lecture, liste des fondations courantes, famille des snapshots historiques et invariants transversaux.

### PARTY_RPG_RECRUITMENT_FOUNDATION

L'ancien état MON20 intermédiaire disait encore que Skills n'avait pas d'économie joueur et utilisait un ancien Save. Il est remplacé par le contrat actuel Skills/Talents/Attributes/Level-Up.

### UI_GAME_FLOW_FOUNDATION

L'ancien menu monolithique est remplacé par le modèle viewport actuel : CharacterSheet, InventoryBag, Skills et Map autonomes ; Persistent HUD global ; Combat HUD combat-only.

### SAVE_PERSISTENCE_FOUNDATION

Réécrit autour de v24 exact-match et des autorités directes dans Character State. Les anciennes étapes v19/v20/v22/v23 restent dans les tickets historiques.

### Combat / Magic

Suppression des mentions Save v9 et des snapshots Spellbook/Status parallèles. Les données courantes vivent dans Character State / DungeonRuntimeState.

### Mouse / Receptacle / Readable / Items

Alignement sur le routage MI1–MI6, les placements typés, les permissions runtime persistées du Receptacle et les champs Readable actuels.

### Technical Debt Register

Le registre est rebaseliné au 9 octobre et ne mélange plus le statut courant avec plusieurs appendices TD07 historiques. Les campagnes closes restent dans leurs docs datés.

## 5. Maps — traitement renforcé

Les trois fichiers de `docs/Architecture/Maps` ont été **entièrement reconstruits** :

```text
GRIMROCK_PROJECT_MAP.md
GRIMROCK_PROJECT_MAP_MERMAID.md
Grimrock_MindMap_Architecture_Cible_v2_XMind.md
```

Corrections principales :

- suppression des anciens compteurs 419/511 et 964 ;
- corpus courant 620 Markdown avant DOC-ARCH01, 621 après son rapport ;
- Save v24 au lieu de v23 ;
- Map présentée comme close et fonctionnelle, pas comme shell ;
- Persistent HUD propriétaire navigation/action bar ;
- Combat HUD combat-only ;
- action bar >= 12 slots ;
- Skills/Skill Points et Talents actuels ;
- 6 classes / 18 branches / 90 Talents / 4 familles à variantes ;
- Quest runtime présent mais persistence encore ouverte ;
- suppression de `DisplayNameOverride` de la palette ;
- `MovingParts` 0..N ;
- suppression des anciennes façades RPG de la carte d'architecture ;
- distinction entre baseline C++ auditée et dernière campagne globale/Shipping.

## 6. Fondations revalidées

Les documents suivants conservent leur corps de référence et reçoivent un bandeau de revalidation DOC-ARCH01 :

```text
ADVANCED_DUNGEON_LOGIC_FOUNDATION
CORE_DUNGEON_LEVEL_GRID
DOOR_MECHANISM_FOUNDATION
ITEM_LIGHT01_DATA_DRIVEN_ITEM_LIGHT
LEVEL_VALIDATION_PANEL_FOUNDATION
LIGHT_CONFIG02_SINGLE_POINT_LIGHT_AUTHORITY
MATERIAL_OWNERSHIP
PARTY_LIGHT01_EQUIPMENT_DRIVEN_PARTY_ILLUMINATION
STARTUP_FLOW01_FRONTEND_DUNGEON
TEST_AUTOMATION_FOUNDATION
WORLD_OBJECT_DEFINITIONS_AND_PLACED_OBJECTS
```

`CORE_DUNGEON_LEVEL_GRID` a aussi été corrigé pour `MovingParts[0..N-1]` au lieu de l'ancienne limite à deux parties.

## 7. Snapshots historiques

35 documents ALIGN / cleanup / TD audit / WORLDOBJ-MIG / recovery ont été conservés intégralement et marqués `DOC-ARCH01 — HISTORIQUE / SNAPSHOT`.

Ils restent utiles pour retracer les migrations, mais ne doivent plus être utilisés comme schéma actuel.

## 8. Validation

DOC-ARCH01 est **strictement documentation-only** :

```text
C++       : inchangé
Blueprint : inchangé
DataAsset : inchangé
Map uasset: inchangée
```

Aucune nouvelle exécution UE n'est requise ni revendiquée.

Dernière globale connue :

```text
9045ef2d
Grimrock 1026/1026
0 warning / 0 failed / 0 not run
Win64 Shipping validé
```

Les validations RPG/UI postérieures restent des validations ciblées.

## 9. Autorité après DOC-ARCH01

Ordre recommandé :

1. `docs/Design/00_PROJECT_OVERVIEW.md`
2. `docs/Design/PROJECT_COMPLETION_ROADMAP.md`
3. `docs/Architecture/PROJECT_SYNTHESIS.md`
4. `docs/Architecture/ARCHITECTURE_INDEX.md`
5. `docs/Architecture/Maps/GRIMROCK_PROJECT_MAP.md`
6. `docs/Architecture/Maps/GRIMROCK_PROJECT_MAP_MERMAID.md`
7. fondations Architecture courantes
8. snapshots historiques.

**DOC-ARCH01 : rebaseline Architecture terminée.**


## 10. Seconde passe de cohérence

Une seconde lecture après publication de DOC-ARCH01 a corrigé trois défauts
strictement documentaires :

- les compteurs Source/tests du snapshot ont été recalculés depuis l'arbre Git ;
- les backticks Markdown échappés accidentellement dans plusieurs fondations et
  cartes ont été restaurés afin que les blocs/code inline se rendent
  correctement ;
- la documentation World Object ne prétend plus qu'un Core Redirect
  `GridObjectArchetypeAsset` est actif dans `DefaultEngine.ini` : aucun n'est
  présent dans la configuration courante.

La seconde passe a également approfondi les trois documents
`docs/Architecture/Maps` à partir des classes réelles
`FGridMapRevealService`, `FGridMapReadModelBuilder`,
`UGridMapWidget`, `UGridMapSurfaceWidget` et
`UGridMapVisualThemeAsset`.

Ces corrections restent **documentation-only**.
