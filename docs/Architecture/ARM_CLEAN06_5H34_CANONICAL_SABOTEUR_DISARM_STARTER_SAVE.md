# ARM-CLEAN06.5H.34 — Saboteur, outils de désamorçage et Save/Continue canoniques

## Deux fixtures de production migrées

Dans `GridARMEMENT01163SaboteurStarterTests.cpp`, suppression de la fonction de chargement direct `DA_Item_DisarmTools_01` et des lectures de `UGridItemDefinitionAsset`.

- `Grimrock.RPG.ARMEMENT01.16.3.SaboteurKitOnly` utilise `FGridCanonicalRegistryFixture` et la seule identité `Item_DisarmTools_01` N0. Elle vérifie le tag `Tool.Disarm`, la politique non-ARM non équipée, la quantité maximale unitaire, les trois kits Rogue, leur sélection et les entrées `CanonicalItemId` sans pointeur historique.
- `Grimrock.RPG.ARMEMENT01.16.3.N1DisarmAndSaveContinue` garde sa vraie `FWorldFixture` et son `UGridItemSubsystem`, déjà utilisé par le gameplay. Le même ItemId canonique est employé dans la création du héros, l'inventaire, le désamorçage, la persistance et la restauration.

Les assertions d'origine demeurent : aucune compétence gratuite, interdiction de désamorcer sans entraînement, aucun RNG consommé au refus, achat du premier rang sur quatre points, succès DC1 sous Dextérité, conservation des outils et de leur GUID, statut du piège désamorcé et SaveGame binaire en mémoire.

## Non-destructif et validations

Aucune modification de la compétence, du piège, d'ItemSystem de production, de Niagara, d'huile, du ciblage, d'asset `.uasset`/`.umap` ou sauvegarde réelle `.sav`. Les 53 packages historiques H.23 restent intacts.

H.35 exige `UE5.5.4/SilentTask = SUCCESS` et `UE5.5.4/LocalAuto = SUCCESS` sur H.34 (Editor, Shipping et Automation `Grimrock.RPG.ARMEMENT01.16.3`). Aucun retrait d'asset sans nouveau contre-audit des sept sauvegardes natives.
