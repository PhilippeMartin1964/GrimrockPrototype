# ARMEMENT01.17 — Compléments Mage et Alchimiste : audit et première dotation N1

**01.11.2026. Statut : ARMEMENT01.17.0 (audit) + ARMEMENT01.17.1 (flasque d'huile initiale Transmutateur) publiés en code. Compilation UE5.5.4 / tests / PIE à confirmer. ARMEMENT01.17 n'est pas clos.**

## Constat sur la production existante

### Mage — Tisseur de surfaces

- Le kit `Kit_Mage_SurfaceWeaver` fournit déjà `DA_Weapon_ElementalStaff_01` N1 en MainHand, à deux mains.
- Les talents `Imbuement` (choix d'affinité), `ElementalConversion` et `TerrainArchitect` sont authorés en RPG03.9.4F2 ; **premier déblocage au niveau 2**, les suivants plus tard.
- Un `SurfaceCatalystToAuthor` figure dans la recette éditoriale, **sans ItemDefinitionId ni contrat N1 réellement autorisé**.
- Il serait incorrect d'attribuer gratuitement `Imbuement`, `ElementalConversion`, une affinité N2, ou de déclarer un item fictif « catalyseur » sans action validée.
- **ARMEMENT01.17.2 reste à concevoir et réaliser** : décider l'effet, les limites, la consommation et les cas d'usage N1 du catalyseur, puis authorer un vrai `UGridItemDefinitionAsset` par l'Editor, vérifier l'interface de lancement, les sources visuelles et la persistance avant de l'ajouter au kit. Ne pas créer une nouvelle mécanique de talent.

### Alchimiste — Transmutateur

- Le kit `Kit_Alchemist_Transmuter` fournit déjà `DA_Weapon_Dagger_01` (MainHand) et `DA_Weapon_ArcaneOrb_01` (OffHand passif).
- Le projet possède déjà le véritable `/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Flask_Oil`, authoré en RPG03.9.6C, **sans nouvelle création binaire**.
- `BuildQuickItemCombatActionDefinition` produit `Action_Alchemist_OilSlick` en QuickItem : Area R4, **2 PA**, `SourceItemQuantityCost=1`, surface `Oil` de **4 rounds** et coût de traversée +1.
- L'action matérielle de la flasque n'exige pas `Talent_Alchemist_Transmuter_OilSlick`, car le talent N2 **débloque une recette** ; offrir une flasque déjà fabriquée à N1 **n'accorde ni le talent, ni la recette, ni un moteur de crafting**.
- **ARMEMENT01.17.1** : attribuer exactement **une flasque d'huile terminée** dans le sac via la recette approuvée, en plus de la dague et de l'orbe ; les autres kits ne reçoivent pas de flasque par cette étape.
- Les **composants de crafting/transmutation** et la sélection des recettes de `MajorTransmutation` restent en attente du futur crafting. Les quatre RequirementIds de recettes N18 **ne sont pas** quatre items.
- La présence de l'action QuickItem dans l'asset ne constitue pas à elle seule une recette PIE de son ciblage/consommation en combat. Tester en jeu avant clôture.

### Apothicaire

`Kit_Alchemist_Apothecary` reçoit déjà une `DA_Item_Potion_Healing_01` N1 par ARMEMENT01.15, avec une action de soin et des ressources visuelles propres. Cette étape ne doit ni créer de deuxième potion ni accorder `EnhancedPotion` N2.

## Contrats et sources

- Code kit : `Source/GrimrockPrototype/Private/RPG/RPGStartingEquipmentPresetCatalog.cpp`.
- Source éditoriale : `docs/Design/ARMEMENT01_9_6_STARTER_KIT_RECIPES.csv`, ligne Transmuter : `KnownSupportItemId=Item_Flask_Oil`, `KnownSupportQuantity=1`, besoin de composants `OilTransmutationComponentsToAuthor` **toujours ouvert**.
- Moteur des surfaces et alchimie existants : `RPG03_9_6B2_ALCHEMIST_TRANSMUTER.md`, `RPG03_9_6C_ALCHEMIST_PRODUCTION_MATERIALIZATION.md`, `RPG03_9_4F2_MAGE_SURFACE_WEAVER_BRANCH.md`.
- `FRPGStartingEquipmentService::TryBuildKit` effectue l'attribution transactionnelle, avec GUID et owner exact ; `ResolveKnownStartingItemDefinition` étend automatiquement la résolution de sauvegarde au nouvel item explicitement présent dans le kit.
- Ne pas injecter un item dans une sauvegarde déjà existante, ni contourner le système de hotbar/Combat QuickItem.
- **Aucun changement UI ou localisation** dans cette étape : `UI.csv → ST_UI → WBP → Game/fr/en` restent inchangés.

## Tests et validation locale

Deux tests sous `Grimrock.RPG.ARMEMENT01.17.1` :

1. `RealOilFlaskNoTalentBypass` : DataAsset réel, action QuickItem actuelle valide, source, PA/Rayon/consommation/Surface/Oil, aucune exigence N2 sur l'action, résolution SaveGame par ID exact.
2. `TransmuterNewGameSaveContinue` : trois orientations Apothicaire/Grenadier/Transmutateur intactes, seulement le Transmutateur reçoit la flasque, équipement N1 intact, création réelle, GUID propriétaire et quantité, sérialisation SaveGame binaire et RestorePartyInventoryState.

Dans PowerShell, après fermeture d'Unreal Editor :

```powershell
cd D:\Development\GrimrockPrototype
git status --short
git pull --ff-only origin master
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.RPG.ARMEMENT01.17.1"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.RPG.ARMEMENT01.13.ApprovedKits"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.RPG.ARMEMENT01.9.Kit"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.RPG.RPG03.9.6"
```

## Recette PIE de ARMEMENT01.17.1

1. New Game → Alchimiste → Transmutateur : dague en MainHand, orbe en OffHand, **une flasque d'huile dans le sac**, sans talent accordé.
2. Comparer les kits Grenadier et Apothicaire : leur composition ne change pas.
3. Combattre avec la flasque d'huile depuis le système QuickItem/hotbar existant : sélectionner la zone, vérifier coût **2 PA**, dépense de **1 flasque**, et surface d'huile de 4 rounds.
4. Répéter avec flasque absente : refus sans dépense de PA, pas de surface artificielle.
5. Save → Quitter → Continue avant consommation : item unique et équipement persistés.
6. Vérifier aussi la présence/absence d'icône de cet item, car les assets RPG03 historiques ne garantissent pas un visuel d'inventaire final.

## Étape suivante : ARMEMENT01.17.2

Le **Mage/Tisseur de surfaces** reste à traiter, car le « catalyseur de surfaces » n'a encore aucune définition d'objet. Il faudra décider s'il s'agit d'un QuickItem consommé fournissant une interaction de surface N1, d'un outil réutilisable qui agit sur des surfaces préexistantes, ou d'un autre objet doté d'un véritable effet ; il ne doit pas conférer les actions de talents N2. Les composants de transmutation attendront un moteur de crafting autoritaire pour ne pas devenir de faux loot.

**Ne pas clôturer ARMEMENT01.17 au seul vu des tests du Transmutateur.**


## ARMEMENT01.17.1.1 — Adaptation de la régression KitDesign (1er novembre 2026)

**Résultats locaux avant correctif :** `Grimrock.RPG.ARMEMENT01.17.1` 2/2,
`Grimrock.RPG.RPG03.9.6` 28/28, `ApprovedKits` 2 réussis + 1 avertissement,
`Grimrock.RPG.ARMEMENT01.9.Kit` 5 réussis + 1 échec historique.

`NoLegacyKitOrSaveMutation` supposait encore deux `KnownSupportItemId`
dans les 18 recettes éditoriales. Le Transmutateur possède désormais la
troisième entrée réelle : `Item_Flask_Oil` ×1. Le test affirme à présent
les identités et quantités exactes par kit : Éclaireur/torche,
Grenadier/bombe et Transmutateur/flasque d'huile, et exige `None` pour les
15 autres. Les cinq besoins de support encore déclarés dans ce CSV
restent à **5** : la flasque terminée n'est pas la fabrication de composants
de transmutation, toujours bloquée par l'absence de crafting.

Aucune modification de gameplay, DataAsset, format de sauvegarde ou
localisation. Le test corrigé reste à compiler et valider sous UE5.5.4 :

```powershell
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.RPG.ARMEMENT01.9.Kit"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.RPG.ARMEMENT01.17.1"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.RPG.ARMEMENT01.13.ApprovedKits"
```

Le jalon ARMEMENT01.17.1 exige en outre la recette PIE de la flasque
(ciblage, coût 2 PA, consommation, surface et Save/Continue) avant clôture.
