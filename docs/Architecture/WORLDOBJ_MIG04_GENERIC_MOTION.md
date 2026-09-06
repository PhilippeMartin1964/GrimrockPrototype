# WORLDOBJ-MIG04 — Motion générique autoritaire

Statut : **clôture technique proposée — validation UE5.5.4 requise**  
Date : **6 septembre 2026**

## Objectif

`MovingParts[].Motion` devient l’unique autorité géométrique et temporelle pour l’animation des mécanismes du monde.

Le runtime ne décrit plus comment un type particulier se déplace. Il manipule uniquement un état logique normalisé :

```text
0 = Closed / Released / Off
1 = Open / Pressed / On
```

La définition visuelle porte le déplacement réel :

```text
MovingPart
├── Mesh
├── LocalTransform
└── Motion
    ├── Type      Rotation | Translation
    ├── Axis      X | Y | Z
    ├── Pivot
    ├── Amount
    └── Duration
```

## Runtime unifié

Les acteurs suivants utilisent le même contrat :

- `AGridDoorActor` ;
- `AGridButtonActor` ;
- `AGridLeverActor` ;
- `AGridPressurePlateActor` ;
- `AGridPitTrapdoorActor`.

Chaque acteur conserve uniquement sa machine d'état logique et ses règles gameplay. La géométrie est appliquée par `AGridMechanismActor::ApplyMovingPartMotionAlpha()` ou `ApplyAllMovingPartMotionsAlpha()`.

Une porte verticale, coulissante ou battante ne nécessite donc plus de code géométrique différent : seul `Motion` change.

## Données spécialisées retirées de l'authoring

Les anciens champs suivants ne sont plus des données d'authoring sérialisables :

```text
DoorAnimation.OpenHeight
DoorAnimation.MoveDuration
LeverAnimation.LeverOffPitch
LeverAnimation.LeverOnPitch
LeverAnimation.ToggleDuration
ButtonAnimation.ButtonPressDistance
ButtonAnimation.ButtonPressDuration
ButtonAnimation.ButtonReleaseDuration
PressurePlateAnimation.ReleasedHeightAboveFloor
PressurePlateAnimation.PressedHeightAboveFloor
PressurePlateAnimation.MoveDuration
PitAnimation.LeftHingeLocation
PitAnimation.RightHingeLocation
PitAnimation.OpenAngleDegrees
PitAnimation.MoveDuration
```

Ils restent temporairement déclarés `Transient` uniquement afin que les anciens appels C++ et certains tests historiques puissent encore compiler jusqu'à `WORLDOBJ-MIG09`. Ils ne constituent plus une source de vérité runtime et ne doivent plus être utilisés pour créer de nouveaux contenus.

### Exceptions comportementales conservées

Deux familles de paramètres restent volontairement dans `Behavior` :

- `ButtonAnimation.ButtonHoldTime` : durée logique pendant laquelle le bouton reste enfoncé avant son relâchement ;
- paramètres de chaîne de porte : présence de la chaîne, course de traction et durée de traction.

Ces valeurs décrivent une règle d'interaction/comportement et non la géométrie de la porte ou du bouton.

## Porte

Le chemin de production est :

```text
InitializeMechanismVisuals
    -> copie StaticPart / MovingParts / Motion
InitializeGridObject
    -> résout MoveDuration depuis Motion
SetDoorOpenState
    -> calcule seulement StartAlpha / TargetAlpha
UpdateAnimation
    -> ApplyAllMovingPartMotionsAlpha(alpha)
```

`InitializeDoor(...)` reste provisoirement comme helper de compatibilité pour les anciens tests et appels Blueprint directs. Il peut encore lire la durée transitoire historique afin de préserver ces tests, mais il ne possède plus de second moteur géométrique. Sa suppression/migration physique appartient à `MIG09`.

## Pit

La trappe ne possède plus de géométrie spécialisée :

```text
Part0.Motion.Pivot  = charnière gauche
Part0.Motion.Amount = angle signé gauche
Part1.Motion.Pivot  = charnière droite
Part1.Motion.Amount = angle signé droite
```

Le provisioning `EnsurePitTrapdoorArchetype()` écrit désormais directement ces données dans `MovingParts` lorsqu'une paire complète de volets existe.

## Compatibilité éditeur temporaire

Le panneau Slate historique du Grid Editor contient encore quelques contrôles nommés d'après les anciens champs. Tant que `MIG09` n'a pas purgé ces contrôles, ils ne doivent pas être considérés comme des paramètres autoritaires : les champs géométriques correspondants sont `Transient` et le runtime de production les ignore.

La purge physique de ces contrôles, des helpers de compatibilité et des noms C++ historiques est explicitement repoussée à `WORLDOBJ-MIG09`, après migration/réenregistrement des assets réels (`MIG08`).

## Tests

Le filtre `Grimrock.WorldObjects.MIG04` protège :

- l'absence des anciennes propriétés géométriques dans les acteurs runtime simples ;
- le caractère `Transient` et non éditable des ponts géométriques dans `Behavior` ;
- le maintien en données éditables des vraies règles gameplay (`ButtonHoldTime`, chaîne de porte).

Première tranche MIG04 validée sous UE5.5.4 sur le commit :

```text
4685c28b970973c4e1c7ea0a4d3e571d02124961
WORLDOBJ-MIG04 unify simple mechanisms on generic motion
```

Validation utilisateur :

```text
Filter                 : Grimrock.WorldObjects
Succeeded              : 11
Succeeded with warnings: 1
Failed                 : 0
Process exit code      : 0
```

## Critère de clôture MIG04

MIG04 est clos lorsque la validation UE5.5.4 confirme que :

1. tous les mécanismes de production utilisent `Motion` pour géométrie et durée ;
2. aucun ancien paramètre géométrique spécialisé n'est sérialisable comme donnée d'authoring ;
3. Button conserve uniquement son temps de maintien comme règle gameplay ;
4. la chaîne de porte reste un comportement indépendant de l'animation géométrique de la porte ;
5. les ponts C++ restants sont explicitement transitoires et réservés à la purge MIG09.
