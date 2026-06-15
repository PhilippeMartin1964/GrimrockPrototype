# Documentation GrimrockPrototype

Ce dossier regroupe la documentation transversale du projet.

Le dossier `docs/Design` contient la mémoire de design gameplay, éditeur, objets, connecteurs et décisions. Les fichiers Markdown placés directement dans `docs/` couvrent surtout la documentation technique transversale : pipeline art/materials/textures, structure `Content`, architecture C++ runtime/editor, tests manuels et licences.

## Ordre de lecture recommandé

### A. Comprendre le gameplay et l'éditeur

Lire d'abord :

- `docs/Design/README.md`
- `docs/Design/GRIMROCK_LOCK_SYSTEM.md` pour le système prospectif de serrures, clés, crochetage, conteneurs verrouillables et serrures piégées.
- `docs/Design/ITEM_AND_PICKUP_ASSET_CREATION_GUIDE.md` pour créer les définitions d'items, les archétypes de pickup et leur entrée de palette.

Ce README oriente vers les documents de design actuels, les audits, les checklists et le journal de décisions.

### B. Pipeline art, materials et textures

Lire dans cet ordre :

1. `docs/00_README_Art_Materials_Textures.md`
2. `docs/01_Content_Structure.md`
3. `docs/02_Texture_Pipeline_BC_N_ORM.md`
4. `docs/M_GrimrockSurface_Master.md`
5. `docs/03_M_GrimrockSurface_Masked_Master.md`
6. `docs/04_Material_Instances_Migration.md`

### C. Architecture C++

- `docs/Architecture_Runtime_Editor_Split.md`
- `docs/Architecture_LevelAsset_Editor_Runtime.md`
- `docs/Architecture/ARCHITECTURE_INDEX.md`
- `docs/Design/JALON_RUNTIME_DUNGEON_STATE.md`

Ces documents complètent `docs/Design` pour la séparation C++ entre le module runtime et le module editor, pour la responsabilité DataAsset / map / acteur avant le multi-niveaux, et pour la validation de la couche d'état runtime en mémoire du donjon multi-niveaux.

### D. Tests manuels

- `docs/Design/PIE_TEST_FROM_EDITOR.md`
- `docs/Runtime_Test_Button_Door.md`
- `docs/Tests/RECEPTACLE_RUNTIME_TESTS.md`
- `docs/Design/10_GRID_EDITOR_UI_CONSISTENCY_CHECKLIST.md`

### E. Licences

- `docs/FONT_LICENCES.md`

## Règle de priorité

En cas de contradiction sur le design gameplay, les objets, les connecteurs ou l'éditeur, `docs/Design/99_DECISIONS_LOG.md` et les documents `docs/Design` les plus récents priment.

Les documents techniques racine ne doivent pas dupliquer les règles gameplay/editor détaillées dans `docs/Design`.
