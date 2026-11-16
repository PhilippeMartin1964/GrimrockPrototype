# ARM-CLEAN06.4X.16.13 — Transférer les armes avant de retirer le plancher de la trappe

## Régression PIE observée

Le propriétaire confirme que `3b55adf` (X.16.12) **ne règle pas le problème** : plusieurs armes placées sur une trappe piégée tombent visuellement après son ouverture, mais **n'apparaissent pas** dans la cellule voisine d'arrivée au niveau inférieur. Il faut distinguer clairement `UE5.5.4/LocalAuto SUCCESS` et recette PIE **ÉCHEC**. L'ensemble de la migration ARM-CLEAN06 est **en cours**, pas terminé : 06.4 partiel, 06.5 suppression définitive et 06.6 recette intégrale non achevées.

## Correctif de causalité

`FinalizePitGameplayStateChange` retirait d'abord le `FloorISM` via `RefreshConcealedPitCellFloor`, puis passait au transfert des objets. Or les armes sont des corps Chaos, susceptibles de perdre leur support et de bouger pendant la décision PIT02.

- `DropWorldItemsThroughOpenPitAtCell` est désormais exécuté **avant le retrait du sol**, alors que le gameplay de la fosse est déjà à `Open`.
- Au lieu d'écraser la cellule enregistrée par une nouvelle cellule physique potentiellement transitoire à l'ouverture, un item est candidat lorsque **sa cellule de placement OU la cellule de son mesh** est sur la trappe.
- Le transfert utilise le décalage horizontal réel du `MeshComponent` plutôt que la racine de l'acteur qui ne suit pas nécessairement Chaos.
- Les logs au niveau `Log` (pas uniquement `Verbose`) donnent : `GridPit TrapOpenTransfer`, `GridPit ItemTransfer queued`, `GridPit PendingItem materialized`, ainsi que cellule demandée, cellule matérialisée, racine, mesh et disponibilité du StaticMesh. Le diagnostic doit permettre d'isoler ensuite des problèmes de physique visibles uniquement en PIE.
- Les mêmes règles continuent de s'appliquer à toutes les identités physiques (armes ARM, torches, clés, etc.), sans modifier SaveGame v27 ni aucune donnée personnelle.

## Régression renforcée

Le test automatisé `Grimrock.RPG.ARM.CLEAN06_4X1612.TrapOpensUnderTwoWeaponsRealLevels` est étendu :
- la cellule exactement sous la fosse est vide ; **la destination effective est la cellule voisine (2,1)** ;
- une épée N1 est placée sur la trappe ;
- un couteau N20 possède un emplacement de départ voisin, mais son mesh physique a bougé sur la trappe ;
- à l'ouverture les deux GUID sont dans la file du niveau inférieur, **avec coordonnées (2,1)** ;
- après SaveGame en mémoire et voyage du groupe vers (2,1), vérification de l'acteur, son emplacement en XY, son poids canonique à cette cellule, son mesh et le maintien des mêmes coordonnées après aller-retour.

Cette régression est plus stricte que les précédentes, mais le monde d'automatisation `-NullRHI` ne simule toujours pas toute la physique Chaos PIE. Ne jamais déclarer la régression utilisateur résolue par les seuls statuts Windows.

## Suivi et limites

Attendre les deux statuts Windows `UE5.5.4/SilentTask` et `UE5.5.4/LocalAuto` sur le SHA exact du jalon. En cas de nouvel échec PIE, consulter uniquement les lignes `GridPit TrapOpenTransfer`, `GridPit ItemTransfer queued` et `GridPit PendingItem materialized` afin de distinguer : zéro objet transféré / mauvaise destination / acteur restauré mais invisible en physique. Ne pas imposer un scénario torche inexistant.

Après validation PIE, reprendre la migration globale et réaliser 06.5 puis 06.6 séparément. Aucun .uasset/.umap/.sav ni WBP modifié par ce correctif.
