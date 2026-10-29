# ARMEMENT-LOT02.13.6C — Chute, orientation et persistance des flèches

## 13.6C.1 — Libération physique et véritable pose monde
- `AGridThrownItemActor` n'est pas le pickup : il détient l'instance unitaire jusqu'au passage à `AGridItemActor`.
- Le runtime crée ce pickup avec `TryDropItemInstanceAtWorldTransform`, en utilisant **la position et la rotation monde du MeshComponent de la flèche**. La chute est confiée à Chaos via `ConfigureAsWorldPickup` ; aucune rotation arbitraire sud-nord ou snap au centre n'est effectuée.
- Sur Miss ou collision avant la cible, le même chemin est utilisé si la cellule est valide ; le repli standard demeure pour les obstacles / limites de grille.
- `RefreshWorldItemPhysicsCells` recalcule les cellules depuis les positions simulées : les entrées de ramassage ne doivent pas rester liées à la cellule initiale si la flèche rebondit et atterrit ailleurs.
- Limite physique : le StaticMesh de la flèche doit disposer d'une **simple collision valide pour Chaos**. L'automation peut vérifier les transforms sans une scène Chaos pleinement simulée.
- Le GUID de la flèche n'est jamais recréé pour l'objet au sol.

## 13.6C.2 — Sauvegarde
Le runtime doit enregistrer le transform réel du MeshComponent simulé, distinct de l'acteur racine. Les flèches fichées doivent enregistrer le GUID de leur monstre, l'os et leur transform relatif ; un monstre absent ou mort au chargement doit déclencher une récupération comme pickup, sans duplication.

## 13.6C.2a — Physics Save/Load
Le mesh simulé Chaos, non l'acteur SceneRoot, détient le transform réel.
`FGridRuntimeItemState` sauvegarde désormais facultativement
`WorldMeshTransform` et `bHasWorldMeshTransform`, après synchronisation
des cellules. À la restauration des pickups, ce transform est appliqué
au MeshComponent. Les anciens états conservent leur comportement.

## 13.6C.2b — Flèches fichées et en vol sauvegardées
Les flèches consommées mais encore physiques sont capturées directement
par `TActorIterator<AGridThrownItemActor>` et leur GUID unique. Une flèche
fichée enregistre l'ID persistant du monstre, l'os et sa pose relative ;
une flèche en vol enregistre la dernière pose de mesh pour rechargement
sous forme de pickup. Le monstre n'est pas dupliqué, pas de nouveau dégât.
