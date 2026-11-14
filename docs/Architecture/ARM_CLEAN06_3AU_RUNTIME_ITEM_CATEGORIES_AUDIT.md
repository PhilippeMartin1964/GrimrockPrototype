# ARM-CLEAN06.3AU — Audit runtime des catégories d'inventaire

**Parent :** 06.3AT, commit `1f96372`, testé sur UE 5.5.4.

## Diagnostic confirmé dans les sources de production

Les filtres et tris de `GridInventoryWidget.cpp` et la catégorie
affichée dans `GridInventorySlotWidget.cpp` dépendaient encore de
`UGridItemDefinitionAsset::ItemType`, même lorsqu'un `ItemId`
appartenait à `DA_ItemSystem`. Le nom, le poids et les emplacements
d'équipement étaient pourtant déjà résolus à partir des catalogues
canoniques. Depuis 06.3AT, une ancienne définition peut légitimement
être absente après reconstitution d'une sauvegarde.

Un `DA_Weapon_*` périmé pouvait donc classer une épée comme potion,
un bouclier comme livre, ou les masquer du filtre « Équipement ».

## Règle de catégorie unique pour les trois lecteurs UI

`FGridCanonicalInventoryItemTypeReadModel` est un adaptateur **transitoire
et en lecture seule**, partagé entre le filtre, le tri par type et
l'infobulle. Il ne stocke aucun nouveau champ dans l'ItemSystem,
le SaveGame ou les anciennes définitions.

| Identité physique | Projection UI | Origine |
|---|---|---|
| ARM enregistré N1–N20, `Weapon.Kind=Shield` | `EGridItemType::Shield` | `DA_ItemSystem` |
| ARM enregistré N1–N20, `Weapon.Kind=Weapon/Focus` | `EGridItemType::Weapon` | `DA_ItemSystem` |
| Non-ARM canonique, notamment munitions N0 | `EGridItemType::Misc` temporaire | absence de type `Ammo` dans l'enum UI |
| ItemId canonique à niveau invalide | `EGridItemType::None`, non résolu | refus explicite de fallback |
| ItemId absent du registre | `ItemType` historique si `ItemDefinitionId` correspond | compatibilité transitoire |
| ItemId inconnu sans définition correspondante | `None` | aucune catégorie inventée |

L'absence de type `Ammo` dans l'énumération existante est un
**point d'architecture à résoudre séparément** : la classification
`Misc` n'est pas un ajout de gameplay. Le cas des futurs objets
canoniques non-ARM exige une taxonomie UI dérivée de leur famille,
sans réintroduire des champs d'authoring copiés.

L'affichage UMG ne crée aucune nouvelle chaîne ; les libellés
`ItemTypeWeapon` / `ItemTypeShield` localisés dans le code existant
sont réutilisés. Les capacités de sac et la logique de tri secondaire
restent identiques.

## Lecteurs historiques encore bloquants avant ARM-CLEAN06.4

| Source inspectée | Dépendance résiduelle | Suite |
|---|---|---|
| `GridInventorySlotWidget.cpp` | `if (!Definition) return View` coupe d'autres données de tooltip, malgré la catégorie désormais canonique | sécuriser les actions/effets nécessaires sans inventer d'effet |
| `GridItemContextActionLibrary.cpp` | `Consume`, `Read`, `AddToHotbar` et le motif d'erreur de lancer utilisent des champs historiques | migrer uniquement les actions véritablement authorées |
| `GrimrockPartyPawnItemTransfer.cpp` | chemin de lancer et récupération de définition runtime | qualifier avec `GridLevelRuntimeActorWorldItems.cpp` |
| `GridPartyInventoryComponentHotbar.cpp` | branche QuickItem non ARM dépend des `CombatActions` historiques | relier aux bindings d'actions authentiques |
| `GridPartyInventoryComponentDiagnostics.cpp` | diagnostic de compatibilité peut citer `CompatibleEquipmentSlots` historique | séparer diagnostic et autorité gameplay |
| `RehydrateOwnedItemDefinitions` / SaveGame | schéma courant 27 ; anciens ARM N0 invalides non convertis | conversion Editor ponctuelle sous sauvegarde en 06.4 |

Ce recensement repose sur la lecture des sources C++ : il **n'est pas**
un inventaire exhaustif des références `.uasset` et ne permet aucune
suppression d'assets. Les points ci-dessus exigent chacun un test
concret de lecture canonique ou une preuve d'absence de référence.

## Couverture automatisée

`Grimrock.RPG.ARM.CLEAN06_3.CanonicalInventoryFilterSortAndTooltipTypes`
utilise le registre réel `DA_ItemSystem` et des widgets dans un
`UWorld` transitoire : armes/boucliers N1 et N20, flèches N0,
définitions contradictoires, objets non enregistrés, niveaux ARM N0/N21,
filtres, projections et nombres de slots, tris par type croissant et
décroissant, labels des tooltips.

Conserver `Grimrock.UI.Filter01.*` et toute la suite `Grimrock`.

## Localisation et rendus : état avant/après

**`UI.csv → ST_UI.uasset → WBP → Game/fr/en` : inchangé.**
Aucune clé, chaîne, widget, String Table, traduction ou asset binaire
modifié ; aucune matérialisation `ST_UI` nouvelle requise. Le test
automatique fonctionne en `-NullRHI` ; il ne prouve pas le rendu
PIE/GPU ni un test de cultures visuel fr/en.

**Le jalon 06.3AU ne valide pas encore le passage complet à 06.4.**
