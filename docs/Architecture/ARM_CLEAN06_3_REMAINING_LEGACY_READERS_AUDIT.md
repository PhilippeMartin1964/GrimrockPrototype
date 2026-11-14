# ARM-CLEAN06.3 — Audit des lectures historiques restantes (14.11.2026)

**Point de référence :** `3fe7ad0` (06.3AN) ; analyse statique des sources
C++ sous `Source/` avant 06.3AO. L'objectif de 06.3 est la convergence
des **consommateurs**, non la suppression prématurée des vieux UPROPERTY.
La présence d'un symbole dans la source ne prouve pas un fallback
effectivement exécuté pour un ItemId canonique.

## Lecteurs de production à examiner avant 06.4

| Zone | Ancienne dépendance | Lecture/risque à qualifier | Priorité |
|---|---|---|---|
| Kits de départ | `CanEquipToSlot`, `GetEffectiveHandUsage` | l'item N20 canonique peut rester traité avec une politique de mains historique ; ciblé 06.3AO | haute |
| Dual-wield | `GridDualWieldPolicy.cpp` | classification des deux sources par définition historique ; vérifier parité avec `ResolveBoundAttackForHand` et refus des passifs | haute |
| Widget d'inventaire | `GridInventorySlotWidget.cpp` | branche historique `GetEffectiveHandUsage` ; distinguer les ItemId enregistrés avant conversion | moyenne |
| Menus d'actions | `GridItemContextActionLibrary.cpp` | permissions de mains issues de la définition ancienne ; confronter à équipement canonique | moyenne |
| Actions de combat | `GridTurnManagerPlayerActionCatalog.cpp` | chemin historique `CombatActions` / `TryGetWeaponCatalogueProfile` ; prouver qu'il est totalement isolé des ARM enregistrés | haute |
| Sauvegardes et instances | `GridItemInstanceMaterializationReadModel.h` et `GridPartyInventoryComponent.cpp` | anciens snapshots/valeurs maintenus tant que 06.4 n'a pas converti les binaires | haute |
| Butins et monstres | `GridMonsterLootResolver.cpp`, `GridMonsterDeathComponent.cpp` | distinction entre chemin canonique déjà converti et branche hors registre ; vérifier transferts/Save | moyenne |
| Réceptacles/monde | `GridReceptacleItemMetadataReadModel.h` | branche ancienne uniquement pour les identités encore absentes du registre | moyenne |
| Affichage des armes | `GridWeaponTooltipProjection.cpp`, `GridEquippedWeaponActionIconPalette.cpp` | préférer catalogue visuel canonique et ne jamais afficher une icône d'objet comme icône d'action | moyenne |
| Éditeur / commandlets | `GridWeaponPipelineCommandlet.cpp`, `GridWeaponPilotItemAuthoringCommandlet.cpp`, `GrimrockWeaponDetailsCustomization.cpp` | authoring historique qui réintroduirait des copies après bascule ; supprimer après conversion, sans écrire les binaires directement | haute |

## Critères de sortie 06.3 → 06.4

1. Tous les lecteurs d'ItemId connu sont passés par le registre unique
   ou ont une preuve explicite de priorité canonique testée.
2. Toute branche historique résiduelle est limitée à des ItemId absents
   du `DA_ItemSystem`, en attendant conversion contrôlée UE.
3. Les 82 archétypes × 20 niveaux (1 640 profils), 260 profils passifs,
   24 DataAssets d'armes inventoriés et 18 kits restent intacts.
4. Aucun `.uasset`, niveau, palette ni sauvegarde n'est supprimé à cette
   étape ; préparer la conversion ponctuelle 06.4 par commandlet UE.
5. Aucun verdict « zéro legacy final » n'est prononcé avant 06.5 :
   ce rapport est une cartographie de travail et non un audit exhaustif
   des références binaires (`AssetRegistry` et commandlets requis).

Les scénarios non couverts par tests sur le vrai jeu doivent être
acceptés visuellement en PIE après la conversion de contenus.

## Complément 06.3AR — HUD d'action

Le lecteur HUD `GridCombatHudWidget.cpp` pouvait copier `ItemDefinition.Icon`
et `ApplyCatalogueIcon` de la palette ancienne dans une action d'arme
pourtant enregistrée. Le jalon 06.3AR isole les actions d'équipement
canoniques et interdit toute réutilisation d'icône d'objet pour ces IDs.
Les raccourcis quick item/unknown ID restent provisoirement legacy ;
les données d'authoring en `.uasset` relèvent toujours de 06.4.

## Complément 06.3AS — inventaire/piles sans définition historique

`FGridItemInstanceMaterializationReadModel` pouvait uniquement
matérialiser les identités connues si un asset historique était
encore enregistré. Le jalon 06.3AS retire cette obligation pour
les instances ARM explicitement nivelées et les objets N0 non-ARM,
et rend `MaxStack` du registre prioritaire dans les entrées, les
fusions et la séparation des piles. L'héritage de niveau authoré N0,
la reconstitution SaveGame et les références d'assets binaires
restent des obligations de 06.4.

## Complément 06.3AT — reconstitution de sauvegarde

`RehydrateOwnedItemDefinitions` vérifie désormais les instances physiques
canoniques de l'inventaire actif, de la réserve, de l'équipement et du
curseur contre `DA_ItemSystem` et leur capacité de pile avant de toucher
le registre temporaire. Les anciennes définitions sont **optionnelles**
pour les IDs connus et **obligatoires** pour les IDs absents du registre.
Leurs présences facultatives maintiennent les consommateurs transitoires.
Le format SaveGame 27 n'est pas modifié et les conversions des
enregistrements N0 historiques appartiennent toujours à 06.4.
