# ARM-CLEAN06.5G.71 — Transaction native des catégories

Parent : 1d7e42380d92e2dc97fcf17010c14bc41c49ac4c. Code et script distincts du commit natif.

Le commandlet reprend le plan validé et le compare sur une destination transitoire. Il refuse une destination non conforme, un tag déjà présent en mode Apply, des sources non sauvegardées ou une différence de bytes par rapport aux copies privées. Les 26 définitions sont les seules cibles. Le fingerprint par propriété exclut uniquement les tags PuzzleCategory et doit rester identique lors de l'audit, de la prévisualisation, de l'Apply et de la réouverture indépendante.

Le script reprend les garde-fous 5F : worktree de validation détaché enregistré, parent/sujet/diff exacts, deux statuts de code positifs, répertoire de preuve privé, fichiers ordinaires, hashes SHA256/LFS, copie avant toute écriture, diff exact des 26 packages, rollback comparé en cas d'échec avant commit, suite Grimrock complète après réouverture, contrôle du remote avant commit et push fast-forward. L'agent lance uniquement ce sujet et publie un statut supplémentaire UE5.5.4/ARM-CLEAN06.5G71. Le commit binaire enfant reste soumis aux deux statuts ordinaires. Aucun SaveGame, root, catalogue, niveau ou checkout principal modifié ; logs privés.

Aucun basculement de lecteur runtime dans ce lot. Attendre le commit binaire confirmé et ses validations avant la suppression des lectures historiques de catégorie.

## Relance 5G.71R

Rapport [#209](https://github.com/PhilippeMartin1964/GrimrockPrototype/issues/209) : AUDIT et prior 0/0, APPLY 26/26, REOPEN 26/0, fingerprint égal à chaque étape ; 1 échec Automation : CompleteProductionPreview23, rollback propre. La prévisualisation initiale des 22 armes et des flèches manquait le nouveau tag sur les seules flèches. Le test le reconstitue depuis le type historique Misc, dans un état natif global 26/26, jamais depuis la destination. Toutes les propriétés restent comparées exactement. Nouvelle transaction ancrée sur le parent 2c01895bbbd86cd4691a07c636fd62190fc2f6e7, uniquement les 5 fichiers du lot comme diff de code. Aucun package modifié par ce commit de code.
