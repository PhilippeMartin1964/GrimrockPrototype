# Relecture des objets lisibles et des retours

## Fichiers relus

Les classes runtime de contrôleur, pawn, objets génériques, activation, widget, niveau, portes, items et réceptacles ont été relues, ainsi que les structures Core, l'inspecteur d'objet et la validation du niveau dans le module éditeur. Les documents souris, objets, portes, réceptacles et items ont également été comparés au code.

Le widget réel est `UI/ReadableMessageWidget.h/.cpp`. Les chemins `Runtime/GridReadableMessageWidget.*` cités dans la demande n'existent pas.

## Comportements confirmés

- Le texte placé non vide remplace le texte d'archétype.
- `Notes` et `Tag` ne sont pas affichés au joueur.
- Le message est persistant par défaut et peut être mis à jour par une autre lecture.
- Le clic suivant ferme seulement le message.
- Les six actions de déplacement et de rotation ferment le message puis continuent.
- Les curseurs `Read`, `Take`, `Push`, `Pull`, `PlaceItem`, `CannotPlaceItem` et `Forbidden` sont déjà exploités.

## Corrections appliquées

- ajout d'un retour court temporaire séparé du message lisible ;
- retours pour cible absente, hors de portée, mauvais bord, incompatibilité, réceptacle plein, dépôt échoué et inventaire plein ;
- curseur de chaîne de porte corrigé de `Take` vers `Pull` ;
- texte d'aide de l'inspecteur rendu indépendant de l'action clavier `Use`.

## Validations ajoutées

- objet lisible placé sans texte effectif ;
- notes renseignées sans texte joueur ;
- override ignoré sur un archétype non lisible ;
- objet lisible initialement désactivé.

## Incohérences et limites

- `Locked` existe mais n'est pas alimenté par une politique générale.
- Les refus métier ne fournissent pas de raison structurée commune.
- Le curseur système de repli ne représente pas tous les états.
- Aucun seuil de longueur n'existe pour un texte lisible.
- Les widgets Blueprint et les assets binaires n'ont pas été modifiés.

## Tests manuels recommandés

1. Lire un panneau, vérifier sa persistance, puis fermer par clic, déplacement et rotation.
2. Lire deux panneaux successifs et vérifier la mise à jour du texte.
3. Vérifier qu'un clic de fermeture n'active pas l'objet derrière.
4. Tester un panneau sans texte, un override et l'option de lecture unique.
5. Tester les dépôts sans cible, hors portée, depuis le mauvais bord, incompatibles et dans un réceptacle plein.
6. Remplir l'inventaire et vérifier que l'item monde reste présent.
7. Vérifier les curseurs de panneau, item, bouton, levier, chaîne et réceptacle.
8. Confirmer l'absence de warnings répétés au simple survol.

## Points futurs

Une évolution pourra introduire une raison d'échec structurée pour `CanInteract()` et les commandes métier, puis mapper cette raison vers `Locked`, `Forbidden` ou un texte localisé. Ce travail n'est pas requis pour la fondation actuelle.
