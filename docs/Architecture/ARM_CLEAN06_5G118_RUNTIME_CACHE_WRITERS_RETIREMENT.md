# ARM-CLEAN06.5G.118 — Retrait des écritures runtime de caches

Parent `b11c71154e9201085647c5f82cb0658a4d99120d`. `FGridItemInstanceMaterializationReadModel` valide toujours les niveaux et les définitions, y compris le chemin purement Automation, sans modifier les copies C++ de poids et de nom. `AGridThrownItemActor` laisse les métadonnées au registre sans écriture de cache. `AGridLevelRuntimeActor::GetWorldItemWeightAtCell` ne reconstruit plus de copie de masse même dans les mondes de test non configurés ; le lecteur, désormais fail-closed depuis 5G116, refuse ces identités comme auparavant en production.

Préservation des GUID, propriétaires, quantités, niveaux, lumière et usure d'armure ; tests 5G117 migrés au préalable. Aucun changement de schéma SaveGame, d'asset, de présentation, de gameplay canonique ou de données historiques. La suppression physique des membres C++ reste subordonnée au nettoyage et à l'audit de toutes leurs références.
