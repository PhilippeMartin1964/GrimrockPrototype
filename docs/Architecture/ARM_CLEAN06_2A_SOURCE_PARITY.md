# ARM-CLEAN06.2A — Prévol des sources UE avant conversion canonique

**Statut :** sous-étape préparatoire en lecture seule, pas la livraison des
catalogues et DataAssets convertis (ARM-CLEAN06.2B).

## Pourquoi bloquer les écritures

La baseline ARM-CLEAN06.0 (commit c66bf68a) précède le commit utilisateur
a60031db : quatre définitions Sword ont changé, plusieurs sons Sword et
RatGiant ont été ajoutés, et L_Dungeon a évolué. Un migrateur qui réutiliserait
la seule baseline écraserait ces apports.

Les anciens DataAssets sérialisent encore des données qui exigent une
correspondance explicite : actions et variations sonores, métadonnées de
classes, réactions défensives, visuels et autres champs spécialisés.
Ne pas générer les catalogues canoniques tant qu'une migration sans pertes
n'a pas été démontrée dans Unreal.

## Livrable 06.2A

- Scripts/PreflightARM_CLEAN06_2.ps1 est appelé **une seule fois**, sur un
  SHA identifié par le sujet de commit exact
  « chore(ARM-CLEAN06.2A): audit post-audio assets before canonical migration ».
- Le validateur Windows emploie le véritable commandlet Unreal Editor
  GridArmClean06Baseline, sans option d'écriture et dans un worktree détaché.
- Il recherche la baseline privée 06.0, vérifie son SHA-256
  03c3b09c1430fc8938e4c4bf1cde6fb229334aeda2348ed902c359c4edca6940
  et compare **les valeurs UPROPERTY sérialisées de 56 assets**.
- Il confronte **1 640 profils ARM**, 260 passifs, les IDs et les noms des
  propriétés réellement modifiées et vérifie la présence des sept binaires
  audio/monstre Sword et RatGiant du commit utilisateur.
- Il exporte dans les journaux **privés** le nouveau manifeste UE et un diff
  de noms de champs, sans publier les textes métier ni les chemins locaux.
- Il refuse tout changement dans le worktree ou écart des profils.
- Un résumé limité aux nombres et aux noms des définitions d'armes modifiées
  peut être publié dans une issue GitHub. Un troisième statut GitHub
  UE5.5.4/ARM-CLEAN06.2A décrit l'issue de l'audit indépendamment de
  UE5.5.4/LocalAuto et UE5.5.4/SilentTask.

**Aucun asset n'est converti, sauvegardé, supprimé ou commité dans cette
sous-étape.** Aucune sauvegarde personnelle n'est ouverte ni modifiée.

## Réparation obligatoire de VALIDATION-AUTO01

Le script d'agent distribué à l'entrée de cette étape contenait trois blocs
d'exécution dupliqués et une condition de diagnostic ARM-CLEAN06.1 tronquée.
La sous-étape restaure un unique bloc, reconstitue le test syntaxique des
entrées Automation et conserve une catégorisation publique strictement
limitée pour le test SingleResolver.AmmoAfterRegister.
Le validateur reste autonome ; il n'exige ni git pull ni lancement manuel du
script par le propriétaire.

## Conditions de sortie et prochain travail

1. Le SHA exact reçoit les statuts Windows ; ARM-CLEAN06.2A=success
   prouve seulement le contrôle des **sources historiques**, pas une
   migration des objets.
2. LocalAuto doit devenir success ; s'il reste en échec sur
   AmmoAfterRegister, traiter le diagnostic avant le cutover.
3. ARM-CLEAN06.2B : traduire chaque champ effectif en destination unique,
   refuser les propriétés non mappées, conserver les variations audio et
   construire les nouveaux assets dans UE Editor (sans renommer l'ancien
   pipeline encore lu par le gameplay).
4. Vérifier parité numérique et référentielle sur les véritables binaires
   sauvegardés, avant toute bascule 06.3, conversion 06.4,
   suppression 06.5 ou acceptation PIE 06.6.

Les fichiers de travail utilisateur et leur branche de sauvegarde ne
sont jamais manipulés par cette procédure.
