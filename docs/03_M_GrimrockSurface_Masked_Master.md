# M_GrimrockSurface_Masked_Master

Documentation technique du Master Material **masked** du projet GrimrockPrototype.

Ce matériau maître sert aux surfaces découpées par alpha mais encore rendues comme des surfaces PBR éclairées : mousse, racines, sang, tapis, glyphes non additifs, décorations planes, panneaux découpés, etc.

Il complète :

```text
M_GrimrockSurface_Master
```

qui reste réservé aux surfaces opaques.

---

## 1. Emplacement

```text
Content/GrimrockPrototype/Art/Materials/Masters/M_GrimrockSurface_Masked_Master
```

Instances recommandées :

```text
Content/GrimrockPrototype/Art/Materials/Instances/Decorations/
```

Exemples :

```text
MI_FloorMoss_01
MI_FloorCarpet_01
MI_FloorBloodStain_01
MI_FloorRoots_01
MI_WallGlyph_01
```

---

## 2. Usage

À utiliser pour :

```text
mousse au sol
racines au sol
sang / taches
carpets détourés
glyphes muraux non additifs
ornements muraux découpés
panneaux ou fixations avec masque
```

À ne pas utiliser pour :

```text
murs, sols, plafonds opaques classiques
portes pleines
objets opaques
runes magiques additives
feu, fumée, effets translucides
```

La rune de sol bleue doit rester sur :

```text
M_FloorRuneCircle_Additive
```

ou sur un futur :

```text
M_GrimrockRune_Additive
```

---

## 3. Réglages généraux du matériau

Dans les propriétés du matériau :

```text
Material Domain         = Surface
Blend Mode              = Masked
Shading Model           = Default Lit
Two Sided               = true
Use Material Attributes = false
Opacity Mask Clip Value = 0.0
```

`Opacity Mask Clip Value = 0.0` est volontaire si l’on utilise un paramètre `OpacityMaskClip` dans le graphe.

---

## 4. Paramètres

### 4.1 `01_Textures`

| Paramètre | Type | Description |
|---|---|---|
| `BaseColorTexture` | TextureSampleParameter2D | Texture principale. Peut être RGB ou RGBA. |
| `NormalTexture` | TextureSampleParameter2D | Normal optionnelle. |
| `ORMTexture` | TextureSampleParameter2D | ORM optionnelle. |
| `OpacityMaskTexture` | TextureSampleParameter2D | Masque séparé optionnel. |
| `EmissiveTexture` | TextureSampleParameter2D | Emissive optionnelle. |

### 4.2 `02_UV`

```text
UV_Tiling_U = 1.0
UV_Tiling_V = 1.0
UV_Offset_U = 0.0
UV_Offset_V = 0.0
```

### 4.3 `03_BaseColor`

```text
BaseColorTint = blanc
BaseColorStrength = 1.0
DesaturationAmount = 0.0
```

### 4.4 `04_Normal`

```text
UseNormalTexture = true/false
NormalStrength = 1.0
```

Pour les décorations planes fines, `UseNormalTexture=false` est souvent suffisant.

### 4.5 `05_ORM`

```text
UseORMTexture = true/false
AO_Strength = 1.0
UseRoughnessOverride = true/false
RoughnessOverride = 0.65
RoughnessMultiplier = 1.0
UseMetallicOverride = true/false
MetallicOverride = 0.0
MetallicMultiplier = 1.0
```

Pour les décorations simples, commencer avec :

```text
UseORMTexture = false
UseRoughnessOverride = true
RoughnessOverride = 0.8
UseMetallicOverride = true
MetallicOverride = 0.0
```

### 4.6 `06_OpacityMask`

```text
UseAlphaFromBaseColor = true
UseOpacityMaskTexture = false
InvertOpacityMask = false
OpacityMaskStrength = 1.0
OpacityMaskBias = 0.0
OpacityMaskClip = 0.333
```

### 4.7 `07_Emissive`

```text
UseEmissive = false
UseEmissiveTexture = false
EmissiveColor = blanc
EmissiveStrength = 0.0
EmissivePulseStrength = 0.0
EmissivePulseSpeed = 0.5
```

---

## 5. Bloc UV

Même logique que le master opaque :

```text
TextureCoordinate
    * AppendVector(UV_Tiling_U, UV_Tiling_V)
    + AppendVector(UV_Offset_U, UV_Offset_V)
    = UV_Final
```

Toutes les textures utilisent `UV_Final`.

---

## 6. Bloc BaseColor

```text
BaseColor_Final =
    Desaturate(BaseColorTexture.RGB, DesaturationAmount)
    * BaseColorTint
    * BaseColorStrength
```

Branchement :

```text
BaseColor_Final -> Base Color
```

Le canal alpha de `BaseColorTexture` ne doit pas être utilisé pour la couleur. Il est réservé au bloc opacity mask.

---

## 7. Bloc Normal

```text
NormalStrength = 1.0 -> normal complète
NormalStrength = 0.0 -> normal plate
```

Branchement recommandé :

```text
NormalTexture -> FlattenNormal -> UseNormalTexture switch -> Normal
```

Fallback :

```text
Constant3Vector(0,0,1)
```

---

## 8. Bloc ORM

Convention :

```text
ORM.R = Ambient Occlusion
ORM.G = Roughness
ORM.B = Metallic
```

Fallback si `UseORMTexture=false` :

```text
AO = 1.0
Roughness = RoughnessOverride
Metallic = MetallicOverride
```

Pour les décorations masked, il est recommandé de commencer sans ORM et d’ajouter une ORM seulement si nécessaire.

---

## 9. Bloc Opacity Mask — logique complète

Le bloc opacity mask doit permettre trois cas :

```text
1. Alpha inclus dans BaseColorTexture.A
2. Masque séparé dans OpacityMaskTexture.R
3. Aucun masque -> tout visible
```

### 9.1 Source A : alpha de la BaseColor

Pour les textures RGBA :

```text
T_Deco_FloorMoss_01_RGBA
T_Deco_FloorRoots_01_RGBA
T_Deco_FloorBloodStain_01_RGBA
T_Deco_FloorCarpet_01_RGBA
```

Le branchement est :

```text
BaseColorTexture.RGB -> Base Color
BaseColorTexture.A   -> Opacity Mask source
```

Paramètres d’instance :

```text
UseAlphaFromBaseColor = true
UseOpacityMaskTexture = false
```

### 9.2 Source B : texture masque séparée

Pour les cas :

```text
T_xxx_BC
T_xxx_M
```

Le branchement est :

```text
OpacityMaskTexture.R -> Opacity Mask source
```

Réglages UE5 de `OpacityMaskTexture` :

```text
sRGB = false
Compression Settings = Masks
Texture Group = World
```

Paramètres d’instance :

```text
UseAlphaFromBaseColor = false
UseOpacityMaskTexture = true
```

### 9.3 Priorité entre alpha et texture masque

La priorité recommandée est :

```text
Si UseOpacityMaskTexture = true :
    utiliser OpacityMaskTexture.R
Sinon si UseAlphaFromBaseColor = true :
    utiliser BaseColorTexture.A
Sinon :
    utiliser 1.0
```

Pseudo-graphe :

```text
Mask_AlphaOrFull =
    UseAlphaFromBaseColor ? BaseColorTexture.A : 1.0

Mask_Source =
    UseOpacityMaskTexture ? OpacityMaskTexture.R : Mask_AlphaOrFull
```

### 9.4 Inversion

Pour corriger un masque inversé :

```text
Mask_Oriented =
    InvertOpacityMask ? OneMinus(Mask_Source) : Mask_Source
```

Par défaut :

```text
InvertOpacityMask = false
```

### 9.5 Strength / Bias

```text
Mask_Adjusted = Clamp((Mask_Oriented * OpacityMaskStrength) + OpacityMaskBias, 0, 1)
```

Rôle :

```text
OpacityMaskStrength > 1.0 -> élargit les zones visibles
OpacityMaskStrength < 1.0 -> réduit les zones visibles
OpacityMaskBias positif   -> rend plus de pixels visibles
OpacityMaskBias négatif   -> réduit les halos ou fonds parasites
```

### 9.6 OpacityMaskClip paramétrable

Pour pouvoir régler le seuil par Material Instance :

```text
Material property:
Opacity Mask Clip Value = 0.0
```

Dans le graphe :

```text
Mask_Final = Mask_Adjusted - OpacityMaskClip
Mask_Final -> Opacity Mask
```

Cela équivaut à :

```text
pixel visible si Mask_Adjusted >= OpacityMaskClip
```

Valeurs utiles :

```text
0.10 -> bords larges, plus de pixels visibles
0.25 -> doux, utile pour sang/mousse
0.333 -> seuil standard
0.50 -> découpe plus dure, utile pour tapis ou formes nettes
```

---

## 10. Schéma final du bloc Opacity Mask

```text
BaseColorTexture.A
    -> UseAlphaFromBaseColor ? BaseColorTexture.A : 1.0
    -> Mask_AlphaOrFull

OpacityMaskTexture.R
    -> UseOpacityMaskTexture ? OpacityMaskTexture.R : Mask_AlphaOrFull
    -> Mask_Source

Mask_Source
    -> InvertOpacityMask ? OneMinus(Mask_Source) : Mask_Source
    -> Mask_Oriented

Mask_Oriented
    -> Multiply OpacityMaskStrength
    -> Add OpacityMaskBias
    -> Clamp 0..1
    -> Subtract OpacityMaskClip
    -> Opacity Mask
```

---

## 11. Réglages d’instances recommandés

### `MI_FloorMoss_01`

```text
BaseColorTexture = T_Deco_FloorMoss_01_RGBA
UseAlphaFromBaseColor = true
UseOpacityMaskTexture = false
InvertOpacityMask = false
OpacityMaskStrength = 1.0
OpacityMaskBias = 0.0
OpacityMaskClip = 0.25 à 0.35
UseNormalTexture = false
UseORMTexture = false
UseRoughnessOverride = true
RoughnessOverride = 0.85
UseMetallicOverride = true
MetallicOverride = 0.0
BaseColorStrength = 0.75 à 0.90
UseEmissive = false
```

### `MI_FloorRoots_01`

```text
BaseColorTexture = T_Deco_FloorRoots_01_RGBA
UseAlphaFromBaseColor = true
UseOpacityMaskTexture = false
OpacityMaskClip = 0.30 à 0.40
OpacityMaskBias = 0.0 à 0.02
RoughnessOverride = 0.8
MetallicOverride = 0.0
```

### `MI_FloorBloodStain_01`

```text
BaseColorTexture = T_Deco_FloorBloodStain_01_RGBA
UseAlphaFromBaseColor = true
UseOpacityMaskTexture = false
OpacityMaskClip = 0.10 à 0.25
OpacityMaskBias = -0.03
RoughnessOverride = 0.45 à 0.75
MetallicOverride = 0.0
```

### `MI_FloorCarpet_01`

```text
BaseColorTexture = T_Deco_FloorCarpet_01_RGBA
UseAlphaFromBaseColor = true
UseOpacityMaskTexture = false
OpacityMaskClip = 0.35 à 0.50
UseNormalTexture = true si T_Deco_FloorCarpet_01_N existe
NormalStrength = 0.3 à 0.5
RoughnessOverride = 0.9
MetallicOverride = 0.0
```

---

## 12. Erreurs fréquentes

### Tout est invisible

Vérifier :

```text
OpacityMaskClip trop haut
OpacityMaskBias trop négatif
UseOpacityMaskTexture activé mais texture masque vide
masque inversé
texture RGBA sans alpha détecté
```

### Tout est visible

Vérifier :

```text
UseAlphaFromBaseColor = false
UseOpacityMaskTexture = false
OpacityMaskClip trop bas
alpha entièrement blanc
```

### Carré noir autour de la décoration

Vérifier :

```text
Blend Mode = Masked
UseAlphaFromBaseColor = true
Source Alpha Detected = true
texture bien exportée en 8 bpc RGBA
```

### Halo clair autour des bords

Corriger :

```text
OpacityMaskBias = -0.02 à -0.06
OpacityMaskClip = 0.35 à 0.50
nettoyer / dilater les couleurs utiles sous l’alpha dans la source
```

---

## 13. Checklist de validation

Pour chaque instance masked :

```text
1. Parent = M_GrimrockSurface_Masked_Master.
2. BaseColorTexture contient bien RGB + alpha si UseAlphaFromBaseColor=true.
3. Source Alpha Detected = true dans UE5.
4. OpacityMaskClip réglé par instance.
5. Pas de fond noir ou damier visible.
6. Pas de halo blanc excessif.
7. Test en éditeur.
8. Test en PIE.
9. Test à courte distance caméra.
10. Test sous lumière faible / torche.
```
