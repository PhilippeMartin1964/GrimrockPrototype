# ARM-CLEAN06.4X.16.7.1 — Migrateur de réceptacles, audit seul

La preuve nominative provient de l'issue GitHub #176.
Les deux vrais DataAssets concernés sont :
- DA_Receptacle_TorchHolder -> Item_Torch
- DA_Guardian -> Gem_Blue

La commande native GridArmClean064X71ReceptacleRules
possède trois modes strictement séparés : Audit,
Apply et Verify. **Ce jalon exécute seulement Audit**
dans la suite Automation et n'active aucune commande
de migration binaires.

Avant toute écriture, la commande vérifie :
- exactement 44 définitions natives ;
- exactement 4 niveaux ;
- 6 listes de réceptacles et 2 règles concrètes ;
- 2 listes de serrures et 2 pointeurs ;
- aucune surcharge d'instance ;
- les deux chemins, DefinitionId, index 0,
  source ItemId et registre canonique exacts ;
- les pointeurs historiques conservés ;
- aucune valeur canonique déjà persistée
  pendant le premier Audit / Apply.

Apply, non déclenché à cette étape, écrirait
seulement les deux CanonicalItemId et sauvegarderait
uniquement ces deux DataAssets. Verify requerrait
que ces deux champs soient effectivement persistés
dans un processus UE indépendant.

Le test RealReceptacleMigrationAuditOnly appelle
le mode Audit, contrôle le JSON privé temporaire
et exige packages_saved = 0.

La transaction de conversion réelle, dans un
jalon distinct, exigera backup SHA-256 des deux
packages physiques validés par Git LFS,
réouverture UE indépendante, suite Grimrock
et un contrôle de publication Git par parent exact.

Aucun vrai .uasset, .umap, WBP ou .sav n'est
modifié par X.16.7.1. Ne pas relancer 06.4W/X.5.
