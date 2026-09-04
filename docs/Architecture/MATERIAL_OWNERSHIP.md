# MATERIAL-OWNERSHIP01 — Propriété des matériaux des objets

Date : 2026-09-04  
Statut : **architecture active**

## Décision

Pour les objets construits à partir d'un `GridObjectArchetypeAsset`, le matériau visuel appartient au `StaticMesh` référencé et à ses **Material Slots**.

```text
GridObjectArchetypeAsset
    ├── PreviewMesh
    ├── FixedMesh
    ├── MovingMesh
    ├── PitLeftLeafMesh
    └── PitRightLeafMesh

StaticMesh
    └── Material Slots   ← source de vérité du matériau de rendu
```

Le `GridObjectArchetypeAsset` ne doit plus fournir d'override de matériau pour ces meshes.

## Migration effectuée

L'audit `Grimrock.Architecture.MaterialOwnership.Audit` a trouvé huit archétypes utilisant encore `PreviewMaterial`.

Cinq étaient déjà redondants avec le Material Slot de leur mesh :

- `DA_FloorRuneCircle` ;
- `DA_FloorRubble` ;
- `DA_FloorMoss` ;
- `DA_FloorDebris` ;
- `DA_FloorCarpet`.

Trois partageaient `SM_FloorDecalPlane_01` avec des matériaux différents. Ils ont reçu un mesh dédié afin que le matériau reste porté par le mesh :

- `DA_FloorRoots` → `SM_FloorRoots_01` ;
- `DA_FloorDust` → `SM_FloorDust_01` ;
- `DA_FloorBloodStain` → `SM_FloorBloodStain_01`.

Après migration, l'audit des DataAssets ne trouvait plus aucun override matériel renseigné.

## Champs retirés de l'authoring

Les anciens noms suivants ne font plus partie du schéma réfléchi du `GridObjectArchetypeAsset` :

```text
PreviewMaterial
FixedMaterial
MovingMaterial
PitLeftLeafMaterial
PitRightLeafMaterial
```

Des shims C++ non réfléchis, non sérialisés et toujours nuls sont conservés temporairement pour permettre le retrait progressif des anciens call-sites sans réintroduire une seconde source de vérité. Ils ne peuvent pas être configurés dans un DataAsset et ne peuvent pas fournir de matériau runtime.

Les acteurs runtime génériques et les mécanismes n'appliquent plus le paramètre d'override matériel reçu par leurs anciennes signatures ; le `StaticMeshComponent` conserve donc les Material Slots de son `StaticMesh`.

## Ce qui n'est pas concerné

Cette règle concerne le **matériau de rendu principal du mesh d'un objet**. Elle ne supprime pas les matériaux utilisés pour une présentation ou un effet distinct, par exemple :

- `WorldSparkleMaterial` d'un item : effet VFX de scintillement superposé ;
- matériaux de sélection/preview éditeur ;
- matériaux dynamiques créés pour un effet temporaire explicite ;
- matériaux propres à une UI.

Ces cas ne remplacent pas la source de vérité du matériau principal du `StaticMesh`.

## Validation

Le filtre :

```text
Grimrock.Architecture.MaterialOwnership.Audit
```

vérifie désormais que les cinq anciens noms ne sont plus des propriétés réfléchies du `GridObjectArchetypeAsset` et que les shims de compatibilité restent nuls.

## Nettoyage final

Les cinq anciens overrides sont supprimés de l'API C++. Les initialisations runtime et preview transportent uniquement les meshes; leurs Material Slots fournissent l'apparence normale.
