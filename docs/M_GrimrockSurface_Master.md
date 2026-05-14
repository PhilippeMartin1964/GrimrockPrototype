# M_GrimrockSurface_Master

Documentation technique du Master Material générique principal du projet **GrimrockPrototype**.

Ce matériau maître sert de base commune pour les surfaces opaques classiques du jeu : murs, sols, plafonds, portes, objets interactifs, bois, pierre, métal, os, etc.

Il repose sur le standard suivant :

```text
BaseColor + Normal + ORM
```

Avec la convention ORM suivante :

```text
ORM.R = Ambient Occlusion
ORM.G = Roughness
ORM.B = Metallic
```

---

## 1. Emplacement recommandé

```text
Content/GrimrockPrototype/Art/Materials/Masters/M_GrimrockSurface_Master
```

Les instances dérivées devraient être rangées dans :

```text
Content/GrimrockPrototype/Art/Materials/Instances/
```

Exemples :

```text
Instances/Wall/MI_Wall_Stone_01
Instances/Floor/MI_Floor_Stone_01
Instances/Ceiling/MI_Ceil_Stone_01
Instances/Doors/MI_Door_Wood_01
Instances/Interactables/MI_Button_Metal_01
Instances/Items/MI_Torch_Wood_01
```

---

## 2. Objectif

`M_GrimrockSurface_Master` doit remplacer progressivement les anciens matériaux maîtres spécialisés, par exemple :

```text
M_Master_Floor
M_Master_Wood
M_Master_Metalic
```

Le but est de centraliser la logique commune :

- texture de couleur ;
- normal map ;
- texture ORM ;
- fallback sans ORM ;
- réglage de roughness ;
- réglage de metallic ;
- réglage de l’intensité de normal ;
- teinte ;
- désaturation ;
- tiling U/V séparé ;
- offset U/V séparé ;
- emissive optionnel ;
- pulse emissive optionnel.

Ce master ne gère pas :

- l’Opacity Mask ;
- la transparence ;
- le Height ;
- le Displacement ;
- le Parallax ;
- le BumpOffset.

Ces cas doivent être traités par un autre master, par exemple :

```text
M_GrimrockSurface_Masked_Master
M_GrimrockFloorDecoration_Master
M_FloorRuneCircle_Additive
```

---

## 3. Réglages généraux du matériau

Dans les propriétés de `M_GrimrockSurface_Master` :

```text
Material Domain         = Surface
Blend Mode              = Opaque
Shading Model           = Default Lit
Two Sided               = false
Use Material Attributes = false
```

Aucun branchement ne doit être fait dans :

```text
Opacity
Opacity Mask
World Position Offset
Pixel Depth Offset
```

---

## 4. Paramètres du matériau

Les paramètres doivent être rangés par groupe pour garder les Material Instances lisibles.

---

### 4.1 Groupe `01_Textures`

| Paramètre | Type | Défaut | Description |
|---|---|---:|---|
| `BaseColorTexture` | TextureSampleParameter2D | texture blanche/neutre | Texture de couleur principale. |
| `NormalTexture` | TextureSampleParameter2D | normal map plate | Texture de normal map. |
| `ORMTexture` | TextureSampleParameter2D | ORM neutre | Texture packée AO/Roughness/Metallic. |
| `EmissiveTexture` | TextureSampleParameter2D | noir ou blanc | Texture optionnelle pour l’émission. |

---

### 4.2 Groupe `02_UV`

| Paramètre | Type | Défaut | Description |
|---|---|---:|---|
| `UV_Tiling_U` | ScalarParameter | `1.0` | Répétition horizontale de la texture. |
| `UV_Tiling_V` | ScalarParameter | `1.0` | Répétition verticale de la texture. |
| `UV_Offset_U` | ScalarParameter | `0.0` | Décalage horizontal des UV. |
| `UV_Offset_V` | ScalarParameter | `0.0` | Décalage vertical des UV. |

La séparation U/V est volontaire : elle permet d’ajuster précisément les textures de boutons, de portes ou d’objets dont les UV ne sont pas parfaitement centrés.

---

### 4.3 Groupe `03_BaseColor`

| Paramètre | Type | Défaut | Description |
|---|---|---:|---|
| `BaseColorTint` | VectorParameter | blanc | Teinte multiplicative appliquée à la couleur. |
| `BaseColorStrength` | ScalarParameter | `1.0` | Intensité globale de la couleur. |
| `DesaturationAmount` | ScalarParameter | `0.0` | Désaturation de la texture de couleur. |

Valeurs utiles :

```text
BaseColorStrength  = 0.7 à 1.0
DesaturationAmount = 0.0 à 0.35
```

---

### 4.4 Groupe `04_Normal`

| Paramètre | Type | Défaut | Description |
|---|---|---:|---|
| `UseNormalTexture` | StaticSwitchParameter | `true` | Active ou désactive l’utilisation de la normal map. |
| `NormalStrength` | ScalarParameter | `1.0` | Intensité de la normal map. |

Convention souhaitée :

```text
NormalStrength = 1.0 -> normal map complète
NormalStrength = 0.0 -> surface plate
```

---

### 4.5 Groupe `05_ORM`

| Paramètre | Type | Défaut | Description |
|---|---|---:|---|
| `UseORMTexture` | StaticSwitchParameter | `true` | Utilise ou ignore la texture ORM. |
| `AO_Strength` | ScalarParameter | `1.0` | Intensité de l’occlusion ambiante. |
| `RoughnessMultiplier` | ScalarParameter | `1.0` | Multiplicateur de roughness depuis ORM.G. |
| `RoughnessOverride` | ScalarParameter | `0.65` | Valeur fixe de roughness. |
| `UseRoughnessOverride` | StaticSwitchParameter | `false` | Remplace ORM.G par `RoughnessOverride`. |
| `MetallicMultiplier` | ScalarParameter | `1.0` | Multiplicateur de metallic depuis ORM.B. |
| `MetallicOverride` | ScalarParameter | `0.0` | Valeur fixe de metallic. |
| `UseMetallicOverride` | StaticSwitchParameter | `false` | Remplace ORM.B par `MetallicOverride`. |

Valeurs typiques :

```text
Pierre      Metallic = 0.0 / Roughness = 0.75 à 0.95
Bois        Metallic = 0.0 / Roughness = 0.55 à 0.85
Métal usé   Metallic = 1.0 / Roughness = 0.35 à 0.65
Métal poli  Metallic = 1.0 / Roughness = 0.15 à 0.30
```

---

### 4.6 Groupe `06_Emissive`

| Paramètre | Type | Défaut | Description |
|---|---|---:|---|
| `UseEmissive` | StaticSwitchParameter | `false` | Active ou désactive l’émission. |
| `UseEmissiveTexture` | StaticSwitchParameter | `false` | Utilise `EmissiveTexture` au lieu de la base color comme source emissive. |
| `EmissiveColor` | VectorParameter | blanc | Couleur de l’émission. |
| `EmissiveStrength` | ScalarParameter | `0.0` | Intensité de l’émission. |
| `EmissivePulseStrength` | ScalarParameter | `0.0` | Amplitude du pulse emissive. |
| `EmissivePulseSpeed` | ScalarParameter | `0.5` | Vitesse du pulse emissive. |

Réglages subtils recommandés :

```text
Rune magique discrète :
UseEmissive           = true
UseEmissiveTexture    = true
EmissiveStrength      = 0.5 à 1.5
EmissivePulseStrength = 0.05 à 0.15
EmissivePulseSpeed    = 0.25 à 0.5

Bouton actif discret :
UseEmissive           = true
UseEmissiveTexture    = false
EmissiveStrength      = 0.2 à 0.6
EmissivePulseStrength = 0.0
```

---

## 5. Construction du graphe

---

### 5.1 Bloc UV

Créer un UV final commun à toutes les textures :

```text
UV_Final = TextureCoordinate * Append(UV_Tiling_U, UV_Tiling_V)
UV_Final = UV_Final + Append(UV_Offset_U, UV_Offset_V)
```

Nodes Unreal :

```text
TextureCoordinate
ScalarParameter UV_Tiling_U
ScalarParameter UV_Tiling_V
AppendVector
Multiply

ScalarParameter UV_Offset_U
ScalarParameter UV_Offset_V
AppendVector
Add
```

La sortie du `Add` devient `UV_Final`.

Brancher `UV_Final` dans les entrées UV de :

```text
BaseColorTexture
NormalTexture
ORMTexture
EmissiveTexture
```

---

### 5.2 Bloc BaseColor

Formule :

```text
BaseColor_Final = Desaturate(BaseColorTexture, DesaturationAmount)
BaseColor_Final = BaseColor_Final * BaseColorTint
BaseColor_Final = BaseColor_Final * BaseColorStrength
```

Nodes :

```text
TextureSampleParameter2D BaseColorTexture
Desaturation
Multiply BaseColorTint
Multiply BaseColorStrength
```

Branchement :

```text
BaseColor_Final -> Base Color
```

---

### 5.3 Bloc Normal

Formule souhaitée :

```text
NormalStrength = 1.0 -> normal complète
NormalStrength = 0.0 -> normal plate
```

Implémentation recommandée :

```text
NormalTexture -> FlattenNormal
Flatness = 1.0 - NormalStrength
```

Puis :

```text
UseNormalTexture == true  -> FlattenNormal result
UseNormalTexture == false -> Constant3Vector(0, 0, 1)
```

Branchement :

```text
Normal_Final -> Normal
```

---

### 5.4 Bloc ORM

Convention :

```text
ORM.R = Ambient Occlusion
ORM.G = Roughness
ORM.B = Metallic
```

---

#### Ambient Occlusion

Formule :

```text
AO_FromTexture = Lerp(1.0, ORM.R, AO_Strength)
```

Avec fallback sans ORM :

```text
UseORMTexture == true  -> AO_FromTexture
UseORMTexture == false -> 1.0
```

Branchement :

```text
AO_Final -> Ambient Occlusion
```

---

#### Roughness

Formule :

```text
RoughnessFromTexture = Clamp(ORM.G * RoughnessMultiplier, 0.0, 1.0)

Roughness_FromORM =
    UseRoughnessOverride
        ? RoughnessOverride
        : RoughnessFromTexture

Roughness_Final =
    UseORMTexture
        ? Roughness_FromORM
        : RoughnessOverride
```

Branchement :

```text
Roughness_Final -> Roughness
```

---

#### Metallic

Formule :

```text
MetallicFromTexture = Clamp(ORM.B * MetallicMultiplier, 0.0, 1.0)

Metallic_FromORM =
    UseMetallicOverride
        ? MetallicOverride
        : MetallicFromTexture

Metallic_Final =
    UseORMTexture
        ? Metallic_FromORM
        : MetallicOverride
```

Branchement :

```text
Metallic_Final -> Metallic
```

---

### 5.5 Bloc Emissive

Formule :

```text
Pulse = 1.0 + sin(Time * EmissivePulseSpeed) * EmissivePulseStrength

EmissiveBase =
    UseEmissiveTexture
        ? EmissiveTexture.rgb
        : BaseColor_Final

EmissiveComputed = EmissiveBase * EmissiveColor * EmissiveStrength * Pulse

Emissive_Final =
    UseEmissive
        ? EmissiveComputed
        : float3(0, 0, 0)
```

Nodes Unreal :

```text
Time
Multiply EmissivePulseSpeed
Sine
Multiply EmissivePulseStrength
Add 1.0
```

Puis :

```text
StaticSwitch UseEmissiveTexture
    True  = EmissiveTexture.rgb
    False = BaseColor_Final

Multiply EmissiveColor
Multiply EmissiveStrength
Multiply Pulse

StaticSwitch UseEmissive
    True  = EmissiveComputed
    False = Constant3Vector(0,0,0)
```

Branchement :

```text
Emissive_Final -> Emissive Color
```

---

## 6. Branchements finaux

```text
Base Color        <- BaseColor_Final
Normal            <- Normal_Final
Ambient Occlusion <- AO_Final
Roughness         <- Roughness_Final
Metallic          <- Metallic_Final
Emissive Color    <- Emissive_Final
```

Ne rien brancher dans :

```text
Opacity
Opacity Mask
World Position Offset
Pixel Depth Offset
```

---

## 7. Réglages des textures dans Unreal Engine

---

### BaseColor

```text
sRGB                 = true
Compression Settings = Default
Texture Group        = World
```

---

### Normal

```text
sRGB                 = false
Compression Settings = Normalmap
Texture Group        = WorldNormalMap
```

---

### ORM

```text
sRGB                 = false
Compression Settings = Masks
Texture Group        = World
```

Important : l’ORM est une texture de données, pas une texture de couleur. Elle ne doit pas être corrigée en sRGB.

---

### Emissive

```text
sRGB                 = true
Compression Settings = Default
Texture Group        = World
```

---

## 8. Instances recommandées

---

### 8.1 `MI_Wall_Stone_01`

```text
Parent = M_GrimrockSurface_Master

BaseColorTexture = T_Wall_Stone_01_BC
NormalTexture    = T_Wall_Stone_01_N
ORMTexture       = T_Wall_Stone_01_ORM

UseNormalTexture = true
UseORMTexture    = true

UV_Tiling_U = 1.0
UV_Tiling_V = 1.0
UV_Offset_U = 0.0
UV_Offset_V = 0.0

BaseColorTint      = blanc
BaseColorStrength  = 0.85
DesaturationAmount = 0.10

NormalStrength = 0.8
AO_Strength    = 0.8

UseRoughnessOverride = false
RoughnessMultiplier  = 1.1

UseMetallicOverride = false
MetallicMultiplier  = 1.0

UseEmissive = false
```

---

### 8.2 `MI_Floor_Stone_01`

```text
Parent = M_GrimrockSurface_Master

BaseColorTexture = T_Floor_Stone_01_BC
NormalTexture    = T_Floor_Stone_01_N
ORMTexture       = T_Floor_Stone_01_ORM

UseNormalTexture = true
UseORMTexture    = true

UV_Tiling_U = 1.0
UV_Tiling_V = 1.0
UV_Offset_U = 0.0
UV_Offset_V = 0.0

BaseColorStrength  = 0.8
DesaturationAmount = 0.05

NormalStrength = 0.7
AO_Strength    = 0.8

UseRoughnessOverride = false
RoughnessMultiplier  = 1.15

UseMetallicOverride = false
MetallicMultiplier  = 1.0

UseEmissive = false
```

---

### 8.3 `MI_Door_Wood_01`

```text
Parent = M_GrimrockSurface_Master

BaseColorTexture = T_Door_Wood_01_BC
NormalTexture    = T_Door_Wood_01_N
ORMTexture       = T_Door_Wood_01_ORM

UseNormalTexture = true
UseORMTexture    = true

BaseColorStrength  = 0.9
DesaturationAmount = 0.0

NormalStrength = 0.75
AO_Strength    = 0.9

UseRoughnessOverride = false
RoughnessMultiplier  = 1.0

UseMetallicOverride = true
MetallicOverride    = 0.0

UseEmissive = false
```

---

### 8.4 `MI_Button_Metal_01`

```text
Parent = M_GrimrockSurface_Master

BaseColorTexture = T_Button_Metal_01_BC
NormalTexture    = T_Button_Metal_01_N
ORMTexture       = T_Button_Metal_01_ORM

UseNormalTexture = true
UseORMTexture    = true

BaseColorStrength  = 0.9
DesaturationAmount = 0.05

NormalStrength = 0.8
AO_Strength    = 1.0

UseRoughnessOverride = false
RoughnessMultiplier  = 0.9

UseMetallicOverride = false
MetallicMultiplier  = 1.0

UseEmissive = false
```

---

### 8.5 `MI_Button_Active_Glow`

```text
Parent = M_GrimrockSurface_Master

BaseColorTexture = T_ButtonSigil_BC
NormalTexture    = T_ButtonSigil_N
ORMTexture       = T_ButtonSigil_ORM

UseNormalTexture = true
UseORMTexture    = true

UseEmissive        = true
UseEmissiveTexture = false

EmissiveColor         = bleu pâle ou orange doux
EmissiveStrength      = 0.25
EmissivePulseStrength = 0.05
EmissivePulseSpeed    = 0.5
```

---

---

## 9. Synthèse du pipeline textures GrimrockPrototype

Cette section résume les décisions prises pour le pipeline graphique du projet. Le principe général est de conserver dans Unreal uniquement les textures finales utiles au jeu, et de sortir les sources lourdes 4K du dépôt Git.

### 9.1 Standard final des textures

Le standard principal du projet est :

```text
T_xxx_BC   = BaseColor / Albedo
T_xxx_N    = Normal Map DirectX
T_xxx_ORM  = Occlusion / Roughness / Metallic packés
```

Convention ORM officielle :

```text
ORM.R = Ambient Occlusion
ORM.G = Roughness
ORM.B = Metallic
```

Textures additionnelles uniquement si nécessaire :

```text
T_xxx_RGBA = couleur + alpha réellement utilisé
T_xxx_E    = Emissive dédié
T_xxx_M    = Mask séparé, si le RGBA n'est pas adapté
T_xxx_H    = Height, uniquement pour un cas justifié
```

Par défaut, éviter :

```text
Displacement
Height
AO / Roughness / Metallic séparés si une ORM existe
RGBA lorsqu'aucun alpha n'est utile
textures finales 16 bpc inutiles dans Content/
```

---

## 10. Réglages importants des textures dans Unreal Engine 5

### 10.1 `T_xxx_BC` — BaseColor

```text
Source finale recommandée : 8 bpc RGB
Alpha                   : non, sauf besoin réel
sRGB                    : true
Compression Settings    : Default
Texture Group           : World
Mip Gen Settings        : FromTextureGroup
```

À utiliser pour : pierre, bois, métal, os, sol, mur, plafond, porte, objet opaque.

### 10.2 `T_xxx_N` — Normal Map

```text
Source finale recommandée : 8 bpc RGB
Alpha                   : non
sRGB                    : false
Compression Settings    : Normalmap
Texture Group           : WorldNormalMap
Mip Gen Settings        : FromTextureGroup
```

Important : une normal map est une texture de données. Ne pas appliquer de correction colorimétrique ou d'ajustement de luminosité/contraste.

### 10.3 `T_xxx_ORM` — AO / Roughness / Metallic

```text
Source finale recommandée : 8 bpc RGB
Alpha                   : non
sRGB                    : false
Compression Settings    : Masks
Texture Group           : World
Mip Gen Settings        : FromTextureGroup
```

Canaux :

```text
R = AO
G = Roughness
B = Metallic
```

Valeurs par défaut si une map manque :

```text
AO absent        -> canal R blanc
Roughness absent -> gris moyen ou RoughnessOverride dans l'instance
Metallic absent  -> canal B noir pour pierre / bois / tissu / os
Metal uniforme   -> MetallicOverride dans l'instance, ou canal B blanc si nécessaire
```

### 10.4 `T_xxx_RGBA` — Texture avec alpha

```text
Source finale recommandée : 8 bpc RGBA
sRGB                    : true si RGB = couleur
Compression Settings    : Default
Usage                   : uniquement si l'alpha est réellement utilisé
```

Exemples : rune, mousse détourée, racines, sang, glyphe, icône UI.

### 10.5 Tableau récapitulatif

| Type | Fichier | Source finale | sRGB UE5 | Compression UE5 | Remarque |
|---|---|---:|---:|---|---|
| BaseColor | `T_xxx_BC` | 8 bpc RGB | true | Default | Couleur visible |
| Normal | `T_xxx_N` | 8 bpc RGB | false | Normalmap | Texture de données |
| ORM | `T_xxx_ORM` | 8 bpc RGB | false | Masks | R=AO, G=Roughness, B=Metallic |
| Emissive | `T_xxx_E` | 8 bpc RGB | true | Default | Optionnel |
| RGBA | `T_xxx_RGBA` | 8 bpc RGBA | true | Default | Seulement si alpha utile |
| Height | `T_xxx_H` | 8/16 bpc gris | false | Masks | Exception uniquement |
| Displacement | `T_xxx_D` | variable | false | Masks | Hors Content par défaut |

---

## 11. Règle d'export GIMP / Photoshop

Le problème rencontré avec une texture 2048 beaucoup plus claire que la 4096 venait d'un export GIMP en :

```text
16 bpc RGBA
```

alors que la source était une image classique :

```text
8 bpc RGB / 24 bits
```

Règle officielle du projet :

```text
T_xxx_BC   -> 8 bpc RGB, sRGB, sans alpha inutile
T_xxx_N    -> 8 bpc RGB, pas de conversion colorimétrique, sans alpha inutile
T_xxx_ORM  -> 8 bpc RGB, pas de conversion colorimétrique, sans alpha inutile
T_xxx_RGBA -> 8 bpc RGBA uniquement si l'alpha est réellement utilisé
```

### 11.1 Export correct dans GIMP pour une BaseColor

1. Ouvrir l'image source 4K.
2. `Image > Mode > RGB`.
3. Supprimer le canal alpha si présent et inutile.
4. `Image > Precision > 8-bit integer`.
5. `Image > Color Management > Convert to sRGB`.
6. `Image > Scale Image`, puis 2048 ou 1024.
7. Export PNG ou TGA en vérifiant :

```text
Pixel format = 8 bpc RGB
```

Éviter pour une BaseColor opaque :

```text
16 bpc RGB
16 bpc RGBA
8 bpc RGBA si l'alpha ne sert pas
```

### 11.2 Ouverture d'une Normal Map dans GIMP

Si GIMP affiche une popup du type :

```text
Keep the Embedded Working Space?
Convert the image to the built-in sRGB color profile?
```

Pour une normal map ou une ORM, choisir :

```text
Keep
```

Raison : une normal map et une ORM sont des textures de données. Il ne faut pas modifier leurs valeurs RGB par une conversion colorimétrique.

### 11.3 Résolutions recommandées

```text
Sources téléchargées       : 4096, stockées hors Content
Murs / sols principaux     : 2048
Plafonds                   : 1024 ou 2048
Portes proches caméra      : 1024 ou 2048
Boutons / leviers / plaques: 1024
Décorations planes         : 1024
Runes / glyphes            : 1024
Icônes UI                  : 256 ou 512
```

---

## 12. Générer une texture ORM avec GIMP

Une ORM regroupe trois images en une seule texture RGB :

```text
R = Ambient Occlusion
G = Roughness
B = Metallic
```

### 12.1 Préparation

Toutes les images sources doivent avoir la même résolution, par exemple :

```text
2048 x 2048
```

Pour AO, Roughness et Metallic, éviter les conversions colorimétriques. Ces images sont des données, pas des couleurs visibles.

### 12.2 Cas idéal : AO + Roughness + Metallic disponibles

Dans GIMP :

1. Ouvrir les trois images.
2. Vérifier `Image > Precision > 8-bit integer`.
3. `Colors > Components > Compose`.
4. Mode : `RGB`.
5. Assigner :

```text
Red   = AO
Green = Roughness
Blue  = Metallic
```

6. Exporter sous :

```text
T_xxx_ORM.png
```

7. Vérifier :

```text
Pixel format = 8 bpc RGB
```

### 12.3 Cas courant : Metallic absent

Pour pierre, bois, tissu, os :

```text
R = AO
G = Roughness
B = noir
```

Le canal bleu noir signifie :

```text
Metallic = 0.0
```

### 12.4 Cas courant : AO absent

```text
R = blanc
G = Roughness
B = Metallic ou noir
```

Le canal rouge blanc signifie :

```text
AO = 1.0, donc pas d'occlusion spécifique
```

### 12.5 Cas avec seulement Roughness

```text
R = blanc
G = Roughness
B = noir
```

C'est suffisant pour beaucoup de matériaux simples non métalliques.

### 12.6 Import UE5 de l'ORM

```text
sRGB                 = false
Compression Settings = Masks
Texture Group        = World
```

---

## 13. Pourquoi Height / Displacement est exclu par défaut

Pour GrimrockPrototype, le vrai volume doit venir prioritairement des meshes : murs, colonnes, alcôves, portes, cadres, décorations murales, etc.

La Normal Map suffit pour simuler :

```text
joints de pierre
fissures
aspérités
bois usé
métal marqué
```

Height / Displacement est exclu du master principal parce que cela ajoute :

```text
fichiers supplémentaires
poids disque et Git
complexité shader
risques d'artefacts visuels
coût GPU supplémentaire
maintenance plus lourde
```

Règle du projet :

```text
Height / Displacement restent dans _ExternalArtSource/
Ils ne sont pas importés dans Content/ par défaut.
```

Exceptions possibles :

```text
gravure importante
glyphe sculpté
bouton très proche caméra
relief spécifique justifié
matériau spécialisé ultérieur
```

---

## 14. Structure finale réaliste du dossier Content

Structure cible :

```text
Content/GrimrockPrototype/
├── Blueprints/
│   ├── Editor/
│   ├── Runtime/
│   └── UI/
│
├── Core/
│   ├── DataAssets/
│   │   ├── Levels/
│   │   ├── Palettes/
│   │   └── Archetypes/
│   │       ├── Doors/
│   │       ├── Interactables/
│   │       ├── Items/
│   │       ├── Receptacles/
│   │       ├── Triggers/
│   │       └── Decorations/
│   └── Input/
│
├── Maps/
│   ├── L_GrimrockEditor.umap
│   └── L_GrimrockRuntime.umap
│
└── Art/
    ├── Materials/
    │   ├── Masters/
    │   ├── Instances/
    │   └── Editor/
    │
    ├── Textures/
    │   ├── Final/
    │   │   ├── Floor/
    │   │   ├── Wall/
    │   │   ├── Ceiling/
    │   │   ├── Doors/
    │   │   ├── Interactables/
    │   │   ├── Items/
    │   │   └── Decorations/
    │   └── Generated/
    │       ├── Icons/
    │       ├── UI/
    │       └── Runes/
    │
    ├── Meshes/
    │   ├── Editor/
    │   ├── Floor/
    │   ├── Wall/
    │   ├── Ceiling/
    │   ├── Doors/
    │   ├── Interactables/
    │   ├── Items/
    │   └── Decorations/
    │
    └── Icons/
```

---

## 15. Exemple de fichiers finaux par catégorie

### 15.1 Murs

```text
Art/Textures/Final/Wall/Stone_02/
├── T_Wall_Stone_02_BC.uasset
├── T_Wall_Stone_02_N.uasset
└── T_Wall_Stone_02_ORM.uasset

Art/Materials/Instances/Wall/
└── MI_Wall_Stone_02.uasset
```

### 15.2 Sols

```text
Art/Textures/Final/Floor/Stone_01/
├── T_Floor_Stone_01_BC.uasset
├── T_Floor_Stone_01_N.uasset
└── T_Floor_Stone_01_ORM.uasset

Art/Materials/Instances/Floor/
└── MI_Floor_Stone_01.uasset
```

### 15.3 Plafonds

```text
Art/Textures/Final/Ceiling/StoneVault_01/
├── T_Ceil_StoneVault_01_BC.uasset
├── T_Ceil_StoneVault_01_N.uasset
└── T_Ceil_StoneVault_01_ORM.uasset

Art/Materials/Instances/Ceiling/
└── MI_Ceil_Stone_01.uasset
```

### 15.4 Portes

```text
Art/Textures/Final/Doors/Wood_01/
├── T_Door_Wood_01_BC.uasset
├── T_Door_Wood_01_N.uasset
└── T_Door_Wood_01_ORM.uasset

Art/Materials/Instances/Doors/
└── MI_Door_Wood_01.uasset
```

### 15.5 Objets interactifs

```text
Art/Textures/Final/Interactables/Button_Metal_01/
├── T_Button_Metal_01_BC.uasset
├── T_Button_Metal_01_N.uasset
└── T_Button_Metal_01_ORM.uasset

Art/Textures/Final/Interactables/Lever_Metal_01/
├── T_Lever_Metal_01_BC.uasset
├── T_Lever_Metal_01_N.uasset
└── T_Lever_Metal_01_ORM.uasset
```

### 15.6 Décorations

```text
Art/Textures/Final/Decorations/Floor/RuneCircle_01/
├── T_Deco_FloorRuneCircle_Blue_RGBA.uasset
└── T_Deco_FloorRuneCircle_Blue_E.uasset        // optionnel

Art/Textures/Final/Decorations/Floor/Roots_01/
└── T_Deco_FloorRoots_01_RGBA.uasset
```

---

## 16. Sources externes hors Unreal et hors Git

Les textures téléchargées en 4K, les fichiers Blender et les images sources doivent être conservés hors `Content/` :

```text
GrimrockPrototype/
└── _ExternalArtSource/
    ├── Textures/
    │   ├── Floor/
    │   ├── Wall/
    │   ├── Ceiling/
    │   ├── Doors/
    │   ├── Interactables/
    │   └── Decorations/
    ├── Blender/
    ├── Exports/
    │   ├── FBX/
    │   └── PNG_Final_PreUE/
    └── References/
```

Ce dossier doit être ignoré par Git.

Exemple :

```text
_ExternalArtSource/Textures/Wall/Tiles083_4K/
├── Tiles083_4K-PNG_Color.png
├── Tiles083_4K-PNG_NormalDX.png
├── Tiles083_4K-PNG_AmbientOcclusion.png
├── Tiles083_4K-PNG_Roughness.png
└── Tiles083_4K-PNG_Displacement.png
```

Dans Unreal, seules les versions finales sont importées :

```text
T_Wall_Stone_02_BC
T_Wall_Stone_02_N
T_Wall_Stone_02_ORM
```

---

## 17. Git, Git LFS et fichiers ignorés

### 17.1 `.gitignore`

```gitignore
Binaries/
DerivedDataCache/
Intermediate/
Saved/
.vs/
*.sln.DotSettings.user

_ExternalArtSource/
_LocalExports/
_Temp/
```

### 17.2 `.gitattributes` recommandé

```gitattributes
*.uasset filter=lfs diff=lfs merge=lfs -text
*.umap filter=lfs diff=lfs merge=lfs -text
*.ubulk filter=lfs diff=lfs merge=lfs -text
*.uexp filter=lfs diff=lfs merge=lfs -text

*.fbx filter=lfs diff=lfs merge=lfs -text
*.blend filter=lfs diff=lfs merge=lfs -text
*.png filter=lfs diff=lfs merge=lfs -text
*.tga filter=lfs diff=lfs merge=lfs -text
*.exr filter=lfs diff=lfs merge=lfs -text
```

---

## 18. Checklist d'import d'une nouvelle texture

Pour chaque nouveau matériau téléchargé :

```text
1. Stocker les sources 4K dans _ExternalArtSource/.
2. Choisir la résolution finale : 2048 ou 1024.
3. Exporter BC en 8 bpc RGB sRGB.
4. Exporter N en 8 bpc RGB sans conversion colorimétrique.
5. Générer ORM en 8 bpc RGB : R=AO, G=Roughness, B=Metallic.
6. Ne pas importer Height / Displacement sauf exception.
7. Importer uniquement BC / N / ORM dans Content/GrimrockPrototype/Art/Textures/Final/.
8. Vérifier les réglages UE5 : sRGB, Compression, Texture Group.
9. Créer ou mettre à jour la Material Instance.
10. Tester dans la scène avec le vrai éclairage du donjon.
11. Documenter la source dans Docs/Assets/AssetManifest.csv.
```

---

## 19. Diagnostic rapide si une texture paraît trop claire dans UE5

Si une texture 2048 paraît plus claire que la 4096 alors qu'elle semble identique dans GIMP :

```text
1. Vérifier le format d'export GIMP : éviter 16 bpc RGBA.
2. Réexporter en 8 bpc RGB.
3. Vérifier que l'alpha est absent si inutile.
4. Tester un import sous nouveau nom, pas seulement Reimport.
5. Vérifier sRGB et Compression Settings.
6. Vérifier Mip Gen Settings et Texture Group.
7. Tester temporairement NoMipmaps si nécessaire.
```

Cause déjà rencontrée dans le projet :

```text
Export GIMP en 16 bpc RGBA -> texture lavée / éclaircie dans UE5.
```

Solution :

```text
Export final BaseColor = 8 bpc RGB.
```

---

## 20. Résumé opérationnel

Le pipeline final GrimrockPrototype est :

```text
Sources 4K téléchargées     -> _ExternalArtSource/, hors Git
Textures finales UE5        -> Content/GrimrockPrototype/Art/Textures/Final/
Matériaux maîtres           -> Art/Materials/Masters/
Instances                   -> Art/Materials/Instances/
Standard opaque             -> BC + N + ORM
Master opaque principal     -> M_GrimrockSurface_Master
Height / Displacement       -> exclus par défaut
Git                         -> projet jouable + textures finales, pas bibliothèque source 4K
```

Règle principale :

```text
Le dépôt Git doit contenir le jeu, pas toute la bibliothèque de textures source.
```

