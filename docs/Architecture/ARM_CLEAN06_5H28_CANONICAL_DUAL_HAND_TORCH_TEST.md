# ARM-CLEAN06.5H.28 — Torches N0 équipées sans définition héritée

## Fixture canonique

`Grimrock.RPG.ARM.CLEAN06_4X17.TorchCanonicalNonArmHandEquip` ne charge plus `DA_Item_Torch` et n'enregistre plus l'ancien `UGridItemDefinitionAsset` dans l'inventaire. Le test utilise `Item_Torch` N0 dans `UGridItemSubsystem`, dont les slots autorisés MainHand et OffHand, le caractère non-ARM et le profil lumineux sont explicitement contrôlés.

Le modèle de lecture `FGridCanonicalEquipmentSlotsReadModel::Resolve` reçoit `nullptr` pour la définition historique : la liste des deux mains provient donc intégralement de la définition canonique. Les deux instances distinctes gardent leurs GUID et leur état allumé après équipement. Les contrôles de l'éclairage réel du groupe, de l'intensité et du rejet de l'équipement des flèches N0/N1 restent inchangés.

## Fixtures à conserver

`UGridItemDefinitionAsset` reste inclus dans ce fichier pour `OldAmmo` et les autres objets **transitoires** fabriqués volontairement afin de prouver les refus/fallbacks. Ce changement n'autorise pas encore la suppression de l'ancienne classe C++ ni celle des 53 packages historiques H.23.

Aucun `.uasset`, `.umap`, `.sav`, VFX, lampe, HUD ou code gameplay n'est modifié.

## Validation

La publication de H.29 est bloquée jusqu'à `UE5.5.4/SilentTask = SUCCESS` ET `UE5.5.4/LocalAuto = SUCCESS` sur le SHA H.28, avec compilation Editor/Shipping/Automation et tests `Grimrock.RPG.ARM.CLEAN06_4X17`, `Grimrock.RPG.ARM.CLEAN06_4X23` et illumination. Les vérifications natives des sauvegardes demeurent nécessaires avant tout retrait.
