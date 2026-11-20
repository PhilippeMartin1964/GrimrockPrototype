# ARM-CLEAN06.5G.56 — Restitution des arrivées différées

Parent 5fcaf31c714821937bef2416c2eb939eecb2be08. Publication après deux statuts SUCCESS.

Une arrivée n'est plus normalisée (GUID de clé contradictoire, quantité zéro, propriétaire incorrect) ni retirée sans acteur enregistré. Le dépôt interne réservé à ce chemin interdit le chaînage d'une fosse et diffère la notification des plaques : matérialisation puis retrait de la file, puis notification. Le dépôt public conserve l'ordre historique. Si un lien change de niveau, la boucle s'arrête avant de traiter une autre arrivée. Le test PIT03 ajoute deux refus et conservation de la file sur GUID contradictoire et quantité zéro, en plus du round-trip existant. Les scénarios réentrants Lua et le rendu réel Chaos restent à vérifier en PIE ; aucune preuve de réentrance générale n'est revendiquée. Aucun asset/save/schéma modifié.
