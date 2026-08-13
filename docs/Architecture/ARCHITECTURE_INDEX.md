# Index de l'architecture

## Objet

Cet index présente les contrats d'architecture actuellement vérifiés contre le C++ de GrimrockPrototype. Les documents de fondation décrivent le fonctionnement en vigueur. Les notes de cleanup consignent les audits, corrections et limites observés pendant chaque passe.

Les documents de `docs/Design/` peuvent décrire des intentions prospectives. Ils ne constituent pas nécessairement le contrat runtime tant que le code et les documents de fondation ne les confirment pas.

## Ordre de lecture recommandé

1. [Synthèse globale du projet](PROJECT_SYNTHESIS.md)
2. [Carte détaillée XMind](Maps/GRIMROCK_PROJECT_MAP.xmind) ou sa
   [source Markdown](Maps/GRIMROCK_PROJECT_MAP.md)
3. [Donjon, niveau et grille](CORE_DUNGEON_LEVEL_GRID.md)
4. [Archétypes et objets placés](OBJECT_ARCHETYPES_AND_PLACED_OBJECTS.md)
5. [Panneau de validation du niveau](LEVEL_VALIDATION_PANEL_FOUNDATION.md)
6. [Interaction souris](MOUSE_INTERACTION_FOUNDATION.md)
7. [Liens, événements et commandes](LINK_EVENT_COMMAND_FOUNDATION.md)
8. [Portes et mécanismes](DOOR_MECHANISM_FOUNDATION.md)
9. [Réceptacles](RECEPTACLE_SYSTEM_FOUNDATION.md)
10. [Ramassage et placement des items](ITEM_PICKUP_AND_PLACEMENT_FOUNDATION.md)
11. [Objets lisibles et retours](READABLE_OBJECTS_AND_FEEDBACK_FOUNDATION.md)

## Documents de fondation

| Document | Portée |
|---|---|
| [CORE_DUNGEON_LEVEL_GRID.md](CORE_DUNGEON_LEVEL_GRID.md) | Donjon, niveaux, cellules, murs, édition de base et génération runtime. |
| [OBJECT_ARCHETYPES_AND_PLACED_OBJECTS.md](OBJECT_ARCHETYPES_AND_PLACED_OBJECTS.md) | Archétypes, palette, données placées, overrides, aperçu et acteurs runtime. |
| [LEVEL_VALIDATION_PANEL_FOUNDATION.md](LEVEL_VALIDATION_PANEL_FOUNDATION.md) | Messages de validation, catégories, tableau de bord et navigation vers les problèmes. |
| [MOUSE_INTERACTION_FOUNDATION.md](MOUSE_INTERACTION_FOUNDATION.md) | Trace de visibilité, priorité du clic, portée, interactions de bord et curseurs. |
| [LINK_EVENT_COMMAND_FOUNDATION.md](LINK_EVENT_COMMAND_FOUNDATION.md) | Identité des liens, événements source, conditions et commandes cible. |
| [DOOR_MECHANISM_FOUNDATION.md](DOOR_MECHANISM_FOUNDATION.md) | État des portes, passabilité, animation, commandes et chaîne. |
| [RECEPTACLE_SYSTEM_FOUNDATION.md](RECEPTACLE_SYSTEM_FOUNDATION.md) | Acceptation, dépôt, retrait, contenu, événements et commandes de réceptacle. |
| [ITEM_PICKUP_AND_PLACEMENT_FOUNDATION.md](ITEM_PICKUP_AND_PLACEMENT_FOUNDATION.md) | Identité des items, accessibilité, inventaire, curseur et transferts. |
| [READABLE_OBJECTS_AND_FEEDBACK_FOUNDATION.md](READABLE_OBJECTS_AND_FEEDBACK_FOUNDATION.md) | Textes lisibles, messages persistants, retours courts et états de curseur. |

## Conceptions prospectives liées

| Document | Portée |
|---|---|
| [GRIMROCK_LOCK_SYSTEM.md](../Design/GRIMROCK_LOCK_SYSTEM.md) | Spécification de design pour serrures murales, clés, crochetage abstrait, conteneurs verrouillables et serrures piégées. Ce document complète les fondations existantes mais ne remplace pas un futur contrat runtime validé contre le C++. |

## Notes de cleanup

- [CORE_CLEANUP_NOTES.md](CORE_CLEANUP_NOTES.md)
- [OBJECT_SYSTEM_CLEANUP_NOTES.md](OBJECT_SYSTEM_CLEANUP_NOTES.md)
- [MOUSE_INTERACTION_CLEANUP_NOTES.md](MOUSE_INTERACTION_CLEANUP_NOTES.md)
- [LINK_EVENT_COMMAND_CLEANUP_NOTES.md](LINK_EVENT_COMMAND_CLEANUP_NOTES.md)
- [DOOR_MECHANISM_CLEANUP_NOTES.md](DOOR_MECHANISM_CLEANUP_NOTES.md)
- [RECEPTACLE_SYSTEM_CLEANUP_NOTES.md](RECEPTACLE_SYSTEM_CLEANUP_NOTES.md)
- [ITEM_PICKUP_AND_PLACEMENT_CLEANUP_NOTES.md](ITEM_PICKUP_AND_PLACEMENT_CLEANUP_NOTES.md)
- [READABLE_OBJECTS_AND_FEEDBACK_CLEANUP_NOTES.md](READABLE_OBJECTS_AND_FEEDBACK_CLEANUP_NOTES.md)
- [LEVEL_VALIDATION_PANEL_CLEANUP_NOTES.md](LEVEL_VALIDATION_PANEL_CLEANUP_NOTES.md)

Une synthèse transversale de la première relecture est disponible dans [ARCHITECTURE_CONSISTENCY_AUDIT.md](ARCHITECTURE_CONSISTENCY_AUDIT.md).

## Tests

- [Checklist de régression du donjon de test](../Tests/TEST_DUNGEON_PASS_CHECKLIST.md)
- [Rapport de régression du donjon de test](../Tests/TEST_DUNGEON_PASS_REPORT.md)

## Règles transversales

1. Les DataAssets restent les sources persistantes.
2. Les acteurs runtime sont transitoires et reconstruits depuis les données persistantes.
3. Les outils et acteurs éditeur modifient les DataAssets, pas un état runtime parallèle.
4. Le premier hit bloquant `ECC_Visibility` est autoritaire pour l'interaction souris.
5. Les interactions placées sur une arête dépendent de la cellule, du bord et de l'orientation du groupe.
6. Les liens utilisent `ObjectId` comme identité principale, jamais le tag ou l'archétype.
7. Les portes utilisent `RuntimeBlockedDoorEdges` comme source de vérité pour la passabilité.
8. Les items utilisent `ItemDefinitionId` comme identité fonctionnelle.
9. Les messages lisibles, retours courts, curseurs et logs sont des canaux séparés.
10. Les Blueprints et assets configurent les variantes concrètes sans remplacer les contrats C++ documentés.
