# ARM-CLEAN06.3Z — Prévisualisation des objets libres depuis ItemSystem

**Base vérifiée :** ARM-CLEAN06.3Y, SHA `3d1f5d4`, `UE5.5.4/SilentTask` et `UE5.5.4/LocalAuto` success.

## Mise en conformité

`UGridEditorPreviewComponent` utilise désormais une projection transitoire `FGridEditorLooseItemPreviewReadModel` plutôt que de charger directement le visuel historique d'une **identité déjà enregistrée**. Le `WorldMesh`, `WorldVisualTransform` et `VisualId` viennent de `UGridItemSubsystem::ResolveItem`; une identité canonique à niveau invalide ne reprend jamais `DA_Weapon_*.WorldMesh`.

L'éditeur de niveau n'a pas toujours de `GameInstance`. Pour un monde éditeur, un `UGameInstance` transitoire et un `UGridItemSubsystem` transitoire référencent **le même** `DA_ItemSystem` configuré dans `DefaultGame.ini` (aucun second fichier de catalogue) et restent propriétaires de leurs références pendant les rebuilds. En PIE, le composant réutilise directement le sous-système du `GameInstance` réel. Les identités hors registre et le cas du registre indisponible conservent temporairement l'affichage historique, sans migration binaire.

**Sémantique exacte :** un placement d'arme avec `ItemLevel=0` signifie « hériter du `DefaultWeaponItemLevel` de la définition de placement », comme `AGridLevelRuntimeActor::AddPlacedItemActor`. Un placement de flèches non ARM avec `ItemLevel=0` est une valeur **réelle**. Le modèle ne modifie ni `FGridLooseItemInstance`, ni le niveau sauvegardé, ni une instance runtime. Les placements explicitement N1/N20 conservent leur niveau. `IsPreviewableLooseItem` lit la même autorité et ne filtre plus une arme parce que son ancien DataAsset n'a pas de mesh.

## Tests

`Grimrock.RPG.ARM.CLEAN06_3.EditorLooseItemCanonicalPreview` charge le vrai `DA_ItemSystem`, compare ARM054/031/075 à niveaux 1/20 et héritage 0→3, vérifie les meshes et transformations, le refus N21, les flèches N0 et un objet non enregistré. Après push, **les deux statuts Windows** doivent être verts sur le SHA exact.

Aucun `.uasset`, `.umap`, SaveGame, WBP, texte localisé ou classe Editor module n'est modifié. `UI.csv → ST_UI → WBP → Game/fr/en` inchangé. Le rendu d'aperçu hors PIE reste une vérification visuelle à faire ultérieurement, distincte de `-NullRHI`.
