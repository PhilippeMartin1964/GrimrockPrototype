# ARM-CLEAN06.4X.61 — quatre définitions et dernières références de loot

**Parent binaire :** `84d7324af420687e5a17a9696642a80cb88d3a28`. X.60B : Stone N0 pointerless ; 1995 tests réussis, dont 31 avec avertissements ; SilentTask + LocalAuto SUCCESS ; éditeur/PIE 8/8 validés par le propriétaire.

## Evidence native et décisions

Le préflight X.60B du 18 novembre 2026 a exporté les cinq sources. Snapshot privé de l'opérateur : `D:\GrimrockNativeSnapshots\ARM_CLEAN06_4X60\bc977fa2a4e0-20261118103226570\preflight.json`. Les quatre autres sources sont des placeholders sans mesh, icône, équipement ni action.

| ItemId | Ancien type / catégorie | Poids kg | MaxStack | Définition canonique |
|---|---|---:|---:|---|
| GoblinKnife | Weapon | 0.5 | 1 | UGridNonArmamentDefinitionAsset |
| EmptyVial | Component | 0.1 | 10 | UGridGenericStackableDefinitionAsset |
| Item_RatMeat | Food | 0 | 99 | UGridConsumableDefinitionAsset |
| Item_RatTooth | Component | 0 | 99 | UGridGenericStackableDefinitionAsset |

Aucun des quatre ne possède d'archétype ARM, de slot, d'action, de QuickItem, de tags historiques, de mesh, d'icône, de lumière ou de VFX. GoblinKnife conserve son ItemId et son classement historique d'arme mais n'acquiert **aucun droit d'équipement ou d'attaque**. RatMeat conserve sa classe de nourriture, sans effet inventé. L'ancien modèle affichait Consume pour Food indépendamment d'une action réelle ; le véritable exécuteur refusait déjà cet objet sans QuickItem. Le nouveau modèle ne présente pas cette commande inutilisable. Aucun changement du gameplay fonctionnel.

On conserve les FText de source, paramètres de présentation et politique GenericThrow (inactive, aucun slot), masse, pile et niveau N0. La seule addition sémantique aux tags vides est un tag réservé *de présentation* `InventoryCategory.Weapon`, `InventoryCategory.Food` ou `InventoryCategory.Component`, afin de préserver tooltip/tri/filtre. Le modèle UI lit ces tags exclusivement depuis ResolveItem ; ils ne donnent aucune permission. Un conflit entre les trois tags ou un tag de cette famille sur un ARM est rejeté par Validate. Les objets existants sans tag conservent leur comportement.

## Préflight et tests

FGridEditorLootDefinitionProjection charge directement les quatre sources natives et refuse tout écart au type, poids, pile, absence d'actions, d'équipement et de présentation. Reproduit la définition dans le registre *transitoire*, préserve les 34 entrées ordonnées et quatre catalogues, vérifie par réflexion chaque propriété de définition, par résolution N0 nom/description/masse/pile/catégorie et absence de permissions, et rejette N1 physique. Deux vraies tables : Goblin [Knife, Stone, Vial] ; Rat [Meat, Tooth] ; 128 seeds par table inchangés (tous les résultats, tirages, quantités, ordre). XP 125/500, chances/quantités du Goblin MON17.7 préservées ; le manifeste conserve les valeurs réelles du Rat sans en inventer.

Tests `Grimrock.RPG.ARM.CLEAN06_4X61.FourLootDefinitionProjection` et `LootPresentationCategories` ; X.48 exige 4 anciennes références et 34 définitions OU 0 et 38. Les gates antérieurs acceptent la seule extension exacte de quatre identités, en gardant leurs invariants (22 ARM, catalogue, 95 occurrences du monde, etc.). X.37 vérifie les vrais catalogues via un root transitoire des 34 origines ; son ancien migrateur reste strictement épinglé. TD07 n'exempte que les cinq lignes exactes des deux monstres. Aucun test de suppression de classe ou de SaveGame encore autorisé.

## Transaction native protégée — sept packages, un commit binaire

`GridArmClean064X61NativeLootCutover`, `Scripts/ApplyARM_CLEAN06_4X61_NativeLootCutover.ps1`.

- Trois existants : DA_ItemSystem, DA_MON_GoblinThrower, DA_MON_RatGiant ; sauvegardes privées SHA256 et receipts byte-equal.
- Quatre créations strictement allowlistées : CanonicalStage/Definitions/DA_Canonical_GoblinKnife, DA_Canonical_EmptyVial, DA_Canonical_Item_RatMeat, DA_Canonical_Item_RatTooth. Leur absence initiale ainsi que leur filtre LFS sont vérifiés. Les anciennes définitions sources restent intactes.
- Le plan est projeté entièrement et validé avant la première écriture ; aucun état partiel accepté. Manifeste normalisé du root, deux monstres et quatre sources, toutes les nouvelles définitions comparées à une projection fraîche. Pas de changement des catalogues, niveaux, palette, loot chance/quantité, XP, IA ou sauvegardes.
- Fresh Verify, suite Automation Grimrock complète, sept empreintes, dirty set exact 3 M + 4 ??, index/LFS vérifié. Commit atomique sur master uniquement, jamais force push. En cas d'échec avant commit : restauration des trois originaux, suppression **uniquement** des quatre fichiers prouvés absents avant Apply (aucun autre fichier). Si changement externe, arrêt sans écraser le travail utilisateur.

**Avant Apply :** 34 définitions et 4 anciens pointeurs du périmètre 107. **Après X.61B :** 38 définitions et zéro de ces 107 pointers, pas encore zéro legacy global. Il demeure 10 définitions alchimie hors registre, anciennes sources/catégories/catalogues, WBP d'icônes, SaveGames v27. Ne pas exécuter anciens migrateurs.

### Validation UI / localisation

Workflow relu : UI.csv → ST_UI → WBP → Game/fr/en. Aucun texte nouveau, aucun WBP ni passe de matérialisation ST_UI ; les catégories réutilisent les libellés existants, DisplayName/Description restent des FText venant des sources. Tester en PIE les filtres/absence de commandes injustifiées et la conservation des noms ; aucune prétention de nouvel asset visuel ou traduction. Les quatre restent sans mesh/icon comme avant, donc leur apparition visuelle n'est pas un critère réalisable tant que leurs visuels ne sont pas authorés. Automation doit vérifier les identités et la capacité de pile ; consulter les logs de drop pour ces placeholders. 
