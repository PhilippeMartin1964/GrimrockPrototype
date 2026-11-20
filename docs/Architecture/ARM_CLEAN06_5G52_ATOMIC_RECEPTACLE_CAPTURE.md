# ARM-CLEAN06.5G.52 — Capture atomique des réceptacles

Parent 8c471e9151846c0d95948199969b9e6ad905bf7f. Publication après deux statuts SUCCESS.

Le lecteur de sauvegarde des réceptacles reconstruisait des identités depuis les anciens assets, sautait les lignes invalides, créait des GUID et clampait les quantités. Il retourne désormais un bool et écrit la sortie seulement lorsque toutes les lignes du réceptacle ont passé identité/niveau/quantité/GUID uniques et cohérence physique acteur. Un réceptacle vide reste capturable sans instance de registre. CaptureCurrentLevelRuntimeState collecte et vérifie le sous-ensemble de réceptacles avant de vider le précédent snapshot du niveau ; une erreur de réceptacle conserve donc le snapshot précédent au lieu de provoquer une perte silencieuse. L'ordre de capture des autres systèmes ne change pas.

Le test 5G52 part d'un vrai N12, produit un snapshot valide, introduit un GUID contradictoire puis une quantité hors capacité, prouve la conservation de la sortie et du snapshot précédent et la capture positive après correction. 3BG exige explicitement l'échec de capture contradictoire. Cette garantie est limitée aux réceptacles : capture des objets libres/projectiles, doublons entre propriétaires, échecs tardifs du SaveGame doivent être audités séparément. Aucun asset/sauvegarde/schéma écrit ; validation Windows et PIE/Shipping encore requises.

Avant publication, les trois appelants ignorant le bool sont corrigés : transition de niveau et initialisation de snapshot avant commandes de monstres. Le test vérifie aussi le refus de transition et la conservation du niveau source, du réceptacle et de son acteur. L'arrêt préalable des actions de combat lors d'une transition reste inchangé.
