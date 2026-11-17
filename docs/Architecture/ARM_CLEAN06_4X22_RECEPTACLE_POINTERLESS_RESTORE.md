# ARM-CLEAN06.4X.22 — Restauration d'ARM canonique contenu sans source historique

**Date :** 17 novembre 2026. **Parent :** `168594fe504af3ca8cb5489fe6534ea9fd8fca9e`. **C++ et tests uniquement, sans binaires Unreal ni fichiers SaveGame.**

## Correction ciblée

Au chargement de l'état runtime d'un réceptacle, les contenus enregistrés dans `DA_ItemSystem` continuent à résoudre exactement leur `ItemDefinitionId` et leur `ItemLevel`. Pour les ARM :

- Si `ResolveRuntimeItemDefinition` fournit encore une définition historique, le chemin historique est **maintenu** jusqu'à la migration native des catalogues de présentation (X.20).
- Si cette définition **n'existe plus**, l'acteur est désormais créé sans ancien pointeur et initialisé par `InitializeFromCanonicalItem`, comme pour les objets libres déjà restaurés canoniquement. Aucune conversion d'un ARM N0 en N1 n'est permise.
- Quand `RestoreRuntimeContainedItem` reçoit un acteur **déjà initialisé canoniquement** avec le même ID et le même niveau, `AddContainedItem` ne recherche plus artificiellement une ancienne définition à enregistrer dans `FGridContainedReceptacleItem`. Les autres insertions et fixtures historiquement définies restent inchangées.

Ce changement ne retire pas les champs `ItemDefinition`, n'altère pas l'ordre des événements d'insertion/suppression et ne modifie pas le flux de sauvegarde. Les valeurs `GUID`, quantité, niveau N1–N20 et lumière de l'instance sauvegardée restent autoritaires. Un actor canonique ne peut pas être réinterprété comme un ancien armement par une définition de secours.

## Couverture de tests

Une nouvelle automation `Grimrock.RPG.ARM.CLEAN06_4X22.PointerlessCanonicalArmReceptacleRestore` réutilise le monde de test X.21 : le réceptacle produit un ARM initial N20 canonique, capture son état dans `FGridRuntimeReceptacleState`, vide son contenu puis accepte un acteur canonique reconstruit à partir de `FGridItemInstance`. Vérification que `ContainedItems[0].ItemDefinition` et `ItemActor.ItemDefinitionAsset` restent nuls et que GUID/ItemId/N20/lumière sont inchangés.

Les tests existants `Grimrock.RPG.ARMEMENT01.8.Receptacles.*` et la suite `Grimrock` restent requis dans VALIDATION-AUTO01.

## Limites

Un test en mémoire ne prouve pas l'ancienne sauvegarde v27 sur disque, ni la parité visuelle d'un ARM lorsque les propriétés persistantes du catalogue n'ont pas été matérialisées dans Unreal Editor. Les **22 profils visuels ARM réels** et les politiques physiques correspondantes doivent être migrés et comparés avant le retrait des anciens pointeurs des niveaux ou des réceptacles. 06.4 reste partielle ; **06.5 BLOQUÉE**.
