# WORLDOBJ-ITEMCLASS01 — Autorité des acteurs d'item

Statut : contrat courant.

## Décision

`UGridWorldObjectDefinitionAsset` ne définit plus de `ItemActorClass` authorable ou sérialisé.
`AGridReceptacleActor` ne définit plus de `ContainedItemActorClass` authorable ou sérialisé.

Un World Object conserve une seule classe d'acteur configurable :

```text
GridWorldObjectDefinitionAsset
└── RuntimeActorClass
```

Un item est défini par son `UGridItemDefinitionAsset`. Lorsqu'une représentation Actor est nécessaire dans le monde ou dans un réceptacle, le runtime utilise le chemin générique `AGridItemActor`, puis initialise cet acteur depuis l'Item Definition.

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

## Compatibilité native

WORLDOBJ-ITEMCLASS01 retire les deux champs de la réflexion Unreal afin que les anciennes valeurs sérialisées dans les DataAssets ou Blueprints ne soient plus des autorités d'authoring. Des membres C++ non réfléchis, initialisés à `nullptr`, restent temporairement présents comme pont de compilation pour les call-sites historiques ; ils ne peuvent pas être configurés par contenu et conduisent donc au fallback générique `AGridItemActor`.

Le contrat public à retenir est : **Item Definition = identité/comportement/visuel de l'item ; RuntimeActorClass = classe du World Object ; aucun réceptacle ne choisit l'Actor Class de son contenu.**
