# ARM-CLEAN06.5G.92.1 — Preuve bornée et diagnostic

5G92 a compilé et passé Automation, mais le statut privé global a échoué avant les deux statuts dérivés. La cause n'est pas connue. La projection complète de chaque propriété est maintenant encodée UTF-8 puis condensée en SHA-1 avec son nom de propriété, pour borner la taille du rapport et éviter une éventuelle limite de décodage JSON. Ce digest sert à comparer des états non adversariaux, pas à authentifier les archives (SHA256 inchangé). Aucune réduction du périmètre projeté. Les tests existants de sensibilité s'appliquent aux digests. Le script refuse les projections absentes ou mal formées; l'agent publie une catégorie fixe d'échec, jamais le message de l'exception.

Relancer sous le préfixe de commit 5G92. Aucun changement de schéma ni de sauvegarde originale. Les cinq statuts positifs sont requis avant retrait des caches réfléchis.
