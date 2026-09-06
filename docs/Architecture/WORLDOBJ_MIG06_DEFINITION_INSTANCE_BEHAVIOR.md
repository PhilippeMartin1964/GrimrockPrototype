# WORLDOBJ-MIG06-A — Séparer la définition du comportement d'instance

## Statut

Première tranche structurelle de WORLDOBJ-MIG06.

MIG06-A introduit le stockage sparse pour les nouveaux objets placés et conserve un pont de lecture strict pour les assets historiques. La suppression des derniers consommateurs directs est volontairement terminée dans MIG06-B avant de déclarer MIG06 entièrement clos.

## Problème traité

Avant MIG06, sélectionner un archétype dans la palette revenait à faire conceptuellement :

```text
PlacedObject.Behavior = Definition.DefaultBehavior
```

Chaque objet du niveau conservait donc une copie complète du comportement partagé. Après placement, modifier la définition ne modifiait plus l'objet déjà placé : deux sources de vérité existaient.

## Contrat cible

```text
UGridObjectArchetypeAsset
└── DefaultBehavior
    └── comportement partagé / réutilisable

FGridLevelObjectData
└── Behavior
    └── uniquement les données réellement propres à cette instance
```

La résolution effective devient :

```text
Definition.DefaultBehavior
        +
Instance sparse overrides
        =
Effective Behavior
```

## Compatibilité des niveaux existants

La migration des `.uasset` réels n'a lieu qu'en MIG08. Il faut donc distinguer sans ambiguïté :

- un ancien objet dont `Behavior` est encore un snapshot complet ;
- un nouvel objet dont `Behavior` contient des overrides sparse.

`UGridLevelAsset` possède pour cela :

```text
SparseBehaviorOverrideObjectIds
```

Si l'`ObjectId` est absent, l'objet est pré-MIG06 et son `Behavior` historique reste autoritaire tel quel.

Si l'`ObjectId` est présent, `GridObjectInstanceBehavior::Resolve()` part de `Archetype.DefaultBehavior` puis applique uniquement les données d'instance.

Cette solution évite de modifier prématurément la structure monolithique `FGridLevelObjectData`, dont la découpe typée appartient à MIG07.

## Données réellement propres à l'instance

MIG06-A considère déjà comme locales :

- destination de téléporteur ;
- destination/configuration de transition ;
- état initial/configuration locale de fosse ;
- contenu initial d'un réceptacle ;
- état initial verrouillé/déverrouillé d'une serrure.

Les champs déjà séparés dans `FGridLevelObjectData` restent naturellement locaux :

- position/cellule/bord ;
- état enabled/active initial ;
- `Tag` ;
- `LogicId` ;
- overrides de texte lisible ;
- définitions directes d'item/monstre/compagnon.

## Données appartenant à la définition

La résolution sparse reprend depuis l'archétype les règles partagées, notamment :

- paramètres génériques de bouton ;
- règles de serrure autres que l'état initial ;
- capacité et règles génériques de réceptacle ;
- règles de plaque de pression ;
- comportement générique de porte ;
- paramètres de présentation, mesh, Motion, audio, classe runtime et placement déjà portés par l'archétype.

Le premier consommateur runtime migré explicitement dans cette tranche est le bouton : `ButtonHoldTime` est désormais résolu depuis la définition pour un objet sparse.

## Grid Editor

### Nouveau placement

`PlaceSelectedObject()` ne copie plus `ObjectBehavior` intégralement.

Il appelle :

```text
GridObjectInstanceBehavior::BuildSparseOverrides(...)
```

puis marque le nouvel `ObjectId` dans `SparseBehaviorOverrideObjectIds`.

### Sélection

`SelectObjectAtSelection()` et `SelectObjectById()` reconstruisent le comportement effectif avec :

```text
GridObjectInstanceBehavior::Resolve(LevelAsset, Object, Archetype)
```

L'inspecteur continue donc de voir une configuration complète compréhensible, sans que le LevelAsset doive en stocker une copie complète.

Les items MIG05, qui n'ont plus d'ObjectArchetype, reconstruisent leur état temporaire d'édition depuis leurs références directes `ItemDefinitionAsset` et `ReadableContent`.

### Modification

`ApplyBehaviorToSelectedObject()` convertit automatiquement l'objet sélectionné vers le contrat sparse.

`ResetSelectedObjectBehaviorFromArchetype()` signifie désormais réellement « reprendre la définition », et non plus « recopier la définition dans l'instance ».

## Ponts MIG06-A encore présents

Trois familles restent volontairement recopiées temporairement dans le conteneur sparse parce que du code historique les lit encore directement sans passer par le resolver :

1. `PressurePlateWeight` — lu directement par `UGridActivationComponent` ;
2. `DoorAnimation` — la partie chaîne de porte possède encore un chemin direct ;
3. paramètres génériques de `Receptacle` — encore lus directement par certaines validations/parties d'éditeur.

Ces copies sont des **ponts de migration**, pas le contrat cible. Le resolver les ignore lorsqu'il construit le comportement effectif : la définition reste conceptuellement autoritaire.

MIG06-B doit router ces consommateurs par la résolution Definition+Instance puis retirer ces trois copies temporaires de `BuildSparseOverrides()`.

## Tests

Famille :

```text
Grimrock.WorldObjects.MIG06
```

Contrats couverts :

- les valeurs partagées viennent de la définition pour une instance sparse ;
- les données d'instance remplacent correctement les valeurs correspondantes ;
- un objet pré-MIG06 non marqué conserve exactement son snapshot historique ;
- un nouvel objet placé par le Grid Editor est marqué sparse ;
- sélectionner un objet sparse reconstruit les valeurs de définition ;
- modifier un comportement ne réintroduit pas un champ partagé comme override ;
- les trois ponts MIG06-A restants sont testés explicitement afin qu'ils ne puissent pas être confondus avec le contrat final.

## Suite

WORLDOBJ-MIG06-B :

- router `UGridActivationComponent` vers la résolution effective pour les plaques de pression ;
- router complètement la chaîne de porte vers la définition ;
- router validation/inspecteur des réceptacles vers le comportement effectif ;
- supprimer les trois ponts de copie correspondants ;
- ajouter les garde-fous permettant de déclarer MIG06 clos avant MIG07.
