# ARM-CLEAN06.4X.66F — Sécurité du rollback natif

## Incident et périmètre

X.66B avait produit et rouvert 12 packages, 48 définitions et 14 actions, puis exécuté 2005 tests sans échec. Le checkpoint MON135 supplémentaire a déclenché CheckSaveSet après Automation ; un second CheckSaveSet dans catch a empêché le rollback. X.66D a restauré les 12 états initiaux et archivé les candidats et le checkpoint prouvé, sans modifier les sept sauvegardes du reçu. X.66E (parent 46f42513f42eccae7a1b2c778146e62e0a083de7) a validé l'isolation UserDir avec les deux statuts Windows. La migration native reste à effectuer.

## Correctif

Quatre phases séparées sont toujours tentées avant commit, à HEAD et branche inchangés : restauration des seules saves effectivement installées (ou déjà à l'original), restauration des packages connus, vérification stricte des saves, vérification des 12 états et du checkout. Les erreurs sont agrégées, la cause de la transaction reste signalée ; aucune erreur SaveGames ne court-circuite la phase native. L'absence de preuve, la concurrence, une sauvegarde inconnue, une modification étrangère au lot ou un commit déjà créé empêchent les opérations non sûres. Les états natifs avant et après Apply et l'issue du rollback sont persistés en privé. Les nouveaux packages ne sont retirés qu'après preuve des octets et copie SHA256 dans RollbackCandidates ; les deux anciens sont restaurés par comparaison et remplacement, sans écraser des modifications concurrentes identifiées. Un Apply interrompu sans preuve complète est conservé pour audit : ce n'est pas une autorisation de supprimer à l'aveugle. Les snapshots, les reçus ou les saves ne sont jamais ajoutés à Git. Un rollback incomplet interdit toute relance ou démarrage jeu avant audit.

## Tests et suite

TestARM_CLEAN06_4X66_Rollback.ps1 utilise uniquement des fichiers synthétiques temporaires : injection d'erreur de restauration SaveGames et de changement de set, restauration effective d'un fichier malgré ces deux erreurs, archive des candidats, rétablissement de l'absence, refus d'octets inconnus, de reçu candidat absent, de snapshot corrompu, résultat global succès/échec. ValidateUE exécute ce test avant le build et la suite Grimrock. Aucun changement C++ de production ni du schéma SaveGame v27 dans ce lot. Après SilentTask/LocalAuto success sur ce SHA, le script natif X.66 peut être relancé sur le checkout principal propre et mis à jour; les rapports du précédent essai restent privés et sont conservés. La preuve des 48/14 persistés, les deux statuts du SHA binaire, puis le retrait historique et la validation fonctionnelle restent à réaliser. NullRHI n'est pas PIE visuel.

## Correctif F.1 après validation Windows

Le SHA f3bf06b62aa94f5b97ffd0643fce3e629ab2bcc1 a échoué au PREFLIGHT (issue #195 ; détail brut privé). Le risque d'interop PowerShell 5.1 de File.Replace avec $null est traité par NullString.Value explicite et un helper unique pour package/save, contrôlé sur installation et restauration synthétiques. Les phases de tests sont désormais des constantes autorisées dans le diagnostic public, sans aucune modification de la politique de confidentialité. La cause précise de l'échec initial n'est pas prouvée par le diagnostic public ; attendre les nouveaux statuts exacts. Aucune migration native effectuée.
