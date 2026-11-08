# ARMEMENT-LOT02.17 — Validation des 18 kits de départ N1

## Contrat

Ce jalon prolonge ARMEMENT-LOT02.16 et ARMEMENT01.13.2. Six classes de production proposent trois kits approuvés chacune, soit 18 choix propres aux classes. Le choix dans le wizard, la création réelle du personnage et l'écriture/relecture sur disque doivent conserver les objets physiques sans duplication ni disparition.

**Références :** docs/Design/ARMEMENT01_9_6_STARTER_KIT_RECIPES.csv (18 recettes approuvées), Config/Armement/WeaponProductionManifest.csv (22 armes Active), FRPGStartingEquipmentPresetCatalog::ApprovedKits (dotations effectivement attribuées dans le jeu packagé).

## Équipement prévu à la création

Tous les armements ARMxxx sont de niveau N1. Chaque objet vaut une unité sauf indication contraire. Aucun kit ne confère automatiquement un talent supérieur.

| Classe | Orientation | MainHand | OffHand | Sac |
|---|---|---|---|---|
| Warrior | Guardian | ARM014 | ARM075 | — |
| Warrior | Breaker | ARM031 (deux mains) | — | — |
| Warrior | WeaponMaster | ARM015 | — | ARM025 |
| Rogue | Assassin | ARM002 | — | ARM054 |
| Rogue | Shadow | ARM001 | ARM013 | — |
| Rogue | Saboteur | ARM003 | — | Item_DisarmTools_01 |
| Ranger | Marksman | ARM045 | — | ARM001, Item_Arrows_01 ×12 |
| Ranger | Hunter | ARM038 | — | ARM057 |
| Ranger | Scout | ARM048 | — | ARM001, Item_Torch, Item_Arrows_01 ×12 |
| Mage | Evoker | ARM062 | — | — |
| Mage | Arcanist | ARM061 | ARM070 | — |
| Mage | SurfaceWeaver | ARM067 (deux mains) | — | Item_SurfaceCatalyst_01 |
| Priest | Restoration | ARM069 | — | Item_Potion_Healing_01 |
| Priest | Protection | ARM020 | ARM075 | — |
| Priest | Exorcism | ARM069 | ARM081 | — |
| Alchemist | Grenadier | ARM001 | — | ARM058, Item_Bomb_Fire |
| Alchemist | Apothecary | ARM001 | — | Item_Potion_Healing_01 |
| Alchemist | Transmuter | ARM001 | ARM070 | Item_Flask_Oil |

Note de cohérence : le CSV de conception est antérieur à plusieurs objets de soutien réellement réalisés. Ses colonnes ProductionReadiness et PlannedSupportNeed sont historiques, pas un diagnostic d'absence en novembre 2026. Les dotations actuelles incluent notamment les outils Saboteur, les flèches ×12 de chaque Rôdeur archer, les potions Restoration/Apothecary, le catalyseur SurfaceWeaver et la flasque d'huile Transmuter. Ne pas ajouter des recettes de transmutation ou talents N2 fictifs.

## Validation automatisée

Le test historique Grimrock.RPG.ARMEMENT01.13.ApprovedKits.ThreeOptionsPerClass contrôle les 18 options et la sélection dans le read-model du wizard. Le test Grimrock.RPG.ARMEMENT01.13.ApprovedKits.CreateAllEighteen vérifie déjà une vraie attribution par CreateInitialCharacter pour chacune des 18 orientations, les emplacements, quantités, GUID, N1, définitions enregistrées et ownership. **ARMEMENT-LOT02.17 étend ce dernier test** pour les 18 kits :

1. SaveGameToSlot écrit un slot de disque temporaire indépendant pour chaque kit ;
2. LoadGameFromSlot recharge les données sérialisées et vérifie la compatibilité de version ;
3. un NOUVEAU UGridPartyInventoryComponent restaure l'inventaire et réhydrate les définitions par identité exacte du catalogue de démarrage ;
4. les 18 cas comparent kit ID, identité du héros, nombres physiques d'objets, GUID, slot/sac, N1, quantités, propriétaire, résolution des définitions et invariants d'ownership ;
5. chaque slot de test est supprimé par un garde de nettoyage, même en cas d'échec.

Le test complémentaire Grimrock.RPG.ARMEMENT.LOT02.DiskPersistence22N1 utilise en outre les véritables entrées Pawn.SaveCurrentGame / Pawn.LoadCurrentGame pour le catalogue global de 22 armements. **Limite explicite** : le test des 18 kits utilise les API SaveGame et un composant d'inventaire neuf, et non un cycle de fermeture de l'application ou le rendu visuel du wizard. Il ne prouve donc pas une navigation PIE complète dans les écrans du jeu.

## Recette PIE finale, à observer dans Unreal Editor

Dans L_MainMenu, lancer New Game pour les six classes : chacune doit afficher trois kits, un choix par défaut pertinent, une description localisée et la possibilité de sélectionner l'un de ses deux autres kits. La classe suivante ne doit pas hériter de la sélection précédente. Les anciens kits de création ne doivent pas figurer dans le wizard.

Créer ensuite les 18 variantes et consulter pour chacune l'équipement MainHand, OffHand et le sac : contrôler l'objet prévu, sa quantité, son niveau et son icône. Vérifier spécifiquement les armes à deux mains, les deux dotations de 12 flèches, les outils, potions, catalyseur, bombe et huile. Tester les boutons de sélection réels du WBP et la lisibilité du résumé dans les langues fr/en.

Pour une variante représentative de chacune des six classes : Save → Quitter réellement → relancer → Continue, puis comparer le kit sélectionné, les objets, l'inventaire, les raccourcis et l'état de L_Dungeon. Le statut des tests Windows en mode NullRHI ne remplace pas cette observation.

## Statut de clôture

Sur le SHA final du commit master, relever **séparément** UE5.5.4/SilentTask et UE5.5.4/LocalAuto. Si les deux sont success, l'implémentation et les 18 tours de disque automatisés sont acquis. **La validation finale visuelle PIE/Continue reste ouverte tant qu'aucune exécution avec interface ne l'a prouvée.**

Aucun fichier uasset/umap/WBP, nouvelle clé ST_UI, arme, talent ou recette de classe de production n'est modifié par ce jalon.
