# ARMEMENT — Guide unique de fabrication et de validation (INDUS05)

**Procédure active — version 1.0 du 26 octobre 2026.** Tous les comptes rendus `docs/Design/ARMEMENT01_*.md` et les anciennes sections détaillées de ce guide sont **historiques**, non des commandes à exécuter aujourd'hui. L'ancienne version de ce guide reste accessible dans l'historique Git. **Ce guide est le point de vérité opérationnel.**

**État du chantier :** INDUS01/02/03/04 validés par l'utilisateur ; INDUS05.1–05.4 implémentés sur `master`, validation locale UE5.5.4 et recette de clôture INDUS05 **attendues**. Ne pas transformer publication Git en PASS Unreal.

## 1. La réponse à la demande initiale

Une nouvelle arme ordinaire **ne doit pas nécessiter de nouveau script PowerShell, commandlet C++, test par arme ni guide de jalon individuel**.

| Rôle | Autorité | Ce qui n'y figure PAS |
| --- | --- | --- |
| Caractéristiques N1–N20 | `docs/Design/ARMEMENT01_Catalogue_Armes_Boucliers_Niveaux_1_20_ARMEMENT01_2.xlsx` | Identifiants de mesh, chemins d'icônes |
| Narratif | Feuille `Narratif` du **même XLSX** (82 lignes `NarrativeV1`, `APPROVED` pour le prototype) | Calculs de dégâts, pouvoirs automatiquement implémentés |
| Identités et références techniques | `Config/Armement/WeaponProductionManifest.csv` | Dés, portée, poids, PA et autres statistiques |
| Point d'entrée PowerShell **unique pour l'utilisateur** | `Scripts/WeaponPipeline.ps1` | Recettes spécifiques Guerrier, Assassin, Bouclier |
| Commandlet Editor commun | `UGridWeaponPipelineCommandlet` | Un commandlet par arme |
| Catalogue runtime | `DA_WeaponCatalogue`, `DA_WeaponVisualCatalogue` | Texte éditorial du narratif |
| Lecture d'info-bulle | `UGridInventorySlotWidget::GetTooltipView()`, `FGridWeaponTooltipProjection` et widgets natifs | Valeurs de combat écrites dans les WBP |
| Tests génériques | `Grimrock.RPG.ARMEMENT.Industrial` et `Industrial05` | Un nouveau test par nouvel archétype |

Le manifeste initial comprend **8 archétypes Active** : ARM001, ARM002, ARM014, ARM015, ARM025, ARM031, ARM045 et ARM075 ; **ARM003 est Planned**. Les **73 autres archétypes** ne sont pas encore déclarés dans le manifeste. Deux objets historiques (Shuriken et ThrowingKnife) sont **hors pipeline standard** jusqu'à une migration volontaire et justifiée.

Le catalogue contient **82 archétypes, 1 640 profils N1–N20, dont 260 passifs**. `NarrativeV1` contient 82 descriptions de prototype approuvées : cela **n'implique pas** que 82 DataAssets ont été créés. Les capacités notées `PLANNED` dans le lore ne sont pas exécutées.

## 2. Démarrage et contrôles sans écriture

Fermer Unreal Editor avant de lancer les scripts :

```powershell
cd D:\Development\GrimrockPrototype
git branch --show-current
git status --short
git pull --ff-only origin master

# Inventaire global de la production déjà déclarée
.\Scripts\WeaponPipeline.ps1 -EngineRoot D:\UE_5.5 -Action Audit -All

# Vérification de cohérence du manifeste, des DataAssets et des profils
.\Scripts\WeaponPipeline.ps1 -EngineRoot D:\UE_5.5 -Action Validate -All

# Export narratif en lecture seule depuis l'unique XLSX
.\Scripts\WeaponPipeline.ps1 -EngineRoot D:\UE_5.5 -Action Narrative
```

`Audit`, `Plan`, `Validate` et `Narrative` **sans `-Apply`** ne modifient pas les assets métier. Ils peuvent écrire rapports, logs et CSV exportés dans `Saved/`. Les profils à 20 niveaux sont déjà dans `DA_WeaponCatalogue` ; on ne régénère pas une classe C++ par niveau.

Rapports standards : `Saved/ARMEMENT-INDUS/INDUS02_<Action>_<Id|All>.csv`, les JSON et logs associés, `INDUS03_Narrative.csv`, puis `INDUS05_ActiveMatrix.csv` lors des tests.

## 3. Fabrication d'une arme standard (exemple ARM003, quand son art sera prêt)

**Préconditions :** ligne `ARM003` déclarée `Planned` puis basculée intentionnellement à `Active` seulement lorsque l'art et les données nécessaires sont disponibles ; chemins UE **réels et explicites** dans le manifeste. Les textures/meshes/MI sont préparés dans Unreal/Blender par la voie artistique. Ni un chemin inventé ni une capacité `PLANNED` ne créent un asset.

```powershell
# Préparation, strictement sans écritures .uasset
.\Scripts\WeaponPipeline.ps1 -Action Plan  -Archetype ARM003
.\Scripts\WeaponPipeline.ps1 -Action Build -Archetype ARM003

# Appliquer séparément, APRÈS préflight READY et vérification des chemins :
.\Scripts\WeaponPipeline.ps1 -Action Build -Archetype ARM003 -Stage Visual  -Apply
.\Scripts\WeaponPipeline.ps1 -Action Build -Archetype ARM003 -Stage Item    -Apply
.\Scripts\WeaponPipeline.ps1 -Action Build -Archetype ARM003 -Stage Palette -Apply

# Injecter dans le seul ItemDefinition existant sa description APPROVED :
.\Scripts\WeaponPipeline.ps1 -Action Narrative -Archetype ARM003
.\Scripts\WeaponPipeline.ps1 -Action Narrative -Archetype ARM003 -Apply

# Validation finale
.\Scripts\WeaponPipeline.ps1 -Action Validate -All
```

**Il s'agit d'un exemple de séquence, PAS d'une permission d'exécuter `-Apply` pour ARM003 aujourd'hui.** L'étape Visual produit/raccorde au plus `DA_WeaponVisualCatalogue`, Item au plus un `DA_Weapon_*`, Palette au plus `DA_ObjectPalette_Default`. Ces opérations ne sont **pas transactionnelles ensemble**. Refaire le préflight et vérifier Git après chaque étape, ne pas effacer ou réécrire des packages existants différents. Si une famille échappe aux quatre BuildKind (`MeleeOneHand`, `MeleeTwoHand`, `Ranged`, `ShieldPassive`), réaliser d'abord une revue du comportement spécifique sans forcer son passage dans le handler standard.

**Le placement dans un donjon et la recette PIE restent des opérations Unreal** et ne sont pas promis par le commandlet. Une validation automatisée n'est pas un ramassage/combat réellement observé.

## 4. Narratif intégré — fini les deux scripts parallèles

`WeaponNarrativeApply.ps1` et `WeaponNarrativePrototypeBatch.ps1` ont été supprimés par INDUS05.2. Le **même `WeaponPipeline.ps1`** expose désormais :

```powershell
# Export du schéma NarrativeV1 (82 lignes), sans écrire dans Content/
.\Scripts\WeaponPipeline.ps1 -Action Narrative

# Audit d'une seule arme : READY, PRESENT_SAME, BLOCKED_REVIEW ou CONFLICT
.\Scripts\WeaponPipeline.ps1 -Action Narrative -Archetype ARM031

# Import contrôlé d'une arme, seulement si son DataAsset existe et n'est pas dirty
.\Scripts\WeaponPipeline.ps1 -Action Narrative -Archetype ARM031 -Apply

# Audit de tous les archétypes Active du manifeste
.\Scripts\WeaponPipeline.ps1 -Action Narrative -All

# Import des actifs non modifiés, un asset à la fois, avec rapport groupé
.\Scripts\WeaponPipeline.ps1 -Action Narrative -All -Apply
```

- `-All -Apply` exige **82/82 lignes APPROVED** dans l'export du classeur. Le choix de cette approbation collective est une **décision explicite pour le prototype**, et ne vaut pas validation finale du lore ou des effets spéciaux.
- Un item en `Planned`, inexistant ou déjà dirty est respectivement `SKIPPED_PLANNED`, `SKIPPED_NOT_CREATED`, `SKIPPED_DIRTY` ; il **n'est jamais créé ni écrasé** par Narrative.
- `-Apply` sur un item à description différente existante reste bloqué par `CONFLICT`, à résoudre manuellement plutôt qu'écraser.
- Le commandlet interne `GridWeaponPipeline -Action=NarrativeItem` est réutilisé, sans nouveau commandlet. Un Build Editor a lieu **une seule fois par invocation** plutôt que pour chaque arme.
- Le batch inspecte les sauvegardes individuelles `SAVED`/`PRESENT_SAME`, les modifications hors cible et la suite `Grimrock.RPG.ARMEMENT.Industrial` après application. Les rapports individuels et `INDUS05_Narrative_All.csv/.json` restent dans `Saved/ARMEMENT-INDUS/`.
- `LoreFR`, `ParticulariteFR`, `NotesConception` restent non exécutables. Seule `DescriptionCourteFR` est publiée dans `UGridItemDefinitionAsset::Description` et affichée dans `Text_Description`.

La feuille `Narratif` se modifie **dans le XLSX**, pas dans un export provisoire. Les statistiques de combat ne doivent jamais être ajoutées au manifeste technique.

## 5. Campagne générique INDUS05.3

Les **quatre tests existants** sous le préfixe `Grimrock.RPG.ARMEMENT.Industrial` restent inchangés et vérifient manifeste, profils N1, items/visuels, palette et idempotence.

Trois tests supplémentaires communs `Grimrock.RPG.ARMEMENT.Industrial05` :

1. `ActiveN1ToN20TooltipAndNarrative` : parcourt **toutes** les lignes Active du manifeste et les vingt niveaux de chaque item, vérifie l'identité des définitions, la construction du read-model d'info-bulle N1–N20, le statut passif, le maximum nominal d'armure et `Description` non vide. Produit `Saved/ARMEMENT-INDUS/INDUS05_ActiveMatrix.csv`.
2. `LegacyThrowingItemsPreserved` : contrôle la présence non altérée de Shuriken et ThrowingKnife, volontairement hors manifeste standard.
3. `SinglePublicPipeline` : exige le point d'entrée générique et vérifie que six anciens scripts (quatre recettes par arme + deux scripts narratifs) ont disparu.

**Total attendu : sept tests** sous le préfixe `Grimrock.RPG.ARMEMENT.Industrial`, puis les suites de non-régression UI et RPG. La compilation/l'automatisation sur le poste local n'a **pas encore été validée** pour INDUS05.

```powershell
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.RPG.ARMEMENT.Industrial"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.UI.WeaponTooltip04"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.UI.EquipmentTooltip043B"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.RPG.SHIELD02.4B"
git status --short
```

Pour l'intégration du pipeline lui-même, ajouter **l'exécution de `Audit -All`, `Validate -All`, `Narrative -All`** ci-dessus ; un `Narrative -All -Apply` idempotent est approprié **seulement** si le contenu déjà approuvé et publié dans les assets de production est inchangé. Aucun test de code ne prouve une transaction multi-asset ni une recette PIE.

## 6. Publication Git : rigueur sans micro-tickets

- `master` uniquement, commits atomiques (INDUS05.1, .2, .3, .4), jamais de branche/PR, jamais de `force push`.
- Vérifier `git status --short` avant et après `-Apply`. Aucun script ne fait `git add`, `git commit` ou `git push`.
- Ne committer que les DataAssets intentionnellement produits et validés, jamais `git add -A` si l'Editor a modifié d'autres fichiers.
- Une recette PIE par **famille représentative**, pas un nouveau thread par arme : main unique, deux mains, arc, bouclier passif, objet spécifique/legacy.
- Comparaison et protection du bouclier : ARM075 N1 possède un maximum nominal de **5 points**, une réserve restante variable `0..5`, rechargée après victoire normale ou repos explicite ; ne pas convertir ces points en durabilité ou inventer un bonus additionnel.

## 7. Exceptions et documents historiques

**Conservés :** tous les contenus UE `.uasset`, assets shuriken/couteau de lancer, le registre/palette, les profils N1–N20 et les spécifications C++ encore incluses par des tests ARMEMENT01. Leurs fichiers restent utiles tant que l'équivalence des tests n'a pas été démontrée ; **ne pas supprimer des spécifications parce que leur commandlet a été retiré**.

**Retirés par INDUS05.1 :** quatre commandlets spécifiques (deux authoring + deux visual registration) et leurs en-têtes, quatre wrappers PowerShell correspondants. **Retirés par INDUS05.2 :** deux wrappers narratifs. Soit **14 fichiers obsolètes supprimés** sans suppression d'assets de jeu.

**Archives :** les nombreux `docs/Design/ARMEMENT01_*.md`, `docs/Design/ARMEMENT_INDUS03_*.md` et les notices INDUS04 conservent la traçabilité historique. Ils peuvent citer des scripts supprimés et des états `DRAFT` antérieurs : **ne pas suivre leurs commandes comme recettes actuelles**. Le présent fichier est le guide opérationnel, et `docs/Architecture/ARMEMENT_PIPELINE_CONTRACT.md` reste le contrat d'architecture. Les anciennes versions de ce guide sont accessibles via Git.

**Limites restantes :** art à produire pour les autres archétypes, déclaration progressive de lignes dans le manifeste, handlers pour d'autres BuildKind (armes de lancer spéciales), actions spéciales réellement programmées, localisation narrative multilingue, placement dans les niveaux et éventuels tests PIE par famille. Ces fonctionnalités **ne sont pas revendiquées comme achevées par INDUS05**.

## 8. Critères de clôture

INDUS05 sera **validé et clôturé** uniquement lorsque vous aurez fourni :
1. build Development Editor UE5.5.4 réussi ;
2. campagne Industrial **7/7** sans avertissement ni échec, puis non-régressions UI/RPG ;
3. `WeaponPipeline.ps1 -Action Audit -All` et `-Action Narrative -All` en lecture seule, sans changement métier ;
4. un `-Action Narrative -All -Apply` idempotent sur les huit assets déjà publiés, ou une vraie nouvelle arme `Planned → Active` avec art prêt et recette PIE ;
5. `git status --short` conforme (aucun changement inopiné).

Ne pas confondre **INDUS05 validé** et **82 armes jouables**. INDUS05 signifie que le **processus** pour en créer est uniforme, robuste et documenté.
## 9. ARMEMENT-LOT01 — état de préparation des armes de mêlée N1 (27 octobre 2026)

**Périmètre strict :** ARM003 (Stylet, Rogue/Saboteur, MainHand), ARM013 (Main-gauche, Rogue/Shadow, OffHand), ARM020 (Masse d'armes, Priest/Protection, MainHand). Ces trois placements N1 figurent dans les recettes `ARMEMENT01_9_6_STARTER_KIT_RECIPES.csv` avec `ApprovedDesign`. Les chiffres N1–N20 et les descriptions FR demeurent exclusivement dans le XLSX canonique, sans duplication ci-dessous.

**Audit du dépôt `master` à `30944215d2e8` (inspection des chemins versionnés, pas exécution UE) :**

| ID | Manifeste | Item produit | Mesh / icône / MI / textures PBR de production | Décision |
| --- | --- | --- | --- | --- |
| ARM003 | `Planned`, `MeleeOneHand`, `Rogue`, `MainHand` | Aucun | Aucun des 6 packages artistiques du manifeste présent | `ART_MISSING` attendu ; ne pas appliquer |
| ARM013 | Non déclaré | Aucun | Aucun asset identifié dans les dossiers canoniques Weapons | Compléter le manifeste uniquement après validation des véritables chemins artistiques et du contrat de slot |
| ARM020 | Non déclaré | Aucun | Aucun asset identifié dans les dossiers canoniques Weapons | Compléter le manifeste uniquement après validation des véritables chemins artistiques |

Pour ARM003 seulement, les noms et références **déjà inscrits au manifeste** sont : `Weapon_Stiletto_01`, `Attack_Stiletto_01`, `VIS_Stiletto_01`, `DA_Weapon_Stiletto_01`, `SM_Weapon_Stiletto_01`, `T_Weapon_Stiletto_01`, `MI_Stiletto`, ainsi que `T_Stiletto_01_BC`, `T_Stiletto_01_N` et `T_Stiletto_01_ORM` dans `Art/Textures/Final/Weapons/Stiletto/`. Ces **références Planned ne prouvent pas l'existence des packages**. Ne pas attribuer d'identifiants ou de chemins devinés à ARM013 et ARM020.

**Cahier technique commun pour Blender et Unreal, directement imposé par `GridWeaponPipelineCommandlet::LoadArt` :**

- Véritable `UStaticMesh` importé, au moins un LOD, bounds finis et demi-étendue maximale d'au moins 5 cm ; matériau non vide dans les Static Material Slots.
- Véritable `UMaterialInstance` avec parent valide et **affectée à un slot du mesh** (un nom de MI dans le manifeste ne suffit pas).
- Véritable icône `UTexture2D` carrée, dimension importée d'au moins 256 × 256 ; inspection du rendu à 48 × 48 dans l'interface.
- Trois textures `UTexture2D` PBR réellement importées : Base Color (`sRGB=true`), Normal (`sRGB=false`), ORM (`sRGB=false`) ; dimensions valides et chemins explicites dans le manifeste.
- Art à distinguer visuellement : lame d'estoc étroite pour ARM003, arme secondaire avec silhouette de garde identifiable pour ARM013, tête contondante sur manche pour ARM020. **Ces indications sont uniquement des intentions graphiques, non de nouvelles statistiques ni actions de combat.** Ajuster échelle, pivot, tenue en main et matériaux par inspection des pilotes validés, puis valider dans Unreal.

**ARM013 — vérifications techniques impératives :** le parseur actuel accepte une arme offensive déclarée `MainHand;OffHand`, mais pas un champ `OffHand` seul ; ne pas choisir les compatibilités sans confirmation du contrat de l'arme. `CanEquipToSlot()` autorise généralement une arme à une main dans les deux mains, tandis que `CanProvideAttackFromSlot()` exige le slot dans `CompatibleEquipmentSlots` et une action offensive valide. Vérifier l'attaque réelle depuis `OffHand` via le catalogue d'actions et le combat en PIE. La particularité narrative « Parade en main secondaire » est `PLANNED`, **jamais** une action automatique ou un talent N2 de départ.

**Ordre de production sans substitution :**

1. Faire l'inventaire local non destructif : `WeaponPipeline.ps1 -Action Audit -All` ; exécuter la suite existante `Grimrock.RPG.ARMEMENT01.11.1` (elle couvre les trois armes, y compris OffHand et les identités N1).
2. Produire/importer l'art authentique par arme dans Blender/Unreal, puis vérifier ses ressources et l'éventuelle occupation de packages avant toute écriture. ARM013/ARM020 nécessitent ensuite leurs lignes `Planned` dans le manifeste avec chemins réellement approuvés. Ne basculer à `Active` qu'après préflight et revue du contenu.
3. Réutiliser exclusivement `WeaponPipeline.ps1` : `Plan`, `Build` sans `-Apply`, puis `Build -Stage Visual|Item|Palette -Apply` séparément, en n'exécutant chaque étape que lorsque son préflight est `READY`. Appliquer `Narrative -Archetype ARMxxx -Apply` sur le DataAsset réellement créé, uniquement depuis `NarrativeV1 APPROVED` du XLSX.
4. Vérifier `Validate -All`, la suite générique `Grimrock.RPG.ARMEMENT.Industrial`, les tests `Grimrock.RPG.ARMEMENT01.11.1`, les tests UI comparatifs et SaveGame existants, puis PIE : attaque/PA, comparaison avec arme équipée/main vide, infobulle FR, ramassage/équipement/dépôt, sauvegarde et rechargement. Pour ARM020, tester conjointement ARM075 ; pour ARM013, tester explicitement OffHand. Ne pas attribuer de talent de niveau 2.
5. Contrôler `git status --short`, sélectionner uniquement les packages voulus, effectuer **un commit atomique maximum** par étape validée, puis pousser `master`. Les 18 nouveaux kits ne sont pas activés automatiquement par ce lot.

**Verdict de cet audit distant :** 0/3 armes fabriquées dans les répertoires canoniques ; aucune phase `-Apply` ni validation UE5.5.4/PIE effectuée par cet audit. Le classeur XLSX et les contenus binaires des catalogues doivent être inspectés par le pipeline local pour confirmer les valeurs et textes réellement projetés.
