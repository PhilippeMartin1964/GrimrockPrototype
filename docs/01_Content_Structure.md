# GrimrockPrototype — Structure finale Content / Art / Textures

Ce document décrit l’arborescence cible retenue pour le contenu Unreal Engine du projet **GrimrockPrototype**.

Le choix principal est de séparer les éléments de rendu (`Art`) des éléments de géométrie (`Meshes`) et des éléments gameplay (`Core`, `Blueprints`, `Maps`).

---

## 1. Racine `Content/GrimrockPrototype`

Structure cible :

```text
Content/
└── GrimrockPrototype/
    ├── Art/
    │   ├── Materials/
    │   ├── Textures/
    │   └── Icons/
    │
    ├── Meshes/
    ├── Blueprints/
    ├── Core/
    └── Maps/
```

Le dossier `Meshes` est volontairement laissé en dehors de `Art`. Ce choix est cohérent pour GrimrockPrototype, car les meshes représentent souvent de la géométrie structurelle ou gameplay : murs, sols, portes, objets interactifs, previews éditeur, etc.

---

## 2. `Art/Materials`

Structure recommandée :

```text
Art/
└── Materials/
    ├── Masters/
    │   ├── M_GrimrockSurface_Master.uasset
    │   ├── M_GrimrockSurface_Masked_Master.uasset
    │   ├── M_FloorRuneCircle_Additive.uasset
    │   ├── M_Ceiling_Editor.uasset
    │   ├── M_EditorGrid.uasset
    │   ├── M_Object_Hover.uasset
    │   └── M_PP_EditorOutline.uasset
    │
    └── Instances/
        ├── Assets/
        ├── Ceiling/
        ├── Decorations/
        ├── Door/
        ├── Floor/
        └── Wall/
```

### Remarque sur singulier / pluriel

Le projet utilise encore certains dossiers au singulier (`Door`, `Wall`, `Floor`) et certains au pluriel (`Decorations`). Ce n’est pas bloquant. L’important est d’éviter les doublons de type :

```text
Decoration/     // ancien
Decorations/    // nouveau
```

Une fois une convention choisie, ne pas recréer l’ancienne.

---

## 3. `Art/Textures/Final`

Toutes les textures finales importées dans UE5 doivent être rangées ici.

```text
Art/
└── Textures/
    └── Final/
        ├── Assets/
        ├── Ceiling/
        ├── Decorations/
        │   └── Floor/
        │       ├── Blood_01/
        │       ├── Carpet_01/
        │       ├── Moss_01/
        │       └── Roots_01/
        ├── Door/
        ├── Floor/
        ├── Metalness/
        └── Wall/
```

Le dossier `Final` ne doit contenir que les textures prêtes pour UE5 :

```text
8 bpc RGB pour BC/N/ORM
8 bpc RGBA uniquement pour les textures à alpha utile
résolution finale 1024 ou 2048 selon usage
noms normalisés T_xxx_BC / T_xxx_N / T_xxx_ORM / T_xxx_RGBA
```

---

## 4. Nommage recommandé

### Préfixes Unreal

```text
M_     = Material master
MI_    = Material instance
T_     = Texture
SM_    = Static mesh
BP_    = Blueprint
DA_    = DataAsset
WBP_   = Widget Blueprint
IA_    = Input Action
IMC_   = Input Mapping Context
```

### Textures

```text
T_Category_Name_Variant_Channel
```

Exemples :

```text
T_Wall_Stone_01_BC
T_Wall_Stone_01_N
T_Wall_Stone_01_ORM

T_Floor_Stone_01_BC
T_Floor_Stone_01_N
T_Floor_Stone_01_ORM

T_Deco_FloorMoss_01_RGBA
T_Deco_FloorCarpet_01_RGBA
T_Deco_FloorBloodStain_01_RGBA
```

---

## 5. Règles de rangement

### À faire

```text
Material Masters        -> Art/Materials/Masters
Material Instances      -> Art/Materials/Instances/<Catégorie>
Textures finales        -> Art/Textures/Final/<Catégorie>
Textures générées       -> Art/Textures/Generated/<Catégorie>
Icônes outil/editor     -> Art/Icons ou Art/Textures/Generated/Icons
Meshes                  -> Meshes/<Catégorie>
DataAssets              -> Core/DataAssets/<Catégorie>
```

### À éviter

```text
textures placées dans Art/Materials/Instances
material instances placées dans Art/Textures
sources 4K dans Content/
Height / Displacement importées sans usage réel
anciens dossiers typo ou doublons : Moos_01, Decoration, Ceil si déjà remplacés
```

---

## 6. Dossier externe local

Les sources lourdes doivent rester hors Unreal :

```text
_ExternalArtSource/
├── Textures/
│   ├── Wall/
│   ├── Floor/
│   ├── Ceiling/
│   ├── Door/
│   ├── Assets/
│   └── Decorations/
├── Blender/
├── Exports/
└── References/
```

Ce dossier sert à conserver :

```text
textures 4K originales
height / displacement
fichiers Photoshop / GIMP
exports intermédiaires
fichiers Blender sources
références et licences
```

Il ne doit pas être nécessaire pour ouvrir le projet Unreal, mais permet de régénérer les textures finales si besoin.

---

## 7. Migration / renommage d’assets

Tout déplacement ou renommage d’asset Unreal doit être fait dans le **Content Browser UE5**, jamais directement dans l’explorateur Windows.

Procédure :

```text
1. Renommer / déplacer dans le Content Browser.
2. Save All.
3. Clic droit sur Content/GrimrockPrototype.
4. Fix Up Redirectors in Folder.
5. Save All.
6. Fermer / rouvrir l’éditeur si nécessaire.
7. Vérifier avec Reference Viewer.
8. Commit Git.
```

---

## 8. Points déjà corrigés pendant la restructuration

Les corrections de structure validées pendant le travail :

```text
Moos_01      -> Moss_01
Decoration   -> Decorations
Ceil         -> Ceiling
T_Meta_02_*  -> T_Metal_02_*
textures décoratives déplacées hors Materials/Instances
```

Ces corrections réduisent les ambiguïtés et rendent l’arborescence plus stable pour les prochains ajouts.
