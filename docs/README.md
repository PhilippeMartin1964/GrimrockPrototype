# Documentation GrimrockPrototype

Ce dossier regroupe la documentation transversale du projet. `docs/Design` porte les décisions/jalons ; `docs/Architecture` porte les contrats techniques durables et le bilan courant.

## Ordre de lecture recommandé

### 0. Comprendre l’ensemble du projet

- `docs/Architecture/PROJECT_SYNTHESIS.md` — synthèse globale ;
- `docs/Architecture/Maps/GRIMROCK_PROJECT_MAP.md` — carte détaillée textuelle et autoritaire ;
- `docs/Architecture/Maps/GRIMROCK_PROJECT_MAP_MERMAID.md` — vues visuelles Mermaid ;
- `docs/Architecture/ARCHITECTURE_INDEX.md` — index des contrats.

### A. Gameplay et éditeur

- `docs/Design/README.md`
- `docs/Design/99_DECISIONS_LOG.md`
- `docs/Design/PROJECT_COMPLETION_ROADMAP.md`
- `docs/Design/ITEM_AND_PICKUP_ASSET_CREATION_GUIDE.md`
- `docs/Design/GRIMROCK_LOCK_SYSTEM.md`

### B. Pipeline art / meshes / materials / textures

1. `docs/00_README_Art_Materials_Textures.md`
2. `docs/01_Content_Structure.md`
3. `docs/05_Static_Mesh_Blender_UE5_5_4_Pipeline.md`
4. `docs/02_Texture_Pipeline_BC_N_ORM.md`
5. `docs/M_GrimrockSurface_Master.md`
6. `docs/03_M_GrimrockSurface_Masked_Master.md`
7. `docs/04_Material_Instances_Migration.md`

### C. Architecture C++

- `docs/Architecture_Runtime_Editor_Split.md`
- `docs/Architecture_LevelAsset_Editor_Runtime.md`
- `docs/Architecture/ARCHITECTURE_INDEX.md`
- `docs/Design/JALON_RUNTIME_DUNGEON_STATE.md`

### D. Tests manuels

- `docs/Design/PIE_TEST_FROM_EDITOR.md`
- `docs/Runtime_Test_Button_Door.md`
- `docs/Tests/RECEPTACLE_RUNTIME_TESTS.md`
- `docs/Design/10_GRID_EDITOR_UI_CONSISTENCY_CHECKLIST.md`

### E. Licences

- `docs/FONT_LICENCES.md`

## Historique documentaire

Git est l’historique. Les documents courants ne sont pas dupliqués avec des suffixes de date.

## Règle de priorité

En cas de contradiction, `docs/Design/99_DECISIONS_LOG.md` et les documents de clôture/validation les plus récents priment.
