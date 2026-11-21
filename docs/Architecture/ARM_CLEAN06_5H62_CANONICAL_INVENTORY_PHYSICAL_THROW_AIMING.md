# ARM-CLEAN06.5H.62.1 — Lancer natif ARM depuis le sac (remédiation)

## Incident H.62 et portée H.62.1

Le commit H.62 a reçu `UE5.5.4/LocalAuto = FAILURE` avec le diagnostic GitHub générique « Échec testing/BUILD ». Aucun journal Windows ni URL de log ne sont publiés dans ce statut ; la cause exacte n'est **pas démontrée**. Plutôt que d'ajouter des hypothèses sur l'input UMG dans la fixture MON12, H.62.1 isole la fonctionnalité que ce test doit prouver : **transfert physique canonique depuis le sac vers un projectile**, sans ancien DataAsset.

Le test `Grimrock.RPG.ARMEMENT.LOT02.16.3.RealThrowingN1` résout toujours trois armes N1 (ARM054, ARM057, ARM058) depuis `UGridItemSubsystem`, attache un `FGridCanonicalLootTestScope` au monde, matérialise une unité en sac, puis utilise directement `AGrimrockPartyPawn::TryThrowSelectedCharacterInventoryItem` avec une direction non nulle. Le moteur vérifie lui-même les attributs, la balistique et les règles de poids avant de consommer l'objet.

## Contrats vérifiés

Une seule unité est consommée, un projectile monde physique et récupérable est créé, son ItemId et son niveau N1 sont conservés, et son GUID est distinct du GUID du sac. Aucun coût de combat ni journal d'attaque fictif n'est produit. Les tests de lancer depuis une arme équipée (H.61) restent inchangés : leur transfert conserve le GUID d'origine.

La **phase de visée de souris/contrôleur** n'est plus exécutée par ce scénario MON12 H.62.1. Elle reste couverte séparément par `Grimrock.TechnicalDebt.TD02_1.WorldItemsContract`, qui teste `BeginSelectedCharacterInventoryItemThrowAiming` et `HandlePhysicalThrowAimingHit` avec placement de proximité et lancer à distance. Ce repli n'est pas une preuve supplémentaire du hotbar HUD ; la migration du comportement de visée ne doit pas être présentée comme validée par MON12.

Aucun code gameplay, Niagara, `.uasset`, `.umap` ou `.sav` modifié. Les 53 packages historiques H.23 demeurent protégés.

H.63 exige `UE5.5.4/SilentTask = SUCCESS` et `UE5.5.4/LocalAuto = SUCCESS` sur H.62.1, compilations Editor/Shipping et tests `Grimrock.RPG.ARMEMENT.LOT02.16.3.RealThrowingN1` et `Grimrock.TechnicalDebt.TD02_1.WorldItemsContract`. Les sept sauvegardes doivent être auditées nativement avant toute suppression.
