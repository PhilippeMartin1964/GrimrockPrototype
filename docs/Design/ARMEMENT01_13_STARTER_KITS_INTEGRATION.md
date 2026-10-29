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


## 13.2 — Activation des 18 kits et tests de régression (29 octobre 2026)

**Statut : intégration C++ préparée sur GitHub ; validation locale UE 5.5.4 et PIE nécessaire avant clôture.**

- Le wizard et CreateInitialCharacter génèrent les trois kits approuvés de chaque classe dans une copie transitoire ; aucune mutation des six DA_Class_*.
- La nouvelle identité de kit est Kit_<ClassId>_<BranchId>, sans conversion implicite des anciens identifiants.
- Les tests de création et de wizard ARMEMENT01.9 ont été adaptés. Le test SixClassKits.CatalogueAndOverride conserve la validation autonome des 12 kits historiques.
- La suite Grimrock.RPG.ARMEMENT01.13.ApprovedKits couvre les 18 créations réelles, le choix du wizard, les quantités, les slots, les GUID, l'unicité de propriété et le snapshot de sauvegarde.
- Les anciennes définitions d'objets sont toujours résolues par leur ID exact pour réhydrater les sauvegardes, sans substitution.
- Le package binaire ST_UI.uasset généré sur le poste local doit être ajouté à un commit distinct après validation ; il ne figure pas dans le commit distant de code.

### Vérification locale

1. Préserver la modification locale ST_UI.uasset et synchroniser master.
2. Exécuter Scripts/ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.RPG.ARMEMENT01.13.ApprovedKits".
3. Exécuter les filtres historiques Grimrock.RPG.ARMEMENT01.9.SixClassKits, Grimrock.RPG.ARMEMENT01.9.WizardSelection et Grimrock.RPG.ARMEMENT01.9.Localization.
4. Vérifier en PIE les 18 orientations et le cycle sauvegarde/Continue avant clôture.

## Validation locale ARMEMENT01.13.2 — 29.10.2026

- UE 5.5.4 Development Editor : compilation validée.
- Suite `Grimrock.RPG.ARMEMENT01.13.ApprovedKits` : 2 réussites, 1 avec avertissement, 0 échec, sortie 0.
- Suite `Grimrock.RPG.ARMEMENT01.9` : 30 réussites, 2 avec avertissements, 0 échec, sortie 0.
- Ces résultats valident le code de sélection/attribution et la non-régression automatisée. La validation PIE et le cycle Save → Quitter → Continue sur disque restent distincts.
- Les 18 kits approuvés sont l'unique référence de création de nouveaux personnages. Les 12 recettes historiques ne doivent pas déterminer le gameplay futur ; leur sort (suppression ou maintien minimal pour anciennes sauvegardes) n'est pas encore décidé.
- L'asset `Content/Localization/StringTables/ST_UI.uasset` a été régénéré en local lors de ARMEMENT01.13.1 ; sa modification binaire locale reste à publier, sans l'écraser depuis GitHub.

## ARMEMENT01.14.1 — Contrat réel des munitions initiales des deux Rôdeurs

- Nouveau test : `Grimrock.RPG.ARMEMENT01.14.RangerStarterAmmunition`.
- Instancie les véritables classes Ranger/Marksman et Ranger/Scout avec `CreateInitialCharacter` ; vérifie l'arc équipé, la liaison explicite `RequiredAmmunitionItemDefinitionId`, les 12 flèches, le débit unitaire jusqu'à zéro, le refus de surconsommer, l'identité des objets et les invariants de propriété.
- Ce test vérifie le **contrat d'inventaire/munitions** avec les assets de production ; les projectiles visibles, collisions, dégâts, portée et PA exigent toujours le PIE combat.
- Les cinq compléments non-armement (désamorçage, catalyseur, soin, Apothicaire, transmutation) restent des développements distincts ; leurs comportements doivent être définis avant création des assets, sans inventer des compétences niveau 2.


## ARMEMENT01.14.1 — Validation locale du contrat de munitions (29 octobre 2026)

- Filtre `Grimrock.RPG.ARMEMENT01.14.RangerStarterAmmunition` : **1 test réussi**, zéro avertissement, zéro échec, zéro non exécuté, code de sortie 0.
- Le test couvre les arcs de départ `Kit_Ranger_Marksman` et `Kit_Ranger_Scout`, la dotation et consommation des 12 flèches, et l'intégrité de l'inventaire ; il ne prétend pas valider à lui seul le tir visible ou les dégâts en PIE.

## ARMEMENT01.14.2 — Munitions limitées aux actions de tir

- `FGridRangedAmmunitionPolicy` expose un contrôle contextuel de disponibilité et de consommation, avec `bRangedAttack` explicite.
- Les deux chemins de l'exécution (`TryPlayerAttack`) et la disponibilité dans le catalogue d'actions appliquent cette même condition : une action **non distante** depuis une arme requérant des flèches ne doit ni être désactivée, ni consommer de flèche.
- Les attaques distantes continuent à exiger et consommer exactement une flèche après validation de l'action ; les attaques refusées ne décomptent pas de munition ni de PA.
- Le test existant `Grimrock.RPG.ARMEMENT.LOT02.AmmunitionInventoryContract` est étendu pour contrôler la non-consommation des attaques non distantes et la consommation explicite des attaques distantes.
- **Validation locale UE/Automation attendue** : `Scripts/ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.RPG.ARMEMENT.LOT02.AmmunitionInventoryContract"`, puis vérification PIE de l'attaque équipée réelle.
