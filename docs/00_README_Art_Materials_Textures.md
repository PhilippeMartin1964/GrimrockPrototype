# GrimrockPrototype — Documentation Art, Meshes, Textures et Materials

Statut : document actif.

Ce document est le README spécialisé du pipeline Art / Meshes / Materials / Textures. Il ne couvre pas le design gameplay/editor, documenté dans `docs/Design`.

Ce dossier regroupe la documentation issue du travail de restructuration du pipeline **Art / Meshes / Textures / Materials** de GrimrockPrototype.

L’objectif est de garder un projet Unreal Engine 5.5 lisible, maintenable et raisonnable en taille Git, tout en conservant une base de géométrie et de matériaux robuste pour un dungeon crawler à déplacement case par case.

---

## Ordre de lecture recommandé

1. [`01_Content_Structure.md`](./01_Content_Structure.md)  
   Décrit l’arborescence recommandée dans `Content/GrimrockPrototype`, les conventions de rangement et les règles de nommage.

2. [`05_Static_Mesh_Blender_UE5_5_4_Pipeline.md`](./05_Static_Mesh_Blender_UE5_5_4_Pipeline.md)  
   Décrit le pipeline Static Mesh : préparation Blender, export FBX, axes, échelle, smoothing groups, normales/tangentes, import Interchange UE5.5.4, collisions UCX et vérifications après import.

3. [`02_Texture_Pipeline_BC_N_ORM.md`](./02_Texture_Pipeline_BC_N_ORM.md)  
   Décrit le pipeline texture final : `BC`, `N`, `ORM`, `RGBA`, export GIMP / Photoshop, réduction 4K vers 2K/1K, génération d’ORM et réglages UE5.

4. [`M_GrimrockSurface_Master.md`](./M_GrimrockSurface_Master.md)  
   Documentation technique du master material opaque. Cette partie doit rester la référence détaillée pour les surfaces opaques.

5. [`03_M_GrimrockSurface_Masked_Master.md`](./03_M_GrimrockSurface_Masked_Master.md)  
   Documentation technique du master material masked, avec détail du branchement `UseAlphaFromBaseColor`, `OpacityMaskTexture` et `OpacityMaskClip`.

6. [`04_Material_Instances_Migration.md`](./04_Material_Instances_Migration.md)  
   Guide de migration des Material Instances vers les nouveaux masters, checklist de validation et points d’audit.

---

## Décisions principales

### Static Meshes

Référence Blender → FBX → Unreal Engine 5.5.4 :

```text
docs/05_Static_Mesh_Blender_UE5_5_4_Pipeline.md
```

Principes :

```text
échelle 1:1 en centimètres
Z-Up
FBX Blender : -Y Forward / Z Up
transformations appliquées dans Blender
Smoothing = Face
triangulation fixée dans Blender
normales Blender conservées
tangentes MikkTSpace recalculées par UE
materials/textures non importés depuis le FBX
collision décidée explicitement selon le mesh
```

### Masters conservés / créés

```text
M_GrimrockSurface_Master          // surfaces opaques PBR
M_GrimrockSurface_Masked_Master   // surfaces découpées par alpha
M_FloorRuneCircle_Additive        // rune / effet magique spécifique
M_Ceiling_Editor                  // matériau éditeur
M_EditorGrid                      // grille éditeur
M_Object_Hover                    // feedback éditeur
M_PP_EditorOutline                // post-process outline éditeur
```

### Standard texture final

```text
T_xxx_BC    = BaseColor, 8 bpc RGB, sRGB=true
T_xxx_N     = Normal, 8 bpc RGB, sRGB=false, Compression=Normalmap
T_xxx_ORM   = R=AO, G=Roughness, B=Metallic, 8 bpc RGB, sRGB=false, Compression=Masks
T_xxx_RGBA  = couleur + alpha, 8 bpc RGBA, uniquement si alpha réellement utilisé
```

### Sources lourdes

Les sources 4K téléchargées, fichiers de travail GIMP/Photoshop, displacement, height, exports intermédiaires, etc., doivent rester hors `Content/` et hors Git, dans un dossier externe local comme :

```text
_ExternalArtSource/
```

### Règle importante découverte pendant le debug

Ne pas exporter les textures finales en `16 bpc RGBA` par erreur.

Pour les textures finales UE5 :

```text
BC  -> 8 bpc RGB
N   -> 8 bpc RGB
ORM -> 8 bpc RGB
RGBA -> 8 bpc RGBA seulement si l’alpha est réellement utilisé
```

Cette règle évite les écarts de luminosité observés entre une texture 4096 originale et une version 2048 exportée depuis GIMP.

---

## État cible

La structure actuelle vise à séparer clairement :

```text
Art/Materials/        // masters et material instances
Art/Textures/Final/   // textures finales importées UE5
Art/Textures/Generated/ // textures générées spécifiques
Meshes/               // géométrie gameplay / structurelle, laissée hors Art volontairement
Core/                 // DataAssets, Input, données gameplay
Blueprints/           // Blueprints runtime/editor/UI
Maps/                 // niveaux
```

Cette séparation doit permettre de continuer à enrichir le donjon sans transformer le dépôt GitHub en bibliothèque de sources 4K.
