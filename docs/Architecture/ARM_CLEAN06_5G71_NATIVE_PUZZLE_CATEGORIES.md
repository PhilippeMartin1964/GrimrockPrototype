# ARM-CLEAN06.5G.71 — Transaction native des catégories

Parent : 1d7e42380d92e2dc97fcf17010c14bc41c49ac4c. Code et script distincts du commit natif.

Le commandlet reprend le plan validé et le compare sur une destination transitoire. Il refuse une destination non conforme, un tag déjà présent en mode Apply, des sources non sauvegardées ou une différence de bytes par rapport aux copies privées. Les 26 définitions sont les seules cibles. Le fingerprint par propriété exclut uniquement les tags PuzzleCategory et doit rester identique lors de l'audit, de la prévisualisation, de l'Apply et de la réouverture indépendante.

Le script reprend les garde-fous 5F : worktree de validation détaché enregistré, parent/sujet/diff exacts, deux statuts de code positifs, répertoire de preuve privé, fichiers ordinaires, hashes SHA256/LFS, copie avant toute écriture, diff exact des 26 packages, rollback comparé en cas d'échec avant commit, suite Grimrock complète après réouverture, contrôle du remote avant commit et push fast-forward. L'agent lance uniquement ce sujet et publie un statut supplémentaire UE5.5.4/ARM-CLEAN06.5G71. Le commit binaire enfant reste soumis aux deux statuts ordinaires. Aucun SaveGame, root, catalogue, niveau ou checkout principal modifié ; logs privés.

Aucun basculement de lecteur runtime dans ce lot. Attendre le commit binaire confirmé et ses validations avant la suppression des lectures historiques de catégorie.
