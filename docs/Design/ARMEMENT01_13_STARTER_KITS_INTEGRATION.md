# ARMEMENT01.13 — Intégration des 18 kits N1 aux six classes

## 13.1 — Préparation sans bascule du wizard (29 octobre 2026)

**État : code préparé sur master, validation C++ UE5.5.4 à faire localement.** Ne pas annoncer la mise en production des nouveaux kits à ce stade.

L'autorité des compositions reste `docs/Design/ARMEMENT01_9_6_STARTER_KIT_RECIPES.csv` (18 recettes ApprovedDesign) et `Config/Armement/WeaponProductionManifest.csv` (22 armes Active). Le runtime conserve une représentation C++ déterministe nécessaire au jeu packagé, sans lecture du CSV en jeu.

`FRPGStartingEquipmentPresetCatalog::TryPopulateApprovedTransientClass` prépare 3 kits par classe dans une **copie temporaire** du DataAsset. Cette méthode est distincte de `TryPopulateTransientClass` (12 anciens kits) et n'est **pas encore appelée par le wizard**. Les six `DA_Class_*` et les cartes ne sont pas touchés.

Le nouveau catalogue de 18 recettes :
- utilise exactement les armes N1, positions MainHand / OffHand / Bag et identités `Kit_<ClassId>_<BranchId>` du CSV ;
- inclut torche ×1 pour Ranger/Scout et bombe ×1 pour Alchemist/Grenadier ;
- fournit 12 flèches au Ranger/Marksman selon le contrat LOT02.5 ; **12 flèches provisoires** également au Ranger/Scout, à valider en gameplay avant activation ;
- **n'invente aucun des cinq compléments en attente** : désamorçage, catalyseur de surfaces, consommable de soin, préparation de l'Apothicaire, composants de transmutation ; les armes de base restent présentes ;
- ne distribue aucun talent ou sort N2, ne verrouille aucune classe à un type d'arme et ne modifie aucun format SaveGame.

La résolution exacte des `ItemDefinitionId` pour SaveGame est étendue aux nouvelles définitions et maintient l'accès aux items des **12 anciens kits** sans substitution d'ID.

### Prévalidation locale

1. Fermer UE, faire `git pull --ff-only origin master`.
2. Compiler le Development Editor avec `Scripts/ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipAutomation`.
3. Vérifier `git status --short`. Aucun package produit par cette première étape.

### Dépendance obligatoire avant activation

`Localization/Source/UI.csv` contient désormais 18 nouvelles clés `CharacterCreation.StartingKit.Name.Kit_*`. **ST_UI.uasset ne les possède pas encore** tant que l'auteur Unreal n'a pas synchronisé la table. Il est obligatoire de matérialiser **l'asset existant** avec le commandlet actuel `UIStringTableAuthoring` puis de committer exclusivement `Content/Localization/StringTables/ST_UI.uasset`.

Le basculement des nouveaux kits dans le wizard (13.2) est **bloqué** tant que l'import ST_UI et les tests locaux ne sont pas verts. Il faudra adapter en un lot les assertions historiques « 2 kits par classe », conserver les audits historiques explicitement sur les 12 anciens kits, vérifier les 18 créations de personnages par `CreateInitialCharacter` avec GUID, emplacements et quantités, puis Save/Continue et PIE.

### Clôture 13.2 — à obtenir après import ST_UI

- Sélection effective de trois orientations par classe, titres localisés, résumé d'équipement ;
- équipement N1 réellement attribué par le service transactionnel, refus atomique des ID erronés ;
- 12 anciens identifiants sauvegardés toujours résolus sans migration silencieuse ;
- aucune mutation des six classes production, aucun équipement fabriqué pour les compléments manquants ;
- campagne de tests C++ complète et vérification PIE du wizard.
