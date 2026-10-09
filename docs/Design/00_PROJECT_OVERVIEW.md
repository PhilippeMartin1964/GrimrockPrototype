# GrimrockPrototype — Vue d’ensemble du projet

## Objectif

GrimrockPrototype est un dungeon crawler Unreal Engine 5.5.4 en C++ inspiré de
*Legend of Grimrock 2* : vue subjective case par case, Grid Editor, mécanismes
data-driven, Logic/Lua, IA de monstres, combat tactique, groupe RPG,
progression, magie et, à terme, création de niveaux par les joueurs.

## État courant — 9 octobre 2026

Baseline C++ auditée par DOC-AUDIT02 :

```text
6f98a0ef5599f37d3d544529aa5a790d647e8251
UI-RPG-CODE-AUDIT01 close validated cleanup
```

Le commit DOC-AUDIT02 qui suit est **documentation-only** ; il peut donc faire
avancer `master` sans changer cette baseline C++ auditée.

La dernière **campagne globale + Shipping** fournie par l'utilisateur reste la
baseline runtime/content du 4 octobre 2026 :

```text
9045ef2db75c09997db4fc65dbf99d4598f4df5c

Grimrock
Succeeded               : 1026
Succeeded with warnings : 0
Failed                  : 0
Not run                 : 0

Win64 Shipping
Build + Cook + Stage + Package + Pak + Archive : OK
Cook : 0 error / 0 warning
AutomationTool ExitCode : 0
```

Les changements postérieurs à cette baseline ont fait l'objet de validations
**ciblées**, pas d'une nouvelle campagne globale/Shipping. La documentation ne
doit donc pas présenter `6f98a0ef` comme une nouvelle baseline Shipping.

Validations ciblées récentes acquises :

```text
RPG-SKILL01                               8/8 + PIE
RPG-LEVELUX01 / régressions associées    42/42 + PIE
RPG-ATTR01                              10/10 + PIE
UI-RPG-DESC01                           19/19 + PIE six classes
UI-RPG-DESC01.QA16                       4/4
UI-RPG-CODE-AUDIT01                      1/1
MON20.8.SkillsPage                        8/8
MON15.4                                   7/7
RPG03.10 PIE six classes                  1/1
warnings / failures                       0
```

## Jalons majeurs

```text
MON13–MON20                    CLOS / VALIDÉS
MON21.1                        CLOS
MON21.2 Quest Runtime          VALIDÉ
MON21.3 Quest Event->Command   VALIDÉ
MON21.4 Quest Persistence      EN ATTENTE
MON21.5 Journal                À FAIRE
MON21.6 Map                    VALIDÉ — CLOS
MAP-THEME01                    VALIDÉ — CLOS
MON21.7 Codex                  À FAIRE
MON21.8 Cross-System Closure   À FAIRE
MON22 Vertical Slice           À FAIRE

UI-RPG06                      VALIDÉ — CLOS
RPG-SKILL01                   VALIDÉ — CLOS
RPG-LEVELUX01                 VALIDÉ — CLOS
RPG-ATTR01                    VALIDÉ — CLOS
UI-RPG-DESC01                 VALIDÉ — CLOS
UI-RPG-CODE-AUDIT01           VALIDÉ — CLOS
```

Les tickets `RPG-TALENT-FIX01..07` sont intégrés ; D02/D03 restent
explicitement différés aux vrais systèmes Lock/Trap et actions monde hors
combat. D09 reste futur Crafting.

`UI-COMBAT-UNIFY02` décrit le contrat C++/UMG courant du Combat HUD ; son
document de référence conserve explicitement l'état de validation UMG/PIE qui
reste à fournir.

## Architecture de référence

```text
GrimrockLua
    ↓
GrimrockPrototype
    ↓
GrimrockPrototypeEditor
```

Principes :

- DataAssets et grille = autorités de conception/logique ;
- Actors runtime = reconstruction transitoire ;
- Event -> Command = bus gameplay ;
- Logic et Lua orchestrent sans créer une voie parallèle ;
- C++ porte logique, calculs, invariants et read models ;
- Blueprint/UMG porte composition, configuration et présentation ;
- aucune compatibilité arrière Save/DataAsset/Blueprint exigée pendant le prototype ;
- SaveGame courant : **v24 exact-match**.

### Progression RPG courante

```text
Talents
URPGClassAsset::ProgressionChoices
    -> FRPGClassProgressionService
    -> FRPGClassProgressionTransactionService
    -> FGridSkillsPageService
    -> FGridTalentTreeView

Skills
FGridCharacterInventoryState::SkillRanks
    -> FRPGSkillService
    -> FRPGSkillPointService
    -> FGridSkillsPageService

Attributes
FGridCharacterInventoryState::Attributes
    -> FRPGAttributePointService
```

Les façades historiques `FRPGTalentRuntimeService`,
`FRPGSkillRuntimeService` et la projection plate
`FGridTalentEntryView` n'existent plus.

## Campagne / Map

MON21.6 est clos. La Map possède exploration persistante, secrets filtrés,
projection multi-dalles/multi-étages, symboles, navigation étage,
zoom/pan/recenter et rendu texturé data-driven via
`UGridMapVisualThemeAsset`.

MON21.4 reste réellement ouvert : le C++ courant possède le runtime Quest mais
`UGrimrockPartySaveGame` ne porte encore aucun snapshot Quest. Journal et
Codex restent des surfaces futures.

## Documentation courante

Ordre recommandé :

1. `docs/Design/00_PROJECT_OVERVIEW.md`
2. `docs/Design/PROJECT_COMPLETION_ROADMAP.md`
3. `docs/Design/UI_ARCHITECTURE_CURRENT.md`
4. `docs/Architecture/PROJECT_SYNTHESIS.md`
5. `docs/Architecture/ARCHITECTURE_INDEX.md`
6. `docs/Design/99_DECISIONS_LOG.md`
7. `docs/Design/DOC_AUDIT02_DESIGN_CPP_COHERENCE.md`
8. `docs/Design/DOC_CLOSURE01_REBASELINE_DOCUMENTATION.md` — baseline historique du 4 octobre

Les tickets datés plus anciens restent des **snapshots historiques**. Ils ne
priment jamais sur les références courantes lorsque l'API, le SaveGame ou le
statut d'un jalon a évolué.

## Règle de travail

- travail sur `master` ;
- un ticket = un commit atomique ;
- pas de force-push ;
- tests déclarés verts uniquement avec sortie UE fournie par l’utilisateur ;
- assets binaires modifiés uniquement via Unreal Editor ;
- mise à jour documentaire durable à la clôture d’un jalon.
