# ARM-CLEAN06.5G.8 — Contrat physique unique dans ItemSystem

Après 5G.5–7 validés, le résolveur central refuse toute combinaison ItemId/niveau invalide : ARM N0, non-ARM N1–N20. Le résolveur ne retourne aucun champ de projection sur erreur. Les anciens tests de projection N1 des munitions et consommables sont réorientés vers le vrai N0 ; tests de refus aux frontières monde/projectile/inventaire conservés. L'audit natif couvre 466 résolutions valides et 542 combinatoires invalides (22 + 26×20). Le commandlet d'audit historique 4Q conserve son nombre de sondes et refuse désormais les niveaux non-ARM non nuls ; les reçus historiques restent intacts et ne sont pas régénérés. Aucun asset, sauvegarde ou texte joueur modifié.

L'objectif est de ne plus disséminer la validation de niveaux physiques dans le runtime. Ce lot n'élimine pas encore les API de compatibilité hors registre ; suppression de champs, classes, fichiers et fixtures historiques reste à réaliser.
