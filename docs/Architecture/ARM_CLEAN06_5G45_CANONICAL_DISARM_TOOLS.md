# ARM-CLEAN06.5G.45 — Désamorçage : autorité canonique exclusive

Parent 4cbeb8f3d391409bf56aadc6146792c85dbb7c7a. Publication après deux statuts SUCCESS.

`HasDisarmTool` ne reçoit plus de pointeur vers UGridItemDefinitionAsset et ne consulte plus FindItemDefinition. La résolution du vrai Item_DisarmTools_01 N0, son tag, une pile réellement possédée et une quantité valide sont nécessaires pour le calcul des chances et l'exécution du désamorçage. Aucun changement des règles de compétence, Dexterité, RNG, détection, dégâts, réutilisation ou persistance des pièges.

Les fixtures ARM01.16.2A, TRAP01.4, TRAP01.8 et Saboteur N1/Save utilisent le vrai registre dans leur monde transitoire ; les assertions de gameplay sont conservées, l'ancien contrôle de présence cache du Saboteur devient un contrôle de résolution canonique et d'absence de pointeur historique. Le test 5D ajoute le refus de registre absent et d'une quantité hors capacité. Les tests d'authoring historique encore présents et le SaveGame de test en mémoire ne sont pas une migration des packages ou sauvegardes joueur.

Le menu contextuel générique, les lecteurs d'alchimie et le cache historique lui-même restent hors du périmètre de ce lot. Aucun asset ou schéma modifié ; validation Windows et Shipping à confirmer.
