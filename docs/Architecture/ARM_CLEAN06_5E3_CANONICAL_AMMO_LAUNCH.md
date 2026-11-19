# ARM-CLEAN06.5E.3 — Lancement canonique des munitions

Parent `bbe1705d8492a8f20065571f17eb5ee27c4c3d78` : validations SilentTask et LocalAuto SUCCESS.

Le point différé 3BB (vitesse depuis l'ancien DA_Item_Arrows_01) est retiré. Le même préflight de disponibilité/exécution résout la munition N0 via AmmoKindId. L'arme spécialisée active possède la priorité de vitesse ; sinon GenericThrow.InitialSpeed de la munition possède le clamp historique 400–4800 cm/s. Vitesse non finie/nulle/négative ou configuration invalide : refus, jamais substitution arbitraire. Une arme hors registre peut garder son ID de munition historique, mais une munition inscrite utilise toujours ses propres masse et vitesse canoniques. Les identités hors registre conservent leur ancien chemin.

Le débit d'une unité reste après création réussie du projectile. Le préflight et le débit refusent les piles canoniques malformées (niveau N1, quantité hors capacité, instance invalide) avant d'autoriser la suppression par ItemId. Le spawn d'une munition enregistrée reçoit nullptr au lieu de l'ancienne définition. Pas de changement de GUID, trajectoire valide, orientation, hit/miss, chute, récupération, Save/Continue, coûts ou cadence du tir ; le 1200 de restauration immédiate d'une flèche fichée n'est pas une vitesse de tir et reste hors périmètre.

Tests 3BB renforcés : parité réelle de GenericThrow.InitialSpeed avec le vieux DA avant retrait, vitesse exacte N1, clamp, priorité spécialisée, refus, débit 2→1→0, projection N0 avec vrai composant de vol. Les tests 3BC de récupération/Save restent requis. Pas de nouvelle clé de localisation, de WBP, de binaire ou de sauvegarde modifiée. Le message existant de disponibilité du HUD reste inchangé. Deux statuts exact-SHA avant poursuite, rendu PIE non attesté.
