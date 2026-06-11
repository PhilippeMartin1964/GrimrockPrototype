# Notes d’audit des liens, événements et commandes

## Fichiers relus

- `Source/GrimrockPrototype/Public/Core/GridTypes.h`
- `Source/GrimrockPrototype/Public/Core/GridLevelAsset.h`
- `Source/GrimrockPrototype/Private/Core/GridLevelAsset.cpp`
- `Source/GrimrockPrototype/Public/Runtime/GridActivationComponent.h`
- `Source/GrimrockPrototype/Private/Runtime/GridActivationComponent.cpp`
- `Source/GrimrockPrototype/Public/Runtime/GridLevelRuntimeActor.h`
- `Source/GrimrockPrototype/Private/Runtime/GridLevelRuntimeActor.cpp`
- acteurs runtime de bouton, levier, plaque, porte, trigger et réceptacle
- `Source/GrimrockPrototypeEditor/Public/EditorTools/GridLevelEditorActor.h`
- `Source/GrimrockPrototypeEditor/Private/EditorTools/GridLevelEditorActor.cpp`
- mode, toolkit et widgets sous `Source/GrimrockPrototypeEditor/`
- documents d’architecture et illustrations sous `docs/`

## Corrections apportées

- le levier respecte maintenant `SourceEvent` pour toutes les commandes, y compris `Toggle` ;
- le composant d’activation ne réémet plus les événements d’item déjà émis par le réceptacle ;
- une condition non applicable ou incomplète échoue avant `bInvertCondition` ;
- les erreurs de condition indiquent source, cible, acteurs résolus, condition et paramètre fautif ;
- un événement provenant d’un `ObjectId` absent est rejeté avec diagnostic ;
- les choix du panneau de liens correspondent aux émetteurs et commandes réellement actifs.

## Validations ajoutées

- source et cible valides et présentes ;
- doublon exact ;
- événement non émis par le type source ;
- commande incompatible avec le type cible ;
- condition non applicable à la cible ;
- paramètres de condition absents ou invalides ;
- auto-lien comme boucle potentielle ;
- source ou cible initialement désactivée.

## Incohérences constatées et conservées

- `Used`, `Entered`, `Exited`, `Opened`, `Closed`, `Enabled` et `Disabled` n’ont pas d’émetteur C++ actif ;
- `Enable`, `Disable`, `Lock`, `Unlock`, `Spawn`, `Despawn`, `Teleport` et `ShowMessage` ne sont pas dispatchées ;
- les conditions lisent uniquement un réceptacle cible ;
- le panneau de liens n’édite pas les conditions ;
- plusieurs cibles acceptent une commande d’état dont l’effet spécialisé reste limité ;
- il n’existe ni interface de commande commune ni détection de cycle indirect.

Ces éléments n’ont pas été supprimés afin de préserver les données et API existantes.

## Questions ouvertes

- faut-il activer progressivement les valeurs d’enum réservées ou réduire l’UI et les données à un sous-ensemble versionné ?
- les commandes d’état génériques doivent-elles être refusées pour les types sans effet concret ?
- les conditions futures doivent-elles lire la source, la cible ou un contexte explicite ?
- faut-il une interface runtime commune pour remplacer le dispatch central par type ?
- l’édition des conditions doit-elle rejoindre le panneau de liens ou un inspecteur dédié ?

## Tâches futures recommandées

- ajouter des tests automatisés événement → condition → commande ;
- définir le contrat des valeurs d’enum actuellement inactives avant de les exposer ;
- détecter les cycles indirects si les chaînes de liens deviennent récursives ;
- ajouter une édition structurée des conditions sans dupliquer leur logique runtime.

## Correctif bidirectionnel des mécanismes

### Cause

Les interactions directes des leviers et les entrées/sorties de plaques émettaient déjà les bons événements. En revanche, `SetTargetActiveState()` mettait à jour `ActiveObjectIds` et le visuel lorsqu’un lien commandait un levier ou une plaque, sans émettre `Activated` ou `Deactivated`. Une chaîne de mécanismes s’arrêtait donc sur cette cible.

### Comportement restauré

- un levier commandé émet `Activated` en devenant actif et `Deactivated` en redevenant inactif ;
- une plaque commandée suit le même contrat ;
- une commande qui demande l’état déjà présent ne réémet aucun événement ;
- `Toggle` reste une commande cible utilisable avec tout `SourceEvent` valide ;
- une protection de réentrée bloque le redispatch cyclique du même objet source pendant la chaîne courante.

La validation et le panneau de liens acceptaient déjà `Activated` et `Deactivated` pour les leviers et plaques. Ils acceptaient aussi `Toggle`, `Open`, `Close`, `Activate` et `Deactivate` selon le type cible ; aucune correction éditeur supplémentaire n’était nécessaire.

### Tests manuels minimaux

- levier `Activated -> Open` ;
- levier `Deactivated -> Close` ;
- levier `Activated -> Toggle` ;
- levier `Deactivated -> Toggle` ;
- plaque `Activated -> Open` ;
- plaque `Deactivated -> Close` ;
- plaque `Activated -> Toggle` ;
- plaque `Deactivated -> Toggle` ;
- vérifier un seul événement par mouvement de levier ;
- vérifier qu’une plaque ne répète pas l’événement tant que le groupe reste sur sa cellule ;
- vérifier qu’une commande répétant l’état courant ne produit pas de second événement ;
- vérifier qu’une boucle de liens est rejetée avec le diagnostic `cyclic link dispatch`.
