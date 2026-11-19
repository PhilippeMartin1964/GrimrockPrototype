# ARM-CLEAN06.5D.5 — Retrait des contributions historiques ARM

## Preuve et périmètre

Parent `6a475dcb3c35bf533f12e940a7fb3e26bf778b91` : `UE5.5.4/SilentTask` et `UE5.5.4/LocalAuto` SUCCESS. Le test natif `Grimrock.RPG.ARM.CLEAN06_5D4.ArmExtraContributionsRetirementEvidence` inspecte les 22 sources du manifeste réel, exige identité et chemin exacts, compare toutes les contributions hors armure et toutes les résistances à leurs valeurs par défaut. Il reste inchangé. Ce retrait ne crée pas de nouveau champ de catalogue.

## Contrat runtime

- Un ARM enregistré valide ne contribue que son armure physique/magique issue de ItemSystem et du catalogue, au niveau réel N1–N20. L'ancien pointeur, même de même identité, ne peut plus imposer de bonus ou résistance.
- Une identité enregistrée invalide ne revient jamais au legacy.
- Les bonus/résistances N0 non-ARM spécialisés, réserves par instance, pénalités et refus de niveaux invalides de 5D.4 restent inchangés.
- Les identités non inscrites conservent temporairement leur comportement historique ; ce lot n'autorise pas la suppression des anciens assets.
- Gameplay, diagnostics et prévisualisation utilisent le même modèle de lecture.

## Validation

3AD/3AE remplacent les anciennes assertions transitoires de contribution ARM par une comparaison complète au résultat canonique : pointeur absent ou contradictoire, N1/N20 et niveaux invalides, armure absorbée et prévisualisation, absence de résistance ; régression des identités non inscrites et test natif 5D.4 conservés. Deux statuts Windows SUCCESS sur le SHA exact sont requis. Aucun fichier binaire, aucune sauvegarde, aucun WBP ni texte joueur modifié. Recette PIE/Save/Continue/Cook/Shipping encore due ; les autres lecteurs et références historiques restent à traiter avant retrait global.
