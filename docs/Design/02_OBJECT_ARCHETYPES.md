# GrimrockPrototype — Définitions d’objets

Statut : **modèle actif après WORLDOBJ-CLASS01**, 2026-09-14.

## Définitions canoniques

Le projet utilise une définition par nature de donnée :

| Nature | Définition canonique | Référence de palette |
|---|---|---|
| Objet fixe ou mécanisme de grille | `UGridWorldObjectDefinitionAsset` | `DefaultWorldObjectDefinition` |
| Item ramassable | `UGridItemDefinitionAsset` | `DefaultItemDefinition` |
| Monstre | `UGridMonsterDefinitionAsset` | `DefaultMonsterDefinition` pour son spawn |
| Compagnon narratif | `URPGStoryCompanionAsset` | `DefaultStoryCompanionDefinition` |

Une entrée d'item directe ne possède ni définition world-object compagnon ni icône dupliquée.

## World Object Definition

`UGridWorldObjectDefinitionAsset` décrit un concept réutilisable. Ses principaux groupes sont :

```text
Definition
  DefinitionId
  DisplayName
  SupportedType (Gameplay Type)
  Description

Placement
  PlacementSurface
  DefaultLocalPosition U / V / N

Spatial Behavior
Interaction / Light
Visual composition
RuntimeActorClass
DefaultBehavior
AudioEvents
```

Gameplay Type est l'unique classification fonctionnelle principale. Les capacités comme la
lecture, l'interaction ou la lumière restent portées par leurs flags et paramètres réels.

`Relocation` couvre les escaliers, portails et passages automatiques. Pit reste distinct.

## Palette

Une `FGridObjectPaletteEntry` possède `PaletteCategory`. Ce champ est l'unique source du
groupement de palette, y compris pour les entrées d'items directes. La définition référencée
ne porte aucune catégorie de palette.

```text
FGridObjectPaletteEntry
  EntryId
  PaletteCategory
  Icon
  DefaultWorldObjectDefinition / DefaultItemDefinition / autres références spécialisées
```

Le libellé affiché provient directement de la définition référencée. La palette ne possède
aucune seconde autorité de nom.

`PaletteCategory=None` s'affiche comme Uncategorized, mais échoue à la validation des palettes
officielles.

## Placement et résolution

Le placement world-object utilise `PlacementSurface` et `DefaultLocalPosition.U/V/N`. La face
d'un placement mural appartient à l'instance via `WallSide`.

La définition porte les valeurs permanentes. L'instance porte la cellule, l'orientation, les
états initiaux propres au puzzle et les rares overrides explicitement autorisés. Le runtime
résout ces deux sources sans recopier une seconde définition complète.

Les escaliers officiels utilisent `SupportedType=Relocation`, `PlacementSurface=Floor` et une
destination par défaut non configurée. Le level designer renseigne la destination réelle dans
Selected Object. Leurs entrées de palette utilisent `PaletteCategory=Navigation`.

Voir [Référence des paramètres](11_GRID_WORLD_OBJECT_DEFINITION_PARAMETERS_REFERENCE.md) et
[Relocation](GRID_RELOCATION_DATA.md).
