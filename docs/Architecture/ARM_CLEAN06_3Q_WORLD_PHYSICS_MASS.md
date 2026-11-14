# ARM-CLEAN06.3Q — Masse physique de pickup depuis le registre unique

**Base :** ARM-CLEAN06.3P validé par les tests Windows sur son SHA.

Le modèle `FGridWorldPhysicsMassReadModel` utilise la masse `WeightKg` de `UGridItemSubsystem` pour les objets enregistrés dans `DA_ItemSystem`, au niveau réel de l'instance tel que capturé lors de `AGridItemActor::ApplyCanonicalWorldVisual`. Il ne multiplie **jamais** par la quantité : le corps Chaos correspond à un seul mesh. La politique existante `bUseItemWeightAsWorldPhysicsMass` reste une option temporaire de l'ancienne définition ; seule la valeur numérique de la masse est migrée à ce stade.

L'acteur mémorise uniquement un `CanonicalWorldItemLevel` **transitoire** (ni nouvelle propriété SaveGame ni ancien snapshot statique). `ConfigureAsWorldPickup` et les objets déjà simulés lors d'une restauration appellent `RefreshWorldPickupMass`, qui conserve la désactivation de l'override si l'opt-in est absent. Un ItemId connu mais invalide (N0) n'utilise jamais `DA_Weapon_*.Weight`.

Les trajectoires de déplacement, la gravité, le matériau, la rotation initiale, le placement des réceptacles et les transformations sauvegardées sont inchangés par le code ; la masse effective d'un mesh peut modifier les réactions Chaos si les anciens DataAssets contenaient une valeur différente, ce qui rend la recette PIE physique **obligatoire** avant la conclusion visuelle de 06.3.

Le test `Grimrock.RPG.ARM.CLEAN06_3.CanonicalWorldPhysicsMass` compare les références N1/N20 du vrai ItemSystem sur ARM014/031/075/081 à une ancienne définition contradictoire, vérifie qu'une quantité x4 ne multiplie pas la masse Chaos, le refus N0 et le maintien des opt-in legacy.

**Sortie automatisée :** un commit `master`, `UE5.5.4/SilentTask` et `UE5.5.4/LocalAuto` success sur le SHA exact. Rendu GPU et physique PIE à valider séparément.
