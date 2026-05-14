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

## 9. Workflow d’import texture compatible avec ce master

Pour chaque nouveau matériau téléchargé, produire idéalement :

```text
T_Name_BC
T_Name_N
T_Name_ORM
```

Exemple :

```text
T_Wall_Stone_01_BC
T_Wall_Stone_01_N
T_Wall_Stone_01_ORM
```

Avec :

```text
T_Name_BC  = BaseColor / Albedo
T_Name_N   = Normal DirectX
T_Name_ORM = R: AO / G: Roughness / B: Metallic
```

Si une map est absente :

```text
AO absent        -> canal R blanc
Roughness absent -> utiliser RoughnessOverride
Metallic absent  -> canal B noir pour pierre/bois, blanc pour métal uniforme
```

---

## 10. Pourquoi Height / Displacement est exclu

Ce master est volontairement limité à :

```text
BaseColor + Normal + ORM
```

Raisons :

- la Normal Map donne déjà l’essentiel du relief visuel ;
- les murs, sols et plafonds du projet sont modulaires et relativement simples ;
- le vrai volume doit venir des Static Meshes ;
- Height / Displacement ajoute des fichiers lourds ;
- cela complique inutilement les matériaux ;
- cela augmente le coût shader ;
- cela peut produire des artefacts à proximité des murs ;
- cela alourdit Git sans bénéfice suffisant pour le prototype actuel.

Les Height / Displacement peuvent rester dans :

```text
_ExternalArtSource/
```

mais ne doivent pas être importés dans `Content/GrimrockPrototype` par défaut.

---

## 11. Contrôle qualité après création

Tester au minimum :

### Test pierre avec ORM

```text
MI_Test_Wall_Stone
```

Vérifier :

```text
BaseColor visible
Normal visible sous lumière rasante
Roughness correcte
Metallic = 0
AO pas trop noir
```

### Test bois sans metallic

```text
MI_Test_Door_Wood
UseMetallicOverride = true
MetallicOverride = 0.0
```

Vérifier que le bois ne ressemble pas à du plastique brillant.

### Test métal

```text
MI_Test_Button_Metal
```

Vérifier :

```text
Metallic proche de 1
Roughness contrôlable
Normal pas trop forte
```

### Test sans ORM

```text
UseORMTexture = false
RoughnessOverride = 0.7
MetallicOverride  = 0.0
```

Le matériau doit compiler et rester visuellement correct.

### Test emissive discret

```text
UseEmissive           = true
EmissiveStrength      = 0.5
EmissivePulseStrength = 0.05
EmissivePulseSpeed    = 0.5
```

Vérifier que le pulse reste subtil.

---

## 12. Checklist de validation

Avant de considérer `M_GrimrockSurface_Master` comme terminé :

```text
[ ] Le matériau est Opaque.
[ ] Les paramètres sont groupés correctement.
[ ] Les UV utilisent U et V séparés.
[ ] BaseColorTint fonctionne.
[ ] BaseColorStrength fonctionne.
[ ] DesaturationAmount fonctionne.
[ ] NormalStrength fonctionne.
[ ] UseNormalTexture fonctionne.
[ ] UseORMTexture fonctionne.
[ ] AO_Strength fonctionne.
[ ] RoughnessMultiplier fonctionne.
[ ] RoughnessOverride fonctionne.
[ ] MetallicMultiplier fonctionne.
[ ] MetallicOverride fonctionne.
[ ] UseEmissive fonctionne.
[ ] UseEmissiveTexture fonctionne.
[ ] EmissiveStrength fonctionne.
[ ] EmissivePulseStrength fonctionne.
[ ] EmissivePulseSpeed fonctionne.
[ ] Aucune entrée Opacity / Opacity Mask n’est branchée.
[ ] Aucun Height / Displacement / Parallax n’est présent.
[ ] Les textures ORM sont importées avec sRGB=false et Compression=Masks.
[ ] Les textures Normal sont importées avec Compression=Normalmap.
```

---

## 13. Évolution prévue

Après validation de ce master, créer un second master pour les matériaux découpés :

```text
M_GrimrockSurface_Masked_Master
```

Il reprendra la même logique générale, mais avec :

```text
Blend Mode = Masked
Opacity Mask
Opacity Mask Clip Value
```

Il servira pour :

```text
FloorRoots
FloorMoss
FloorBloodStain
FloorCarpet détouré
WallGlyphe
WallInscription avec masque
petites décorations planes
```

Pour les runes magiques fortement emissive/additive, conserver éventuellement un master spécialisé :

```text
M_FloorRuneCircle_Additive
```

---

## 14. Résumé court

`M_GrimrockSurface_Master` est le master opaque général du projet.

Il couvre :

```text
pierre
bois
métal
sol
mur
plafond
porte
bouton
levier
plaque
support de torche
objets opaques
```

Il repose sur :

```text
BaseColor + Normal + ORM
```

Il exclut volontairement :

```text
Opacity Mask
Translucency
Height
Displacement
Parallax
```

Il devient la base officielle des Material Instances opaques du projet GrimrockPrototype.
