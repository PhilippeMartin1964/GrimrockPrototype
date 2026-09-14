# GrimrockPrototype — Système d’objets de donjon

Statut : **contrat actif après WORLDOBJ-CLASS01**, 2026-09-14.

## Classification

Un objet du monde possède une seule classification fonctionnelle principale :
`UGridWorldObjectDefinitionAsset::SupportedType`, affichée comme **Gameplay Type**.

Les Gameplay Types couvrent notamment Door, Button, PressurePlate, Lever, Decoration,
MonsterSpawn, ItemSpawn, Light, Relocation, Trigger, Receptacle, Item, Logic,
StoryCompanion, CustomRecruiter et Pit.

`Relocation` est le type canonique des escaliers, portails et passages qui déplacent
automatiquement le groupe. Pit reste distinct. Il n'existe aucun Gameplay Type Teleporter,
aucune Functional Category et aucune seconde enum de classification fonctionnelle.

Les capacités transversales restent décrites par leurs vraies données :

- `bIsInteractable` pour l'interaction directe ;
- `bIsReadable` et les données de lecture ;
- `bIsLightSource` et les paramètres lumineux ;
- `DefaultBehavior` pour les règles partagées ;
- `RuntimeActorClass` pour l'implémentation runtime.

Une inscription peut donc être `Decoration` avec `bIsReadable=true`. Une torche fixe peut
être `Decoration` ou `Light` selon son rôle, avec `bIsLightSource=true` lorsqu'elle produit
une lumière.

## Définition et placement

Une `UGridWorldObjectDefinitionAsset` porte l'identité et le comportement réutilisables :

```text
DefinitionId
DisplayName
SupportedType (Gameplay Type)
Description
PlacementSurface / DefaultLocalPosition
Spatial Behavior
Interaction / Light
StaticPart / MovingParts
RuntimeActorClass
DefaultBehavior
AudioEvents
```

Un `FGridWorldObjectInstance` porte l'identité du placement, sa cellule, son orientation et
ses données locales. Le placement doit conserver un `Type` cohérent avec le Gameplay Type de
sa définition.

## Palette

La palette possède sa propre présentation éditoriale. Chaque
`FGridObjectPaletteEntry` porte explicitement :

```text
EntryId
DisplayNameOverride
PaletteCategory
Icon
DefaultWorldObjectDefinition ou DefaultItemDefinition
```

`PaletteCategory` est l'unique autorité de groupement. Une définition ne connaît pas sa
catégorie de palette. Une entrée officielle sans catégorie est invalide.

Les escaliers `Stairs_Up` et `Stairs_Down` sont des définitions `Relocation`, placées sur
`Floor`, et leurs entrées appartiennent à `Navigation`. Pit est un Gameplay Type distinct et
son entrée appartient à `Hazards`.

## Runtime

Le runtime résout la définition et la configuration de l'instance. Les objets interactifs
reçoivent les commandes supportées par leur Gameplay Type. Les Relocations s'exécutent
automatiquement lorsque le groupe termine son entrée dans leur cellule et si leur état
d'activation est actif.

Une relocation sur le niveau courant ne reconstruit pas le niveau. Une relocation vers un
autre niveau utilise `TravelToDungeonLevel`. Pit conserve son chemin de chute dédié.

Les commandes et événements de téléportation de monstres, tels que
`EGridObjectCommand::Teleport` et `EGridObjectEvent::MonsterTeleported`, restent indépendants
de la classification des World Objects.

## Validation

La validation contrôle directement le Gameplay Type, la surface de placement, les données
requises par le comportement et la cohérence entre définition, entrée de palette et placement.
Elle ne recommande ni ne compare de catégorie fonctionnelle secondaire.

Voir aussi :

- [Référence des paramètres](11_GRID_WORLD_OBJECT_DEFINITION_PARAMETERS_REFERENCE.md)
- [Règle Definition / Instance](12_GRID_OBJECT_INSTANCE_BEHAVIOR_RULE.md)
- [Relocation](GRID_RELOCATION_DATA.md)
- [Architecture des définitions et placements](../Architecture/WORLD_OBJECT_DEFINITIONS_AND_PLACED_OBJECTS.md)
