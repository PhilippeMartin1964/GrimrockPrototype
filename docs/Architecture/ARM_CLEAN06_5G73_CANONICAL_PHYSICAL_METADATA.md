# ARM-CLEAN06.5G.73 — Métadonnées physiques strictes

Parent 42343220eba7aaa0f1716b4d84195bd3dfbdbe9f. Publication seulement après les deux statuts SUCCESS du parent.

Le lecteur FGridReceptacleItemMetadataReadModel partagé avec FGridWorldItemLevelReadModel ne peut plus accepter un ItemId inconnu avec un registre configuré ni récupérer niveau, masse ou nom de son ancienne définition. Le chemin ancien est réservé aux fixtures Automation sans registre, et refuse les masses non finies/négatives. ARM et non-ARM inscrits restent résolus au niveau physique exact. Aucun champ ni package retiré ; tests 3AI et 3AJ vérifient le refus d'une identité inconnue, les données vides en cas de refus et la fixture isolée positive.
