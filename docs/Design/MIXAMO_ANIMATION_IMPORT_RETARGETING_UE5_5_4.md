# Import et retargeting d'une animation Mixamo — UE5.5.4

## 1. But

Cette procédure décrit le workflow réutilisable validé pour importer une animation Mixamo dans GrimrockPrototype sans créer un nouveau personnage ou un nouveau Skeleton par animation.

Cas déjà utilisés :

- Throw / attaque du GoblinThrower ;
- Death ;
- Hurt / Hit Reaction (`Pain Gesture.fbx`).

Le principe est toujours :

```text
FBX animation Mixamo
  -> Skeleton source Mixamo commun
  -> IK_Mixamo
  -> RTG_Mixamo_To_GoblinThrower
  -> IK_GoblinThrower
  -> Goblin_Mixamo_TPose
  -> Animation Sequence GoblinThrower
  -> AnimMontage si nécessaire
```

## 2. Import FBX — animation seulement

Dans le Content Browser, importer le `.fbx` dans le dossier de travail Mixamo déjà utilisé par le GoblinThrower.

Dans `Import Content` / `InterchangeGenericAssetsPipeline` :

```text
Common
Use Source Name for Asset         = true
Offset Translation                = 0 / 0 / 0
Offset Rotation                   = 0 / 0 / 0
Offset Uniform Scale              = 1.0

Common Skeletal Meshes and Animations
Import Only Animations            = true
Skeleton                          = goblin_d_shareyko_Skeleton
Import Meshes in Bone Hierarchy   = false
Use T0As Ref Pose                 = false

Skeletal Meshes
Import Skeletal Meshes            = false
Import Morph Targets              = false
Create Physics Asset              = false

Animations
Import Animations                 = true
Import Bone Tracks                = true
Animation Length                  = Source Timeline
Use 30Hz to Bake Bone Animation   = false
Custom Bone Animation Sample Rate = 0
Snap to Closest Frame Boundary    = false

Materials
Import Materials                  = false

Textures
Import Textures                   = false
```

Les options Static Mesh / Collision / Nanite visibles dans le pipeline n'ont pas d'effet utile lorsque l'import est strictement `Import Only Animations` et que les imports de meshes sont désactivés.

### Erreur à éviter

Ne pas sélectionner le Skeleton créé par un précédent FBX indépendant, par exemple un Skeleton portant le nom d'une animation Death.

Le Skeleton source commun validé est :

```text
goblin_d_shareyko_Skeleton
```

Le but est d'éviter :

```text
1 FBX -> 1 nouveau Skeleton
```

et de conserver au contraire un pipeline Mixamo source stable.

## 3. Contrôle de l'Animation Sequence source

Après import :

- ouvrir l'Animation Sequence ;
- vérifier le Skeleton source ;
- jouer l'animation complète ;
- vérifier qu'elle correspond à l'action attendue ;
- vérifier qu'il n'existe pas de dérive spatiale importante ;
- laisser `Enable Root Motion` désactivé.

Pour GrimrockPrototype, la grille reste autoritaire. Une animation ne doit pas déplacer réellement le monstre entre les cellules.

De petits mouvements de bassin, pieds ou torse sont acceptables tant que le personnage reste globalement sur place.

## 4. Retargeting

Ouvrir :

```text
RTG_Mixamo_To_GoblinThrower
```

Contrôler :

```text
Source IK Rig Asset    = IK_Mixamo
Source Preview Mesh    = goblin_d_shareyko
Target IK Rig Asset    = IK_GoblinThrower
Target Preview Mesh    = SK_GoblinThrower
Current Retarget Pose  = Goblin_Mixamo_TPose
```

Ne pas recréer d'IK Rig, de Retargeter ou de pose pour chaque animation.

Désactiver `Play Ref Pose`, sélectionner l'Animation Sequence source dans l'Asset Browser et vérifier visuellement le résultat sur `SK_GoblinThrower`.

Si la prévisualisation est correcte :

```text
Export Selected Animations
```

Puis renommer selon la convention :

```text
A_GoblinThrower_<Action>
```

Exemples :

```text
A_GoblinThrower_Death
A_GoblinThrower_Hurt
```

## 5. Contrôle de l'animation retargetée

Ouvrir l'asset exporté et vérifier :

- Skeleton = `SKEL_GoblinThrower` ;
- Preview Mesh = `SK_GoblinThrower` ;
- Root Motion = disabled ;
- Loop désactivé pour les actions one-shot ;
- absence de dérive ou rotation permanente indésirable ;
- lecture correcte du début à la fin.

## 6. Création d'un AnimMontage

Lorsque le runtime joue cette action sous forme de montage :

1. clic droit sur `A_GoblinThrower_<Action>` ;
2. `Create Anim Montage` ;
3. renommer :

```text
AM_GoblinThrower_<Action>
```

Le pipeline GoblinThrower utilise :

```text
DefaultGroup.DefaultSlot
```

Ne pas créer un nouveau Slot sans besoin architectural démontré.

## 7. Auto Blend Out selon le type d'action

### Hurt / Hit Reaction

```text
Enable Auto Blend Out = true
```

Le personnage doit revenir naturellement au graphe normal après la réaction.

Valeurs validées comme point de départ pour `AM_GoblinThrower_Hurt` :

```text
Blend In  = 0.25 s
Blend Out = 0.25 s
```

Ces valeurs peuvent être affinées après validation PIE sans modifier le contrat C++.

### Death

```text
Enable Auto Blend Out = false
```

La dernière pose reste tenue comme cadavre. Dans MON17.8.4, la durée observée de `A_GoblinThrower_Death` était `3.6333333 s` et `DeathExpectedDuration` a été configuré en conséquence.

### Attack

Réutiliser le Slot et la configuration déjà validés pour l'attaque concernée. La synchronisation gameplay d'une attaque reste pilotée par le système de combat, pas par le simple fait que le montage se termine.

## 8. Nommage recommandé

```text
Source FBX                 fichier externe, nom Mixamo libre
Animation source Mixamo    nom issu de l'import
Animation retargetée       A_GoblinThrower_<Action>
Montage                     AM_GoblinThrower_<Action>
```

Le nom du fichier source n'a pas besoin de suivre la nomenclature Unreal.

Exemple Hurt :

```text
Pain Gesture.fbx
  -> animation Mixamo source
  -> A_GoblinThrower_Hurt
  -> AM_GoblinThrower_Hurt
```

## 9. Checklist avant utilisation runtime

```text
[ ] Import Only Animations = true
[ ] Skeleton source = goblin_d_shareyko_Skeleton
[ ] Import Skeletal Meshes = false
[ ] Import Meshes in Bone Hierarchy = false
[ ] Import Materials = false
[ ] Import Textures = false
[ ] animation source lisible et stable
[ ] RTG_Mixamo_To_GoblinThrower utilisé
[ ] Goblin_Mixamo_TPose sélectionnée
[ ] animation retargetée sur SKEL_GoblinThrower
[ ] Root Motion disabled
[ ] nom A_GoblinThrower_<Action>
[ ] montage AM_GoblinThrower_<Action> si requis
[ ] Slot DefaultGroup.DefaultSlot
[ ] Auto Blend Out configuré selon l'action
[ ] validation visuelle PIE avant clôture
```

## 10. Règle de projet

Les `.uasset` sont authorés et sauvegardés depuis Unreal Editor. Les changements C++/Markdown ne doivent jamais tenter de fabriquer ou modifier directement ces assets binaires.
