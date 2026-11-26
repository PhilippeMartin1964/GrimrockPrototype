# ARMEMENT-INSPECTION02.1 — Socle commun générique

**État :** code et tests livrés, validation UE5.5.4 sur le SHA publié requise. Aucune recette GPU ni artistique dans ce lot.

Références : [plan](ARMEMENT_INSPECTION02_PLAN_IMPLEMENTATION.md), [spécification](ARMEMENT_INSPECTION01_SPECIFICATION.md), [addendum](ARMEMENT_INSPECTION01A_ADDENDUM.md).

## Implémentation

- `UGridItemSubsystem::TryGetDefaultItemLevel` lit le type de la définition enregistrée pour initialiser une sélection auteur. N1 pour ARM, N0 pour non ARM. Aucune correction de niveau physique ni recherche historique. `FGridItemInspectionReadModel` expose trois lectures : défaut auteur, niveau exact, instance physique. `FGridItemInspectionView` porte la projection canonique et les capacités dérivées ; les réserves restantes ne sont lues que pour une instance physique. Les lectures d'instance réemploient le contrat des Readables et ne modifient pas les données. Le futur raccordement joueur devra encore revalider la source réelle.
- `FGridItemMeshPresentation` extrait le comportement MID de `AGridItemActor` à l'identique : mêmes quatre paramètres, mêmes multiplicateurs, mêmes overrides explicites, N1 neutre sans MID supplémentaire. Le preview et l'acteur jouable utilisent la même fonction ; les propriétaires conservent chacun leurs MID.
- `AGridItemPreviewActor` est transitoire, non plaçable, sans interface d'interaction, sans collision, physique ou navigation. Seuls les mondes EditorPreview/GamePreview peuvent recevoir une présentation. Le root d'observation est indépendant du transform visuel authoré. L'absence de mesh ou l'échec de résolution efface l'image précédente et retourne un état explicite. Les effets persistants sont ceux résolus et sont appliqués via le composant existant, sans lumière de gameplay ni sparkle de ramassage.
- `UGridItemVisualEffectsComponent` détache et revalide les références pendant la suppression des effets. Les Niagara persistants générés sont transitoires, sans auto-destruction ; le cleanup n'utilise jamais une référence potentiellement invalidée par la désactivation. L'acteur preview efface son état à la sélection, à EndPlay et à Destroyed.
- Pas de scène de capture, de viewport, d'UI, de simulation ponctuelle, de réglages artistiques nouveaux, de changement des quatre catalogues ou de modification native d'assets. Les limites de projection actuelles, notamment ornement non raccordé, restent à évaluer dans les lots prévus. Les cinq bandes sont conservées ; les vingt rendus distincts ne sont pas encore implémentés.

## Tests

| Automation | Contrôle |
| --- | --- |
| Grimrock.Items.Inspection.Resolve | Énumération des 48 définitions, 22 ARM et 440 résolutions de niveaux, N0 non ARM, refus des niveaux invalides et ID absent. |
| Grimrock.Items.Inspection.Capabilities | Arme, bouclier, réserve réelle, simulation, objet N0, lecture d'instance et invalidation. |
| Grimrock.Items.Inspection.PresentationParity | Même mesh/transform/finition que runtime, MID distincts, édition isolée, retour N1, repli sans mesh. |
| Grimrock.Items.Inspection.Lifecycle | Refus d'un monde de jeu normal, N17/N16 effets persistants, nettoyage répété, destruction. |

Les assertions ci-dessus sont écrites mais ne constituent pas des résultats d'exécution. L'agent Windows compile Development Editor et Shipping et exécute Grimrock selon AGENTS.md. Les deux statuts exacts doivent réussir avant INSPECTION02.2. Ce lot ne requiert aucune manipulation de WBP ni modification de sauvegarde. Aucun rendu noir ne sera revendiqué avant le viewport natif.

## Suite

INSPECTION02.2 : onglet générique de l'éditeur, catalogue, caméra, fond noir, informations résolues, ouverture des ressources. Première intervention du propriétaire après disponibilité d'un vrai viewport, pas pour éditer manuellement le C++.
