# ARM-CLEAN06.5H.31 — Contrat huile du Transmutateur depuis ItemSystem

## Migration limitée au test d'action

La fixture `Grimrock.RPG.ARMEMENT01.17.1.RealOilFlaskNoTalentBypass` n'utilise plus `DA_Item_Flask_Oil` : son ItemId `Item_Flask_Oil` N0, son caractère non-ARM, ses tags d'alchimie/flasque et sa liaison d'action `Action_Alchemist_OilSlick` sont contrôlés dans le registre canonique. La définition d'action complète est lue avec `ResolveDetailedNonArmAction` et non reconstruite par `BuildQuickItemCombatActionDefinition` sur un ancien asset.

Les assertions conservées vérifient la politique QuickItem, 2 PA, 4 cases, dépense d'une flasque, aucune compétence de niveau 2 requise, une surface Oil, sa durée et son modificateur de traversée. Si des tags réellement absents du registre sont détectés, l'agent Windows doit refuser la migration plutôt que dissimuler la perte de données.

## Exclusion explicite

Le test distinct `Grimrock.RPG.ARMEMENT01.17.1.TransmuterNewGameSaveContinue` conserve provisoirement `DA_Item_Flask_Oil` car il caractérise encore les pointeurs historiques des kits source et leurs transformations. Les tests d'authoring et de SaveGame doivent être convertis dans un lot distinct avant suppression native, sans affaiblir l'identité/quantité/GUID des kits.

Aucun `.uasset`, `.umap`, `.sav`, gameplay, effet visuel ou schéma persistant n'est modifié. Aucun des 53 packages H.23 n'est supprimé.

H.32 est bloqué jusqu'à `UE5.5.4/SilentTask = SUCCESS` et `UE5.5.4/LocalAuto = SUCCESS` sur ce SHA précis (Editor, Shipping, Automation, notamment `Grimrock.RPG.ARMEMENT01.17.1`).
