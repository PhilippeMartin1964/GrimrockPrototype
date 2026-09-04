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

## Champs retirés de l'authoring et de l'API

Les anciens noms suivants ne font plus partie du schéma réfléchi ni de l'API C++ du `GridObjectArchetypeAsset` :

```text
PreviewMaterial
FixedMaterial
MovingMaterial
PitLeftLeafMaterial
PitRightLeafMaterial
```

Aucun shim C++ de compatibilité matériau n'est conservé. Les anciens noms ont disparu de la réflexion Unreal et de l'API C++ ; les call-sites utilisent directement les signatures mesh-only actuelles.

Les acteurs runtime génériques, les mécanismes et la preview ne transportent plus de paramètre de matériau d'archétype ; ils assignent le `StaticMesh` et conservent donc ses Material Slots.

## Ce qui n'est pas concerné

Cette règle concerne le **matériau de rendu principal du mesh d'un objet**. Elle ne supprime pas les matériaux utilisés pour une présentation ou un effet distinct, par exemple :

- `WorldSparkleMaterial` d'un item : effet VFX de scintillement superposé ;
- `ChainMaterial` du mécanisme de chaîne optionnel d'une porte ;
- matériaux de sélection/preview éditeur ;
- matériaux dynamiques créés pour un effet temporaire explicite ;
- matériaux propres à une UI.

Ces cas ne remplacent pas la source de vérité du matériau principal du `StaticMesh`.

## Validation

Le filtre :

```text
Grimrock.Architecture.MaterialOwnership.Audit
```

vérifie que les cinq anciens noms ne sont plus des propriétés réfléchies du `GridObjectArchetypeAsset`.

## État final

Les cinq anciens overrides sont supprimés du schéma Unreal et de l'API C++. Aucun shim de propriété ou de signature matériau ne subsiste. Les initialisations runtime, les mécanismes et la preview transportent uniquement les meshes ; leurs Material Slots fournissent l'apparence normale.
