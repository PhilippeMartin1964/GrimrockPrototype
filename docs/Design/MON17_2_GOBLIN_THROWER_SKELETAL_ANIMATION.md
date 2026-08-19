# MON17.2 — Gobelin lanceur — Skeletal Mesh / Skeleton / AnimBP

Statut : **VALIDÉ ET CLOS sous UE5.5.4**  
Référence MON17.1 : `2e00f67accbdb01763cb1a3a5a5771350c3884a2`  
Correction générique d'orientation : `c8ebcb4ab850bf7be01e8c81203b56309c2fec3d`

## 1. Objectif atteint

MON17.2 donne au **Gobelin lanceur** (`MON_GoblinThrower`) une représentation squelettique complète et fonctionnelle en réutilisant le pipeline monstre existant. Aucun Actor C++ spécifique au Gobelin et aucun second système d'animation n'ont été introduits.

Le contrat de production validé est :

```text
DA_MON_GoblinThrower
    ├── MonsterActorClass     -> BP_MON_GoblinThrower_C
    ├── SkeletalMesh          -> SK_GoblinThrower
    ├── AnimationClass        -> ABP_MON_GoblinThrower_C
    ├── VisualScale           -> (1,1,1)
    ├── VisualOffset          -> (0,0,0)
    └── VisualRotationOffset  -> (Pitch=0, Yaw=-90, Roll=0)
             ↓
BP_MON_GoblinThrower / AGridMonsterActor
             ↓
UGridMonsterAnimInstance
             ↓
SKEL_GoblinThrower + ABP_MON_GoblinThrower
```

L'attaque `Attack_ThrowKnife`, son exécution projectile et le maintien tactique de distance restent volontairement hors MON17.2.

## 2. Assets de production validés

Le Gobelin provient de l'asset tiers **CGTrader Goblin_Bomber**, fourni comme projet Unreal Engine 5.5 natif puis réorganisé sous le domaine du projet :

```text
/Game/GrimrockPrototype/Monsters/GoblinThrower/
├── Animation/
│   ├── ABP_MON_GoblinThrower
│   ├── A_GoblinThrower_Idle
│   └── A_GoblinThrower_Walk
├── Blueprints/
│   └── BP_MON_GoblinThrower
├── Data/
│   └── DA_MON_GoblinThrower
├── Materials/
├── Meshes/
│   ├── SK_GoblinThrower
│   ├── SKEL_GoblinThrower
│   ├── PHYS_GoblinThrower
│   └── SM_Bomb
└── Textures/
```

`A_GoblinThrower_Walk` est une animation **in-place** : le mesh reste physiquement sur place et `Enable Root Motion` est désactivé. La position logique et le déplacement restent donc autoritaires côté grille.

## 3. Pipeline visuel générique réutilisé

`UGridMonsterDefinitionAsset` fournit la présentation par données :

```text
SkeletalMesh
AnimationClass
VisualScale
VisualOffset
VisualRotationOffset
MonsterActorClass
```

`AGridMonsterActor::ApplyDefinitionVisuals()` charge et applique ces données au `USkeletalMeshComponent` runtime.

`AGridEditorPreviewObjectActor::InitializeMonsterPreviewObject()` applique le même contrat à l'aperçu editor-only.

`UGridMonsterAnimInstance` reste l'unique pont natif commun aux Animation Blueprints de monstres. Il expose notamment :

```text
MonsterState
bIsMoving
bIsTurning
bIsDead
MoveAlpha
TurnDirection
CurrentHealth
MaxHealth
CurrentCell
Facing
```

L'Animation Blueprint ne décide jamais de la position logique, du pathfinding, de la cible, du coût en PA ou du résultat d'une attaque.

## 4. ABP_MON_GoblinThrower

Configuration validée :

```text
Target Skeleton = SKEL_GoblinThrower
Parent Class    = UGridMonsterAnimInstance
```

Le graphe MON17.2 fournit le comportement visuel minimal :

```text
Idle
  ↕ bIsMoving
Walk
```

L'animation est visible et fonctionnelle en PIE. Aucune logique `RangedKeeper`, projectile ou combat spécifique au Gobelin n'est placée dans l'AnimBP.

## 5. BP_MON_GoblinThrower

Configuration validée :

```text
Parent = AGridMonsterActor
```

Le Blueprint est effectivement choisi par `DA_MON_GoblinThrower.MonsterActorClass` et utilisé par le pipeline `MonsterSpawn` en PIE.

Il réutilise les composants génériques requis par le pipeline monstre, notamment `MonsterMovement` et `MonsterBehavior`. `MonsterCombat`, `MonsterDeath`, le `SkeletalMeshComponent`, l'audio et les VFX restent fournis par la classe native existante.

Aucun comportement Gobelin spécifique n'a été ajouté à l'Event Graph.

## 6. DA_MON_GoblinThrower — présentation finale MON17.2

Valeurs validées :

```text
MonsterActorClass     = BP_MON_GoblinThrower_C
SkeletalMesh          = SK_GoblinThrower
AnimationClass        = ABP_MON_GoblinThrower_C
VisualScale           = (1,1,1)
VisualOffset          = (0,0,0)
VisualRotationOffset  = (0,-90,0)
```

Le Gobelin est correctement posé au sol avec `VisualScale = 1` et ne nécessite aucun offset de position.

## 7. Correction générique d'orientation — VisualRotationOffset

Le Skeletal Mesh importé possède un axe local différent de l'axe visuel attendu par le système de grille :

```text
InitialFacing = North
```

apparaissait initialement visuellement orienté vers l'ouest.

Le mesh n'a volontairement pas été corrigé dans Blender. Un paramètre générique a été ajouté à `UGridMonsterDefinitionAsset` :

```cpp
FRotator VisualRotationOffset = FRotator::ZeroRotator;
```

Ce paramètre est appliqué :

- dans `AGridMonsterActor::ApplyDefinitionVisuals()` ;
- dans `AGridEditorPreviewObjectActor::InitializeMonsterPreviewObject()`.

Le principe est important :

```text
Facing                = orientation logique autoritaire sur la grille
VisualRotationOffset  = correction locale propre au mesh
```

Pour le Gobelin :

```text
Pitch = 0
Yaw   = -90
Roll  = 0
```

La correction a été validée dans UE5.5.4 : `InitialFacing=North` apparaît désormais visuellement North, en aperçu éditeur comme en runtime.

Commit de la correction :

```text
c8ebcb4ab850bf7be01e8c81203b56309c2fec3d
Add generic monster visual rotation offset
```

## 8. Tests automatisés MON17.2

Filtre :

```text
Grimrock.Monsters.MON17.2
```

Tests présents :

```text
Grimrock.Monsters.MON17.2.PresentationBridgeContract
Grimrock.Monsters.MON17.2.VisualRotationOffsetContract
```

`PresentationBridgeContract` a été exécuté localement sous UE5.5.4 avec résultat :

```text
Success
```

Il protège le contrat générique Mesh / Skeleton / AnimBP : mesh réel, Skeleton valide, parent `UGridMonsterAnimInstance`, Skeleton cible de l'AnimBP et compatibilité entre Skeletons.

`VisualRotationOffsetContract` a été ajouté avec la correction `c8ebcb4`. Il protège la valeur par défaut nulle et la possibilité de stocker une correction locale de yaw. Aucun résultat d'exécution séparé de ce second test n'a été fourni dans le journal de clôture ; la correction elle-même a en revanche été validée manuellement dans UE5.5.4 sur le Gobelin réel.

## 9. Validation aperçu éditeur

Après `Reload Current`, les contrôles suivants sont validés :

- Gobelin visible hors PIE : **OK** ;
- `MissingSkeletalMesh` disparu : **OK** ;
- `VisualScale=(1,1,1)` : **OK** ;
- `VisualOffset=(0,0,0)` : **OK** ;
- pieds au sol : **OK** ;
- `InitialFacing=North` visuellement correct avec `VisualRotationOffset=(0,-90,0)` : **OK** ;
- le même contrat de correction est utilisé par le preview et le runtime : **OK**.

## 10. Validation PIE / intégration gameplay

Spawn observé :

```text
[GridMonsterSpawn]
DefinitionId=MON_GoblinThrower
Class=/Game/GrimrockPrototype/Monsters/GoblinThrower/Blueprints/BP_MON_GoblinThrower.BP_MON_GoblinThrower_C
Cell=(28,24)
Facing=North
RuntimeLevel=Into_The_Dark
```

Le Gobelin apparaît animé en PIE et participe au pipeline gameplay existant.

Le combat automatique se déclenche correctement :

```text
[MON14.1] Automatic combat started
Reason=PatrolVision
```

Son initiative de base observée est `12`, conformément à `DA_MON_GoblinThrower`.

Un combat réel a validé le cycle complet :

```text
HP 10 -> 6 -> 2 -> 0
```

Puis :

- mort détectée ;
- occupation libérée ;
- événement `MonsterDied` émis ;
- victoire détectée ;
- récompense XP `125` appliquée.

Logs significatifs :

```text
[GridExperience] Monster=BP_MON_GoblinThrower_C_0 ... Reward=125 Applied=125

[GridMonsterDeath] Commit Monster=BP_MON_GoblinThrower_C_0
Cell=(28,24)
OccupancyReleased=true

[GridCombat] Type=Victory Message="Victoire."
```

Aucun des diagnostics suivants n'a été observé :

```text
MissingSkeletalMesh
PresentationWarning
erreur de spawn
```

## 11. Comportement volontairement non corrigé en MON17.2

Lorsque le Gobelin reçoit son tour à distance 1, le journal montre :

```text
MonsterTurnStarted
→ EndingRound
```

Ce comportement est attendu à ce stade car `Attack_ThrowKnife` exige :

```text
MinRangeCells = 2
```

MON17.2 ne doit pas contourner cette règle.

La séparation de responsabilités reste :

```text
MON17.3 = exécuter Attack_ThrowKnife lorsque la position actuelle est déjà valide
MON17.4 = RangedKeeper, maintien de distance, recul et choix de case
```

## 12. Clôture MON17.2

MON17.2 est **VALIDÉ ET CLOS** parce que :

- le contrat générique Mesh / Skeleton / AnimBP a été validé sous UE5.5.4 ;
- `SK_GoblinThrower` et `SKEL_GoblinThrower` sont intégrés ;
- `A_GoblinThrower_Idle` et `A_GoblinThrower_Walk` sont disponibles ;
- la marche est in-place, sans Root Motion ;
- `ABP_MON_GoblinThrower` cible le bon Skeleton et utilise `UGridMonsterAnimInstance` ;
- `BP_MON_GoblinThrower` est utilisé réellement par le pipeline runtime ;
- `DA_MON_GoblinThrower` référence les assets de production ;
- `VisualScale`, `VisualOffset` et `VisualRotationOffset` sont validés ;
- l'aperçu éditeur est fonctionnel ;
- le spawn et l'animation PIE sont fonctionnels ;
- le Gobelin entre normalement dans le combat, meurt et attribue son XP ;
- aucun `MissingSkeletalMesh` ni `PresentationWarning` n'a été observé ;
- aucune logique MON17.3/MON17.4 n'a été introduite prématurément.

## 13. Suite

Le travail autoritaire suivant est :

```text
MON17.3 — Distinct Attack Set — Attack_ThrowKnife
```

MON17.3 doit rendre l'attaque projectile effectivement exécutable quand le Gobelin est déjà à portée et avec une ligne de vue valide. Le repositionnement et le maintien de distance restent réservés à MON17.4.
