# Notes de stabilisation du ramassage et du placement

## Fichiers relus

- noyau : `GridTypes.h`, `GridObjectBehavior.h`, `GridObjectArchetypeAsset.h`, `GridLevelAsset.h` ;
- items : `GridItemActor.*`, `GridItemDefinitionAsset.*`, `GridInventoryTypes.h`, `GridItemTransferService.*` ;
- runtime : `GrimrockPlayerController.*`, `GrimrockPartyPawn.*`, `GridInteractionUtils.*`, `GridInteractableInterface.h`, `GridReceptacleActor.*`, `GridLevelRuntimeActor.*` ;
- éditeur : `GridLevelEditorActor.*` et les chemins d'inspection des objets placés ;
- documentation : fondations souris, objets, liens, portes, réceptacles et spécification historique des réceptacles.

## Comportements confirmés

- Le premier impact `ECC_Visibility` possède le clic ; aucun repli ne traverse un obstacle.
- Le ramassage monde va directement dans l'inventaire du personnage sélectionné.
- L'acteur monde est supprimé seulement après l'ajout réussi.
- Un inventaire plein laisse l'item dans le monde.
- Un curseur refusé reste inchangé.
- Le service de transfert restaure explicitement sa source après un échec de destination.
- `TryDropCursorItem()` n'implémente pas encore le dépôt libre.
- `HeldItemActor` est visuel et ne porte pas la propriété logique de l'item.

## Corrections appliquées

- Le survol d'un `AGridItemActor` consulte maintenant `AGridLevelRuntimeActor::CanPartyPickupItemActor()`.
- Le survol et `TryPickupItemActor()` partagent `CanPartyPickupItemEntry()`.
- Un item placé sur une arête de la cellule du groupe exige désormais que le groupe regarde cette arête.
- Le précontrôle de survol ne produit pas de journal `Warning` à chaque image ; l'action refusée conserve les diagnostics existants.
- La validation éditeur détecte les définitions absentes, vides ou contradictoires et les items placés sur une cellule non accessible.

## Incohérences trouvées

- Le survol annonçait `Take` avant la validation réelle de cellule et d'arête.
- Toute arête de la cellule du groupe était auparavant acceptée, même si le groupe regardait ailleurs.
- La validation de niveau ne signalait pas les définitions d'item absentes ou contradictoires.
- La documentation historique de ramassage décrivait l'ancienne règle trop permissive.

## Comportements conservés

- Les items contenus délèguent leur interaction au réceptacle.
- Le dépôt souris avec curseur exige un réceptacle directement touché.
- Les chemins inventaire et curseur existants ne sont pas refondus.
- Le placement d'un item sur une arête reste une possibilité propre aux items placés.

## Points à surveiller

- `SetCursorItem()` est une primitive publique qui peut remplacer directement le contenu courant ; les chemins métier vérifiés contrôlent l'occupation avant de l'utiliser.
- `AddPlacedItemActor()` désactive actuellement la lumière d'un item monde après sa génération. Cette règle doit rester explicite si des torches allumées au sol sont attendues.
- Le runtime conserve une solution de repli historique basée sur `ArchetypeId` lors de certaines résolutions. Les nouvelles données doivent renseigner une définition d'item.
- Les différences entre supports de torche simples et retournables résident en partie dans les classes Blueprint et les actifs, hors du code C++ audité.

## Validation manuelle

Exécuter la liste de tests de `ITEM_PICKUP_AND_PLACEMENT_FOUNDATION.md`, avec une attention particulière aux quatre orientations, à l'inventaire plein, au curseur occupé et au retrait d'un support de torche.

Les tests PIE ne sont pas automatisés dans cette passe ; ils sont documentés pour exécution dans l'éditeur.

## Nettoyage des diagnostics runtime

- l'évaluation d'acceptation appelée par le survol est désormais silencieuse ;
- les diagnostics complets de réceptacle sont optionnels et `VeryVerbose` ;
- les refus de clic réels conservent un message court sans répéter toutes les règles d'acceptation ;
- les logs normaux d'initialisation des réceptacles ont été supprimés ;
- le diagnostic de classe, maillage et transform de génération runtime a été abaissé à `VeryVerbose` ;
- les helpers soupçonnés de code mort ont été vérifiés et conservés lorsqu'ils participent encore à la capture d'état ou à la résolution runtime.

## Points futurs

- Décider si une torche placée dans le monde doit être allumée par défaut.
- Encapsuler `SetCursorItem()` si des appels directs non contrôlés apparaissent.
- Ajouter le dépôt monde seulement avec une cible et une règle de placement explicites.
- Étendre la validation aux doublons suspects entre item initial d'un réceptacle et item placé lorsque les données d'actifs pourront être inspectées sans modifier les `.uasset`.
