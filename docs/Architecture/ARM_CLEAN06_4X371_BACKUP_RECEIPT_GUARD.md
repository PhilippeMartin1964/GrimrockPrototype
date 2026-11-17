# ARM-CLEAN06.4X.37.1 — Verrouiller le reçu des 24 sauvegardes avant X.38

**Date :** 17 novembre 2026. **Parent :** `5071d6eb4cf65b76e8f8bcd3f9bae1fc37d54687`.

Le jalon X.37 autorisait le mode `-Apply` lorsque le reçu privé
déclarait le bon SHA, le bon schéma et 24 lignes. Le commandlet ne
vérifiait pas **quels** fichiers étaient présents ni si leurs copies
étaient réellement restaurables. Ce niveau de validation est insuffisant
pour un SavePackage sur les deux catalogues ARM réels.

## Correctif atomique, sans changement de données

La nouvelle fonction pure `ARMCLEAN064X371::ValidateReceipt`
vérifie : SHA40 exact en minuscules, schéma, caractère vérifié, comptes
22+2, **24 noms complets de fichiers uniques** tirés de l'allowlist
fermé X.37 et chaînes SHA-256 de 64 caractères hexadécimaux. Une
ligne manquante, doublonnée, inventée ou contenant un `../` échoue
avant toute tentative de SavePackage.

`CheckPrivateBackupManifest` lit ensuite les **24 copies réelles**
dans le répertoire privé du manifeste. Pour chaque chemin autorisé,
il relit le fichier vivant correspondant et exige une **égalité
binaire intégrale octet par octet** avec le backup. Une absence ou une
divergence bloque `-Apply`. PowerShell continue, de son côté, de
calculer/vérifier les véritables SHA-256 avant de fournir ce reçu.
Le commandlet n'invente pas un second calcul SHA-256 : il vérifie
l'existence et la restauration exacte des 24 binaires, indépendamment
du flag `verified` déclaré.

## Preuves

`Grimrock.RPG.ARM.CLEAN06_4X371.NativeBackupReceiptClosedAllowlist`
couvre reçu exact, HEAD contradictoire, duplication, absence,
traversal, empreinte malformée et `verified=false`. Le test
n'écrit aucun fichier. Suite Windows `Grimrock` obligatoire sur
le commit final.

**Limites :** la comparaison des fichiers ne vérifie pas le hash
SHA-256 elle-même (c'est la responsabilité du runner PowerShell).
Le test ne démontre pas l'Apply natif, la parité visuelle PIE ni
le résultat X.38. Aucune modification `.uasset`, `.umap`, `.sav`
ou Git LFS. **ARM-CLEAN06.4 partielle, 06.5 bloquée.**
