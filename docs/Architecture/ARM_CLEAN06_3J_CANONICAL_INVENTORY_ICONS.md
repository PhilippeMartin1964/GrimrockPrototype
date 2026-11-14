# ARM-CLEAN06.3J — Icônes d'objet du sac et de l'équipement canoniques

**Base :** ARM-CLEAN06.3I, commit `7c6b9f8`, validation automatique Windows `UE5.5.4/LocalAuto = success`.

## Périmètre et source d'autorité

Le widget C++ `UGridInventorySlotWidget::SetItem` utilise désormais `FGridWeaponVisualUiResolver::ResolveInventoryItemIcon` pour une vraie instance (GUID, ItemId, ItemLevel). Quand `UGridItemSubsystem` connaît l'`ItemId`, l'icône vient strictement de `FGridResolvedItem::ItemIcon` dans le **seul** `DA_ItemSystem` / `DA_WeaponVisualCatalogue`, sans consulter les champs `Icon`, `WeaponVisualCatalogue`, `BaseIcon` copiés sur `DA_Weapon_*`. Le `VisualId` de `FGridResolvedItem`, exposé uniquement de façon transitoire, provient directement de `FGridCanonicalWeaponVisual::VisualId`.

Le même cache `CachedIconTexture` et `CachedVisualId` sert à la cellule d'inventaire et à `GetTooltipView` ; la distinction **icône d'objet ≠ icône d'action** est préservée. La texture soft est chargée lors du rafraîchissement du slot, jamais sur Tick. Les 22 armes ARM enregistrées sont concernées, les familles non enregistrées continuent temporairement à utiliser `FGridWeaponVisualUiResolver::Resolve`. Un identifiant canonique connu et invalide (ex. N0) a une icône vide, sans repli visuel sur une ancienne icône ; le niveau persistant n'est jamais corrigé silencieusement.

Aucun widget UMG, String Table, CSV de localisation ou texte joueur n'est modifié ; le changement est exclusivement la source C++ de la référence iconographique. La checklist de `docs/UI_WBP_LOCALIZATION_WORKFLOW.md` a été relue : **UI.csv inchangé → ST_UI inchangée → WBP inchangé → Game/fr/en inchangé**. Aucune clé ou nouvelle matérialisation n'est requise puisqu'aucun texte visible n'est ajouté, supprimé ou modifié. La validation visuelle de la nouvelle source d'icône reste distincte du test de localisation.

## Validation de non-régression

`Grimrock.RPG.ARM.CLEAN06_3.CanonicalInventoryItemIcons` utilise le vrai `DA_ItemSystem` sauvegardé, ARM014/ARM031/ARM075/ARM081 N1 et N20, une ancienne icône volontairement contradictoire et des instances avec GUID. Il vérifie la parité `ItemIcon`, `VisualId`, l'absence de fallback pour N0 et le maintien du résultat pour un objet non enregistré.

Le test ne demande ni WBP ni GPU ; la qualité du dessin, l'échelle et la transparence de l'icône demandent ultérieurement une validation PIE visuelle. **Aucune suppression de définition, de palette, de référence soft ni de SaveGame n'est permise à cette étape.**

## Suites de migration

Les meshes `AGridItemActor`, la prévisualisation d'objet éditeur et les objets tenus `AGrimrockPartyPawn::EquipHeldItem` ne reçoivent pas tous encore l'`ItemLevel` de l'instance. Ils restent pour un jalon de raccordement explicite ; ne **pas** substituer artificiellement N1 et ne pas supprimer les champs visuels anciens avant la conversion des assets en 06.4.
