# Audit de cohérence de l'architecture

## Référence

- **Commit audité** : `09fd112f16626a706a9f61c4781628d6db2d6e53`
- **Date de l'audit** : 11 juin 2026
- **Périmètre** : sources C++ et documentation Markdown du socle d'architecture

## Objectif

Cette première passe vérifie que les documents de `docs/Architecture/` décrivent le comportement réellement présent dans les modules runtime, Core et éditeur. Elle recherche les écarts factuels sans lancer de refonte gameplay ni modifier les assets.

## Documents relus

Les huit documents de fondation et leurs notes de cleanup ont été relus :

- [Donjon, niveau et grille](CORE_DUNGEON_LEVEL_GRID.md)
- [Archétypes et objets placés](OBJECT_ARCHETYPES_AND_PLACED_OBJECTS.md)
- [Interaction souris](MOUSE_INTERACTION_FOUNDATION.md)
- [Liens, événements et commandes](LINK_EVENT_COMMAND_FOUNDATION.md)
- [Portes et mécanismes](DOOR_MECHANISM_FOUNDATION.md)
- [Réceptacles](RECEPTACLE_SYSTEM_FOUNDATION.md)
- [Ramassage et placement des items](ITEM_PICKUP_AND_PLACEMENT_FOUNDATION.md)
- [Objets lisibles et retours](READABLE_OBJECTS_AND_FEEDBACK_FOUNDATION.md)

L'[index de l'architecture](ARCHITECTURE_INDEX.md) donne accès aux notes associées.

## Sources C++ consultées

La relecture a notamment couvert :

- `Core/GridTypes.h`, `GridDungeonAsset.h/.cpp`, `GridLevelAsset.h/.cpp`, `GridObjectBehavior.h` et `GridObjectArchetypeAsset.h/.cpp` ;
- `Runtime/GridLevelRuntimeActor.h/.cpp`, `GridRuntimeObjectActor.h/.cpp` et `GridActivationComponent.h/.cpp` ;
- `Runtime/GrimrockPlayerController.h/.cpp`, `GrimrockPartyPawn.h/.cpp` et `GridInteractableInterface.h` ;
- `Runtime/GridDoorActor.h/.cpp`, `GridDoorSystemComponent.h/.cpp`, `GridReceptacleActor.h/.cpp` et `GridItemActor.h/.cpp` ;
- `UI/ReadableMessageWidget.h/.cpp` et `Runtime/GridGenericObjectActor.h/.cpp` ;
- `EditorTools/GridLevelEditorActor.h/.cpp`, le mode d'édition, son toolkit et les panneaux d'inspection concernés.

## Verdict

La documentation d'architecture est globalement alignée avec le C++ actuel. Aucun écart grave code ↔ architecture n'a été identifié pendant cette première passe. Les corrections appliquées concernent principalement une phrase obsolète sur les diagnostics de réceptacle, une précision sur l'inventaire ouvert dans le flux souris, et la création d'un index global.

## Écarts trouvés

1. `RECEPTACLE_SYSTEM_FOUNDATION.md` indiquait encore que chaque évaluation journalisait temporairement l'état complet. Le code est silencieux par défaut et ne produit ce diagnostic qu'avec `bLogDiagnostics=true`, au niveau `VeryVerbose`.
2. `MOUSE_INTERACTION_FOUNDATION.md` ne précisait pas que l'inventaire ouvert bloque le clic de gameplay seulement lorsqu'aucun item n'est porté au curseur.

## Corrections appliquées

- correction de la description des diagnostics de réceptacle ;
- ajout de la règle d'inventaire ouvert dans la priorité du clic gauche ;
- création de [ARCHITECTURE_INDEX.md](ARCHITECTURE_INDEX.md) ;
- création du présent document de synthèse ;
- contrôle des liens Markdown ajoutés et des références aux SVG existants.

## Points surveillés mais non corrigés

- `FindRuntimeActor()` dans `GridReceptacleActor.cpp` reste une dette technique future ;
- certains refus utilisateur réels sont encore journalisés en `Warning` ;
- les SVG n'ont pas été validés visuellement dans l'éditeur ;
- les comportements dépendants des Blueprints ou des `.uasset` restent à vérifier en PIE.

## Limites de l'audit

- audit fondé sur les sources C++ et Markdown ;
- aucune modification d'asset ;
- aucun test PIE automatisé ;
- vérification des SVG limitée à leur présence, leur syntaxe XML et leurs références ;
- compilation UBT exécutée après les modifications documentaires ;
- la cohérence des configurations binaires reste hors périmètre.

## Recommandations

1. Utiliser [ARCHITECTURE_INDEX.md](ARCHITECTURE_INDEX.md) comme point d'entrée de toute nouvelle passe.
2. Mettre à jour le document de fondation concerné dans le même commit qu'une évolution de contrat C++.
3. Réserver les notes de cleanup aux constats, migrations et dettes, sans y dupliquer le contrat principal.
4. Vérifier en PIE les comportements dépendants des Blueprints avant de considérer leur présentation comme stabilisée.
5. Réexécuter un audit transversal après une évolution majeure des identités, interactions, liens ou transferts.
