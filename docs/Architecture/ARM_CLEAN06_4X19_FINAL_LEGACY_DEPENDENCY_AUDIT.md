# ARM-CLEAN06.4X.19 — audit natif des dépendances avant 06.5

**17 novembre 2026 — ARM-CLEAN06.4 NON TERMINÉ ; 06.5 BLOQUÉE.**
Audit du contenu et des sources de `master=ae6742b1694355bc71c3a49a4b2d6f7a408fa69d`.
Ce parent ajoute seulement le préflight documentaire au code fonctionnel `9e8331c1`.
[Mission #182](https://github.com/PhilippeMartin1964/GrimrockPrototype/issues/182).
Le préflight distant initial reste consultable dans Git au parent `ae6742b1`.

## 1. Preuves obtenues, sans conversion

- UE **5.5.4-40574608**, PythonScript commandlet nouvellement écrit hors dépôt :
  lecture AssetRegistry synchrone de **1486 assets /Game**, propriétés natives de
  **238 DataAssets** du projet, puis CDO d'un WBP référent. Aucun ancien migrateur lancé.
- **239 packages** inspectés par propriétés/CDO, plus cinq redirecteurs examinés
  séparément : **244 empreintes SHA-256 inchangées** avant/après, y compris après
  la vérification du résolveur. Aucun `.uasset`, `.umap` ou SaveGame écrit.
- Zéro erreur de lecture des propriétés des DataAssets/CDO. Les trois avertissements
  du commandlet concernent des alias Python dépréciés du schéma monde
  (`archetype_id`, `default_archetype`, `action`), pas trois conversions ARM ratées.
- Initialisation native du registre dans un monde SIE temporaire vide, GameInstance
  de base, **aucun GrimrockPartyPawn ni propriétaire de sauvegarde** : registre
  configuré et **452 résolutions réussies** (22 ARM × N1–N20 + 12 non-ARM à N0).
  Les 22 ARM à N0 sont refusés. Ce test n'est pas une recette de combat/présentation.
- Comparaison des valeurs natives de **82 archétypes**, neuf champs N1 et cinq règles
  de progression : **1148 comparaisons, zéro différence**. Cette parité de données
  ne prouve pas les sons, Niagara, collisions ou états mutables dans chaque lecteur.
- Les sources et assets de gameplay restent ceux de `9e8331c1`, dont PIT02 est
  validé par le propriétaire selon le point de reprise. Cette recette n'est pas rejouée
  ni présentée comme une nouvelle preuve X.19.

Le native manifest décrit les valeurs chargées par UE et les dépendances de package,
pas l'emplacement de chaque octet sur disque. Les pointeurs non nuls relevés sont
recoupés avec les dépendances AssetRegistry. Il distingue absence de propriété,
valeur nulle et ID absent ; un nom de fichier ne tient pas lieu de preuve.

## 2. Déjà matérialisé, avec portée exacte

| Ensemble | État natif actuel |
|---|---|
| `CanonicalStage/DA_ItemSystem` | 34 définitions uniques : 22 ARM, 12 non-ARM ; quatre références globales vers les catalogues canoniques |
| `CanonicalStage/Definitions/*` | 34 définitions spécialisées ; les ARM ont les cinq champs de base + ArchetypeId ; noms et poids identiques à leurs 34 sources historiques |
| `DA_CanonicalWeaponCatalogue` | 82 archétypes et six préférences de classe ; données N1/progressions comparées ci-dessus |
| `DA_CanonicalWeaponVisualCatalogue` | 22 liaisons, 22 profils visuels, 12 icônes d'actions ; ne pas inventer 60 définitions manquantes à partir des 82 archétypes |
| `DA_ActionCatalogue` | 19 actions et quatre actions détaillées |
| `DA_ActionPresentationCatalogue` | cinq présentations et une présentation détaillée |
| Quatre niveaux | 44 loose dans `DA_GridLevel_00`, 17 contenus initiaux dans `_00`, un contenu initial dans `_01`, zéro ItemSpawn dans les quatre |
| `DA_ObjectPalette_Default` | 33 collectibles directs avec ID et niveau canoniques |
| 95 occurrences | 44 + 18 + 33 : toutes identités enregistrées, niveaux physiques ARM N1 / non-ARM N0, aucun conflit ancien ID / nouvel ID détecté |
| Six `DA_Class_*` | `StartingEquipmentKits` vides dans les six packages ; presets actuellement construits par C++, pas six assets de kits à remigrer |

Le dossier `CanonicalStage` contient donc bien **39 DataAssets** : 34 définitions,
les quatre catalogues et la racine. Ils coexistent encore avec **48 définitions
historiques**. « Projection canonique matérialisée » ne signifie pas « ancienne
classe et anciens lecteurs supprimables ».

### 24 fichiers DA_Weapon_* contre 22 ARM : correspondance native

Préfixe exact des packages ci-dessous :
`/Game/GrimrockPrototype/Core/DataAssets/Weapons/`.
Les 23 premières définitions ci-dessous sont de classe historique
`GridItemDefinitionAsset`, pas déjà de classe canonique finale.

| Package relatif | ItemId natif | Jointure canonique |
|---|---|---|
| `DA_Weapon_ArcaneOrb_01` | `Weapon_ArcaneOrb_01` | `ARM070` |
| `DA_Weapon_ArcaneWand_01` | `Weapon_ArcaneWand_01` | `ARM061` |
| `DA_Weapon_AssassinDagger_01` | `Weapon_AssassinDagger_01` | `ARM002` |
| `DA_Weapon_Dagger_01` | `Weapon_Dagger_01` | `ARM001` |
| `DA_Weapon_Dart_01` | `Weapon_Dart_01` | `ARM058` |
| `DA_Weapon_ElementalStaff_01` | `Weapon_ElementalStaff_01` | `ARM067` |
| `DA_Weapon_FireWand_01` | `Weapon_FireWand_01` | `ARM062` |
| `DA_Weapon_GreatAxe_01` | `Weapon_GreatAxe_01` | `ARM031` |
| `DA_Weapon_HolyAegis_01` | `Weapon_HolyAegis_01` | `ARM081` |
| `DA_Weapon_HolyScepter_01` | `Weapon_HolyScepter_01` | `ARM069` |
| `DA_Weapon_Javelin_01` | `Weapon_Javelin_01` | `ARM057` |
| `DA_Weapon_LongSword_01` | `Weapon_LongSword_01` | `ARM015` |
| `DA_Weapon_Mace_01` | `Weapon_Mace_01` | `ARM020` |
| `DA_Weapon_MainGauche_01` | `Weapon_MainGauche_01` | `ARM013` |
| `DA_Weapon_RecurveBow_01` | `Weapon_RecurveBow_01` | `ARM048` |
| `DA_Weapon_RoundShield_01` | `Weapon_RoundShield_01` | `ARM075` |
| `DA_Weapon_ShortBow_01` | `Weapon_ShortBow_01` | `ARM045` |
| `DA_Weapon_ShortSpear_01` | `Weapon_ShortSpear_01` | `ARM038` |
| `DA_Weapon_Shuriken` | `Shuriken` | `non-ARM` |
| `DA_Weapon_Stiletto_01` | `Weapon_Stiletto_01` | `ARM003` |
| `DA_Weapon_StraightSword_01` | `Weapon_StraightSword_01` | `ARM014` |
| `DA_Weapon_ThrowingKnife_01` | `Weapon_ThrowingKnife_01` | `ARM054` |
| `DA_Weapon_WarPick_01` | `Weapon_WarPick_01` | `ARM025` |
| `DA_Weapon_ThrowingKnife` | redirecteur, aucune définition supplémentaire | résolution native vers `DA_Weapon_ThrowingKnife_01` |

Il y a **22 ARM + Shuriken non-ARM + un redirecteur = 24**, sans doublon de
lancer à supprimer sur simple comptage. Les deux anciens catalogues voisins
`DA_WeaponCatalogue` et `DA_WeaponVisualCatalogue` ne font pas partie du motif
`DA_Weapon_*`. Les 22 définitions ARM ont encore les deux options et les deux
références vers ces catalogues historiques.

## 3. Manifeste des références historiques restantes

**107 occurrences de pointeur vers les 48 définitions historiques**, dans neuf
packages. Leur détail complet (champ, index, GUID/cellule lorsque applicable,
ancien package/ID, nouvel ID, niveau, contexte) figure dans les CSV hors dépôt.

| Package source exact | Occurrences |
|---|---:|
| `/Game/GrimrockPrototype/Core/DataAssets/DA_Lock_CopperWall` | 1 |
| `/Game/GrimrockPrototype/Core/DataAssets/DA_Lock_IronWall` | 1 |
| `/Game/GrimrockPrototype/Core/DataAssets/DA_ObjectPalette_Default` | 33 |
| `/Game/GrimrockPrototype/Core/DataAssets/DA_Receptacle_TorchHolder` | 1 |
| `/Game/GrimrockPrototype/Core/DataAssets/GridObjectArchetypeAsset/DA_Guardian` | 1 |
| `/Game/GrimrockPrototype/Core/DataAssets/GrimrockLevels/DA_GridLevel_00` | 64 |
| `/Game/GrimrockPrototype/Core/DataAssets/GrimrockLevels/DA_GridLevel_01` | 1 |
| `/Game/GrimrockPrototype/Monsters/GoblinThrower/Data/DA_MON_GoblinThrower` | 3 |
| `/Game/GrimrockPrototype/Monsters/RatGiant/Data/DA_MON_RatGiant` | 2 |

Décomposition : **95 pointeurs** conservés avec les 95 IDs matérialisés,
**quatre règles de définition** déjà converties, **trois overrides inactifs**
et **cinq entrées de loot**. Les nombres 95 et 107 ne sont donc pas contradictoires.

En plus de ces 107 occurrences :

- **44 références aux anciens catalogues** : `weapon_catalogue` et
  `weapon_visual_catalogue` dans chacune des 22 anciennes définitions ARM ;
- **une référence WBP** :
  `/Game/GrimrockPrototype/Blueprints/UI/Combat/WBP_GridCombatHudAction`, CDO
  `equipped_weapon_action_icon_palette` →
  `/Game/GrimrockPrototype/UI/Icons/Combat/DA_EquippedWeaponActionIcons` ;
- le redirecteur ThrowingKnife décrit ci-dessus pointe encore vers la définition
  historique `_01`. Ce lien est compté séparément des propriétés des DataAssets.

Le graphe AssetRegistry n'a trouvé aucun autre référent direct à ces cibles parmi
les 1486 packages /Game : tous sont couverts par les DataAssets, le WBP et ce
redirecteur. Cela n'exclut ni les chargements C++ par chemin/ID ni les fichiers
SaveGame externes. Aucun besoin de resauvegarde massive des `.umap` n'est établi.

### Règles déjà converties : conserver le résultat, préparer le retrait ciblé

| Package sous `/Game/GrimrockPrototype/Core/DataAssets/` | Nouvelle valeur | Ancien champ encore non nul |
|---|---|---|
| `DA_Receptacle_TorchHolder` | AcceptedItems[0].CanonicalItemId=`Item_Torch` | AcceptedItems[0].ItemDefinition=DA_Item_Torch |
| `GridObjectArchetypeAsset/DA_Guardian` | AcceptedItems[0].CanonicalItemId=`Gem_Blue` | AcceptedItems[0].ItemDefinition=DA_Item_BlueGem |
| `DA_Lock_CopperWall` | AcceptedKeyIds=[`Key_Copper`] | AcceptedKeyItems=[DA_Item_CopperKey] |
| `DA_Lock_IronWall` | AcceptedKeyIds=[`Key_Iron`] | AcceptedKeyItems=[DA_Item_IronKey] |

Ces quatre identités/règles sont bien converties ; **elles ne sont pas sans legacy**.
Pas de réexécution des migrations X.7/8. Leur futur nettoyage doit suivre le cutover
et la validation des lecteurs/éditeurs, sans changer les règles d'énigme.

Dans `DA_GridLevel_00`, trois anciennes valeurs d'override sont encore sérialisées,
mais leurs interrupteurs sont **false** :

| GUID | Cellule | Champ stocké | Nouvel ID |
|---|---|---|---|
| `0CB018F04E6C3DCB059133A0CA12214F` | 16,25 | receptacle_rules.accepted_items[0].item_definition=Torch | None |
| `12F822EE439884334380F4AED3223847` | 11,24 | accepted_keys.accepted_key_items[0]=CopperKey | liste vide |
| `22CCD36F42A9F3FE8C7C3B852E2F71FA` | 9,23 | accepted_keys.accepted_key_items[0]=IronKey | liste vide |

Elles ne remplacent actuellement pas les définitions converties. Préparer leur
nettoyage contrôlé en conservant l'état inactif, pas une activation/migration
automatique de leurs valeurs dormantes.

### Quatorze identités historiques hors registre

Préfixe exact : `/Game/GrimrockPrototype/Core/DataAssets/Items/`.

| Package relatif | ItemId | Usage établi |
|---|---|---|
| `DA_Item_Antidote` | `Item_Antidote` | aucun référent package ; authoring alchimie |
| `DA_Item_Bomb_Fire` | `Item_Bomb_Fire` | aucun référent package ; authoring alchimie |
| `DA_Item_Bomb_Toxic` | `Item_Bomb_Toxic` | aucun référent package ; authoring alchimie |
| `DA_Item_DefensiveElixir_Fire` | `Item_DefensiveElixir_Fire` | aucun référent package ; authoring alchimie |
| `DA_Item_DefensiveElixir_Ice` | `Item_DefensiveElixir_Ice` | aucun référent package ; authoring alchimie |
| `DA_Item_DefensiveElixir_Lightning` | `Item_DefensiveElixir_Lightning` | aucun référent package ; authoring alchimie |
| `DA_Item_DefensiveElixir_Poison` | `Item_DefensiveElixir_Poison` | aucun référent package ; authoring alchimie |
| `DA_Item_EmptyVial` | `EmptyVial` | loot réel : DA_MON_GoblinThrower |
| `DA_Item_Flask_Acid` | `Item_Flask_Acid` | aucun référent package ; authoring alchimie |
| `DA_Item_Flask_CorrosiveCloud` | `Item_Flask_CorrosiveCloud` | aucun référent package ; authoring alchimie |
| `DA_Item_GoblinKnife` | `GoblinKnife` | loot réel : DA_MON_GoblinThrower |
| `DA_Item_Panacea` | `Item_Panacea` | aucun référent package ; authoring alchimie |
| `DA_Item_RatMeat` | `Item_RatMeat` | loot réel : DA_MON_RatGiant |
| `DA_Item_RatTooth` | `Item_RatTooth` | loot réel : DA_MON_RatGiant |

Les cinq loots sont : gobelin `[GoblinKnife, Stone, EmptyVial]`, rat
`[Item_RatMeat, Item_RatTooth]`. Leurs `ItemDefinitionId` sont **None** et leurs
niveaux N0 ; Stone est déjà enregistré, les quatre autres non.
`GoblinKnife` a ItemType=Weapon mais aucun ArchetypeId et aucune action :
**aucune correspondance ARM ne peut être inventée**. Sa famille/identité finale
est une décision de contenu préalable à sa conversion.

Les dix autres n'ont pas de référent package direct actuel, mais leurs IDs sont
présents dans l'authoring d'alchimie (`RPGAlchemistAuthoring.cpp:40–50`, plus outils
spécifiques pour FireBomb). Ce ne sont pas dix suppressions autorisées. Choisir
explicitement conversion canonique ou retrait de contenu, puis traiter aussi les
outils qui les recréent et les éventuelles occurrences sur copies de sauvegarde.

## 4. Lecteurs C++ et schéma : ce qui interdit encore 06.5

`cpp_candidates.csv` indexe la recherche sur tout `Source/GrimrockPrototype` et
`Source/GrimrockPrototypeEditor` : **1776 mentions**, dont 1000 en tests,
450 Editor, deux mentions des pointeurs dans le schéma de monde et 324 autres
mentions runtime/headers. Ce nombre n'est **pas** un nombre de lecteurs legacy :
il inclut champs canoniques homonymes, diagnostics, types et icônes non-item.
`cpp_readers.csv` classe les groupes revus (A/B/C/D/E), avec fichier, ligne repère,
fonction, champ et condition d'autorité. Les tests sont exclus du verdict production.

| Classe | Lecteur / repère au SHA audité | Autorité constatée / blocage |
|---|---|---|
| A | `GridLevelRuntimeActor.cpp:2146`, AddPlacedItemActor | ARM connu exige encore ItemDefinition et initialisation historique avant projection du mesh |
| A | `GridReceptacleActor.cpp:1115`, AddContainedItem ; `Persistence.cpp:690` | initialisation complète limitée au non-ARM ; ARM contenu encore historique |
| A | `GridReceptacleActor.cpp:296`, ContainsItemTag/Type | accès direct à ItemDefinition, sans résolution du registre dans ces deux fonctions |
| A | `GridItemActor.cpp:81`, InitializeFromItemDefinition | copie LightEmitter/VisualEffects et visuel historique ; ApplyCanonicalWorldVisual ne remplace pas tout |
| A | `GrimrockPartyPawnHeldItem.cpp:32,145`, EquipHeldItem / DoesEquippedItemEmitLight | définition et lumière historiques encore requises ; mesh canonique partiel ensuite |
| A | `GridPlayerAttackPresentationComponent.cpp:432`, ResolveProfile | branche canonique non-ARM à N0 ; les ARM peuvent encore rejoindre PlayerAttackPresentationProfile historique |
| A | `RPGStartingEquipmentService.cpp:47`, TryBuildKit | pointeur obligatoire pour identité/stack/règles, même si poids et niveau deviennent canoniques |
| A/D | `GridEditorPreviewComponent.cpp:159`, AddLooseItemPreview | absence du pointeur bloque avant résolution du mesh canonique |
| A | `GridWallLockActor.cpp:52`, InitializeRuntimeWorldObject | union des anciennes clés et des nouveaux IDs ; retrait encore à faire |
| B | WorldItems PIT02, restauration libre/projectile, poids, actions équipées, DualWield, hotbar, icônes | autorité canonique prioritaire pour IDs connus ; fallback réel reste nécessaire aux IDs absents, dont les quatre loots |
| B | `GridCombatHudWidget.cpp:809` | icônes canoniques pour actions connues ; ancienne palette encore utilisée pour autres sources et référencée par le WBP |
| C | `GridDungeonRuntimeState.h:127,170` | ProjectileDefinitionAsset et PendingWorldItem.ItemDefinitionAsset sont encore UPROPERTY(SaveGame) |
| C | `GridInventoryTypes.h:319,326` | DisplayName et Weight sont encore dans la structure d'instance sérialisée, même si connus ignorés comme autorité |
| D | édition `_07.inl:59`, inspecteur `:1313`, validation `.inl:448`, Palette `:43`, Weapon Details `:54` | écrivent/valident les anciens champs ; cutover Editor préalable au nettoyage des packages |
| D | presets C++ / anciens commandlets et scripts | générations transientes et outils historiques à traiter ; aucun outil ancien exécuté dans X.19 |
| E | Tests et documentation | fixtures/mentions ne prouvent ni un lecteur de production ni une conversion binaire |

**La priorité canonique n'est pas uniforme.** Une icône ou une attaque numérique
correcte par registre ne prouve pas que l'initialisation de l'acteur, la lumière,
les effets et la présentation n'ont plus besoin de la définition historique.
Ce relevé ne prétend pas prouver la parité comportementale de tout le graphe d'appels.

### SaveGame v27 : preuve de schéma, pas preuve de migration de fichiers

Le CDO natif expose SaveVersion=27, les deux champs de pointeur et les structures
attendues. `UGrimrockPartySaveGame::ValidateCurrentState` (`.cpp:270`) exige exactement
la version courante ; `Serialize` (`:293`) valide, mais ne constitue pas une conversion
ponctuelle vers un schéma sans legacy. Les anciens outils d'assets examinés ne
fournissent pas la preuve d'une conversion de fichiers v27.

**Aucun fichier personnel ouvert, copié ou modifié. Aucune conversion sur copies
v27 exécutée ici.** Cette preuve reste manquante : fixtures isolées et protégées,
lecture avant/après, conservation GUID/quantité/N0–N20/états d'armure/poses/files
PIT02 et round-trip disque doivent précéder suppression ou changement des types.

## 5. Décision de conversion, par périmètre

1. **Ne pas rematérialiser les 39 assets CanonicalStage** : leurs données actuelles
   sont présentes. Faire converger les lecteurs A, puis décider une seule autorité
   de package par ItemId. Les 34 sources historiques correspondantes ne sont ni
   librement supprimables ni à recréer numériquement ; la liste des paires est dans
   `legacy_definitions.csv`. Le changement de classe/retrait doit attendre le cutover.
2. **Quatre définitions de loot à convertir/enregistrer**, avec décision explicite
   pour GoblinKnife ; **deux packages de monstres à projeter par ID**, cinq lignes,
   sans toucher chances/quantités. Les dix autres identités hors registre demandent
   une décision de contenu et d'authoring avant retrait.
3. **Après cutover C++/Editor seulement**, transaction native ciblée des neuf packages
   du manifeste pour retirer/projeter les 107 pointeurs. Préserver les 95 GUID/IDs/
   niveaux ; conserver les quatre règles converties ; traiter les trois overrides
   comme inactifs. Pas de nouvelle migration massive ni de rerun W/X.5/X.7/X.8.
4. **WBP_GridCombatHudAction** : retirer sa dépendance à l'ancienne palette seulement
   après parité des catégories d'actions ; contrôler Designer/rendu et variantes
   non enregistrées. Nettoyer ensuite les 44 références de catalogue et le redirecteur
   ThrowingKnife selon la décision de packages finaux. Aucun asset UI modifié ici.
5. **Copies v27 vérifiées**, puis et seulement puis **06.5** : suppression des champs,
   classes et outils historiques avec recherche C++ et nouveau scan de tous les
   référents. **06.6** garde sa recette globale, Cook/Shipping et PIE de présentation.

Un redirecteur tiers supplémentaire, `/Game/F_FreeFlameFx_Pack/FX/NS_Flame_8_Torch`,
référence l'ancien chemin non monté `.../Art/FX/NS_Flame_8_Torch` et ne se résout pas
par ce chemin. La torche canonique charge bien le système actuel
`/Game/GrimrockPrototype/Effects/Fire/Torch/NS_Flame_8_Torch`. Ce reliquat tiers est
signalé pour tri séparé, sans modifier l'asset ni le confondre avec une corruption
de la définition canonique ou une conversion ARM autorisée.

## 6. Livrables et limites de validation

Preuves complètes hors dépôt :
`D:/Development/GrimrockPrototype_Audits/ARM_CLEAN06_4X19/`.

| Fichier | Contenu |
|---|---|
| native_manifest.json / native_properties.csv | propriétés de 238 DataAssets, CDO du WBP, graphe des 1486 packages, schéma natif v27 |
| remaining_references.csv | 107 pointeurs historiques, champs/index/GUID/IDs/niveaux/contexte |
| canonical_occurrences.csv | 95 occurrences matérialisées, jointure exacte et niveaux |
| legacy_definitions.csv | 48 définitions, paires canoniques, archetypes et référents |
| catalogue_references.csv | 44 pointeurs de catalogue + le pointeur du WBP |
| cpp_candidates.csv / cpp_readers.csv | index des mentions et classification des groupes de lecteurs A–E |
| native_resolver.json / catalogue_parity.json | 452 résolutions natives, rejet ARM N0, 1148 comparaisons de données |
| redirectors.json / integrity.json / SHA256.json | résolutions des alias, 244 packages inchangés et empreintes des preuves |
| scripts Python et journaux natifs | nouvelles sondes de lecture uniquement ; aucun migrateur historique |

SHA-256 du manifeste natif :
`d519072fa76c9ebb7575008fedffc963115a5aa7b33304d016b756df7062420d`.

Les commandlets NullRHI prouvent la lecture native et les données, pas le rendu.
Le SIE vide prouve l'initialisation/résolution, pas une scène de combat ni les effets
sur un acteur. Aucun résultat de CI global ou ancienne recette PIT02 ne remplace
ces preuves de contenu. Seuls ce rapport et le point de reprise changent dans Git.
L'issue #182 reste ouverte pour les suites de conversion ; **06.4 reste incomplète
et 06.5 bloquée**.
