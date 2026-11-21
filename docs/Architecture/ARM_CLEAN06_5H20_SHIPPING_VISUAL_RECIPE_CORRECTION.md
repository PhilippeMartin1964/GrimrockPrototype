# ARM-CLEAN06.5H.20 — Rectificatif Shipping, abandon du correctif de visée expérimental

## Faits confirmés par le test utilisateur (21 novembre 2026)

L'exécutable Win64 Shipping correctement sélectionné affiche **la flaque d'huile normalement**. L'absence rapportée au préalable provenait du lancement d'un autre exécutable ; l'hypothèse initiale d'un défaut du matériau ou du Cook ne constitue pas une anomalie reproduite. New Game, inventaire/combat, interactions et Save/Continue sont également validés dans l'exécutable Shipping.

Le système de ciblage jaune/vert reste invisible en Shipping, contrairement à PIE. Son remplacement expérimental par `ULineBatchComponent` dans `SHIPPING-VISUAL-PARITY01` (SHA `f3c8d05c...`) n'a pas rétabli l'affichage chez l'utilisateur. Ce changement n'a donc **pas** reçu la validation visuelle GPU nécessaire ; on le retire entièrement, ainsi que le test lié et le diagnostic optionnel `-GrimrockVisualAudit` dans `GridLevelRuntimeActorSurfaces.cpp`.

Le dossier GitHub [#221](https://github.com/PhilippeMartin1964/GrimrockPrototype/issues/221) conserve la question de la prévisualisation du ciblage pour traitement ultérieur, **sans en faire un blocage ARM-CLEAN06**. Le code d'origine `DrawDebugBox` demeure historiquement limité à PIE (ne constitue pas une solution Shipping), ce défaut de présentation est documenté et reporté.

## Frontière du retour arrière

- Retour exact aux blobs de `8a618a44d056f3ecc8b22d87ea286986d429ec36` pour les deux fichiers `GrimrockPlayerController`, `GridLevelRuntimeActorSurfaces.cpp` et `MON12_8_6_CELL_AREA_TARGETING.md`.
- Suppression du test `GridMON12ShippingTargetingPreviewTests.cpp` et du document d'instrumentation temporaire `SHIPPING_VISUAL_PARITY01_TARGETING_OIL_DIAGNOSTICS.md`.
- **Conservation** des protections antérieures du Cook dans `DefaultGame.ini`, `ValidateUE.ps1` et `ValidatePackage.ps1`. Ces garde-fous ne modifient pas le rendu réussi et assurent la disponibilité des deux matériaux d'huile.
- Aucun `.uasset`, `.umap`, `.sav`, règle de combat ni contrat de sauvegarde modifié.

## Priorité actuelle

Terminer ARM-CLEAN06.5 : après retrait contrôlé de la palette d'icônes historique, il reste **53 packages du groupe de retraite**, dont 48 définitions d'items, 2 catalogues historiques sources d'Editor, 2 Readables effectivement conservés par une sauvegarde v27 et un redirecteur `DA_Weapon_ThrowingKnife`. L'audit de novembre exige un retrait natif contrôlé, pas une suppression Git massive. Avant de supprimer une définition, neutraliser ses consommateurs (tests/Editor), contrôler le registre et les 7 sauvegardes, puis prévoir copie privée, audit avant/après, récupération en cas d'échec et validation UE, PIE et Shipping.

Les deux statuts Windows de chaque commit restent obligatoires. La recette GPU du ciblage n'est pas une condition d'arrêt pour la migration tant que les actions de combat et leur validation fonctionnent.
