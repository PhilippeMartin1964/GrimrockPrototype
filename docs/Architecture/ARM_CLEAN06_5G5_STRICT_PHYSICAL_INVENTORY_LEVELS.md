# ARM-CLEAN06.5G.5 — Niveaux physiques stricts dans l'inventaire

Parent 5d6ad973bbcd0df73edd1bf9c8ec4c8554584457 : deux statuts SUCCESS. Après 5G.4, supprimer également l'héritage du niveau dans le modèle de matérialisation lui-même. Les ARM enregistrées doivent avoir un niveau physique N1–N20 ; les non-ARM restent N0. Le niveau N1 utilisé par certains appels de description de famille n'est jamais un niveau physique valide pour une munition. Le registre reste inchangé. Les chemins sans registre pour les fixtures historiques restent transitoirement disponibles.

Tests 3AG et 3AS : impossibilité d'hériter du N6 historique, refus atomique d'une munition N1 même avec ancien pointeur, stabilité des poids/GUID et conservation des niveaux valides. Ce lot ne modifie pas l'authoring des placements ni des loots ; ces lecteurs seront traités séparément avec leurs preuves de migration.

Aucun asset ou SaveGame modifié.
