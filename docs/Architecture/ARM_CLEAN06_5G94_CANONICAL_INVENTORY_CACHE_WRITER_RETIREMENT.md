# ARM-CLEAN06.5G.94 — Retrait des écritures de caches à l'entrée inventaire

Parent `ef064706203bd57c9e44d318d3bc66e3686bb4c2` : SilentTask, LocalAuto et preuve native 5G93 SUCCESS. Le schéma v27 ne sérialise plus DisplayName/Weight ; la matérialisation sous registre ne doit plus les reconstituer. Effacer les deux membres temporaires uniquement après résolution valide. Les données refusées restent intactes. Aucun changement de GUID, niveau, quantité (hors politique de pile déjà existante), propriétaire, lumière, dette d'armure ni contenu narratif. Le journal utilise le lecteur de poids au lieu du cache.

Les tests 3AG et 3AS vérifient maintenant l'absence des caches et la valeur du lecteur canonique, avec des métadonnées contradictoires et N0/N19/N20 ; 3AX vérifie l'absence de poids stocké. Les tests de caractérisation non canoniques restent inchangés. Aucun asset, sauvegarde, schéma ni texte joueur modifié.

Ce lot ne retire pas encore les écrivains de génération initiale, loot, transfert monde/réceptacle ni les membres C++ ou les audits/fixtures historiques. Aucun nouveau fallback. Critère de sortie : deux statuts Windows du SHA exact SUCCESS. PIE, Save/Continue et Cook/Shipping restent à faire.
