# ARM-CLEAN06.3L — Mesh d'objet tenu par instance équipée

## Périmètre

`AGrimrockPartyPawn::SyncHeldVisualFromSelectedCharacterEquipment` a accès à **l'instance réellement équipée**, pas seulement à son `ItemDefinitionId`. Pour un item présent dans `UGridItemSubsystem`, il résout le `VisualId`, `WorldMesh` et `EquippedMesh` uniquement par `ResolveItem(ItemId, ItemLevel)`, et applique le soft mesh équipé sur `HeldItemActor->MeshComponent`. La résolution canonique hérite de la règle déjà authorée : `EquippedMesh` absent → `WorldMesh`. Pour une identité canonique valide mais à niveau invalide (N0), **aucun repli** sur le mesh historique ; le mesh de présentation est vidé.

Le circuit historique direct `EquipHeldItem(FName)` est conservé temporairement pour les appels qui ne fournissent pas de niveau réel ; cette fonction sert seulement à instancier un acteur porteur et sa configuration lumineuse. Le chemin `SyncHeldVisualFromSelectedCharacterEquipment` applique ensuite le mesh canonique dans la même opération lorsqu'il reçoit l'instance équipée. Les items non enregistrés, les composants lumineux et les effets indépendants ne changent pas. La visibilité habituelle des seules sources lumineuses reste inchangée : **aucune nouvelle arme n'apparaît devant la caméra par défaut**.

La résolution de soft mesh est déclenchée par l'opération de synchronisation existante, sans modification `Tick`, animation, projectile, matériau, placement dans le monde, physique, sauvegarde ni asset UE.

## Validation

`Grimrock.RPG.ARM.CLEAN06_3.CanonicalHeldItemMesh` vérifie ARM014, ARM031, ARM075 et ARM081 N1/N20 sur le vrai `DA_ItemSystem`, identité visuelle, maillage world/equipped, refus des niveaux invalides et absence d'autorité pour item hors registre.

La compilation et la suite Automation complète `Grimrock` doivent être vertes sur le SHA du jalon. Le rendu réel de la torche ou d'une arme lumineuse et ses effets attachés reste **à vérifier visuellement en PIE** avant de déclarer la présentation achevée ; `-NullRHI` ne remplace pas ce contrôle.

## Limites

Le mesh des pickups/placements monde, leur `ItemLevel` et les transformations du catalogue visuel nécessitent un raccordement spécifique. Les champs `WorldMesh` et `EquippedMesh` de l'ancien DataAsset ne doivent pas encore être supprimés.
