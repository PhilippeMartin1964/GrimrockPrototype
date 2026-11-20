# ARM-CLEAN06.5G.116R — Réparer les deux tests qui exigeaient une masse fantôme

Parent `b367caca53a6ddec9faeaba58adbeb04110889d6`, issue GitHub #215 : compilation UBT réussie, 2 tests Automation en échec : `Grimrock.Monsters.MON11.Presentation.ThrownWeaponLifecycle` et `Grimrock.RPG.ARM.CLEAN06_3.CanonicalInventoryDiagnosticWeight`. Le lot 116 supprime volontairement le repli sans ItemSystem dans `FGridItemWeightReadModel`. Les deux tests historiques conservaient les attentes de ce repli :

- MON11 emploie volontairement une définition d'arme historique créée en mémoire pour tester un maillage et une présentation de lancer spécifiques. Après le test de torche réel, le registre est détaché. Un objet ramassable historiquement créé doit garder son GUID, son visuel et ses interactions, mais sa contribution à une plaque de pression doit désormais être 0 kg **sans autorité canonique**. Le test remplace l'attente de masse non nulle par la preuve du refus de masse fantôme ; toutes les assertions de collision, projectile, conversion, ramassage, maillage, pile et rollback sont maintenues.
- 3AC conserve la vérification de la définition d'essai hors registre (poids 99999 kg depuis `UGridItemDefinitionAsset`), mais exige `INVALID` pour la masse unitaire d'une instance inconnue sans registre. Il ne peuple plus un cache de masse non reflété.

Aucun changement de code gameplay, asset, sauvegarde ou noms de joueur. Validation obligatoire `SilentTask` + `LocalAuto` sur le SHA de la réparation. Cette réparation doit être conservée lors de la conversion de la fixture MON11 dans 5G119.
