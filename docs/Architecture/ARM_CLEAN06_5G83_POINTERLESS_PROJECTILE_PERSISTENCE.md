# ARM-CLEAN06.5G.83 — Persistance des projectiles sans pointeur

Parent 778e96c05cef30aba7499f8c26316a81da004741. Publication après double validation Windows du parent.

Retirer les trois derniers accès explicites à ProjectileDefinitionAsset dans la capture et la restauration runtime. Le contrat de GUID, quantité unitaire et identité/niveau est exigé pour tous les projectiles récupérables, même en Automation. Une fixture sans registre ne peut pas capturer un projectile actif ni restaurer une flèche attachée. La restauration canonique conserve l'attachement, la pose et le repli de récupération au sol ; les tests historiques de géométrie LOT02 utilisent une vraie flèche N0, en gardant leur vitesse de lancement explicite, leur rig de monstre, les contrôles de cellule/pose et le même GUID. Les autres fixtures historiques monde sans projectile conservent leur chemin existant.

Les deux champs v27 demeurent réfléchis uniquement pour la preuve de conversion et les tests de compatibilité. Aucune archive, version, map ou DataAsset modifié. Les tests d'impact ne constituent pas une validation visuelle PIE.
