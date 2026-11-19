# ARM-CLEAN06.5E.4C — Réhydratation sans définition historique pour les IDs inscrits

Parent `29d8b144c54fce3518bdf5da0c078797bd17b9eb` deux statuts SUCCESS. Les sauvegardes v27 de production ont déjà été converties en 5B.1 ; aucune répétition de cette migration ni écriture de SaveGame.

- New Game et Continue utilisent RehydrateOwnedItemDefinitions. Valider toutes les instances physiques avant de remplacer la map. N1–N20 ARM, N0 non-ARM, capacité et GUID préservés. Pour chaque ID inscrit, y compris raccourci seul, ne jamais appeler Resolver. Le cache historique de rechargement ne contient que des identités hors registre ; ces dernières exigent un asset d'identité exacte. En cas de refus, map précédente intacte.
- ApplyPhysicalArmorDamage préserve le FText canonique d'un bouclier/équipement dans la trace de combat, indépendamment du cache de sauvegarde ou de l'ancien DA. Les règles d'absorption restent identiques.
- Automation 3AT : monde GI réel, zéro appel historique pour un groupe canonique, anciens DA disponibles mais non retenus, trace de bouclier pointerless, appel unique hors registre, refus et rollback, niveau invalide et raccourci seul. Le test historique 0192 (monde sans GI) continue à caractériser le contrat hors registre.
- Pas de nouveau texte, clé, WBP, .uasset ni SaveGame. Contrats UI relus, UI.csv → ST_UI → WBP → Game/fr/en inchangés. PIE distinct de l'Automation.

Limite du lot : la map rechargée est pointerless pour les IDs inscrits ; les opérations explicites de recherche historique au ramassage, aux transferts/projections de lancer, les fallbacks précédant la matérialisation 5D.3 et la palette WBP restent à supprimer par lots indépendants. Ne pas supprimer prématurément les packages sources ni prétendre que tout lecteur historique a disparu. Validation Windows exacte avant publication suivante.
