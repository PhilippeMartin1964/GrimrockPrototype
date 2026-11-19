# ARM-CLEAN06.5G.42 — Contrôle d’entrée inventaire et curseur

Parent 4e4d417c2fc45b5554b7a7efa2501e10c6b7f948. Publication conditionnée aux deux statuts SUCCESS.

Les entrées CanAdd/Add pouvaient accepter une identité non résolue en utilisant des snapshots. Les placements du curseur ignoraient le retour de ApplyItemDefinitionToInstance. Le nouveau contrôle refuse ces entrées en jeu ; un curseur ou slot simple ne peut pas perdre silencieusement des unités par clamp. Le ramassage multi-stack conserve son traitement transactionnel et la quantité source. Aucun changement de politique de poids, niveau, stacking ou capacité canonique.

L'Automation active conserve les fixtures explicitement enregistrées et les caractérisations sans registre ; une identité inconnue sans définition dans une vraie GameInstance est refusée même pendant ces tests. 3AS vérifie ces refus, les N0 ARM invalides, les quantités, la conservation du curseur lors de l’échec et un transfert canonique positif. Les tests historiques restent en place.

Périmètre limité aux entrées sac/SetCursorItem et placements du curseur vers un slot ou le premier slot libre. Les restaurations depuis une opération monde interrompue, la validation d’ownership, les transferts d’objets déjà possédés et les appels historiques directs de lecteurs doivent encore être audités ; pas de suppression du cache ni de modification de schéma. Aucun asset ou SaveGame modifié. Validation Windows du SHA publié et Shipping ultérieure requises.
