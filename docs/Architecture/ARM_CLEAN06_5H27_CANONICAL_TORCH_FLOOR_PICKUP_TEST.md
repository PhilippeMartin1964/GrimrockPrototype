# ARM-CLEAN06.5H.27 — Test de torche au sol et reprise sans définition historique

## Modification

La fixture `Grimrock.RPG.ARM.CLEAN06_4X18.TorchFloorDropAndPickupLightState` utilise maintenant uniquement l'identité canonique `Item_Torch` N0 de `UGridItemSubsystem`. Le chargement `DA_Item_Torch` (`UGridItemDefinitionAsset`) et l'enregistrement manuel de cette ancienne définition dans l'inventaire ont été retirés, ainsi que l'include devenu inutile.

Le contrat existant reste inchangé : création de la torche allumée au sol, Niagara et lumière ponctuelle canoniques, capture de l'état monde, ramassage, GUID préservé, équipement MainHand et illumination du groupe, puis seconde torche explicitement éteinte conservant son état au ramassage.

La fixture continue d'exiger le registre canonique, la projection de la torche N0 et sa définition d'effets réels; elle refuse explicitement de traiter la torche comme une arme ARM. Les autres tests de restauration Save/Continue du fichier ne sont pas modifiés.

## Exclusions et contrôles

Aucun `.uasset`, `.umap`, `.sav`, fichier UI, Niagara, joueur, règle physique ou scène de jeu n'est modifié. Les 53 packages historiques H.23 restent intacts. Le défaut de ciblage jaune/vert Shipping demeure différé (issue #221).

Validation obligatoire sur le SHA du commit : `UE5.5.4/SilentTask = SUCCESS` puis `UE5.5.4/LocalAuto = SUCCESS` comprenant Editor/Shipping/Automation, en particulier `Grimrock.RPG.ARM.CLEAN06_4X18.TorchFloorDropAndPickupLightState` et `Grimrock.RPG.ARM.CLEAN06_4X1610`. Ne pas publier H.28 avant ces preuves.
