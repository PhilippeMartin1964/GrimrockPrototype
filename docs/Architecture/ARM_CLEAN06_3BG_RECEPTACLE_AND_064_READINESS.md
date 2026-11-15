# ARM-CLEAN06.3BG — Réceptacles et préparation vérifiable de 06.4

**Référence :** `master` avant BG = ARM-CLEAN06.3BF `6f120637267dbd706e41a1685c3075a2e50c9125`, avec `UE5.5.4/SilentTask` et `UE5.5.4/LocalAuto` à `success`.

## Dette runtime résolue dans BG

`GridReceptacleActor.cpp` privilégiait encore `UGridItemDefinitionAsset::ItemDefinitionId` à la place de l'identité physique explicitement enregistrée. Ses objets initiaux, insertions et fallback de capture pouvaient donc requalifier un item canonique. En parallèle, `FGridReceptacleItemMetadataReadModel` refusait le bon ItemId dès qu'un vieil asset associé portait un ID contradictoire.

`FGridReceptacleContainedIdentityReadModel` donne priorité à l'**ItemId explicite enregistré**. Un ancien asset contradictoire ne peut ni renommer l'objet ni devenir son autorité visuelle à l'insertion. La capture de réceptacle vérifie les niveaux physiques et l'accord avec l'acteur matérialisé ; elle conserve le triplet **ItemId / ItemLevel / GUID**. L'absence d'ItemId sur un objet initial ancien peut encore lire sa définition historique : c'est une dette à migrer en 06.4, non un nouveau système d'autorité.

Pour les ItemId hors registre, la priorité historique reste temporairement conservée. Une définition enregistrée ne peut cependant pas renommer un ItemId différent qui n'est pas encore migré.

## Inventaire des dépendances restant à convertir

| Lot | Données/lecteurs observés dans le dépôt | Preuve requise avant suppression 06.5 |
|---|---|---|
| Catalogues ARM | `DA_WeaponCatalogue`, `GridWeaponCatalogueAsset`, `GridArmClean06BaselineCommandlet` | Contrôle exact des 82 archétypes × 20 niveaux = 1 640 profils, dont 260 passifs |
| Définitions historiques | `DA_Weapon_*`, `DA_Item_*`, `GridItemDefinitionAsset` | Migration des identités, niveaux par défaut, poids, pile, slots, noms, textes, meshes et icônes sans double autorité |
| Vols spécialisés | `GridThrownItemActor::InitializeThrownItem`, `GridArmClean06ProjectileVisualProjection` | Parité des axes/rotations, spins, durée de vol, collision et pose à la retombée |
| Jets génériques | `GridCanonicalThrowBallisticsReadModel`, `GridPhysicalThrowStrengthReadModel` | Conversion de la vraie vitesse, du vrai arc et de la vraie présentation **sans chiffres inventés** |
| Flèches N0 | `GridTurnManagerPlayerActions.cpp`, `GridThrownItemActor.cpp` | Vérifier `Item_Arrows_01`, vitesse encore issue d'un vieux asset dans certains chemins, visuel, impact, flèche fichée et Save/Continue |
| Réceptacles / placements | `GridReceptacleActor`, `GridReceptacleItemMetadataReadModel`, niveaux et palettes | Migrer les `InitialContainedItems` sans ItemId explicite ; recouper niveau, GUID, référents et apparence des vrais objets placés |
| Contenus de niveau | `DA_GridLevel_*`, `DA_Dungeon_*`, `DA_ObjectPalette_Default` | Resauvegarder via Unreal Editor et comparer le graphe de références physiques/soft de l'AssetRegistry |
| Inventaires et kits | `RPGStartingEquipmentPresetCatalog`, `GridPartyInventoryComponent`, `GridDungeonRuntimeState` | Conserver les 18 kits inventoriés par l'audit 06.3 et les anciennes instances N0, quantités, propriétaires, mains |
| UI / gameplay connexe | `GridCombatHudWidget`, `GridInventorySlotWidget`, `GridWallLockActor` | Équivalence fonctionnelle, absence de nouvelles références `DA_Item_*` dans les WBP, sans refonte UMG |
| Outils Editor | `GridWeaponPipelineCommandlet`, `GridWeaponPilotItemAuthoringCommandlet`, `GrimrockWeaponDetailsCustomization` | Retirer les générateurs historiques **après** la conversion, sans effacer les contenus utilisés |

**Provenance des volumes :** ARM-CLEAN06.0 répertorie **24 armes, 24 autres items, quatre niveaux, un donjon, une palette**, ainsi que 1 640 profils et 260 passifs. Ces volumes ne sont **pas un nouveau scan AssetRegistry réalisé par BG**. Les valeurs exactes et référents des vrais assets doivent être relus par `GridArmClean06Baseline` au lancement de la conversion. Les anciennes sauvegardes `.sav` n'ont pas été ouvertes ni garanties présentes dans l'environnement de validation.

## Ordre et conditions pour 06.4

1. Rejouer la baseline en lecture seule dans le vrai UE5.5.4 ; comparer tailles/empreintes et graphes de référents, puis inventorier les instances SaveGame réellement disponibles.
2. Préparer un outil de conversion **Editor-only**, explicitement lancé, avec dry-run et sauvegarde, et refuser toute donnée manquante ou ambiguous.
3. Migrer les définitions/catalogues/paramètres physiques puis les niveaux, réceptacles, palettes, kits et UI par l'API Unreal Editor ; faire la parité de chaque valeur et action.
4. Tester l'intégrité d'ItemId, ItemLevel, GUID, poids et consommation ; refaire Save/Continue, projectiles/PIE et résolution d'énigmes sur des niveaux authentiques.
5. Exiger une preuve AssetRegistry sans anciens référents, et une preuve C++ sans anciens lecteurs, **avant 06.5**. Supprimer alors les outils temporaires et champs legacy.
6. Réserver la conclusion 06.6 au Cook/Shipping et aux essais PIE/Chaos/UMG, distincts des tests automatiques `-NullRHI`.

## Conservation des garanties de régression AI/AJ

`CanonicalReceptacleMaterialization` et
`CanonicalWorldItemLevelBySource` refusaient jusqu'ici un
ItemId canonique valide si un ancien DataAsset associé portait un ID
différent. BG remplace **seulement cette attente devenue obsolète**
par des comparaisons aux vraies données enregistrées N6/N7
(identité, niveau, masse et nom). Pour les ItemId encore absents du
registre, le refus de définitions contradictoires reste testé.
Aucun test n'est désactivé et aucune valeur numérique n'est inventée.

## Vérifications BG

Trois Automation `Grimrock.RPG.ARM.CLEAN06_3.ReceptacleIdentity` : `PhysicalInsertionCapture`, `LevelAndLegacyRules`, `ConflictingActorCapture`. Elles utilisent l'acteur réceptacle réel, `DA_ItemSystem`, `Item_Arrows_01` N0, `Weapon_StraightSword_01` N20 et la capture `FGridRuntimeReceptacleState`.

BG ne modifie **aucun** `.uasset`, `.umap`, `.sav`, format SaveGame, WBP ni texte joueur. `UI.csv → ST_UI.uasset → WBP → Game/fr/en` reste inchangé. Les contrôles Windows ne prouvent pas les animations, collisions, événements d'énigme ni le rendu PIE.
