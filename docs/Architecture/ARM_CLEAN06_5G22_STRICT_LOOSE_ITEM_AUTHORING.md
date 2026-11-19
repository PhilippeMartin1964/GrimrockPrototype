# ARM-CLEAN06.5G.22 — Authoring strict des objets libres

Précondition : 5G.21 validé sur son SHA exact. Après la matérialisation runtime et l'aperçu stricts, la création et les modifications de l'identité ou du niveau utilisent exclusivement ItemSystem. Refuser un ancien pointeur même concordant, un ItemId absent/inconnu ou un niveau physique invalide avant mutation. L'inspecteur Slate ne propose plus de picker historique. Un changement explicite d'ID conserve GUID, quantité et niveau compatible ; un changement de famille par l'inspecteur choisit N0 non-ARM ou N1 ARM si nécessaire, sans perdre de quantité. Les modifications de métadonnées sans changement de sélection de palette préservent l'ID explicite et son niveau ; elles ne restaurent pas silencieusement l'ancienne identité de la palette. Le test X.28 couvre cette régression. Les deux entrées Blueprint historiques restent réfléchies mais ne peuvent écrire un pointeur.

Tests ciblés X.26/X.27/X.28, 3AN, TD033, MIG05/MIG09 ; vérifier aussi la suite complète Grimrock, les 44 placements natifs et les 33 entrées directes de palette. Pas de retrait de propriété dans ce lot, ni changement de package ou SaveGame ; le retrait du schéma, du scratch historique et des recherches de transfert reste distinct. L'inspecteur est Slate editor-only, aucun WBP ni texte joueur n'est modifié. Ce commit préparé n'est pas une preuve de compilation ou de parité PIE.

## Correctif 5G.22.1

Issue #203 : FireBombDirectPlacement utilisait une proposition historique hors palette native. Vérifier son rejet, puis authoring canonique N0 dans la copie transitoire avec la définition déjà enregistrée ; ne pas ajouter FireBomb à la palette de production. Aucun changement du comportement strict.
