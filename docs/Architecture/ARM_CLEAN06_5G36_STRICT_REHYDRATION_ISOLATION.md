# ARM-CLEAN06.5G.36 — Isolation du résolveur de réhydratation historique

Parent aaea2b8797bdbac2ee50238fab0cfa8119d30bcc, publication après validation des deux statuts Windows.

La réhydratation de production valide directement les objets physiques et les identités des raccourcis Equipment/QuickItem, sans passer par le collecteur historique. Un raccourci seul ne crée aucune instance et ne présume aucun niveau ; les sorts ne sont pas des objets. Le cache historique est vidé uniquement après validation complète. L'ancien collecteur avec résolveur injectable est compilé exclusivement pour les fixtures Automation. Le test 5G32 étendu vérifie le refus d'un raccourci inconnu malgré un asset disponible, l'intégrité de la carte en cas de refus, puis le succès d'un raccourci canonique sans instance et le maintien du GUID. Aucun changement d'asset, de schéma de sauvegarde ou de sauvegarde joueur.
