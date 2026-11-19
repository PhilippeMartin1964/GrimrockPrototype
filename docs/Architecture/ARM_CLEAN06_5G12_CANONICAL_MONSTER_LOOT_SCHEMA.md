# ARM-CLEAN06.5G.12 — Contrat physique et schéma du loot monstre

Le pointeur réfléchi FGridMonsterLootEntry.ItemDefinitionAsset est retiré ; le seul identifiant persisté est ItemDefinitionId. Le code runtime de mort et de résolution de niveau/métadonnées n'appelle plus de définition historique, n'invente aucun niveau, ne charge aucun asset de définition legacy et refuse un ID inconnu ou un registre absent. Nom, poids et lumière proviennent de ResolveItem ; RNG et distribution restent inchangés. Le runtime conserve les champs de sauvegarde d'instance existants, dont le nettoyage est un lot distinct.

## Preuves et régression

- X.48 et 5A contrôlent la conversion native déjà effectuée : cinq entrées réelles et aucun ancien référent externe. Les tests X.60/X.61 vérifient strictement le seul état final et la parité, avec test réfléchi d'absence de l'ancienne propriété. Les contrôles TD07 continuent de signaler les lignes hors de leur périmètre historique exact.
- MON8, MON9, MON17.6 et ARMEMENT01.8 utilisent la vraie racine ItemSystem dans un GameInstance de test ; MON9 conserve ses IDs synthétiques dans une copie transitoire privée du registre ; aucune édition d'asset source. Les tests couvrent placement, identités, quantité, décès unique, sauvegarde/restore et pickup avec niveaux physiques explicitement authorés. N0 ARM, N1 non-ARM, ID absent/inconnu et absence de registre sont rejetés. Le test MON15.2 conserve l'échec de placement et l'attribution XP indépendants avec Stone canonique.
- Les cinq distributions natives ainsi que les 128 graines de comparaison restent contrôlées. Aucun asset, SaveGame ou WBP modifié. Les helpers de comparaison historique des cinq définitions restent pour l'instant disponibles pour la preuve native.

Publication conditionnée à 5G.11 SUCCESS sur ea4f755f9bfb2fb0cd8606f2e5e0a95710d43e16. Les deux statuts exacts Windows doivent réussir avant toute suppression supplémentaire. La recette finale PIE/Save/Continue/Cook/Shipping reste distincte.
