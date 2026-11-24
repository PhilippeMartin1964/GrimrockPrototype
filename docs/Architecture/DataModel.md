# Modèle relationnel des objets — CLEANUP04-D

Référence fonctionnelle et recette ARM-CLEAN06 : `420dce2ee4f55c3630e8a99fa7ae9d77299a461e`, `master`, 22 novembre 2026. Nomenclature actualisée dans CLEANUP02 et finalisée pour les trois classes d’objets dans CLEANUP04-D.16. [Diagramme SVG](DataModel.svg) · [Consolidation et preuves](../Technical/Consolidation.md).

## Lecture du modèle

Mise à jour CLEANUP02 : noms internes simplifiés, sans évolution du schéma ou des identités. La référence fonctionnelle reste `420dce2e`. 24 types réfléchis et dix propriétés portent désormais des noms métier avec redirections de compatibilité. Les 53 assets sont organisés sous `Items/`, avec conservation des 53 anciens chemins par redirecteurs natifs et de 50 Primary Asset IDs dérivés par alias. Les identités métier restent inchangées. Les anciens chemins restent aussi couverts en Shipping par 53 Core Asset Redirects exacts (chemins d’objet), car les `UObjectRedirector` ne sont pas embarqués comme packages IoStore.

Le SVG initial était un modèle relationnel de 29 entités et 37 relations. Ces 29 entités sont conservées, corrigées ou identifiées comme projections de champs embarqués. Le modèle enrichi comprend 47 entités ou sous-ensembles, répartis en six panneaux. Ce découpage ne change pas le modèle relationnel : les références `FK → [panneau]` relient les domaines sans faire traverser les autres entités par des traits.

- **Vert** : déclaration implémentée, confrontée aux sources et aux preuves disponibles. Cela ne signifie pas que toutes les gammes de contenu sont créées, ni que la recette graphique est terminée.
- **Ambre** : représentation partielle ou en transition ; les actions et présentations compactes/détaillées coexistent. La jointure compacte des statuts reste moins explicite que leurs références détaillées.
- **Gris, bordure pointillée** : proposition future, sans classe, propriété, structure, enum, DataAsset ou mécanique créé dans CLEANUP01.

`PK` et `FK` désignent des identités logiques, pas une base SQL. Une flèche indique une référence ou une composition ; un triangle vide indique l'héritage C++. Les sous-structures embarquées n'ont pas de clé autonome inventée. Le SVG contient du texte et des tracés vectoriels éditables, ainsi que les sources Graphviz de chaque panneau dans `metadata#editable-model-source`. Aucun bitmap n'est embarqué. Pour une édition structurée, extraire ces six sources, modifier les libellés/relations, régénérer les groupes puis contrôler le rendu ; les coordonnées restent également modifiables dans un éditeur SVG.

## A — Identités, définitions et registre

`UGridItemSystemAsset`, matérialisé par `DA_ItemSystem`, référence les définitions et quatre catalogues globaux. `UGridItemSubsystem` charge cette racine depuis `ConfiguredItemSystem`, la valide une fois par GameInstance et construit `ItemIndex`. Les vues `FGridResolvedItem` et `FGridResolvedBoundAction` sont transitoires. Une identité ou un niveau inconnu échoue explicitement ; aucune ancienne définition ne doit compléter un objet enregistré.

La base abstraite `UGridItemDefinitionAsset` contient exactement les cinq champs communs `ItemId`, `DisplayName`, `Description`, `WeightKg`, `Tags`. Une définition d'armement ajoute seulement `ArchetypeId`. La racine référence aujourd'hui **48 définitions**, dont **22 armements** ; ce n'est pas le nombre d'archétypes du catalogue.

| Classe concrète actuelle | Assets | Champs propres / rôle |
| --- | ---: | --- |
| `UGridArmamentDefinitionAsset` | 22 | `ArchetypeId`, jointure vers le catalogue ARM |
| `UGridNonArmamentDefinitionAsset` directement | 7 | Visuels, lecture, lumière, physique et actions non ARM |
| `UGridConsumableDefinitionAsset` | 12 | `MaxStack`, `UseActions` |
| `UGridAmmoDefinitionAsset` | 1 | `AmmoKindId`, `MaxStack` |
| `UGridGenericStackableDefinitionAsset` | 3 | `MaxStack` pour objets génériques empilables |
| `UGridOtherEquipmentDefinitionAsset` | 3 | `EquipmentSlots`, `Bonuses`, `HandPolicy`, `Resistances`, `EquippedMesh`, `MaxStack` |
| **Total** | **48** | Toutes sous la même identité de base |

Les quatre dernières classes héritent de `UGridNonArmamentDefinitionAsset`. `GENERIC_ITEM_VISUAL` de l'ancien SVG correspond donc à des champs de cette classe, pas à une table indépendante 1:1 attachée à tous les objets. Les visuels non ARM n'appartiennent pas au catalogue visuel ARM. Les consommables disposent encore de `UseActions` ; la validation interdit de remplir simultanément cette liste et `ActionBindings` héritée.

Les assets sont organisés sous `Content/GrimrockPrototype/Core/DataAssets/Items/` : racine `DA_ItemSystem`, quatre catalogues dans `Catalogues/` (`DA_WeaponCatalogue`, `DA_WeaponVisualCatalogue`, `DA_ActionCatalogue`, `DA_ActionPresentationCatalogue`) et 48 définitions dans `Definitions/`, notamment `DA_Weapon_ThrowingKnife_01`. `CanonicalStage/` ne contient plus que les 53 redirecteurs de compatibilité ; aucune seconde définition n’y est conservée. Le Cook inclut les assets actifs et les deux Readables protégés dans `Tests/Readables/`. Les anciens chemins cuisinés passent par les redirections d’objet. Les Primary Asset IDs dérivés du nom ont 50 redirections ; aucun ItemId, ActionId, ArchetypeId ou GUID n’est renommé. Les trois noms définitifs appartiennent aux classes actives. Les anciens noms actifs se résolvent par trois Core Redirects de classe. Les types Asset Manager des deux catalogues restent `GridCanonicalWeaponCatalogueAsset` et `GridCanonicalWeaponVisualCatalogueAsset`.

## B — Armement, progression et classes

`DA_WeaponCatalogue` contient **82 archétypes** et **6 préférences de classe**. Les 22 définitions ARM actuelles n'en matérialisent qu'une partie ; la présence d'une ligne numérique ne prouve pas sa disponibilité complète en gameplay. Le rapport natif de retrait résout 466 couples objet/niveau : 22 × 20 ARM + 26 non ARM au niveau zéro.

`FGridWeaponArchetype` porte l'autorité mécanique : `Kind`, `HandPolicy`, `AttackSourceSlots`, type/sous-type de dégâts, attribut d'échelle, précision, PA, portée, N1, croissance, réactions, projectile, affinités et bindings d'actions. `bHandAndKindProven` constitue un garde explicite. Équiper en main gauche ne confère pas automatiquement une attaque en main gauche.

La progression N1..N20 est une projection des bases `DamageDiceN1`, `PhysicalArmorN1`, `MagicalArmorN1` et de cinq `FGridWeaponIntegerGrowthRule`. Chaque règle contient `StepEveryLevels`, `StepAmount` et ses `Exceptions` ; chaque exception est `FGridWeaponGrowthException { ItemLevel, Value }`, correction absolue. Le `StatKey` et le champ `MaxLevel_20` de l'ancien SVG n'existent pas. La règle contenante identifie la statistique ; les bornes de niveau sont un contrat de résolution.

Les affinités sont `PrimaryClassId`, `SecondaryClassIds`, `SecondarySourceLabel` dans l'archétype. Les six `FGridClassPreference` sont des descriptions/conseils de classe dans le catalogue. Il n'existe pas de table `CLASS_AFFINITY` avec enum `RecommendationTier`. Ces conseils ne sont pas des prérequis d'équipement.

`FGridReactionPolicy` porte `ReactionId`, `Trigger`, `Chance`, `Absorption`, sans `ReactionKind` ou `AbsorptionPolicy`. Le projectile mécanique porte activation, paramètres de lancer générique, vitesse, arc, durée et décalage de retombée ; la collision relève des acteurs et de leurs politiques, sans champ `CollisionBehavior` dans cette structure.

## C — Autorités visuelles

`DA_WeaponVisualCatalogue` contient `ArchetypeBindings`, `VisualProfiles`, `ActionIcons`. Une liaison joint `ArchetypeId` à `VisualId`. Le profil possède icône d'objet, meshes monde/équipé/décoration, transformations, finitions, lumière, VFX permanents, scintillement et projectile visuel. `ProjectileAppearance` distingue pose, axes, rotation stable/tumble/spin ; la balistique reste dans le domaine mécanique.

`ActionIcons` est une map de **12 catégories** `EGridEquippedWeaponActionIconKind` vers des textures. Cette map n'est ni une définition d'action ni l'icône de l'objet. L'ancienne palette `DA_EquippedWeaponActionIcons` est retirée ; son absence est contrôlée par Automation. Une catégorie d'icône est dérivée, sans relation FK persistée depuis chaque binding.

Les noms de propriétés exacts remplacent `VisualBindings`, `CosmeticProgression`, `ProjectileVisual` et `IdleLightAndVFX` du SVG initial : respectivement `ArchetypeBindings`, `LevelFinishIds`/décoration, `ProjectileMesh`/`ProjectileAppearance`, `LightEmitter`/`VisualEffects`/scintillement. Le `WorldSparkle*` du SVG abrège les propriétés explicites `bEnableWorldSparkle`, `WorldSparkleMaterial`, `WorldSparkleColor`, `WorldSparkleIntensity`, `WorldSparkleSpeed`, `WorldSparkleVariation`.

## D — Actions, effets et présentation

Le propriétaire d'un `FGridItemActionBinding` est un archétype ARM, une définition non ARM ou un consommable. Chaque binding contient seulement `ActionId` et `PresentationId`.

`DA_ActionCatalogue` possède deux collections effectivement utilisées : `Actions` (`FGridAction`) et `DetailedActions` (`FGridRuntimeAction`, contenant `FGridCombatActionDefinition`). La validation cherche l'identité dans les formes autorisées ; `ResolveDetailedNonArmAction` exige la forme détaillée, valide sa présentation et son identité. La preuve native constate 14 actions détaillées. Ces formes sont signalées en ambre : il faut démontrer leurs rôles et la parité avant de supprimer ou fusionner une représentation.

L'action compacte possède `Kind`, `Targeting`, les flags de portée/ligne de vue, `SourceItemQuantityCost`, `ManaCost`, `CooldownRounds`, `WeaponDamagePercent`, `Effects`. Chaque `FGridActionEffect` possède `Kind`, `Magnitude`, `StatusId` ; le `TypedPayload` initial n'existe pas ici. Le catalogue valide la présence de `StatusId` pour les opérations de statut, pas une table SQL de statuts. `UGridStatusEffectDefinitionAsset` utilise **`EffectId`**, avec durée, cumul, tags, contrôle, périodicité, modificateurs et réactions. Les formes détaillées ont leurs propres payloads et références de statuts : ne pas prétendre qu'une simple chaîne remplace tous ces mécanismes.

`DA_ActionPresentationCatalogue` possède également `Presentations` et `DetailedPresentations`. Le profil compact porte mouvement, temporisations, offsets, dix groupes audio et cinq groupes VFX ; le détaillé contient `FGridPlayerAttackPresentationProfile` avec `PresentationId`. Ces données décrivent les événements réels ; elles n'accordent ni dégâts, ni PA, ni capacités. Sorts, talents et statuts conservent leurs autorités spécialisées ; ils ne deviennent pas des définitions d'objets.

Le tooltip d'inventaire délègue depuis `WBP_ItemTooltip.InitFromInventorySlot` à `UGridItemTooltipWidget::PresentFromInventorySlot`. La vue du slot fournit les textes, poids, caractéristiques et comparaisons à partir du registre actif et de l'instance. Le Designer conserve la mise en forme.

## E — Instances, possession et persistance

La clé d'une instance est `FGridItemInstance.RuntimeObjectId` (`FGuid`). Sa référence de définition s'appelle **`ItemDefinitionId`**, jointe à `ItemId`. `ItemLevel` appartient à l'objet physique ; il est distinct du niveau RPG du personnage. La résolution exige ARM N1..N20 et non ARM N0, même si `FGridItemInstance::IsValid` n'effectue qu'un contrôle structurel N0..N20.

L'instance conserve quantité, propriétaire, emplacement, réserves de protection déjà absorbées, lumière, position et variantes de lecture. Elle ne conserve plus de nom/poids/profil N1 servant d'autorité parallèle. Les deux réserves `AbsorbedEquipmentPhysicalArmor` et `AbsorbedEquipmentMagicalArmor` survivent aux transferts ; leur solde est recalculé depuis l'autorité statique et l'état dépensé.

`FGridPartyInventoryState` agrège personnages actifs, équipements, réserve de personnages et curseur. Un objet physique doit avoir une localisation cohérente : inventaire, équipement, curseur, monde ou réceptacle. `EGridItemOwnerType` comporte aussi `None`, `HeldBySelectedCharacter`, `Removed` ; ce sont des états du modèle, pas de nouveaux conteneurs à créer. Les slots stockent des valeurs d'instance ; l'unicité de possession repose sur les services de transfert et leurs validations, pas sur une référence UObject unique.

`FGridCharacterEquipmentState` possède 18 slots hors `None`. Un objet à deux mains occupe sa source physique et réserve virtuellement la seconde main : ne pas y créer un second GUID, une quantité ou une attaque. La hotbar référence l'action et, pour l'équipement, le GUID et l'emplacement source ; elle ne possède pas une nouvelle instance.

`UGridLevelAsset.LooseItemInstances` et `ItemSpawns` contiennent `ItemId`, niveau, quantité, identité de placement et coordonnées ; le placement libre ajoute transformation locale et lecture. Les réceptacles, serrures, loot et kits initiaux utilisent aussi les identités canoniques : ces producteurs ne sont pas de nouvelles autorités de définition. Les snapshots `FGridRuntimeItemState`, `FGridPendingWorldItemState`, `FGridRuntimeReceptacleState` sont agrégés par `FGridLevelRuntimeState`, puis `FGridDungeonRuntimeState`.

`UGrimrockPartySaveGame.CurrentSaveVersion = 27` exige la version exacte et valide les états au chargement. Il contient inventaire, donjon, quêtes, ordre des portraits, position du groupe et horloge logique. Le niveau RPG et des projections de personnage sont reconstruits ; les vues résolues d'objet et les catalogues ne sont pas des données persistées dans les instances.

**Exception de lecture à préserver :** `ReadableContentAsset` et `ReadableContentId`, plus les overrides d'instance, restent distincts du `ReadText` par défaut. Les assets `DA_BookPage_Test` et `DA_Scroll_Test` sont encore nommés dans les sauvegardes privées v27 ; leur ancienneté ne justifie pas leur suppression. La recette finale ARM-CLEAN06 a validé les sept sauvegardes v27, dont les deux versions plus récentes, ainsi que Save/Continue en PIE et Shipping. Le contrôle ultérieur CLEANUP04-B.1 a confirmé le chargement, la reconstruction et les round-trips sur sept copies isolées. Les originaux ont été préservés.

## Cardinalités entre panneaux

| Relation logique | Cardinalité et contrainte |
| --- | --- |
| Sous-système → racine | 0 ou 1 racine validée par GI ; une tentative de configuration |
| Racine → catalogue de chaque rôle | Exactement 1 pour chaque rôle après validation |
| Racine → définitions | 1 → 0..n structurellement ; 48 présentes, `ItemId` unique |
| Définition ARM → archétype | 0..n définitions → 1 archétype ; pas de catalogue dans chaque définition |
| Instance / placement → définition | 0..n → 1 définition connue ; une identité absente est une erreur |
| Archétype → binding visuel | 1 binding pour chaque archétype effectivement pris en charge ; les 82 lignes ne garantissent pas 82 objets prêts |
| Binding visuel → profil | 0..n → 1 profil existant ; `VisualId` unique dans les profils |
| Munition → kind | Une munition par `AmmoKindId` enregistré ; les doublons sont refusés |
| Archétype → kind requis | 0..n → 0..1 ; un kind requis doit être enregistré |
| Propriétaire d'actions → bindings | 1 → 0..n ; un binding résout action et présentation par leurs IDs |
| Personnage → équipement | Un état d'équipement actif associé par index ; un emplacement possède au plus une instance physique |
| Instance → contenu lisible | 0..n → 0..1 asset ; overrides éventuels dans l'instance |

## F — Équipement futur : extension documentaire

La gamme d'armures et vêtements n'est pas implémentée. Le socle générique d'équipement l'est, avec trois objets non ARM : torche, shuriken et pierre. Aucun asset actuel de casque, vêtement ou bijou n'est déduit de la présence d'un slot.

| Famille proposée | Slots déjà déclarés compatibles conceptuellement |
| --- | --- |
| Armure légère, intermédiaire, lourde | Catégorie de protection ; choisir ensuite `Chest`, `Legs`, etc. selon la pièce |
| Vêtements | `Shirt`, éventuellement `Chest` / `Legs` selon règle de couches à définir |
| Casques | `Head` |
| Épaulières | `Shoulders` |
| Protections corporelles | `Chest`, `Legs` ; aucune occupation de plusieurs slots implicite |
| Brassards / gants | `Bracers` / `Gloves` |
| Ceintures / bottes / capes | `Belt` / `Feet` / `Cloak` |
| Anneaux | `Ring1` ou `Ring2`, une même définition peut accepter les deux |
| Amulettes | `Amulet` |
| Accessoires | `Talisman`, `QuickSlot1`, `QuickSlot2` selon leur usage ; les mains restent régies par `HandPolicy` |

**À réutiliser :** les cinq champs communs, `RuntimeObjectId`, `EquipmentSlots`, `Bonuses`, `Resistances`, `EquippedMesh`, réserves dépensées et services de possession. `FGridEquipmentStatBonus` fournit déjà six attributs, santé/mana maximum, charge et protections physique/magique. `FGridDamageResistanceSet` couvre physique, feu, glace, foudre, poison, sacré, nécrotique et arcane. Ne pas créer un second registre ou dupliquer poids, noms, icônes et protections sur l'instance.

**À définir ultérieurement :** catégorie/sous-catégorie explicite, qualité, prérequis, éventuelle progression des équipements non ARM, bonus de compétences, passifs conditionnels, conditions de cumul/retrait, conflits de couches corporelles et variantes visuelles. Les tags actuels ne constituent pas un schéma validé de ces nouvelles propriétés. Une qualité fixe appartiendrait à la définition/variante ; une qualité tirée ou modifiée nécessiterait un état d'instance et une politique de sauvegarde. Ce choix n'est pas arrêté.

Le niveau physique non ARM est aujourd'hui strictement zéro : appliquer une progression de niveau aux futures armures exigerait une évolution explicite du contrat, des validateurs et de la persistance. Les passifs/réactions ARM et les modificateurs de statuts existants ne prouvent pas un mécanisme générique d'effets au port d'une armure. Toute extension durable devra avoir une autorité unique, une version et une conversion sur copies ; aucun de ces travaux n'est engagé ici.

## Sources de contrôle

Les chemins suivants sont relatifs à la racine du dépôt :

- `Source/GrimrockPrototype/Public/Runtime/Items/GridItemDefinitionAsset.h` : héritage, cinq champs, spécialisations et slots.
- `Source/GrimrockPrototype/Public/Runtime/Items/GridItemCatalogues.h` et son `.cpp` privé : catalogues, structures, validations et jointures.
- `Source/GrimrockPrototype/Public/Runtime/Items/GridItemSubsystem.h` et son `.cpp` privé : configuration, niveaux, projection et actions détaillées.
- `Source/GrimrockPrototype/Public/Runtime/GridInventoryTypes.h` : instance, possession, résistances, bonus et 18 slots.
- `Source/GrimrockPrototype/Public/Runtime/Combat/GridWeaponProgressionTypes.h` : règles de croissance partagées et profil de niveau, détachés de la classe de catalogue historique.
- `Source/GrimrockPrototype/Public/Core/GridLevelAsset.h`, `GridLevelPlacementTypes.h`, `Runtime/GridDungeonRuntimeState.h` : placements et snapshots.
- `Source/GrimrockPrototype/Public/Save/GrimrockPartySaveGame.h` et son `.cpp` privé : version et contrôles de chargement.
- `Source/GrimrockPrototype/Public/RPG/StatusEffects/GridStatusEffectDefinitionAsset.h` : identité `EffectId` et payloads de statut.
- `Config/DefaultGame.ini` : chemin de `DA_ItemSystem` et alias des Primary Asset IDs. Les assets actifs résident sous `Content/GrimrockPrototype/Core/DataAssets/Items/`.

Les packages source de la racine et des deux catalogues conservent leurs anciens imports actifs. Unreal les résout par Core Redirects au chargement. Les packages cuisinés utilisent les noms réfléchis définitifs des deux catalogues. L'inspection binaire statique seule ne prouve pas les valeurs de chaque propriété : la suite native réussie sur le SHA de référence et le rapport natif privé complètent cette vérification. Aucun commandlet de conversion ou de réparation n'a été exécuté pour CLEANUP01.

## Traçabilité des 29 entités du SVG initial

Les noms relationnels sont conservés comme repères documentaires ; les types ci-dessous portent les noms C++ actuels. Les champs obsolètes sont expliqués dans les sections A à E.

| Entité initiale | Correspondance actuelle / porteur |
| --- | --- |
| `ITEM_SYSTEM` | UGridItemSystemAsset · DA_ItemSystem |
| `ITEM_DEFINITION` | UGridItemDefinitionAsset · abstraite |
| `WEAPON_CATALOGUE` | UGridWeaponCatalogueAsset |
| `WEAPON_VISUAL_CATALOGUE` | UGridWeaponVisualCatalogueAsset |
| `ACTION_CATALOGUE` | UGridActionCatalogueAsset · DA_ActionCatalogue |
| `PRESENTATION_CATALOGUE` | UGridActionPresentationCatalogueAsset |
| `ARMAMENT_DEFINITION` | UGridArmamentDefinitionAsset · 22 assets |
| `CONSUMABLE_DEFINITION` | UGridConsumableDefinitionAsset · 12 assets |
| `AMMO_DEFINITION` | UGridAmmoDefinitionAsset · 1 asset |
| `OTHER_EQUIPMENT_DEFINITION` | UGridOtherEquipmentDefinitionAsset · 3 assets |
| `GENERIC_ITEM_VISUAL` | UGridNonArmamentDefinitionAsset · 7 assets directs |
| `WEAPON_ARCHETYPE` | FGridWeaponArchetype |
| `ITEM_ACTION_BINDING` | FGridItemActionBinding |
| `AMMO_KIND` | FName AmmoKindId / RequiredAmmoKindId |
| `CLASS_AFFINITY` | Champs de FGridWeaponArchetype |
| `LEVEL_PROGRESSION` | N1 + 5 FGridWeaponIntegerGrowthRule |
| `REACTION_POLICY` | FGridReactionPolicy |
| `PROJECTILE_POLICY` | FGridProjectilePolicy |
| `PROGRESSION_EXCEPTION` | FGridWeaponGrowthException |
| `VISUAL_BINDING` | FGridWeaponVisualBinding |
| `VISUAL_PROFILE` | FGridWeaponVisual |
| `ACTION_ICON` | EGridEquippedWeaponActionIconKind → texture |
| `ACTION_DEFINITION` | FGridAction |
| `ACTION_EFFECT` | FGridActionEffect |
| `STATUS_EFFECT_ASSET` | UGridStatusEffectDefinitionAsset |
| `PRESENTATION_PROFILE` | FGridActionPresentation |
| `ITEM_INSTANCE` | FGridItemInstance |
| `LEVEL_ASSET` | UGridLevelAsset · DA_GridLevel_* |
| `ITEM_PLACEMENT` | FGridLooseItemInstance / FGridItemSpawnInstance |
