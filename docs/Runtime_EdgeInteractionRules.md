# Règles runtime des interactions sur edge

## Règle autoritaire

Un raycast souris et une distance physique valide ne suffisent pas à autoriser
une interaction. Pour tout objet placé sur un edge, la position logique et
l'orientation du groupe doivent désigner exactement le mur concerné.

Un objet `(ObjectCell, ObjectEdge)` est accessible dans deux cas seulement :

1. `ObjectCell == PartyCell` et `ObjectEdge == PartyFacing` ;
2. `ObjectCell == FrontNeighborCell` et
   `ObjectEdge == Opposite(PartyFacing)`.

Le deuxième cas représente le même mur stocké sur l'edge opposé de la cellule
voisine. Tout mur latéral, arrière ou plus éloigné est refusé, même si son mesh
est visible et proche de la caméra.

## Flux concernés

La validation commune
`AGridLevelRuntimeActor::CanPartyInteractWithEdgeObject()` protège :

- les inscriptions et autres objets lisibles ;
- les boutons et leviers cliquables ;
- les réceptacles, supports de torche, alcôves, autels et bols d'offrande ;
- le retrait d'un item contenu dans un réceptacle ;
- le dépôt d'un item tenu par le curseur dans un réceptacle ;
- le feedback de curseur affiché pendant le survol d'un réceptacle.

Les plaques de pression restent pilotées par l'entrée et la sortie de cellule.
Le ramassage des items au sol conserve les règles décrites dans
`docs/Runtime_ItemPickupRules.md`.

Les refus sont consignés en niveau `Verbose` avec
`Reason=EdgeNotFacingParty`, la cellule et le facing du groupe, ainsi que la
cellule et l'edge de l'objet.

## Tests manuels PIE

1. Lire une inscription placée sur le mur en face du groupe.
2. Vérifier que la même inscription sur un mur latéral visible est refusée.
3. Prendre une torche depuis un support placé sur le mur en face.
4. Vérifier qu'un support latéral visible ne permet pas de prendre sa torche.
5. Depuis une cellule, interagir avec un support stocké dans la cellule devant
   sur l'edge opposé au facing.
6. Vérifier qu'un support dans la cellule devant mais sur un edge Est ou Ouest
   est refusé lorsque le groupe regarde vers le nord.
7. Vérifier qu'un dépôt depuis le curseur vers un réceptacle latéral est refusé
   et affiche le curseur d'interdiction.
8. Vérifier que le dépôt vers le réceptacle en face est autorisé.
9. Vérifier les règles existantes de ramassage des items au sol et sur edge.
10. Vérifier les boutons, leviers, portes et plaques de pression.
