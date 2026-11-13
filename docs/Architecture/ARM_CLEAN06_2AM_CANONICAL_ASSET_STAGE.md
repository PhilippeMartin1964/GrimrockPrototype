# ARM-CLEAN06.2AM — Préparation native des DataAssets canoniques

L'outil **ponctuel** `UGridArmClean06CanonicalStageCommandlet` assemble
les 22 armes du seul manifeste officiel, 1 munition de flèches,
les 82 archétypes (1640 niveaux), les 19 actions (dont 15 liens au
profil neutre partagé), et quatre catalogues.

Il ne modifie **aucun asset historique**. Les deux fichiers hors
manifeste ne sont ni consultés ni migrés.

## Deux modes stricts

Par défaut, le commandlet fonctionne en **lecture seule**.
Il vérifie le registre transitoire et les 28 noms de packages
prévus, puis retourne `DRY_RUN_OK` sans créer le moindre fichier.
C'est le mode autorisé pour le prévol Windows.

`-Apply` est nécessaire pour produire réellement **28 nouveaux
DataAssets** dans
`/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/` :
un `DA_ItemSystem`, quatre catalogues et 23 définitions.
Il refuse toute destination déjà occupée, toute erreur de
parité ou tout registre invalide. Chaque type Unreal est conservé
par duplication du candidat en mémoire ; la racine pointe vers
les 27 packages créés. Le package racine est sauvegardé en dernier.

**Attention :** une panne d'écriture peut laisser des fichiers neufs
partiels dans le seul dossier `CanonicalStage`. Les conserver pour
diagnostic ; ne jamais supprimer ou écraser automatiquement les
assets historiques. Aucun `-Apply` n'est lancé par l'agent de
validation, et aucune écriture n'est possible dans ce mode
sans invocation explicite.

## Portée et prochain contrôle

Ce dossier est **un sas de migration**, pas la structure définitive
et pas une activation automatique dans le jeu. Après écriture,
les 28 packages doivent être vérifiés via UE Editor et Git LFS,
commités sur master avec un diff exactement contrôlé, puis les
références des niveaux, inventaires, HUD, combat et SaveGame
doivent être migrées (06.3–06.5). L'ancien système fonctionne
tant que le cutover n'est pas autorisé.

Le commandlet est temporaire et doit être supprimé au terme
de 06.5. C++ reste autorité et les futurs DataAssets d'arme
n'ajoutent aucun champ hors des six prescrits.
