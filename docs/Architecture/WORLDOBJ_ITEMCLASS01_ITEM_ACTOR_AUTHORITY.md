# WORLDOBJ-ITEMCLASS01 — Autorité des acteurs d'item

Statut : contrat courant.

## Décision

`UGridWorldObjectDefinitionAsset` ne définit plus de `ItemActorClass`.
`AGridReceptacleActor` ne définit plus de `ContainedItemActorClass`.
`AGridLevelRuntimeActor::SpawnItemActorForDefinition()` ne reçoit plus de classe d'acteur d'item préférée.

Un World Object conserve une seule classe d'acteur configurable :

```text
GridWorldObjectDefinitionAsset
└── RuntimeActorClass
```

Un item est défini par son `UGridItemDefinitionAsset`. Lorsqu'une représentation Actor est nécessaire dans le monde ou dans un réceptacle, le runtime instancie directement l'`AGridItemActor` générique, puis initialise cet acteur depuis l'Item Definition.

```text
Receptacle
└── ItemDefinition
      ↓
AGridItemActor
      ↓
InitializeFromItemDefinition(ItemDefinition)
```

## Réceptacles

Les règles et contenus restent data-driven :

- `AcceptedItems[].ItemDefinition` décrit ce que le réceptacle accepte ;
- `InitialContent[].ItemDefinition` décrit ce qu'il contient au départ ;
- `Quantity` décrit la quantité ;
- `VisualPlacementMode` décrit la présentation dans le réceptacle.

Le réceptacle ne choisit pas la classe Unreal de l'item qu'il contient.

## Suppression physique

`ItemActorClass`, `ContainedItemActorClass` et le paramètre `PreferredItemActorClass` sont supprimés du code actif. Aucun membre C++ caché, fallback de classe, pont legacy ou mécanisme de compatibilité n'est conservé.

Le contrat est : **Item Definition = identité/comportement/visuel de l'item ; RuntimeActorClass = classe du World Object ; les représentations d'items utilisent l'AGridItemActor générique.**
