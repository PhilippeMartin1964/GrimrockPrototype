# Notes d’audit des portes et mécanismes

## Fichiers relus

- `Source/GrimrockPrototype/Public/Core/GridTypes.h`
- `Source/GrimrockPrototype/Public/Core/GridObjectBehavior.h`
- `Source/GrimrockPrototype/Public/Core/GridLevelAsset.h`
- `Source/GrimrockPrototype/Public/Runtime/GridDoorActor.h`
- `Source/GrimrockPrototype/Private/Runtime/GridDoorActor.cpp`
- `Source/GrimrockPrototype/Public/Runtime/GridDoorSystemComponent.h`
- `Source/GrimrockPrototype/Private/Runtime/GridDoorSystemComponent.cpp`
- `Source/GrimrockPrototype/Public/Runtime/GridLevelRuntimeActor.h`
- `Source/GrimrockPrototype/Private/Runtime/GridLevelRuntimeActor.cpp`
- `Source/GrimrockPrototype/Public/Runtime/GridActivationComponent.h`
- `Source/GrimrockPrototype/Private/Runtime/GridActivationComponent.cpp`
- validation et panneau de liens sous `Source/GrimrockPrototypeEditor/`
- documents d’architecture existants sous `docs/Architecture/`

## Corrections apportées

- l’ouverture libère maintenant l’arête dès que la commande est acceptée ;
- la fermeture continue de bloquer l’arête avant de lancer l’animation ;
- la fin d’animation resynchronise explicitement passage et position finale ;
- la chaîne appelle le `ToggleDoorOnEdge()` central au lieu de modifier directement l’acteur ;
- les commandes et fins d’animation disposent de diagnostics plus explicites.

## Validation ajoutée

- avertissement pour une porte sur une limite sans cellule voisine ;
- avertissement lorsque le même événement d’une même source ouvre et ferme la même porte.

Les contrôles existants couvrent déjà `Edge=None`, le mur `Solid`, les cibles manquantes, les commandes incompatibles et les objets non générés.

## Décisions d’architecture

- `RuntimeBlockedDoorEdges` est la source de vérité de `CanMove()` ;
- l’acteur est la représentation visuelle et ne commande pas seul la passabilité ;
- `Open`/`Activate` libèrent le passage au début de l’ouverture ;
- `Close`/`Deactivate` bloquent le passage au début de la fermeture ;
- `Toggle` utilise l’état central, y compris pendant une animation ;
- la chaîne suit le même chemin central que les commandes de liens.

## Incohérences constatées et conservées

- `AGridDoorActor::bIsOpen` décrit la position finale, pas nécessairement l’ordre courant pendant l’animation ;
- `ActiveObjectIds` peut contenir l’état initial d’une porte, mais ne pilote pas les commandes ou le déplacement ;
- les événements `Opened` et `Closed` ne sont pas émis ;
- la sauvegarde de niveau ne reprend pas une animation à mi-course ;
- aucune règle n’empêche une fermeture alors que le groupe vient de franchir l’arête.

## Tests manuels recommandés

- bouton `Activated -> Open` ;
- levier `Activated -> Open` et `Deactivated -> Close` ;
- levier `Activated -> Toggle` et `Deactivated -> Toggle` ;
- plaque `Activated -> Open` et `Deactivated -> Close` ;
- réceptacle et trigger vers chaque commande de porte utile ;
- chaîne depuis le bon côté, puis refus depuis le mauvais côté ;
- passage autorisé dès le début de l’ouverture ;
- passage bloqué dès le début de la fermeture ;
- inversion de commande pendant l’animation ;
- état initial ouvert et état initial fermé ;
- porte sur l’arête opposée d’une cellule voisine ;
- plusieurs sources commandant la même porte ;
- restauration après changement de niveau.

## Points futurs

- décider si `Opened` et `Closed` doivent être émis en fin d’animation ;
- clarifier ou retirer l’état de porte dans `ActiveObjectIds` ;
- définir une politique de fermeture lorsque le groupe occupe l’arête ;
- auditer les profils de collision des meshes de porte utilisés par les assets ;
- ajouter des tests automatisés du système de porte.
