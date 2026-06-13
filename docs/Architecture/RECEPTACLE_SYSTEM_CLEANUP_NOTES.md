# Notes d'audit du système de réceptacles

## Fichiers relus

- `docs/Design/RECEPTACLE_SYSTEM.md` et les fondations objet, souris, liens et portes ;
- types noyau, comportement placé et archétypes ;
- acteur de réceptacle, acteur et définition d'item ;
- inventaire du groupe et service de transfert ;
- niveau runtime, contrôleur, pawn et composant d'activation ;
- validation, inspecteur d'objet et panneau de liens de l'éditeur.

## Statut du document Design

Le document Design mélange invariants, architecture cible et fonctionnalités futures. Il est conservé comme spécification historique et prospective avec un renvoi vers la fondation actuelle. Le nouveau document d'architecture est la référence pour le code existant.

## Écarts entre design et code

- `UGridItemTransferService`, le mode de placement et la politique existent,
  mais tous les paramètres runtime ne sont pas persistés par l'objet placé.
- Les coffres avec inventaire dédié, interfaces de conteneur, recettes et sauvegarde complète restent prospectifs.
- Les événements actifs sont `ItemInserted`, `ItemRemoved` et `ItemChanged`. Les événements d'acceptation, refus, plein, vide, verrouillage et déverrouillage du document Design n'existent pas.
- `ItemChanged` accompagne actuellement insertion et retrait ; la consommation l'émet seule.
- Le dépôt runtime utilise un curseur technique et un clic direct, contrairement à la préférence prospective pour un glisser-déposer intégral.
- Le poids est une donnée d'item et une condition de lien, pas une limite de capacité.

## Corrections appliquées

- le précontrôle d'un item équipé utilise désormais la même acceptation par
  définition d'item que les autres chemins ;
- le document Design indique explicitement son statut ;
- les documents objet, souris, liens et portes renvoient vers la nouvelle fondation ;
- les comportements réels, limites et diagnostics sont consolidés.

## Validations ajoutées

- liste `AcceptedItems` vide lorsque l'acceptation universelle est désactivée ;
- entrée `AcceptedItems` ou `InitialContent` sans asset de définition ;
- réceptacle initialement actif sans item initial.

Les validations déjà présentes couvrent le placement, les listes positives vides, les conditions invalides, les commandes incompatibles et les objets désactivés.

## Comportements conservés

- `bCanRemoveItem` est l'unique autorité du retrait joueur ;
- les commandes Enable/Disable Removal modifient cet état runtime ;
- `ConsumeAllItems` émet un `ItemChanged` pour chaque entrée consommée ;
- `MaxContainedItems <= 0` reste interprété comme illimité par le runtime ;
- `AcceptedItems` est résolu depuis les assets vers leurs `ItemDefinitionId` ;
- les règles d'acceptation, d'insertion, de retrait et d'émission des liens restent inchangées.

## Nettoyage des diagnostics runtime

- les logs temporaires de `BeginPlay`, d'initialisation et de création du contenu initial ont été supprimés ;
- `EvaluateItemAcceptance()` est silencieux par défaut, notamment pendant le survol souris ;
- son diagnostic complet reste disponible avec `bLogDiagnostics=true` et au niveau `VeryVerbose` ;
- les refus d'insertion réels conservent un warning court avec l'objet, l'item et la raison ;
- le diagnostic de génération de l'acteur runtime a été abaissé à `VeryVerbose` ;
- les helpers locaux ont été audités : aucun n'était mort après nettoyage, car ils servent encore au diagnostic optionnel, à la capture d'état ou aux liens.

## Tests manuels recommandés

- dépôt compatible et incompatible ;
- dépôt sans viser directement le support, hors portée et derrière un obstacle ;
- retrait autorisé, interdit, verrouillé et avec inventaire plein ;
- réceptacle plein et curseur déjà occupé ;
- `ItemInserted -> Door Open`, `ItemRemoved -> Door Close` ;
- `ItemChanged` avec condition par définition, tag, type, nombre et poids ;
- consommation d'une entrée et de toutes les entrées ;
- verrouillage, déverrouillage, activation et désactivation du retrait ;
- item équipé accepté ou refusé selon sa définition ;
- survol prolongé d'un support compatible et incompatible sans spam dans l'Output Log ;
- clic de dépôt refusé avec un diagnostic court, et dépôt accepté sans warning.

## Points futurs

- décider si le verrouillage doit aussi bloquer l'insertion ;
- décider si `Unlock` doit restaurer une politique mémorisée ;
- décider si une consommation multiple doit agréger `ItemChanged` ;
- renommer les champs historiques d'archétype sans casser les assets ;
- persister les changements runtime de politique et de retrait ;
- ajouter des tests automatisés de transfert et d'émission d'événements ;
- remplacer à terme la recherche mondiale de `FindRuntimeActor()` par une référence directe si le runtime devient propriétaire explicite de tous les réceptacles.
