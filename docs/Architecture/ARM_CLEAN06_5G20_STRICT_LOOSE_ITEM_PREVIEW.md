# ARM-CLEAN06.5G.20 — Aperçu des objets libres strictement canonique

Précondition : parent 5d13606d89de7711437c3727640f83ff5a605cee validé, X.46/X.48 ont prouvé la migration des 44 placements natifs vers les IDs physiques sans pointeur.

- L'aperçu éditeur ne résout plus d'identité ou de niveau depuis un ancien asset. Il exige un GUID valide, un ID canonique explicite, une quantité positive, un registre configuré et une résolution exacte du niveau. Aucun mesh de repli.
- X.25 et 063Z vérifient N1/N20 ARM, N0 non-ARM, rejet invalide, inconnus, pointeurs seuls ou contradictoires, et même un ancien pointeur concordant. X.25 préserve l'assertion sur l'acteur d'aperçu et sa transform.
- ARM0165 vérifie le mesh et la transform des trois vrais IDs canoniques en gardant les profils historiques comme oracle de parité uniquement dans le test. Le test runtime historique de visual resolver reste intact en attendant son traitement.
- Les tests d'éditeur sur la palette native et le placement multiple utilisent déjà les IDs persistés. Aucun package, SaveGame, WBP, texte joueur ou comportement runtime de placement modifié. Le retrait physique de FGridLooseItemInstance.ItemDefinition et les chemins d'authoring restent distincts, et nécessitent les adaptations des autres fixtures.
- Validation Windows exacte puis recette visuelle éditeur et PIE en 06.6.
