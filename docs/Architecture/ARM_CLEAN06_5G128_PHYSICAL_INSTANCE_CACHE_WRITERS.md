# ARM-CLEAN06.5G.128 — Retrait des derniers écrivains de cache d'instance identifiés

Parent exact `765699a66ce9cc19e5661b252e759a36baef18fa`. Retrait groupé des affectations de `FGridItemInstance::DisplayName` et `Weight` identifiées dans les chemins runtime et les fixtures ciblés, sans toucher aux champs physiques (GUID, ItemId, ItemLevel, Quantity, Owner), à la lumière, aux réserves d'armure, ni au contenu lisible.

- Runtime : préparation de projectile (poids d'une branche Automation), restitution depuis un réceptacle et menus « Examine » / « Read ». Les événements Blueprint conservent leur contrat `FGridItemInstance` et leurs paramètres explicites de texte. Les deux membres statiques non réfléchis étaient invisibles au Blueprint depuis 5G.93 ; aucun texte joueur ni rendu Designer n'est modifié.
- Tests : `GridUIItem01`, `GridEquipmentTooltip043B` et `PIT02/PIT03/PIT031` ne recopient plus le nom ou la masse statique dans leurs instances ; les assertions existantes de nom, comparaison d'équipement, quantité, chute et poids canonique restent inchangées.
- Le poids et le nom sont résolus depuis la définition / le registre ; aucun fallback depuis l'instance n'est réintroduit. Les deux membres C++ demeurent temporairement dans `GridInventoryTypes.h` bien qu'ils ne soient déjà plus réfléchis ni sérialisés. Leur suppression physique fera l'objet d'un lot séparé après audit exhaustif des références.

Aucun `.uasset`, `.umap`, WBP, `UI.csv` ou SaveGame modifié. La validation UE5.5.4 `SilentTask` et `LocalAuto` sur le nouveau SHA est obligatoire ; la suite `Grimrock` doit couvrir les modules d'inventaire, de combat, de réceptacles et de fosses. La compilation et l'Automation NullRHI ne prouvent pas le rendu PIE.
