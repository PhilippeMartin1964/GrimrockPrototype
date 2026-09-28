# MON21.6.6 — Map Read Model

Date : **28 septembre 2026**  
Statut : **VALIDÉ**

## 1. Objectif

MON21.6.6 introduit la projection C++ filtrée qui séparera définitivement les autorités gameplay de la future présentation `WBP_GridMap`.

Ce ticket reste limité à **une dalle locale 32×32**. La composition multi-dalles / étages appartient à MON21.6.7.

## 2. Modèle exposé

Le read model produit :

```text
FGridMapTileView
    LevelId
    Cells[]
        LocalCell
        CellType
    Boundaries[]
        LocalCell
        Edge
        Kind = Wall | Door | SecretDoor
        bDoorOpen
```

Il ne transporte ni `ObjectId`, ni `WorldObjectDefinitionId`, ni état secret caché.

Le read model est transitoire : aucun de ses champs ne porte `SaveGame`.

## 3. Filtrage de connaissance

Une cellule n’est ajoutée à `Cells[]` que si :

- elle est marquée `Explored` dans `FGridMapExplorationState` ;
- la cellule authored n’est pas `Empty`.

Une cellule inconnue n’est jamais copiée dans la vue.

Une frontière commune peut néanmoins être projetée depuis le côté exploré, conformément au contrat MON21.6.1. Le builder peut donc consulter les deux côtés de **cette frontière uniquement** pour déterminer si elle est solide, sans exporter la cellule inconnue.

## 4. Portes

La variante de porte est résolue depuis `UGridWorldObjectDefinitionAsset::RuntimeActorClass` :

- dérivée de `AGridDoorActor` -> `Door` ;
- dérivée de `AGridSecretDoorActor` -> secret ;
- définition absente/incohérente -> **fail closed** en `Wall`.

Aucun `DefinitionId == Door_Secret` n’est introduit dans le read model.

État ouvert/fermé :

1. `UGridDoorSystemComponent` vivant si fourni pour la dalle active ;
2. sinon `FGridLevelRuntimeState::Doors` ;
3. sinon état initial authored de l’instance.

`bDoorOpen` signifie ici passage runtime non bloqué.

## 5. Secret strictement caché

Avant découverte :

```text
secret authored
    -> FGridMapBoundaryView
       Kind = Wall
       bDoorOpen = false
       aucune identité authored/persistante exposée
```

Après `MapExploration.IsSecretDiscovered(ObjectId)` :

```text
Kind = SecretDoor
```

La connaissance reste indépendante de la fermeture ultérieure.

## 6. Déduplication

Une frontière physique partagée par deux cellules explorées n’est émise qu’une fois. La clé de déduplication est dérivée de la ligne de grille physique, pas d’un ObjectId.

L’ordre de construction est déterministe : Y, X puis North/East/South/West.

## 7. Validation MON21.6.5 actée

Validation utilisateur du 28 septembre 2026 :

```text
Filter                  : Grimrock.Map.MON21_6_5
Succeeded               : 4
Succeeded with warnings : 0
Failed                  : 0
Not run                 : 0
Process exit code       : 0
Report                  : TD04-20260928-090055
```

MON21.6.5 est **VALIDÉ**.

## 8. Automation MON21.6.6

Filtre : `Grimrock.Map.MON21_6_6`

```text
ReadModel.FiltersUnknownGeometry
ReadModel.DoorStatePrefersLiveRuntime
ReadModel.SecretMetadataIsFiltered
ReadModel.TransientContract
```

Les tests couvrent notamment la frontière connue depuis un côté exploré, l’absence de cellule inconnue dans la vue, le secret indistinguable d’un mur, l’absence de métadonnée secrète et la priorité de l’état de porte vivant.

Validation locale utilisateur du 28 septembre 2026 :

```text
Filter                  : Grimrock.Map.MON21_6_6
Succeeded               : 4
Succeeded with warnings : 0
Failed                  : 0
Not run                 : 0
Process exit code       : 0
Report                  : TD04-20260928-091315
```

MON21.6.6 est **VALIDÉ**.

## 9. Hors périmètre

- coordonnées globales multi-dalles : MON21.6.7 ;
- marqueur groupe par étage : MON21.6.7/6.9 ;
- widget natif et rendu : MON21.6.8 ;
- zoom/pan/recentrage : MON21.6.10 ;
- symboles supplémentaires : MON21.6.12.

Tranche suivante : **MON21.6.7 — Multi-Tile / Floor Projection**.
