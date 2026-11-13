# ARM-CLEAN05 — Audit contradictoire des DataAssets d’armes et du C++

> **ARM-CLEAN06 — NOUVELLE AUTORITÉ (13.11.2026).** [Modèle cible approuvé](ARM_CLEAN06_ITEM_SYSTEM_NORMATIVE.md) · [Migration](ARM_CLEAN06_MIGRATION_PLAN.md). **Audit ou jalon de l'ancien modèle ; ne constitue plus l'architecture cible.** **Cible non encore implémentée ; aucun legacy dans le runtime final.**

**13 novembre 2026 — Audit statique de master au commit 2baa68e4d708bb8d27b8b8f95e0a93a8ec61a6f4.**
**Verdict : NON VALIDÉ sur l’architecture des armes.** La validation UE5.5.4 passée garantit compilation et scénarios Automation couverts, pas élimination des doublons, cohérence de toutes les lectures ni simplicité d’édition.

## Périmètre, preuves et limites
- `UGridItemDefinitionAsset` définit 51 UPROPERTY directes. Une expansion statique de ses structures imbriquées expose 542 chemins possibles (y compris les deux exemplaires distincts de FGridCombatActionDefinition : CombatActions et QuickItemCombatAction). Les tableaux vides ne matérialisent évidemment pas tous ces champs dans un asset donné.
- Structures consultées : `GridItemDefinitionAsset.h`, `GridCombatTypes.h`, `GridInventoryTypes.h`, `GridPlayerAttackPresentationTypes.h`, `GridLightEmitterTypes.h`, `GridItemVisualEffectsTypes.h`. Les lecteurs et writers sont identifiés dans les sources C++ signalées plus loin.
- La comparaison des sources C++ et la validation UE Automation sont des preuves de comportement de code, PAS un dump complet des valeurs non-default des 22 DA_Weapon existants. Cet audit NE PRÉTEND PAS avoir décodé leurs fichiers binaires pour compter chaque champ effectivement renseigné.
- Pour chacune des propriétés du schéma, voir en complément l’inventaire mécanique exhaustif `ARM_CLEAN05_EVERY_EXPOSED_PROPERTY.md` ; il donne chemin, type, déclaration et classification sans prétendre qu’un champ est supprimable parce qu’il ne serait pas trouvé par une simple recherche textuelle.

## Pourquoi l’inspecteur est saturé
1. **Modèle universel unique** : même `UGridItemDefinitionAsset` pour arme, bouclier, torche, potion, bombe, flèche, livre, etc.
2. **Structure universelle d’action** : `CombatActions` contient une liste de `FGridCombatActionDefinition` (44 paramètres directs), laquelle propose de nombreux sous-systèmes de magie, surfaces, effets, pièges et soins.
3. **Double source d’autorité** : données complètes N1 dans les objets, catalogue statistiques ARMxxx, catalogue visuel, plus icônes dédiées aux actions dans une palette.
4. **Fallback legacy permanent** : les champs `Icon`, `WorldMesh`, `EquippedMesh` restent lus même pour des armes de catalogue.
5. **ARM-CLEAN04 ne fait pas ce qui était attendu** : il ajoute un résumé et modifie quelques priorités de catégories, sans éliminer le schéma de 51 champs ni l’arbre complet d’action.

## Classement utilisé
- **GARDER** : information nécessaire et légitime, souvent à présenter une seule fois.
- **DUPLICAT** : valeur recopiée ou concurrente ; migration vers source unique avant retrait.
- **LEGACY** : toujours lu dans des branches de repli ; interdit de supprimer sans réécrire lecteurs et tests.
- **OPTIONNEL / AVANCÉ** : valide pour certaines capacités rares, mais hors édition d’une arme standard.
- **HORS ARME** : nécessaire au modèle universel pour d’autres items, à masquer dans une fiche d’arme.
- **INCOMPLET** : déclaration présente mais l’usage annoncé n’est pas prouvé dans le chemin runtime de la frappe.

## I. 51 propriétés de UGridItemDefinitionAsset, une par une
| # | Propriété | Usage et justification d’existence | Décision pour DA_Weapon |
|---:|---|---|---|
| 1 | `ItemDefinitionId` | Clé stable de définition, référencée par l’inventaire, les saves et les actions. | **GARDER comme identifiant technique, distinct de ARMxxx.** |
| 2 | `DisplayName` | Nom de l’objet affiché, mais recopié de la base N1 du catalogue lors de BuildItem. | **DUPLICAT : choisir une seule source localisable, calculer dans la vue de l’item.** |
| 3 | `Description` | Texte descriptif de l’objet, réutilisé comme repli de description d’action. | **GARDER un texte par objet, mais ne pas le recopier dans les actions simples.** |
| 4 | `ReadText` | Texte intégral à lire, prévu pour livre, parchemin et objet lisible. | **HORS ARME standard ; masquer, sans supprimer du type d’item générique.** |
| 5 | `ItemType` | Discrimine Weapon, Shield, Armor, etc. pour la logique et le sac. | **GARDER en runtime ; dériver de l’archétype/manifeste pour une arme standard.** |
| 6 | `Weight` | Masse de définition : l’inventaire crée un poids d’instance depuis WeightKg, mais plusieurs chemins (lancer, filtre de sac, plaques) lisent encore Weight. | **DUPLICAT et lectures à unifier. Attention : WeightKg est actuellement invariant de N1 à N20.** |
| 7 | `bStackable` | Autorise l’empilement des objets (flèches, potions). | **HORS ARME unitaire : false imposé par IsValidDefinition ; masquer.** |
| 8 | `MaxStackSize` | Nombre maximal d’objets dans une pile. | **HORS ARME unitaire : 1 ; masquer.** |
| 9 | `HandUsage` | Règle de mains exploitable en C++ par GetEffectiveHandUsage et l’équipement. | **GARDER la règle typée, mais supprimer sa concurrence avec HandUsage texte du catalogue.** |
| 10 | `CompatibleEquipmentSlots` | Liste concrète des slots autorisés, utilisée pour les mains et objets portés. | **GARDER contrainte ; simplifier à un choix main gauche/droite/deux mains dans éditeur arme.** |
| 11 | `EquipmentStatBonus` | Onze bonus d’attributs, PV/PM, portage, armure. | **OPTIONNEL pour enchantements ; ArmorBonus/MagicalArmorBonus sont des DUPLICATS de la protection cataloguée.** |
| 12 | `EquipmentResistanceBonus` | Huit résistances élémentaires/physiques additionnelles. | **OPTIONNEL pour équipement enchanté ; masquer pour l’arme standard sans bonus.** |
| 13 | `CombatActions` | Tableau d’actions de 44 champs directs et de nombreux profils imbriqués, conçu pour tout le système RPG. | **TROP LARGE : remplacer en authoring d’arme par liste d’actions simples et profils spéciaux ; conserver moteur universel.** |
| 14 | `bUseWeaponCatalogue` | Commutateur de transition entre objet legacy et arme liée à ARMxxx. | **LEGACY de migration : inférable de l’identité d’archétype/type après bascule contrôlée.** |
| 15 | `WeaponArchetypeId` | Clé ARM001..ARM082 vers les données de combat et visuelles. | **GARDER ; unique lien indispensable entre fiche et archétype.** |
| 16 | `RequiredAmmunitionItemDefinitionId` | Clé des munitions consommées sur attaque distance approuvée. | **GARDER uniquement pour arc/arbalète/arme à munitions ; masquer ailleurs.** |
| 17 | `DefaultWeaponItemLevel` | Niveau assigné une fois à la création d’une instance niveau 0. | **GARDER comme option de création exceptionnelle ; défaut N1 global pour la majorité.** |
| 18 | `WeaponCatalogue` | Soft reference au catalogue statistique, répétée sur chaque arme. | **DUPLICAT de configuration : déplacer vers registre/service central ; ne supprimer qu’après migration.** |
| 19 | `bProvidesQuickItemCombatAction` | Active l’action depuis l’inventaire (consommables). | **HORS ARME standard ; lancer d’arme crée déjà sa quick-action par normalisation.** |
| 20 | `QuickItemCombatAction` | Seconde définition complète d’action générique consommable. | **HORS ARME standard ; ne pas exposer si bProvidesQuickItemCombatAction=false.** |
| 21 | `QuickItemActionIdOverride` | ID fixe d’une action quick-item spéciale. | **HORS ARME standard ; utile potions/flacons particuliers.** |
| 22 | `bProvidesAttackPresentation` | Active le profil audiovisuel de frappe. | **GARDER une activation implicite lorsqu’un profil est assigné ; bool éditable redondant à terme.** |
| 23 | `PlayerAttackPresentationProfile` | Animation held-item, temps, trois phases audio et trois VFX. | **GARDER les effets fonctionnels ; déplacer vers profil référencé/preset, variantes par action si requis.** |
| 24 | `bUseWeaponVisualCatalogue` | Commutateur entre ancien trio d’assets directs et catalogue visuel. | **LEGACY de migration ; éviter deux autorités sur arme de production.** |
| 25 | `WeaponVisualCatalogue` | Soft reference au même catalogue visuel sur les armes de série. | **DUPLICAT de configuration ; résoudre globalement via WeaponArchetypeId.** |
| 26 | `Icon` | Icône d’item local ; actuellement copiée du catalogue visuel et fallback. | **LEGACY / DUPLICAT pour arme cataloguée ; conserver seulement pour items hors catalogue.** |
| 27 | `WorldMesh` | Mesh au sol direct, copié du catalogue visuel et utilisé en fallback. | **LEGACY / DUPLICAT pour arme cataloguée ; garder pour autres items.** |
| 28 | `EquippedMesh` | Mesh direct de l’arme tenue ; LoadHeldMesh en est encore dépendant. | **LEGACY ACTIF : corriger le circuit held-item AVANT suppression.** |
| 29 | `bEnableWorldSparkle` | Active un matériau scintillant sur l’objet au sol. | **OPTIONNEL présentation de loot, indépendant du système attaque.** |
| 30 | `WorldSparkleMaterial` | Matériau de surbrillance de loot. | **OPTIONNEL, ne vaut que si WorldSparkle activé.** |
| 31 | `WorldSparkleColor` | Couleur du scintillement au sol. | **OPTIONNEL, idéalement profil/rareté commun.** |
| 32 | `WorldSparkleIntensity` | Amplitude lumineuse du scintillement. | **OPTIONNEL, idéalement preset commun.** |
| 33 | `WorldSparkleSpeed` | Vitesse temporelle du scintillement. | **OPTIONNEL, idéalement preset commun.** |
| 34 | `WorldSparkleVariation` | Variation/irrégularité de pulsation du matériau. | **OPTIONNEL, profil de loot plutôt que arme standard.** |
| 35 | `bUseItemWeightAsWorldPhysicsMass` | Utilise la masse pour Chaos quand l’item est objet libre. | **OPTIONNEL physique monde ; risque si poids source divergent.** |
| 36 | `WorldPhysicsInitialTiltDegrees` | Petite inclinaison déterministe au lâcher/placement initial. | **OPTIONNEL physique monde ; cacher derrière profil de ramassage.** |
| 37 | `bCombatThrowWeapon` | Autorise la consommation/lancement d’une arme comme projectile récupérable. | **GARDER uniquement pour familles jet (et cas explicites).** |
| 38 | `ThrowSpeed` | Vitesse initiale nominale du projectile. | **OPTIONNEL ; profil commun par famille de lancer.** |
| 39 | `ThrowArc` | Arc / composante balistique du lancer. | **OPTIONNEL ; preset de famille.** |
| 40 | `ThrowLifeSeconds` | Durée de vie du projectile en vol. | **OPTIONNEL ; preset de famille.** |
| 41 | `ThrowImpactDropOffset` | Décalage de chute/pose après impact. | **OPTIONNEL ; preset de projectile.** |
| 42 | `ThrowVisualRelativeRotation` | Correction de rotation du mesh en vol. | **OPTIONNEL ; correction asset import/preset.** |
| 43 | `ThrowVisualRelativeScale` | Échelle du mesh en vol. | **OPTIONNEL ; correction asset import/preset.** |
| 44 | `ProjectileMeshForwardAxis` | Axe local de pointe du mesh importé. | **OPTIONNEL ; propriété du mesh/preset, pas de dégâts.** |
| 45 | `ThrowVisualMode` | Vol orienté, tumbling ou rotation libre. | **OPTIONNEL ; profil de lancer.** |
| 46 | `ThrowVisualTumbleAxis` | Axe de roulis visuel. | **OPTIONNEL ; profil de lancer.** |
| 47 | `ThrowVisualTumbleDegreesPerSecond` | Vitesse de roulis. | **OPTIONNEL ; profil de lancer.** |
| 48 | `ThrowVisualSpinDegreesPerSecond` | Vitesse de rotation continue. | **OPTIONNEL ; profil de lancer.** |
| 49 | `LightEmitter` | 21 paramètres de lumière, scintillement, couleur et décalages. | **HORS ARME standard ; OPTIONNEL pour torche, arme lumineuse ou orbe.** |
| 50 | `VisualEffects` | Tableau Niagara persistant sur l’item, distinct des effets d’attaque. | **OPTIONNEL ; rendre explicite PersistentItemVFX, ne pas confondre AttackVFX.** |
| 51 | `ItemTags` | Tags sémantiques exploités par talents, conditions d’action et restrictions. | **GARDER comme données avancées validées, cacher si vide.** |

## II. 44 paramètres directs de chaque CombatActions[i]
Ce tableau concerne chaque entrée de `FGridCombatActionDefinition`. Une arme offensive cataloguée simple ne devrait pas afficher ses sous-structures de sort/consommable par défaut.

| # | Propriété | Rôle du champ dans le moteur universel | Verdict pour frappe standard |
|---:|---|---|---|
| 1 | `ActionId` | Identifiant stable de l’action de combat. | GARDER dans fiche simplifiée, un ID par action. |
| 2 | `DisplayName` | Nom de l’action pour HUD/tooltip, différent du nom d’arme s’il existe une capacité spéciale. | DUPLICAT pour frappe par défaut ; dériver, override seulement pour action spéciale. |
| 3 | `Description` | Description de l’action. | DUPLICAT pour frappe standard ; conserver override si comportement réellement distinct. |
| 4 | `Icon` | Icône de l’action dans HUD. | DUPLICAT pour action standard : palette d’actions puis catalogue visuel sont déjà prioritaires. |
| 5 | `ActionType` | MeleeAttack, RangedAttack, Ability, Spell... | GARDER comme type de résolution ; dériver de famille pour arme simple. |
| 6 | `SourcePolicy` | Equipment, QuickItem, Ability, Spell... | DÉRIVER : Equipment pour frappe normale d’arme. |
| 7 | `SourceTags` | Tags d’action et conditions projetées. | AVANCÉ pour bonus/status; calculer à partir de ItemTags sans duplication éditée. |
| 8 | `TargetingPolicy` | Sélection cellule/cible/zone. | GARDER pour capacités spéciales ; valeur standard selon attaque. |
| 9 | `ResolutionProfile` | Attack, Effect... | DÉRIVER : Attack pour arme offensive simple ; autres profils côté capacités. |
| 10 | `ActionPointCost` | Coût en PA déclaré dans action. | DUPLICAT de catalogue pour arme N1–N20 ; désormais projeté au runtime. |
| 11 | `ResourceCosts` | Sous-structure ManaCost et SourceItemQuantityCost. | CONDITIONNEL : mana, munitions jetées ; quantité de jet dérivée de bCombatThrowWeapon. |
| 12 | `RangeCells` | Portée de l’action. | DUPLICAT pour attaque de base ; catalogue en est l’autorité. |
| 13 | `bRequiresLineOfSight` | Ligne de vue obligatoire vers la cible. | GARDER comme politique ciblage, dérivable arme distance. |
| 14 | `TargetFilter` | Restrictions de type/status/armure/HP de la cible. | AVANCÉ, nul pour frappe standard. |
| 15 | `MaximumResolvedTargets` | Nombre maximal de cibles effectivement touchées. | AVANCÉ, chaîne/zone. |
| 16 | `ChainJumpRangeCells` | Distance entre les maillons de cible. | AVANCÉ, attaque en chaîne. |
| 17 | `ResolutionCount` | Nombre de résolutions successives d’une même action. | GARDER seulement pour attaque multiple, par défaut 1. |
| 18 | `SubsequentResolutionAccuracyModifier` | Modification de précision après première frappe. | AVANCÉ, attaque multiple. |
| 19 | `AreaRadiusCells` | Rayon d’effet spatial. | AVANCÉ, magie/zone, 0 pour frappe simple. |
| 20 | `AreaShape` | Forme Manhattan/carrée de zone. | AVANCÉ, magie/zone. |
| 21 | `bAllowNoSurfaceReaction` | Autorise action de surface sans réaction canonique. | AVANCÉ, interactions environnement. |
| 22 | `bUsableDuringExploration` | Autorisation déclarative exploration. | À VÉRIFIER : commentaire C++ dit qu’aucun bypass HUD n’est encore réalisé. |
| 23 | `bAreaCenteredOnParty` | Centre zone sur le groupe plutôt que cellule ciblée. | AVANCÉ, effets de zone. |
| 24 | `bAffectsAlliesInArea` | Feu ami possible. | AVANCÉ, sorts de zone. |
| 25 | `Requirements` | Prérequis sémantiques. | AVANCÉ, talents/capacités. |
| 26 | `CooldownRounds` | Durée de recharge en rounds. | GARDER uniquement si arme possède action spéciale ; pas coût normal. |
| 27 | `PresentationProfileId` | Clé de profil présentation annoncée, distincte du PlayerAttackPresentationProfile de l’item. | INCOMPLET pour arme : le composant SFX/VFX résout par ItemDefinitionId, pas par cette clé. |
| 28 | `WeaponAttackProfile` | Profil % dégâts d’arme équipés, réservé aux sorts/abilities. | HORS frappe standard ; utile talents qui empruntent les dégâts d’arme. |
| 29 | `OffensiveProfile` | Attaque spécifique de l’objet, 5 champs dont FGridAttackDefinition NdM+K. | DUPLICAT N1 pour arme cataloguée ; remplacer par profil résolu, garder generic Legacy. |
| 30 | `EffectProfile` | Dégâts/soins/effet direct non-attaque. | HORS frappe standard ; utile consommables, sorts. |
| 31 | `HealingScaling` | Formule de soin dépendant d’attributs. | HORS frappe standard ; sorts/consommables. |
| 32 | `DirectDamageScaling` | Formule de dégâts directs de capacité. | AVANCÉ ; ne pas confondre avec NdM+K du catalogue. |
| 33 | `QuickItemScaling` | Compétence appliquée aux quick-items. | HORS frappe standard ; objets consommés. |
| 34 | `OwnerVariants` | Variantes selon porteur/statuts, choisies en runtime. | AVANCÉ, talents uniques ; ne pas exposer aux armes ordinaires. |
| 35 | `StatusApplications` | Application de brûlure, saignement, etc. sur cible. | OPTIONNEL pour arme enchantée ; pas intrinsèque à toutes. |
| 36 | `StatusRemovals` | Suppression de statuts. | AVANCÉ, capacités spéciales. |
| 37 | `ArmorEffects` | Effets actifs sur les réserves d’armure. | AVANCÉ ; différent de l’armure passive du bouclier. |
| 38 | `MovementEffects` | Poussée, recul, déplacement tactique. | AVANCÉ, capacité spéciale. |
| 39 | `bRelocatePartyToTargetCell` | Téléporte le groupe sur la cellule choisie. | HORS arme standard, à isoler dans action spéciale. |
| 40 | `SurfaceEffects` | Crée surface de feu/huile/etc. | AVANCÉ, magie/environnement. |
| 41 | `SurfaceConversions` | Transforme une surface existante. | AVANCÉ, magie/environnement. |
| 42 | `SurfaceInteraction` | Interaction avant conversion / génération de surface. | AVANCÉ, mécanique de surface. |
| 43 | `TrapEffect` | Déclenche/pose des pièges. | AVANCÉ, compétence/trappe. |
| 44 | `SkillCheck` | Test de compétence conditionnant la réussite. | AVANCÉ, actions ciblées. |

## III. 15 champs du profil de niveau d’arme du catalogue
| # | Champ | Sens | Verdict |
|---:|---|---|---|
| 1 | `ArchetypeId` | Clé unique ARMxxx, liaison entre assets et catalogue. | GARDER. |
| 2 | `ItemLevel` | Niveau 1–20 du profil calculé. | GARDER pour profil résolu / instance, N1 dans base compacte. |
| 3 | `DisplayName` | Nom canonique de l’archétype ; copié dans Item.DisplayName et Action.DisplayName. | GARDER UNE seule source ; source FText localisable à définir avant migration UI. |
| 4 | `Family` | Famille de l’arme, informative et utilisée en production. | GARDER, remplacer String libre par enum/tag typé si règles. |
| 5 | `HandUsage` | Chaîne importée depuis classeur, alors que l’item possède EGridItemHandUsage. | DUPLICAT inadéquat (FString vs enum) ; éliminer la double édition. |
| 6 | `DamageType` | Physique ou magique selon profil. | GARDER. |
| 7 | `PhysicalSubtype` | Tranchant, perforant, contondant ou none. | GARDER si physique. |
| 8 | `ScalingAttribute` | Force/dextérité/etc. pour l’attaque. | GARDER. |
| 9 | `AccuracyBonus` | Bonus de toucher, invariant sur N1–N20 actuellement. | GARDER. |
| 10 | `DamageDice` | bEnabled, DiceCount, DiceSides, FlatBonus, progression compacte. | GARDER source des vrais dégâts NdM+K. |
| 11 | `PhysicalArmor` | Protection physique passive selon niveau. | GARDER pour bouclier/équipement. |
| 12 | `MagicalArmor` | Protection magique passive selon niveau. | GARDER pour bouclier/équipement. |
| 13 | `ActionPointCost` | Coût standard en PA, projeté au runtime. | GARDER, UNE source. |
| 14 | `RangeCells` | Portée standard, projetée au runtime. | GARDER, UNE source. |
| 15 | `WeightKg` | Masse constante par archétype dans la validation actuelle. | GARDER, UNE source. Niveau N1 et N20 ont actuellement le même poids. |

**Attention :** depuis ARM-CLEAN03, le catalogue binaire stocke un BaseProfile N1 et cinq règles `DiceCountGrowth, DiceSidesGrowth, FlatBonusGrowth, PhysicalArmorGrowth, MagicalArmorGrowth`. Chacune contient `StepEveryLevels`, `StepAmount` et une liste `Exceptions` (niveau, valeur). Le cache runtime développe les 1 640 profils en mémoire ; l’économie mesurée de 79,58 % est **sur disque**, pas 79,58 % de mémoire garantie.

## IV. 10 propriétés éditables du catalogue visuel
| # | Champ | Fonction | Verdict |
|---:|---|---|---|
| 1 | `ArchetypeId (Binding)` | Clé ARMxxx qui relie l’arme au profil d’apparence. | GARDER. |
| 2 | `VisualId (Binding)` | Clé du profil visuel choisi pour l’archétype. | GARDER si partage de visuels entre armes. |
| 3 | `VisualId (Profile)` | Clé canonique du profil visuel. | GARDER ; contrôler les doublons. |
| 4 | `FamilyTag` | Tag artistique de famille. | AVANCÉ ; utile pour palettes/presets, pas données de combat. |
| 5 | `BaseIcon` | Icône de l’item et du monde/menus. | GARDER UNE source visuelle ; distincte de l’icône d’attaque si action spéciale. |
| 6 | `WorldMesh` | Mesh d’objet au sol. | GARDER UNE source. |
| 7 | `EquippedMesh` | Mesh de l’arme tenue. | GARDER UNE source ; corriger LoadHeldMesh pour qu’il le résolve. |
| 8 | `WorldVisualTransform` | Pose et taille du mesh au sol. | GARDER. |
| 9 | `OrnamentMesh` | Mesh décoratif (tier/finition). | OPTIONNEL, futur N1–N20 artistique. |
| 10 | `OrnamentRelativeTransform` | Pose de l’ornement. | OPTIONNEL ; ne pas forcer sur toutes les armes. |

## V. Analyse concrète du C++ — défauts et legacy restant
| Gravité | Source actuelle | Constats vérifiés | Décision et vérification nécessaire |
|---|---|---|---|
| **P0** | `GridWeaponPipelineCommandlet.cpp:215–269` | `DisplayName` du catalogue copié dans item et action ; `WeightKg` dans `Weight` ; BaseIcon dans `Item.Icon` et `Action.Icon` ; meshes et statistiques N1 également copiés. | Faire un pipeline **référentiel**, ne plus écrire ces snapshots pour nouvelle production. Migration et tests avant suppression des champs existants. |
| **P0** | `GrimrockPartyPawnHeldItem.cpp:65–67` et `GridItemDefinitionAsset.cpp:340–437` | L’arme tenue utilise `LoadHeldMesh()` donc `EquippedMesh/WorldMesh` locaux, contournant le résolveur visuel qui est déjà utilisé pour monde/UI. | Utiliser un unique résolveur de visuels de catalogue pour arme tenue, monde et inventaire ; conserver fallback pour **items non catalogués**. |
| **P0** | `GridItemDefinitionAsset.cpp:194–230` ; `GridItemContextActionLibrary.cpp:416–441` | `IsPhysicallyThrowable`, `CanBeThrownByStrength` et message de trop-lourd lisent `Definition.Weight` ; `ApplyItemDefinitionToInstance` fixe `ItemInstance.Weight` depuis le profil catalogue. | Centraliser le poids effectif sur l’instance pour lancer et UI. Le poids catalogue est **actuellement invariant** entre niveaux, donc ce conflit n’est pas la preuve d’une divergence N1/N20 en production aujourd’hui. |
| **P0** | `GridInventoryWidget.cpp:1024–1031` et `GridLevelRuntimeActorWorldItems.cpp:1000–1009` | Le filtre/projection d’inventaire et le scan des poids sur plaque utilisent encore la masse de définition, non la masse d’instance. | Vérifier les structures de world-items et faire dépendre toutes les masses du niveau lorsqu’il est présent. |
| **P1** | `GridWeaponVisualUiResolver.cpp:23–38` et `GridCombatHudWidget.cpp:1576–1613` | `ApplyCatalogueIcon` peut écraser une icône d’action personnalisée avec l’icône de l’objet ; ensuite la palette d’action peut encore l’emporter dans `ResolveDisplayedIcon`. | Définir précédence unique : icône d’action spéciale explicite > palette de type d’attaque > icône d’objet par défaut. Tester arme multi-action. |
| **P1** | `GridTurnManagerPlayerActionCatalog.cpp:337–396` | La fiche d’attaque sérialisée est recopiée puis valeurs dégâts, portée et PA sont réécrites depuis le catalogue. L’identité de l’action et ses propriétés de ciblage restent réellement utiles. | Séparer `WeaponActionId/ActionPolicy` du `FGridAttackDefinition` résolu, pour ne plus exposer les dés N1 du modèle universel. |
| **P1** | `GridPlayerAttackPresentationComponent.cpp:427–448` vs `GridCombatTypes.h:2603` | `PresentationProfileId` existe sur l’action mais la présentation de frappe sélectionne `PlayerAttackPresentationProfile` de l’item par ItemDefinitionId. Aucun routage par ActionId/PresentationProfileId n’est prouvé dans ce lecteur. | Centraliser SFX/VFX par action (attaque, parade, impact matière, heavy, critique) ; ne pas déclarer ce champ actif sur les armes sans câblage vérifié. |
| **P1** | `GridWeaponVisualResolver.cpp:13–155` | Références `Definition.Icon/WorldMesh/EquippedMesh` toujours utilisées comme repli de catalogue, et `bLegacyFallback` peut être vrai pour un item pourtant catalogué. | Interdire la **double édition** sur arme de production, conserver le fallback seulement là où la migration/compatibilité l’exige. |
| **P1** | `GridWeaponPipelineCommandlet.cpp:283–375` | `ItemMatches` ne compare plus les anciennes stats N1 mais compare encore `Existing.Icon/WorldMesh` à des chemins du manifeste et impose `HandUsage` / slots. | Dissocier validation de référence catalogue, validation de fiche d’item et validation de compatibilité ; supprimer les obligations de clones après migration. |
| **P1** | `GridItemDefinitionAsset.cpp:153–167` et `GridItemDefinitionAsset.cpp:315–329` | `CanEquipToSlot` peut accepter une main via `HandUsage` alors que `CanProvideAttackFromSlot` exige `CompatibleEquipmentSlots.Contains` : deux règles de mains partiellement concurrentes. | Un résolveur commun de compatibilité mains ; tests main gauche/deux mains/bouclier. |
| **P1** | `GridWeaponCatalogueAsset.cpp:46–126` | Le catalogue exige encore `Profiles` ou `ArchetypeProgressions` et 82×20 profils ; `Profiles` est toujours un champ UPROPERTY exposé, même si vide après migration. | Conserver temporairement pour réimport XLSX ; cacher l’ancienne liste dans le Details, puis la retirer après migration de tous les outils d’import. |
| **P2** | `GridItemDefinitionAsset.cpp:38–54` et `GridCombatHudWidget.cpp:1576–1590` | Fallback des `DisplayName/Description/Icon` de l’item dans l’action, alors que le pipeline initialise déjà leurs copies. | Garder fallback **au moment de la résolution**, ne pas enregistrer les copies dans les DataAssets. |
| **P2** | `GridItemDefinitionAsset.cpp:256–278` et `GridPartyInventoryComponentEquipment.cpp` | Les protections physiques/magiques nominales des armes cataloguées sont désormais calculées à partir de l’instance ; les champs `EquipmentStatBonus.ArmorBonus` et `MagicalArmorBonus` restent pourtant éditables. | Masquer ces deux champs pour arme de production ; conserver des bonus génériques non-catalogue pour armures ordinaires. |
| **P2** | `GridCombatHudWidget.cpp:570–595` | La palette `UGridEquippedWeaponActionIconPalette` peut déjà choisir une icône d’action ; l’icône d’objet doit rester différente. | Séparer clairement « icône d’objet » et « icône de commande ». |
| **P2** | `GridWeaponCatalogueAsset.cpp:159–311` et `GridWeaponPipelineCommandlet.cpp:385–411` | `ClassAffinities` et `ClassPreferences` servent à la validation/recommandation des kits, pas aux dés de dégâts. | Déplacer ces données de design/kit à part de la fiche « statistiques d’arme » après audit des auteurs de kits. |
| **P2** | `GrimrockWeaponDetailsCustomization.cpp:60–113` | ARM-CLEAN04 remonte certaines catégories et cache Reading/QuickItem seulement dans quelques conditions. Aucun filtrage complet du modèle à 51 propriétés et des tableaux d’actions. | Remplacer ce travail cosmétique par un vrai éditeur spécialisé filtrant les catégories/valeurs, avec options avancées séparées. |

## VI. Architecture cible à mettre en place, sans casser les sauvegardes
1. **Identité et statistiques** : une seule fiche d’archétype ARMxxx (nom localisable, masse, type, mains, dégâts/protections N1 et règles N1–N20), pas de clones manuels. Le catalogue compact existe déjà.
2. **Un DA_Weapon minimal** : référence ARMxxx, description spécifique si nécessaire, profil visuel, liste d’actions dédiées, références de SFX/VFX et finitions N1–N20. Les règles techniques communes restent dans les presets/classes, pas 200 champs sur l’item.
3. **Un resolver effectif par instance** pour nom, poids, équipement, attaque, icône, meshes, SFX/VFX ; une seule API utilisée par inventaire, monde, hud, arme tenue, tooltip, plates et lancers.
4. **Capacités avancées composables** : les 44 champs d’action universelle ne sont éditables que dans un asset de capacité spéciale, jamais déroulés pour une attaque de base.
5. **Retour arrière protégé** : conservez ItemDefinitionId, WeaponArchetypeId, RuntimeObjectId, ItemLevel, slot/quantité, réserves d’armure absorbées, Save → Continue. Pas de suppression brute d’UPROPERTY avant migration et validation des .uasset.

## VII. Découpage de correction recommandé
- **ARM-CLEAN05A** : une source de vérité pour nom/masse/icon/mesh ; resolver et tests de chaque lecteur. Rebuild/audit du pipeline sans cloner N1.
- **ARM-CLEAN05B** : corriger HeldItem visuel catalogue, UI icône spécifique/palette et poids effectif des projections/lancers/plaques ; tests ciblés + suite Grimrock.
- **ARM-CLEAN05C** : extraire actions standard des 44 paramètres d’action générique, simplifier le panneau d’arme, presets SFX/VFX et finitions N1–20 ; aucune suppression prématurée.
- **ARM-CLEAN05D** : migration des .uasset, dépréciation contrôlée des anciens champs, tests Automation Windows et recette PIE des 22 archétypes N1, Save/Continue, 2 mains, jet et parade.

**État de ce document : AUDIT TERMINÉ / REFONTE NON VALIDÉE.** Aucune affirmation que les 22 actifs portent toutes les propriétés ou que les fallbacks sont morts sans avoir réalisé la migration.
