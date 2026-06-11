# Règles runtime de ramassage des items

## Principe

La distance physique entre la caméra et un item ne suffit pas à autoriser son
ramassage. Le runtime applique une règle autoritaire fondée sur la cellule
logique du groupe, son orientation et l'edge de placement de l'item.

La vérification de portée du `AGrimrockPlayerController` reste une garde
secondaire pour l'interaction souris.

## Item au sol ou au centre d'une cellule

Un item dont `Edge` vaut `EGridEdge::None` est considéré comme placé au centre
ou au sol de sa cellule.

Il peut être ramassé uniquement lorsque le groupe se trouve sur cette même
cellule. Un item au centre de la cellule située devant le groupe ne peut donc
pas être ramassé à distance, même s'il est visible et à portée du clic.

## Item placé sur un edge

Un item placé sur un edge dans la cellule du groupe peut être ramassé
uniquement lorsque le groupe regarde cet edge.

Il peut aussi être ramassé depuis la cellule adjacente seulement lorsque toutes
les conditions suivantes sont satisfaites :

- la cellule de l'item est la cellule directement devant le groupe ;
- le groupe regarde vers cette cellule ;
- l'edge de l'item est celui qui fait face au groupe, soit l'edge opposé à la
  direction regardée.

Exemple : si le groupe regarde vers le nord, l'item doit être dans la cellule
au nord et placé sur l'edge sud de cette cellule.

## Interaction clavier et interaction souris

L'action clavier `Use` passe par
`AGrimrockPartyPawn::TryUseFrontInteraction()`. Elle tente d'abord de ramasser
un item dans la cellule courante, puis traite les interactions situées devant
le groupe.

Le clic souris sur un item passe par `AGridItemActor::Interact_Implementation()`
puis `AGridLevelRuntimeActor::TryPickupItemActor()`. Ce chemin applique la même
validation logique de cellule, orientation et edge avant tout ajout à
l'inventaire.

Les items contenus dans un `AGridReceptacleActor` ne passent pas par ce
ramassage direct. Leur interaction reste déléguée au réceptacle propriétaire,
notamment pour les supports de torche, alcôves et autels.

## Tests manuels PIE

Préparer un item au sol et plusieurs placements sur edge, puis vérifier :

1. Item central sur la même cellule que le groupe : ramassage autorisé.
2. Item sur un edge de la même cellule : autorisé uniquement si le groupe
   regarde cet edge.
3. Item sur une cellule non adjacente : clic refusé.
4. Item au centre de la cellule devant le groupe : clic refusé.
5. Item dans la cellule devant le groupe, sur l'edge faisant face au groupe :
   ramassage autorisé.
6. Item dans la cellule devant le groupe, sur un autre edge : clic refusé.
7. Tourner le groupe sans le déplacer : le cas autorisé précédent doit être
   refusé tant que le groupe ne regarde plus l'item.
8. Item contenu dans un réceptacle ou support de torche : l'interaction est
   toujours traitée par le réceptacle, sans ramassage direct par le niveau.

Les refus de ramassage liés à la grille sont consignés dans les logs avec la
cellule du groupe, son orientation, la cellule de l'item et son edge.

Voir aussi
[`docs/Architecture/ITEM_PICKUP_AND_PLACEMENT_FOUNDATION.md`](Architecture/ITEM_PICKUP_AND_PLACEMENT_FOUNDATION.md)
pour le cycle complet monde, inventaire, curseur, réceptacle et équipement.
