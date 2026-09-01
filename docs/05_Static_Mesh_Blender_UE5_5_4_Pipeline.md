# GrimrockPrototype — Pipeline Static Mesh Blender → Unreal Engine 5.5.4

Statut : document actif.  
Date : 01.09.2026.

Ce document fixe le pipeline standard du projet **GrimrockPrototype** pour exporter un Static Mesh depuis Blender au format FBX puis l'importer dans Unreal Engine 5.5.4 avec le pipeline Interchange.

Il complète :

- `docs/01_Content_Structure.md` pour le rangement et le nommage ;
- `docs/02_Texture_Pipeline_BC_N_ORM.md` pour les textures ;
- `docs/Design/PIT01_STATIC_INTER_LEVEL_PIT.md` pour le cas concret de `SM_Pit_Stone_01`.

---

## 1. Portée

Ce pipeline concerne les meshes statiques du donjon : sols, murs, plafonds, fosses, portes et parties fixes, boutons, leviers, plaques, décorations et items simples sans armature.

Il ne remplace pas le pipeline Skeletal Mesh / animation.

Convention :

```text
SM_XXX = Static Mesh
```

Pour éviter des diagnostics ambigus, utiliser le même nom dans Blender pour l'objet et ses Mesh Data :

```text
Object Name = SM_XXX
Mesh Data   = SM_XXX
FBX         = SM_XXX.fbx
UE Asset    = SM_XXX
```

Exemple :

```text
Object Name = SM_Pit_Stone_01
Mesh Data   = SM_Pit_Stone_01
FBX         = SM_Pit_Stone_01.fbx
UE Asset    = SM_Pit_Stone_01
```

---

## 2. Préparation dans Blender

Avant export :

1. sélectionner uniquement le mesh à exporter ;
2. vérifier orientation, dimensions et pivot/origine ;
3. appliquer rotation et échelle avec `Ctrl+A > Rotation & Scale`.

État recommandé :

```text
Rotation X = 0°
Rotation Y = 0°
Rotation Z = 0°

Scale X = 1.000
Scale Y = 1.000
Scale Z = 1.000
```

Éviter de corriger une mauvaise échelle au moment de l'import Unreal.

---

## 3. Export FBX depuis Blender

Preset recommandé :

```text
LIMIT TO
[x] Selected Objects

OBJECT TYPES
[ ] Empty
[ ] Camera
[ ] Lamp
[ ] Armature
[x] Mesh
[ ] Other

TRANSFORM
Scale           = 1.00
Apply Scaling   = All Local
Forward         = -Y Forward
Up              = Z Up
[x] Apply Unit
[x] Use Space Transform
[ ] Apply Transform

GEOMETRY
Smoothing       = Face
[ ] Export Subdivision
[x] Apply Modifiers
[ ] Loose Edges
[x] Triangulate Faces
[ ] Tangent Space
```

Les sections Armature et Animation ne sont pas utilisées pour un Static Mesh.

### 3.1 Smoothing Groups

Avec `Smoothing = Normals Only`, Blender peut exporter les normales sans écrire les Smoothing Groups attendus par l'importeur FBX d'Unreal.

UE5 peut alors afficher :

```text
No smoothing group information was found for this mesh ...
Please make sure to enable the 'Export Smoothing Groups' option in the FBX Exporter before exporting the file.
```

Ce warning n'est pas grave si le shading est correct et que les normales sont bien importées.

Le preset du projet utilise néanmoins :

```text
Smoothing = Face
```

afin de fournir les informations attendues par FBX et d'éviter ce warning.

### 3.2 Triangulation

Le projet utilise :

```text
[x] Triangulate Faces
```

afin que la triangulation soit déterminée dans Blender et reste stable entre exports et imports.

---

## 4. Import dans Unreal Engine 5.5.4

### Common

```text
[x] Use Source Name for Asset

Offset Translation    = 0 / 0 / 0
Offset Rotation       = 0 / 0 / 0
Offset Uniform Scale  = 1.0
```

### Common Meshes

```text
Force All Mesh as Type = None
[x] Auto Detect Mesh Type
[ ] Import LODs
[x] Bake Meshes
[ ] Keep Sections Separate

Vertex Color Import Option = Replace
```

### Build — normales et tangentes

Le pipeline standard conserve les normales Blender et laisse UE reconstruire les tangentes :

```text
[ ] Recompute Normals
[x] Recompute Tangents
[x] Use Mikk TSpace
[ ] Compute Weighted Normals

[ ] Use High Precision Tangent Basis
[ ] Use Full Precision UVs
[ ] Remove Degenerates
```

Ne pas activer `Compute Weighted Normals` par défaut : cela modifierait le shading défini dans Blender.

### Static Meshes

```text
[x] Import Static Meshes
[ ] Combine Static Meshes

LOD Group = None
[x] Auto Compute LODScreen Sizes
```

`Import Static Meshes` doit être activé.

### Static Mesh Build

```text
[ ] Build Nanite
[ ] Build Reversed Index Buffer
[ ] Generate Lightmap UVs
[ ] Two-Sided Distance Field Generation

Build Scale = 1.0 / 1.0 / 1.0
```

Nanite n'est pas activé systématiquement pour les petits meshes modulaires du donjon.

`Generate Lightmap UVs` reste désactivé dans le preset actuel. Réévaluer ce point si un workflow de static lighting baked l'exige.

### Skeletal Meshes / Animations

```text
[ ] Import Skeletal Meshes
[ ] Import Animations
```

### Materials / Textures

```text
[ ] Import Materials
[ ] Import Textures
```

Les matériaux et textures sont gérés séparément selon les conventions du projet. L'import FBX ne doit pas créer automatiquement des matériaux ou textures parasites.

---

## 5. Échelle et axes

Le FBX attendu par UE doit être en centimètres et Z-Up.

Exemple observé sur `SM_Pit_Stone_01.fbx` :

```text
File Units          = centimeter
File Axis Direction = Z-UP (RH)
```

Dans ce cas :

```text
Offset Uniform Scale = 1.0
Build Scale          = 1.0 / 1.0 / 1.0
```

Ne pas appliquer un facteur 100 à l'import.

---

## 6. Collision

La collision est un choix par asset.

### 6.1 FBX avec UCX explicites

Si Blender contient des collisions nommées par convention :

```text
UCX_SM_XXX_01
UCX_SM_XXX_02
...
```

utiliser :

```text
[x] Import Collisions
[x] Import Collisions According To Mesh Name
[x] One Convex Hull Per UCX
```

### 6.2 Mesh sans UCX

Ne pas accepter automatiquement une collision de fallback lorsqu'elle détruit le sens gameplay de la géométrie.

Pour une géométrie creuse ou concave importante, créer des UCX explicites ou configurer volontairement la collision après import.

---

## 7. Cas particulier : `SM_Pit_Stone_01`

La fosse occupe une cellule entière du système de grille.

Contrôles attendus :

```text
empreinte X/Y ≈ 200 x 200 cm
pivot cohérent avec les autres sols
centre XY cohérent avec le centre de cellule
surface supérieure alignée avec les sols voisins
Scale UE = 1 / 1 / 1
```

Une collision convexe automatique peut fermer virtuellement le trou.

Pour `SM_Pit_Stone_01` sans UCX explicites :

```text
[ ] Import Collisions
```

Si des UCX ont été conçus autour de l'ouverture :

```text
[x] Import Collisions
[x] Import Collisions According To Mesh Name
[x] One Convex Hull Per UCX
```

La chute du groupe ne dépend pas d'une simulation physique du trou. PIT01 est une mécanique logique de cellule : le groupe entre sur une cellule `Pit`, puis le runtime déclenche la chute et la transition inter-niveaux.

Voir `docs/Design/PIT01_STATIC_INTER_LEVEL_PIT.md`.

---

## 8. Vérifications après import

Avant d'utiliser un nouveau Static Mesh dans un DataAsset ou dans le Grid Editor :

- [ ] ouvrir le Static Mesh Editor ;
- [ ] vérifier dimensions et bounds ;
- [ ] vérifier pivot / origine ;
- [ ] vérifier orientation ;
- [ ] vérifier l'échelle ;
- [ ] vérifier shading et normales ;
- [ ] vérifier les UV ;
- [ ] assigner le matériau voulu ;
- [ ] afficher la collision et vérifier qu'elle correspond au gameplay ;
- [ ] vérifier l'absence de collision qui bouche une ouverture ;
- [ ] tester le mesh dans une cellule du Grid Editor ;
- [ ] vérifier l'alignement avec les meshes voisins.

Pour un élément couvrant une cellule entière, contrôler en particulier les dimensions par rapport au standard de grille 200 x 200 cm.

---

## 9. Résumé du preset

### Blender

```text
Selected Objects = ON
Object Type Mesh = ON
Scale = 1.00
Apply Scaling = All Local
Forward = -Y Forward
Up = Z Up
Apply Unit = ON
Use Space Transform = ON
Apply Transform = OFF
Smoothing = Face
Apply Modifiers = ON
Triangulate Faces = ON
Tangent Space = OFF
```

### Unreal Engine 5.5.4

```text
Import Static Meshes = ON
Offset Uniform Scale = 1.0
Build Scale = 1 / 1 / 1

Recompute Normals = OFF
Recompute Tangents = ON
Use Mikk TSpace = ON
Compute Weighted Normals = OFF

Build Nanite = OFF par défaut
Import Skeletal Meshes = OFF
Import Animations = OFF
Import Materials = OFF
Import Textures = OFF
```

La collision reste un choix conscient par asset et ne doit jamais être laissée au fallback lorsqu'elle peut modifier la géométrie jouable.
