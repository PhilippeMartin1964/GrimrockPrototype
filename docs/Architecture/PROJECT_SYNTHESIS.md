# GrimrockPrototype — Synthèse globale du projet

> **Référence courante — DOC-ARCH01, 9 octobre 2026.**
>
> Baseline C++ auditée : \`6f98a0ef5599f37d3d544529aa5a790d647e8251\`
> (\`UI-RPG-CODE-AUDIT01\`). Les commits DOC-AUDIT02 / DOC-ARCH01 sont
> documentaires et ne modifient pas cette baseline C++.
>
> Dernière campagne **globale + Shipping** fournie par l'utilisateur :
> baseline runtime/content \`9045ef2db75c09997db4fc65dbf99d4598f4df5c\`,
> **1026/1026**, 0 warning, 0 échec, 0 non exécuté ; Win64 Shipping
> Build/Cook/Stage/Package/Pak/Archive validé, cook 0 error / 0 warning.

## 1. Vue d'ensemble

GrimrockPrototype est un dungeon crawler UE 5.5.4 en C++ inspiré de
*Legend of Grimrock 2* : exploration case par case, Grid Editor, mécanismes
data-driven, Event → Command, Logic/Lua, inventaire/équipement, combat tactique,
IA, progression RPG, magie, Map persistante et fondation Quest.

Architecture de modules :

\`\`\`text
GrimrockLua
    ↓
GrimrockPrototype
    ↓
GrimrockPrototypeEditor
\`\`\`

Le Runtime ne dépend jamais du module Editor. Le module Editor consomme le
modèle Core/Runtime au lieu de créer une seconde autorité.

## 2. Principes structurants

1. **Une seule autorité par donnée.**
2. **Définition ≠ instance ≠ état runtime ≠ état sauvegardé.**
3. La grille est autoritaire pour déplacement, occupation, LOS, ciblage et
   frontières.
4. \`UGridLevelAsset\` est l'autorité authored d'un niveau.
5. \`AGridLevelRuntimeActor\` reconstruit la représentation vivante.
6. Event → Command reste le bus gameplay ; Logic et Lua y reviennent.
7. C++ porte règles, transactions, validations et read models.
8. Blueprint/UMG porte composition, assets, style et présentation.
9. Pendant le prototype, le Save utilise **exact-match** et ne maintient pas de
   migration arrière.
10. Git conserve l'historique ; les documents de migration ne sont pas des
    contrats courants.

## 3. Donjon et placements

\`\`\`text
UGridDungeonAsset
    -> FGridDungeonLevelEntry
        -> UGridLevelAsset
            -> Cells
            -> WorldObjectInstances
            -> LooseItemInstances
            -> MonsterSpawns
            -> ItemSpawns
            -> LogicObjects
            -> Links
            -> QuestDefinitions
            -> LevelVariables
            -> LuaScripts
\`\`\`

Les placements sont typés. L'ancien stockage monolithique n'est plus une
autorité active.

### World Objects

\`UGridWorldObjectDefinitionAsset\` porte notamment :

- \`DefinitionId\`, \`DisplayName\`, \`SupportedType\` ;
- règles de placement et comportement spatial ;
- interaction / readable / lumière ;
- \`StaticPart\` et \`MovingParts[]\` ;
- \`AudioEvents\` ;
- \`MapSymbolStyle\` de présentation ;
- \`RuntimeActorClass\`.

\`FGridWorldObjectInstance\` porte l'identité et les différences locales :

- \`InstanceId\` ;
- \`WorldObjectDefinitionId\` ;
- cellule / surface / facing ;
- \`LogicId\` ;
- \`ReadableTextOverride\` ;
- \`FGridWorldObjectInstanceConfig\`.

La palette ne possède plus de \`DisplayNameOverride\`. Son libellé effectif
vient directement de la définition Item ou World Object référencée.
\`PaletteCategory\` reste l'autorité de groupement Editor.

## 4. Grid Editor

Le Grid Editor repose sur :

\`\`\`text
FGridLevelEdMode
FGridLevelEdModeToolkit
AGridLevelEditorActor
Slate panels / authoring services
\`\`\`

Fonctions actuelles : cellules, murs, placements typés, palette, Selected
Object, links/connectors, variables, Lua, monstres/patrouilles, relocations,
pits, validation, preview, Overview Map, Playtest, Undo/Redo et Erase one-shot.

Le long terme reste un éditeur joueur autonome réutilisant les mêmes assets et
contrats, pas un second modèle de niveau.

## 5. Runtime exploration et interaction

\`AGrimrockPartyPawn\` porte le déplacement grille, rotation, free look,
head-bob, transitions, pits et orchestration runtime.

\`AGrimrockPlayerController\` arbitre le clic souris via
\`ResolveLeftMouseInteraction()\` et le hover d'item tenu via
\`ResolveCursorItemHoverCursor()\`.

Ordre conceptuel :

\`\`\`text
UI / message lisible
    -> item tenu : wall lock / receptacle / world drop / throw
    -> interaction monde via IGridInteractableInterface
    -> fallback silencieux
\`\`\`

Le hover n'exécute aucune mutation. Les règles métier restent dans les acteurs,
services de transfert et runtime.

## 6. Event → Command, Logic, Lua et Quest

\`\`\`text
Object Event
    -> UGridActivationComponent
        -> FGridObjectLink
            -> Command
            -> LogicExecute
            -> LuaCallback
            -> Quest*
\`\`\`

Lua 5.4 est sandboxé dans \`GrimrockLua\`. Les scripts de puzzle utilisent
\`grid.command(...)\` pour revenir au dispatcher canonique.

Quest :

- \`UGridQuestDefinitionAsset\` = définition ;
- \`UGridQuestSubsystem\` = autorité runtime de campagne ;
- \`FGridCampaignQuestRuntimeState\` = état runtime transient ;
- MON21.3 route \`QuestStart\`, \`QuestCompleteObjective\`,
  \`QuestComplete\`, \`QuestFail\`.

**MON21.4 Quest Persistence reste ouvert** : le SaveGame courant ne contient
pas encore le snapshot Quest.

## 7. Items, inventaire et équipement

\`UGridItemDefinitionAsset\` est l'unique définition collectible.
\`FGridItemInstance\` porte l'identité runtime et l'ownership.
\`UGridPartyInventoryComponent\` reste l'autorité du groupe/inventaire.

\`\`\`text
FGridPartyInventoryState
    ActiveCharacters
    ActiveEquipment
    CharacterPool
    SelectedCharacterIndex
    CursorItem
\`\`\`

Les bindings de barre d'actions sont persistés par personnage dans
\`CombatHotbarSlots\`. Le contrat courant prévoit au minimum **12 slots**.

\`UGridItemTransferService\` conserve l'atomicité des transferts entre
inventaire, équipement, curseur, monde et réceptacles.

## 8. Combat et monstres

\`UGridTurnManagerComponent\` est l'autorité combat :

- initiative globale ;
- round / combattant actif ;
- PA individuels ;
- PAM du groupe ;
- catalogue d'actions ;
- ciblage ;
- paiement atomique ;
- résolution ;
- victoire/défaite.

Le Combat HUD est une projection et ne décide pas des coûts ou de l'initiative.

Les monstres restent data-driven via \`UGridMonsterDefinitionAsset\` et des
composants spécialisés : mouvement, comportement, combat, mort, audio, VFX.
Perception, occupation et pathfinding restent basés sur la grille.

## 9. RPG courant

### Personnage

\`\`\`text
Durable
    Experience
    SelectedClassProgressionChoiceIds
    Attributes
    Resources
    SkillRanks
    KnownSpellIds
    StatusEffects
    InventorySlots
    CombatHotbarSlots
    identity authored

Transient / reconstruit
    Level
    DerivedStats
    ClassDefinition / ClassDisplayName
    RaceDisplayName
    Portrait / ClassIcon
\`\`\`

### Skills

\`\`\`text
SkillRanks
    -> FRPGSkillService
    -> FRPGSkillPointService
    -> FGridSkillsPageService
\`\`\`

L'économie Skill Points n'est pas persistée : points accordés et dépensés sont
reconstruits depuis Level + SkillRanks. Le Safe Undo ne traverse pas la session
courante.

### Talents

\`\`\`text
URPGClassAsset::ProgressionChoices
    -> FRPGClassProgressionService
    -> FRPGClassProgressionTransactionService
    -> FGridSkillsPageService
    -> FGridTalentTreeView
\`\`\`

Production : **6 classes / 18 branches / 90 Talents conceptuels**, dont
**86 simples et 4 familles à variantes**. Les anciennes façades
\`FRPGTalentRuntimeService\`, \`FRPGSkillRuntimeService\` et la projection plate
\`FGridTalentEntryView\` ont été supprimées.

### Attributes et Level Up

- \`FRPGAttributePointService\` dérive le budget depuis le niveau et les
  attributs durables ;
- aucun compteur Attribute Point n'est persisté ;
- \`URPGLevelUpNotificationSubsystem\` ne porte qu'une file transitoire de
  toasts ;
- \`LastAcknowledgedLevel\` et l'ancien Level-Up modal n'existent plus.

## 10. Magic et Status Effects

\`KnownSpellIds\` vit directement dans le personnage durable. Le Spellbook
runtime est une façade/projection.

\`Character.StatusEffects\` est durable ; les références de définition
transientes sont réhydratées depuis les identités d'effet.

La transaction de cast reste autoritaire pour PA/mana/cible/effets.

## 11. UI courante

\`\`\`text
Viewport
├── WBP_CharacterSheet       fenêtre autonome
├── WBP_InventoryBag         fenêtre autonome
├── WBP_GridSkills           fenêtre autonome Skills + Talents
├── WBP_GridMap              fenêtre autonome Map
├── WBP_GridCombatHud        combat uniquement
└── WBP_GridPersistentHud    navigation + action bar persistante

WBP_GrimrockMenu
└── shell temporaire Journal / Recipes / Codex / Spellbook
\`\`\`

\`UGridPersistentHudWidget\` est le propriétaire de la navigation globale et de
la barre d'actions. \`UGridCombatHudWidget\` est réservé à la présentation
combat.

UI-COMBAT-UNIFY02 est le contrat C++ courant ; la validation UMG/PIE de sa
hiérarchie finale reste explicitement non revendiquée.

## 12. Map

MON21.6 et MAP-THEME01 sont clos.

\`\`\`text
FGridLevelRuntimeState::MapExploration
    -> FGridMapReadModelBuilder
    -> FGridMapFloorView
    -> UGridMapWidget / UGridMapSurfaceWidget
    -> UGridMapVisualThemeAsset
    -> rendu NativePaint
\`\`\`

La Map supporte exploration persistante, secrets filtrés, multi-dalles,
multi-étages, symboles, navigation d'étage, zoom/pan/recenter et rendu texturé.
Elle reste une projection : aucune topologie ou exploration n'est authorée dans
UMG.

## 13. Save / Continue

\`UGrimrockPartySaveGame::CurrentSaveVersion = 24\`.

Contrat :

\`\`\`text
SaveVersion == 24 -> validate -> restore
SaveVersion != 24 -> reject
\`\`\`

Enveloppe persistée :

- \`PartyInventoryState\` ;
- \`DungeonRuntimeState\` ;
- \`CurrentDungeonLevelId\` ;
- position/facing du groupe.

La MapExploration est incluse dans le runtime state. Quest ne l'est pas encore.

## 14. Validation

Harness :

\`\`\`text
Scripts/ValidateUE.ps1
Scripts/ValidatePackage.ps1
\`\`\`

Dernière campagne globale + Shipping : baseline \`9045ef2d\`,
**1026/1026**, zéro warning/échec, Shipping validé.

Les changements postérieurs ont des validations ciblées, notamment RPG-SKILL01,
RPG-LEVELUX01, RPG-ATTR01, UI-RPG-DESC01 et UI-RPG-CODE-AUDIT01. Aucun document
ne doit les transformer artificiellement en nouvelle campagne globale.

## 15. État fonctionnel

\`\`\`text
MON13–MON20                  CLOS / VALIDÉS
MON21.1                      CLOS
MON21.2 Quest Runtime        VALIDÉ
MON21.3 Quest Event/Command  VALIDÉ
MON21.4 Quest Persistence    EN ATTENTE
MON21.5 Journal              À FAIRE
MON21.6 Map                  VALIDÉ / CLOS
MAP-THEME01                  VALIDÉ / CLOS
MON21.7 Codex                À FAIRE
MON21.8 Cross-System Closure À FAIRE
MON22 Vertical Slice         À FAIRE

RPG-SKILL01                  VALIDÉ / CLOS
RPG-LEVELUX01                VALIDÉ / CLOS
RPG-ATTR01                   VALIDÉ / CLOS
UI-RPG-DESC01                VALIDÉ / CLOS
UI-RPG-CODE-AUDIT01          VALIDÉ / CLOS
\`\`\`

## 16. Documentation de référence

Ordre conseillé :

1. \`docs/Design/00_PROJECT_OVERVIEW.md\`
2. \`docs/Design/PROJECT_COMPLETION_ROADMAP.md\`
3. \`docs/Architecture/PROJECT_SYNTHESIS.md\`
4. \`docs/Architecture/ARCHITECTURE_INDEX.md\`
5. \`docs/Architecture/Maps/GRIMROCK_PROJECT_MAP.md\`
6. \`docs/Architecture/Maps/GRIMROCK_PROJECT_MAP_MERMAID.md\`
7. \`docs/Design/99_DECISIONS_LOG.md\`
8. \`docs/Design/DOC_AUDIT02_DESIGN_CPP_COHERENCE.md\`
9. \`docs/Architecture/ARCHITECTURE_REBASELINE_2026_10_09.md\`

Les documents MIG/cleanup/audit datés restent des snapshots historiques.
