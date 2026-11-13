# ARM-CLEAN06.3B — Supprimer l'outil de création ponctuelle, conserver les preuves

Le commit UE natif 06.2AO a publié **28 nouveaux DataAssets Git LFS**
et le commit 06.3A a validé leur lecture par le
`UGridItemSubsystem` à N1 et N20. Le commandlet
`GridArmClean06CanonicalStage` et son wrapper
`PublishARMCanonicalStage.ps1` avaient terminé leur usage unique.

Ce jalon supprime le commandlet C++ (header et source), le script
PowerShell de publication et son déclencheur dans
`UEValidationAgent.ps1`. Le patrimoine des 28 DataAssets réels
ne change pas ; aucune arme hors manifeste n'est touchée.

Le test historique `CompleteProductionPreview23` est conservé,
mais appelle maintenant **directement les 28 assets sauvegardés** :
4 catalogues comparés propriété réfléchie par propriété réfléchie,
23 définitions comparées intégralement, validation du registre
et intégrité de toutes les règles 82 ARM × 20 niveaux et 260 passifs.
L'ancien `Main(TEXT(""))` n'existe plus : le test permanent
ne dépend plus d'un commandlet de conversion.

Il reste volontairement des lecteurs historiques de gameplay et
des opérations de SaveGame/niveaux à migrer en 06.3–06.5.
Ce nettoyage est un retrait effectif de code non nécessaire,
pas une déclaration prématurée de bascule finale.
