# ARM-CLEAN06.3P — Mesh canonique lors de l'insertion dans un réceptacle

**Base :** jalon 06.3O, validation Windows sur son SHA exact exigée avant publication.

Le jalon 06.3M avait raccordé le mesh des objets *restaurés* dans un réceptacle, mais `AGridReceptacleActor::AddContainedItem` utilisait toujours l'ancienne représentation lors d'une insertion ou de la création du contenu initial. Désormais, après avoir construit la `FGridItemInstance` transitoire qui sert déjà à `ApplyVisualPlacement`, le code appelle `AGridItemActor::ApplyCanonicalWorldVisual` avec son vrai GUID, son ItemId, son niveau fixé et sa quantité. Cette résolution ne remplace ni la position ni la politique `PhysicalAtHit` : elle précède le placement et la simulation Chaos.

Une identité ARM enregistrée lit uniquement `DA_ItemSystem` et son `DA_WeaponVisualCatalogue` pour le mesh et l'offset. Un niveau invalide connu ne reprend pas de mesh legacy ; un objet hors registre conserve son rendu historique. Aucune conversion de niveau 0, sauvegarde, acteur, asset binaire, WBP ou règle d'interaction n'est introduite.

Le test `Grimrock.RPG.ARM.CLEAN06_3.FreshReceptacleItemVisual` charge le vrai registre, ARM014/031/075/081 N1/N20, et vérifie les références, transformations et refus du niveau 0. Les tests historiques des réceptacles restent dans `Grimrock`. La validation automatisée en `-NullRHI` ne remplace pas la vérification ultérieure du rendu dans le donjon.

**Critère :** un commit unique `master`, `UE5.5.4/SilentTask` et `UE5.5.4/LocalAuto` success sur le SHA publié.
