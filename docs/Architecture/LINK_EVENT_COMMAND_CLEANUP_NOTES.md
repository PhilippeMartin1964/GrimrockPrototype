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
