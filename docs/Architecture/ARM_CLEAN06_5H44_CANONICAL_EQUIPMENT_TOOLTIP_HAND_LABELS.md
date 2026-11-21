# ARM-CLEAN06.5H.44 — Libellés de mains du tooltip sans ancien DataAsset

## Migration

La fixture `Grimrock.UI.EquipmentTooltip043D.TwoHandsHeader` lit maintenant `Weapon_GreatAxe_01` (ARM031 N1) et `Weapon_WarPick_01` (ARM025 N1) depuis `UGridItemSubsystem` dans un monde d'automation transitoire. L'inventaire est lié à ce monde. Aucun `DA_Weapon_GreatAxe_01` ni `DA_Weapon_WarPick_01` n'est chargé ou enregistré pour construire le tooltip.

Le test passe `nullptr` pour toute définition ancienne en laissant `UGridInventorySlotWidget::GetTooltipView` retrouver le registre du monde via le composant d'inventaire du parent. Il vérifie les politiques MainHand et OffHand des armes et la génération des libellés ST_UI existants.

## Contrats inchangés

L'arme deux mains affiche le libellé localisé « deux mains » et non « MainHand » ; l'arme une main conserve les deux labels MainHand et OffHand sans être étiquetée deux mains. Les paquets de production sont lus sans modification.

Aucun code UI, gameplay, WBP, `.uasset`, `.umap` ou `.sav` n'est modifié. Les 53 packages historiques H.23 restent présents.

H.45 interdit sans `UE5.5.4/SilentTask` et `UE5.5.4/LocalAuto` SUCCESS sur le SHA H.44, comprenant compilations UE5.5.4 Editor / Shipping et test `Grimrock.UI.EquipmentTooltip043D.TwoHandsHeader`. Les sept sauvegardes doivent toujours subir l'audit natif avant suppression.
