# ARM-CLEAN06.5H.37 — Actions de restauration d'armure sur ARM075 canonique

`GridRPGShield024BRestoreTests.cpp` n'a plus besoin de charger ni d'enregistrer le DataAsset historique `DA_Weapon_RoundShield_01`. La fixture associe son `UWorld` de jeu isolé à un `UGameInstance` et un `UGridItemSubsystem` configurés, libérés après les tests. `Weapon_RoundShield_01` N1 est résolu directement en `FGridResolvedItem`, en vérifiant cinq points d'armure physique et l'absence de cache de définition historique.

Les deux tests existants sont maintenus : restauration `Party` deux fois avec plafond d'armure, restauration `Ally` ciblée, protections contre les modifications de membres non sélectionnés ou à terre, coût/acceptation de l'action, conservation du GUID, non-rétablissement HP/mana et concordance de l'UI.

Aucun gameplay, art, `.uasset`, `.umap` ou `.sav` n'est modifié. Les 53 anciens packages restent en place, sans autorisation de retrait.

Validation requise avant H.38 : `UE5.5.4/SilentTask` et `UE5.5.4/LocalAuto` tous deux SUCCESS sur le SHA de H.37 (Editor, Shipping et Automation `Grimrock.RPG.SHIELD02.4B`). L'audit natif de sept sauvegardes demeure obligatoire avant toute suppression.
