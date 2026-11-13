# ARMEMENT01.17 — Compléments Mage et Alchimiste : audit et première dotation N1

> **ARM-CLEAN06 — ARCHITECTURE CIBLE APPROUVÉE (13.11.2026).** [Contrat](../Architecture/ARM_CLEAN06_ITEM_SYSTEM_NORMATIVE.md), [migration](../Architecture/ARM_CLEAN06_MIGRATION_PLAN.md). Les mécaniques/test historiques demeurent ; les anciens schémas DataAsset sont remplacés. **Non implémentée : zéro legacy dans le runtime final.**


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


## ARMEMENT01.17.2A — Catalyseur de surfaces, contrat N1 et fabrication (4 novembre 2026)

**Choix limité N1 :** une fiole consommable `Item_SurfaceCatalyst_01` dans
`Kit_Mage_SurfaceWeaver`, plus tard, avec un seul effet : appliquer
`SurfaceInteraction=Fire` à **une cellule déjà dotée d'une surface
canonique compatible**, portée 4 avec ligne de vue, coût **2 PA en combat**,
une fiole consommée à la résolution. La validation canonique refuse l'eau
et les cellules neutres ; Oil→Fire, Ice→Water, Poison→Fire, etc. sont
résolus par C6 sans inventer de nouveaux dégâts.
Il ne crée aucune surface à partir d'une cellule vide, ne possède
aucun `OwnerVariant`, aucune affinité, aucun bonus de talent et n'attribue
ni Imprégnation N2 ni Conversion élémentaire N6. Différent du
`Action_Alchemist_Catalyst` N14 (`AnyCanonical`).

**Identités :** `DA_Item_SurfaceCatalyst_01`,
`Item_SurfaceCatalyst_01`, `Action_Mage_SurfaceCatalyst_01`.
Présentation de l'objet en `FText NSLOCTEXT` (autorité du DataAsset,
pas d'ajout de clé WBP). Les références visuelles de la flasque d'huile
existante sont réutilisées provisoirement pour l'icône et le mesh ;
la différenciation iconographique et la traduction EN restent à réaliser.

**Séquence protégée :** authoring par
`FRPGStartingSurfaceCatalystAuthoring` dans le véritable Unreal Editor.
Un test `Grimrock.RPG.ARMEMENT01.17.2.ProductionCatalyst` exige le
`.uasset` réel. L'agent Windows n'est autorisé à exécuter
`AuthorStartingSurfaceCatalyst.ps1` que si ce test est le **seul**
échec de la suite complète et que le worktree est propre. Le script
ne crée et ne publie que ce `.uasset` via Git LFS et refuse tout autre
fichier modifié, origin ou master inattendu. Le nouveau commit est ensuite
validé normalement.

**ARMEMENT01.17.2B (à suivre après asset validé) :** distribution réelle
dans le kit Mage/Tisseur (une fiole), usage en combat via le pipeline
QuickItem et vérification de l'usage lors de l'exploration, tests
New Game/Save→Continue/PIE ; aucune attribution dans les sauvegardes
déjà créées. Le vrai item n'est pas déclaré opérationnel avant cette étape.


## ARMEMENT01.17.2B — Dotation N1 et exécution réelle du catalyseur

Après fabrication par Unreal du DataAsset natif, `Kit_Mage_SurfaceWeaver`
contient exactement un `DA_Item_SurfaceCatalyst_01` dans le sac,
en plus de son bâton élémentaire deux mains. Le Mage conserve son
niveau 1 et ses premiers talents au niveau 2 : aucune affinité N2
ni `ElementalConversion` N6 n'est accordée.

Le manifeste éditorial reflète `Item_SurfaceCatalyst_01` ×1,
et le nombre de recettes ayant un support encore à produire passe de
cinq à quatre. Les 17 autres kits et les anciennes sauvegardes ne
sont pas rétroactivement modifiés.

Pipeline de combat : l'item s'assigne à la hotbar QuickItem.
Un clic active le ciblage Cell R4/LOS, et la confirmation déclenche
`SurfaceInteraction=Fire` par C6 sur une surface préexistante.
Consommation de 1 item et 2 PA **seulement en cas de cible acceptée**.
Pas d'huile nouvelle créée ; pas de pouvoir de talent conféré.
Les interactions non canoniques refusent le ciblage.

Nouveaux tests :
- `Grimrock.RPG.ARMEMENT01.17.2B.SurfaceWeaverNewGameSaveContinue` :
  trois kits Mage, équipement, bag/GUID, save binaire et restore.
- `Grimrock.RPG.ARMEMENT01.17.2B.CatalystCombatHotbar` :
  cible invalide, coût réel, réaction Oil→Fire et refus sur Water.

La hotbar hors combat ne réimplémente pas ici le catalyseur :
seule l'huile est actuellement compatible exploration. Avant de
déclencher Fire hors combat, il faudra définir la durée temporelle
de toutes les surfaces persistantes, et pas seulement Oil. Il s'agit
d'une réserve de gameplay explicite, non d'un déblocage implicite.

Revue PIE/GPU et traduction anglaise de `NSLOCTEXT` à vérifier
séparément ; l'icône et le mesh du catalyseur réutilisent
provisoirement ceux d'une flasque existante. Aucun texte WBP
ni ST_UI ajouté dans ce jalon.


## ARMEMENT01.17.2C.2 — Identité visuelle native du catalyseur

Création par commandlet Unreal Editor (worktree Windows dédié) de six
ressources de production :

- `T_UI_Item_SurfaceCatalyst_01.uasset` : icône 256×256 RGBA,
  dessin procédural de fiole ambrée, armature bronze et rune centrale,
  propriété `UGridItemDefinitionAsset::Icon`. Le contenu est original.
- `SM_SurfaceCatalyst_01.uasset` : variante de la géométrie **déjà
  validée** de la flasque d'huile, avec au moins son premier slot
  de matériau remplacé ; **pas** de sculpture inédite fidèle à la
  planche conceptuelle dans ce jalon.
- `M_SurfaceCatalyst_Flask.uasset` : surface bronze/lueur orangée.
- `M_SurfaceCatalyst_Ember.uasset` : surfaces lumineuses d'étincelles
  et langues de feu 3D ; paramètres CatalystGlowColor / Strength.
- `M_SurfaceCatalyst_RunePulse.uasset` : décalque de rune-annulaire
  opaque au centre, translucide ailleurs, opacité CatalystOpacity.
- Modification limitée à `DA_Item_SurfaceCatalyst_01.uasset` pour
  affecter le mesh/texture ; **aucune mutation mécanique de QuickItem**.

Le même matériau Ember est utilisé par les projections temporaires
et les flammes tant que la surface RPG03.6 Fire existe. L'icône est
indépendante de celle de l'huile ; le mesh reste une variante provisoire.
D'autres assets (Niagara complexes, sons) ne sont **pas** prétendus
produits dans ce jalon et pourront constituer une passe artistique.

Test `Grimrock.RPG.ARMEMENT01.17.2C.ProductionVisualAssets` : asset natif,
texture 256², mesh et références d'item, domaines/blends. La fabrication
automatique est autorisée **uniquement si ce test est le seul échec**
du harness complet. Le helper publie cinq nouveaux .uasset et le
DataAsset modifié, tous explicitement allowlistés, sur master via Git LFS.

Contrôle final : `UE5.5.4/LocalAuto=success` du nouveau commit contenant
les assets, suivi de la recette PIE/GPU des effets 3D et du contraste
entre deux consommables de la hotbar.
