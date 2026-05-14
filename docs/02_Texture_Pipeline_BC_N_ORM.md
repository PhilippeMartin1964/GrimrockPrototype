# GrimrockPrototype — Pipeline textures BC / N / ORM / RGBA

Ce document décrit le pipeline texture standard du projet **GrimrockPrototype**.

Il sert à produire des textures finales propres, légères et cohérentes pour Unreal Engine 5.5.

---

## 1. Standard final

Pour les surfaces opaques classiques, utiliser :

```text
T_xxx_BC
T_xxx_N
T_xxx_ORM
```

Avec :

```text
BC  = BaseColor / Albedo
N   = Normal Map DirectX
ORM = Occlusion / Roughness / Metallic packés
```

Convention ORM :

```text
ORM.R = Ambient Occlusion
ORM.G = Roughness
ORM.B = Metallic
```

Pour les surfaces découpées par alpha :

```text
T_xxx_RGBA
```

Avec :

```text
RGB = couleur
A   = masque d’opacité
```

---

## 2. Résolutions recommandées

```text
Murs principaux      : 2048
Sols principaux      : 2048
Portes proches       : 1024 ou 2048
Plafonds             : 1024 ou 2048
Boutons / leviers    : 1024
Plaques / supports   : 1024
Décorations planes   : 1024
Icônes UI / outils   : 256 ou 512
Sources externes     : 4096 hors Content / hors Git
```

Le 4K peut rester une source de travail, mais ne doit pas être importé systématiquement dans UE5.

---

## 3. Réduction 4K vers 2K ou 1K

Tailles :

```text
4K = 4096 x 4096
2K = 2048 x 2048
1K = 1024 x 1024
```

### Avec GIMP

```text
Image > Scale Image
Width / Height = 2048 ou 1024
Interpolation = LoHalo, NoHalo, Cubic ou Lanczos
```

### Avec ImageMagick

```bash
magick input.png -filter Lanczos -resize 2048x2048 output.png
magick input.png -filter Lanczos -resize 1024x1024 output.png
```

---

## 4. Règle d’export importante : éviter le 16 bpc RGBA involontaire

Pendant le debug, un problème de luminosité a été identifié : une texture 2048 exportée depuis GIMP était beaucoup plus claire dans UE5 que la source 4096, alors qu’elle semblait identique dans GIMP et Windows Photos.

Cause : export en `16 bpc RGBA` au lieu de `8 bpc RGB`.

Règle finale :

```text
T_xxx_BC   -> 8 bpc RGB, sRGB, sans alpha inutile
T_xxx_N    -> 8 bpc RGB, données, sans conversion colorimétrique
T_xxx_ORM  -> 8 bpc RGB, données, sans conversion colorimétrique
T_xxx_RGBA -> 8 bpc RGBA uniquement si l’alpha est réellement utilisé
```

### Dans GIMP pour une BaseColor

```text
Image > Mode > RGB
Image > Precision > 8-bit integer
Image > Color Management > Convert to sRGB
Supprimer le canal alpha si inutile
Exporter PNG ou TGA
Pixel format attendu : 8 bpc RGB
```

### Pour Normal / ORM

Ces textures sont des données, pas des couleurs artistiques.

Si GIMP affiche une popup de profil couleur :

```text
Choisir Keep
```

Puis :

```text
Image > Precision > 8-bit integer
Supprimer le canal alpha si inutile
Exporter en 8 bpc RGB
```

Ne pas appliquer :

```text
Auto Levels
Brightness / Contrast
Desaturate
Color Management Convert
```

sur les normales ou ORM, sauf raison technique précise.

---

## 5. Réglages UE5 par type de texture

| Type | Format source recommandé | sRGB | Compression | Texture Group | Remarques |
|---|---|---:|---|---|---|
| `T_xxx_BC` | 8 bpc RGB | true | Default | World | Couleur visible. |
| `T_xxx_N` | 8 bpc RGB | false | Normalmap | WorldNormalMap | Normal DirectX pour Unreal. |
| `T_xxx_ORM` | 8 bpc RGB | false | Masks | World | R=AO, G=Roughness, B=Metallic. |
| `T_xxx_RGBA` | 8 bpc RGBA | true | Default | World | Alpha utile pour masked. |
| `T_xxx_M` | 8 bpc grayscale/RGB | false | Masks | World | Masque séparé éventuel. |
| `T_xxx_E` | 8 bpc RGB | true | Default | World | Emissive, couleur visible. |
| `T_xxx_H` | variable | false | Masks/Grayscale | World | À éviter par défaut. |

---

## 6. Générer une ORM avec GIMP

Une ORM regroupe trois maps en une seule texture RGB :

```text
R = Ambient Occlusion
G = Roughness
B = Metallic
```

### Cas idéal : AO + Roughness + Metallic disponibles

Dans GIMP :

```text
1. Ouvrir les trois images.
2. S’assurer qu’elles ont la même taille.
3. Les passer en 8-bit integer si nécessaire.
4. Couleurs > Composants > Composer.
5. Mode = RGB.
6. Rouge = AO.
7. Vert = Roughness.
8. Bleu = Metallic.
9. Exporter en T_xxx_ORM.png, 8 bpc RGB.
```

Dans UE5 :

```text
sRGB = false
Compression Settings = Masks
```

### Si Metallic absent

Pour pierre, bois, tissu, os :

```text
B = noir complet
```

Car :

```text
Metallic 0.0 = non métallique
```

### Si AO absent

```text
R = blanc complet
```

Car :

```text
AO 1.0 = aucune occlusion ajoutée
```

### Si Roughness absent

Deux options :

```text
1. créer un canal G gris moyen, par exemple 0.65 à 0.80 pour pierre/bois ;
2. ne pas utiliser d’ORM et régler RoughnessOverride dans la Material Instance.
```

Pour GrimrockPrototype, la deuxième option est acceptable au début.

---

## 7. Height / Displacement

Par défaut, ne pas importer :

```text
Height
Displacement
```

Raisons :

```text
la Normal Map donne déjà le relief visuel utile ;
le projet utilise des murs/sols/plafonds modulaires ;
le vrai volume doit venir des Static Meshes ;
ces maps alourdissent Git et Content ;
elles compliquent les masters ;
elles peuvent créer des artefacts ou des coûts shader inutiles.
```

Elles peuvent rester dans :

```text
_ExternalArtSource/
```

et être réintroduites plus tard pour un cas spécifique : dalle sculptée, relief magique, paroi organique, etc.

---

## 8. Textures RGBA pour le master masked

Pour `M_GrimrockSurface_Masked_Master`, les décorations comme mousse, racines, sang ou tapis utilisent souvent :

```text
T_Deco_FloorMoss_01_RGBA
T_Deco_FloorRoots_01_RGBA
T_Deco_FloorBloodStain_01_RGBA
T_Deco_FloorCarpet_01_RGBA
```

Dans ces textures :

```text
RGB = couleur
A   = découpe
```

Dans la Material Instance :

```text
UseAlphaFromBaseColor = true
UseOpacityMaskTexture = false
```

---

## 9. Diagnostic des problèmes fréquents

### Texture 2048 plus claire que 4096

Vérifier :

```text
export GIMP en 8 bpc RGB, pas 16 bpc RGBA
sRGB correct
pas d’alpha inutile
pas de conversion couleur sur les textures de données
```

### Texture masked avec carré visible

Vérifier :

```text
Blend Mode = Masked
Source Alpha Detected = true
UseAlphaFromBaseColor = true
OpacityMaskClip correct
```

### ORM donne un rendu bizarre

Vérifier :

```text
sRGB = false
Compression = Masks
R = AO, G = Roughness, B = Metallic
pas de convention RMA/MRA inversée
```

### Normal inversée

Vérifier que la normal est DirectX :

```text
NormalDX pour UE5
```

Si elle est OpenGL, inverser le canal vert ou récupérer une version DirectX.
