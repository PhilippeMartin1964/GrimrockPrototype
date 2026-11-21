# ARM-CLEAN06.5H.22 — Butin Gobelin : supprimer la dépendance des tests aux trois anciens DA_Item

**Objectif :** continuer le nettoyage des consommateurs de `UGridItemDefinitionAsset` avant le retrait natif des 48 sources historiques, après H.21. Aucun asset n'est effacé dans ce lot.

Le test `Grimrock.Monsters.MON17.7.ProductionLootBaseline` vérifiait trois anciens packages `DA_Item_GoblinKnife`, `DA_Item_Stone` et `DA_Item_EmptyVial` par `LoadObject<UGridItemDefinitionAsset>`. Ce schéma créait artificiellement une condition d'existence des anciennes définitions après la migration canonique des tables de butin.

Désormais, le test charge la seule racine `CanonicalStage/DA_ItemSystem`, recherche les trois `ItemId` par `UGridItemSystemAsset::ItemDefinitions`, vérifie les identités et conserve **toutes** les assertions sur la table de butin du Gobelin, les chances, quantités et calcul `ExpectedItemsPerKill`. Aucun octroi d'objet, règle de drop, mécanique de combat ni SaveGame n'est modifié.

La suite Automation Editor et la compilation Win64 Shipping sont obligatoires sur le nouveau SHA. **Le nombre de packages historiques demeure 53 (48 sources, 2 catalogues d'authoring, 2 Readables protégés, 1 redirecteur)** jusqu'à une transaction native distincte et vérifiée. La suppression est bloquée par l'ancien audit strict tant que les sources ne sont pas entièrement préparées.
