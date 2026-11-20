# ARM-CLEAN06.5G.70 — Isolation des catégories et parité

Parent : 0a57d949798dc71bf0bb8538123ec05de7d304ff, double validation Windows positive.

Avant la conversion native, la catégorie des énigmes n'est pas une exigence de talent ni un modificateur QuickItem. Le registre refuse les tags réservés ARM, inconnus, en double ou en conflit. Les fixtures sans catégorie restent valides ; aucun changement aux actions, aux autres tags ni aux permissions.

Les projections historiques du butin et de l'alchimie vérifient chaque propriété ; seule l'addition exacte et ordonnée de la catégorie historique est permise dans une comparaison de migration, jamais une suppression de tags ni un remplacement arbitraire. La projection alchimie transitoire reçoit cet ajout uniquement après avoir vérifié les propriétés natives. Le test natif 5G69 porte la preuve globale : soit aucun des 26 ajouts, soit tous les 26, jamais un lot partiel. Le plan refuse destination non transitoire, alias et preview déjà remplie.

Aucun package, niveau ou SaveGame écrit. La prochaine transaction native doit sauvegarder les 26 packages exacts hors worktree, vérifier les hashes et le diff, réouvrir, exécuter Automation et publier seulement après succès. Aucun lecteur runtime de catégorie activé ici.
