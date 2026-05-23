# GrimrockPrototype — Migration des Material Instances et checklist

Statut : guide de migration / audit. À utiliser seulement lors de migrations de Material Instances.

Ce document décrit la migration des Material Instances vers les nouveaux masters :

```text
M_GrimrockSurface_Master
M_GrimrockSurface_Masked_Master
M_FloorRuneCircle_Additive
```

---

## 1. Règle de parentage

### Surfaces opaques

Parent :

```text
M_GrimrockSurface_Master
```

Exemples :

```text
MI_Wall_Stone_01
MI_Wall_Stone_02
MI_Wall_Stone_03
MI_Wall_Stone_04
MI_Wall_Stone_05
MI_Floor_Stone_01
MI_Floor_Stone_02
MI_Ceil_Wood_01
MI_Ceil_Stones_01
MI_Wood_02
MI_Button_01
MI_Support_01
MI_Support_02
MI_WallPanel_01
```

### Surfaces masked

Parent :

```text
M_GrimrockSurface_Masked_Master
```

Exemples :

```text
MI_FloorBloodStain_01
MI_FloorCarpet_01
MI_FloorMoss_01
MI_FloorRoots_01
```

### Cas dédié / effet magique

Parent :

```text
M_FloorRuneCircle_Additive
```

Exemple :

```text
MI_FloorRuneCircle_Blue
```

---

## 2. Migration des opaques

Pour chaque Material Instance opaque :

```text
1. Parent = M_GrimrockSurface_Master.
2. BaseColorTexture = T_xxx_BC.
3. NormalTexture = T_xxx_N.
4. ORMTexture = T_xxx_ORM si disponible.
5. UseORMTexture = true si ORM existe, sinon false.
6. MetallicOverride = 0.0 pour pierre/bois/tissu/os.
7. MetallicOverride = 1.0 pour métal uniforme sans texture metallic.
8. Ajuster BaseColorStrength / DesaturationAmount selon ambiance donjon.
9. Ajuster NormalStrength si le relief est trop fort.
10. Tester sous lumière faible.
```

Réglages de départ utiles :

```text
Pierre mur :
BaseColorStrength = 0.55 à 0.75
DesaturationAmount = 0.10 à 0.25
NormalStrength = 0.7 à 0.9
AO_Strength = 0.8 à 1.0
RoughnessMultiplier = 1.0 à 1.2
MetallicOverride = 0.0

Bois :
BaseColorStrength = 0.75 à 0.95
NormalStrength = 0.6 à 0.9
RoughnessOverride ou RoughnessMultiplier = 0.6 à 0.85
MetallicOverride = 0.0

Métal :
BaseColorStrength = 0.7 à 1.0
NormalStrength = 0.6 à 1.0
RoughnessOverride = 0.25 à 0.65
MetallicOverride = 1.0 si pas de metallic texture
```

---

## 3. Migration des masked

Pour chaque Material Instance masked :

```text
1. Parent = M_GrimrockSurface_Masked_Master.
2. BaseColorTexture = T_xxx_RGBA si alpha inclus.
3. UseAlphaFromBaseColor = true.
4. UseOpacityMaskTexture = false.
5. Source Alpha Detected = true dans la texture UE5.
6. UseNormalTexture = false au début, sauf tapis ou relief utile.
7. UseORMTexture = false au début.
8. UseRoughnessOverride = true.
9. UseMetallicOverride = true, MetallicOverride = 0.0.
10. Ajuster OpacityMaskClip / Bias.
```

---

## 4. Instances décoratives validées comme candidates masked

```text
MI_FloorBloodStain_01
MI_FloorCarpet_01
MI_FloorMoss_01
MI_FloorRoots_01
```

Leur texture principale doit être une `RGBA` si l’alpha est intégré :

```text
T_Deco_FloorBloodStain_01_RGBA
T_Deco_FloorCarpet_01_RGBA
T_Deco_FloorMoss_01_RGBA
T_Deco_FloorRoots_01_RGBA
```

---

## 5. Points de nommage corrigés / à surveiller

Corrections réalisées pendant la restructuration :

```text
Moos_01      -> Moss_01
Decoration   -> Decorations
Ceil         -> Ceiling
T_Meta_02_*  -> T_Metal_02_*
```

Points à surveiller :

```text
Glyphe / Glyph : choisir une convention si nécessaire.
Door / Doors : ne pas dupliquer les deux conventions dans la même zone.
Wall / Walls, Floor / Floors : garder cohérent avec l’existant.
```

---

## 6. Checklist après renommage / déplacement

Dans UE5 :

```text
1. Déplacer / renommer dans Content Browser uniquement.
2. Save All.
3. Content/GrimrockPrototype -> Fix Up Redirectors in Folder.
4. Save All.
5. Ouvrir les Material Instances modifiées.
6. Vérifier les textures assignées.
7. Ouvrir Reference Viewer si doute.
8. Tester en PIE.
9. Fermer / rouvrir l’éditeur si nécessaire.
10. Commit Git.
```

---

## 7. Checklist visuelle

### Surface opaque

```text
pas trop claire
pas trop saturée
normal lisible mais pas exagérée
roughness cohérente
metallic correct
pas de texture manquante
```

### Surface masked

```text
pas de carré noir
pas de damier
pas de halo clair excessif
bords lisibles à distance
mousse / sang / racines intégrés au sol
pas de scintillement excessif
```

### Runtime Grimrock

Tester avec :

```text
caméra proche du mur
plafond visible
torche ou lumière faible
PIE runtime
rotation sur place
déplacement case par case
```

---

## 8. Audit recommandé avec Codex

Prompt utile :

```text
Analyser uniquement Content/GrimrockPrototype/Art.

Objectifs :
1. lister les Material Masters ;
2. lister les Material Instances et leur parent ;
3. identifier les instances opaques qui doivent utiliser M_GrimrockSurface_Master ;
4. identifier les instances masked qui doivent utiliser M_GrimrockSurface_Masked_Master ;
5. signaler les textures placées dans un dossier Materials ou Instances ;
6. signaler les incohérences de nommage restantes ;
7. ne modifier aucun fichier.

Produire un tableau :
Asset path | Type | Parent actuel | Parent recommandé | Problème éventuel | Risque.
```

---

## 9. Ordre de travail recommandé après migration

```text
1. Valider tous les murs opaques.
2. Valider sols et plafonds.
3. Valider portes et objets interactifs.
4. Valider les quatre décorations masked : Moss, Carpet, Blood, Roots.
5. Garder RuneCircle sur son master additive.
6. Supprimer les anciens masters seulement après Reference Viewer.
7. Nettoyer les redirectors.
8. Documenter les nouvelles conventions si elles changent.
```
