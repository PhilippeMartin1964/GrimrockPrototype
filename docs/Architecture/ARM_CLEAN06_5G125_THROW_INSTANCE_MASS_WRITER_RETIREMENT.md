# ARM-CLEAN06.5G.125 — Lancers physiques sans copies de poids

Parent `a65582fcbc571663e469a4fdc5a96c81cdc3a5ae`. Retirer les cinq affectations de `Weight` dans les instances de projectiles des chemins curseur, équipement, inventaire et combats, réservées historiquement à `WITH_DEV_AUTOMATION_TESTS`. Le calcul de masse pour les logs continue de résoudre `ItemSystem` à niveau physique ou, uniquement dans une fixture isolée, la masse d'une ancienne définition explicitement fournie ; un objet sans définition et sans registre n'invente plus une masse copiée depuis l'instance. Préserver les GUID, les divisions de piles, la force, la vitesse, l'arc, les coûts d'action, les spawns et les rollback de transfert.

Les tests des lancers PIE, MON11 et du système des projectiles restent inchangés : validation Windows obligatoire avant la suite. Aucun changement d'assets, de sauvegardes ou de règles canoniques.
