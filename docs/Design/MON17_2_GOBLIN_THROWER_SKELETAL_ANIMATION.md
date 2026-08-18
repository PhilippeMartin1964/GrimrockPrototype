# MON17.2 — Gobelin lanceur — Skeletal Mesh / Skeleton / AnimBP

Statut : **EN COURS — contrat C++ validé 1/1, assets UE5.5.4 à intégrer et valider**  
Référence MON17.1 validée : `2e00f67accbdb01763cb1a3a5a5771350c3884a2`

## 1. Objectif

MON17.2 donne au **Gobelin lanceur** (`MON_GoblinThrower`) sa représentation squelettique propre sans créer de pipeline visuel ou d'animation parallèle au Rat Géant.

Le résultat attendu est :

```text
DA_MON_GoblinThrower
    ├── MonsterActorClass -> BP_MON_GoblinThrower
    ├── SkeletalMesh      -> SK_GoblinThrower
    ├── AnimationClass    -> ABP_MON_GoblinThrower_C
    ├── VisualScale
    └── VisualOffset
             ↓
AGridMonsterActor / BP_MON_GoblinThrower
             ↓
UGridMonsterAnimInstance
             ↓
Skeleton compatible + AnimBP
```

L'attaque de couteau, son montage, le projectile et l'impact restent hors MON17.2 et appartiennent à MON17.3.

## 2. Audit du pipeline existant

Le pipeline visuel est déjà générique.

`UGridMonsterDefinitionAsset` possède déjà :

```text
SkeletalMesh
AnimationClass
VisualScale
VisualOffset
MonsterActorClass
```

`AGridMonsterActor::ApplyDefinitionVisuals()` charge le `SkeletalMesh`, applique `VisualOffset` / `VisualScale` et affecte `AnimationClass` au `USkeletalMeshComponent`.

`AGridEditorPreviewObjectActor::InitializeMonsterPreviewObject()` applique le même contrat à l'aperçu editor-only.

Le warning observé à la fin de MON17.1 :

```text
[GridMonsterSpawn] Preview skipped ... Definition=MON_GoblinThrower Reason=MissingSkeletalMesh
```

est donc le comportement attendu tant que `DA_MON_GoblinThrower.SkeletalMesh` n'est pas renseigné.

Aucune classe C++ `GoblinThrowerActor`, aucun composant visuel Gobelin spécifique et aucun second système d'animation ne sont nécessaires.

## 3. Contrat Animation Blueprint commun à tous les monstres

`UGridMonsterAnimInstance` est le pont natif commun. Un Animation Blueprint de monstre doit en dériver afin de recevoir les signaux gameplay existants :

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

L'Animation Blueprint ne décide jamais du déplacement logique, de la cible, du résultat d'une attaque ni du coût en PA.

MON17.2 impose donc au Gobelin :

1. un `USkeletalMesh` possédant un `USkeleton` valide ;
2. un `ABP_MON_GoblinThrower` dérivé de `UGridMonsterAnimInstance` ;
3. un Skeleton cible identique ou compatible avec celui de `SK_GoblinThrower` ;
4. aucune Root Motion pour le déplacement de grille.

## 4. Arborescence UE recommandée

Pour rester symétrique avec `RatGiant` :

```text
Content/GrimrockPrototype/Monsters/GoblinThrower/
├── Animation/
│   ├── ABP_MON_GoblinThrower
│   ├── A_GoblinThrower_Idle        // si disponible
│   └── A_GoblinThrower_Walk
├── Blueprints/
│   └── BP_MON_GoblinThrower
├── Data/
│   └── DA_MON_GoblinThrower
├── Materials/
├── Meshes/
│   ├── SKEL_GoblinThrower
│   └── SK_GoblinThrower
└── Textures/
```

Le `DA_MON_GoblinThrower` créé en MON17.1 doit être déplacé dans `Data/` si nécessaire avec les outils de déplacement/redirector d'Unreal, jamais par déplacement brut dans l'explorateur Windows.

## 5. Import Skeletal Mesh / Skeleton

Importer le modèle Gobelin depuis sa source réelle dans :

```text
/Game/GrimrockPrototype/Monsters/GoblinThrower/Meshes
```

Noms cibles :

```text
SK_GoblinThrower
SKEL_GoblinThrower
```

Contrôles avant de poursuivre :

- le mesh est bien un **Skeletal Mesh**, pas un Static Mesh ;
- le Skeleton est présent et assigné ;
- les matériaux/textures sont correctement rattachés ;
- le Gobelin est debout dans l'axe attendu du projet ;
- l'échelle d'import ne doit pas être compensée par un scale d'Actor runtime ;
- le mesh ne possède pas de Root Motion nécessaire à son déplacement logique ;
- aucune animation d'attaque n'est obligatoire à ce stade.

`VisualScale` et `VisualOffset` de la DataAsset servent uniquement à l'ajustement visuel dans la case.

## 6. Animation Blueprint minimal

Créer un **Animation Blueprint** nommé :

```text
ABP_MON_GoblinThrower
```

avec :

```text
Target Skeleton = SKEL_GoblinThrower
Parent Class    = GridMonsterAnimInstance / UGridMonsterAnimInstance
```

Le graph minimal de MON17.2 doit seulement pouvoir représenter :

```text
Idle
  ↕ bIsMoving
Walk
```

`A_GoblinThrower_Walk` doit être une animation sur place. Une vraie animation `A_GoblinThrower_Idle` est préférable ; si elle n'est pas encore disponible, une pose de référence temporaire peut suffire pour valider le câblage, mais MON17.2 ne sera clos qu'avec une présentation visuelle acceptable en PIE.

Ne pas ajouter dans l'AnimBP :

- sélection de cible ;
- calcul de portée ;
- pathfinding ;
- consommation de PA ;
- logique `RangedKeeper` ;
- résolution du projectile.

## 7. Blueprint Actor gameplay

Le Gobelin ne doit pas utiliser directement la classe native `AGridMonsterActor` en production, car le pipeline MON13 attend aussi les composants gameplay ajoutés au Blueprint monstre.

Créer :

```text
/Game/GrimrockPrototype/Monsters/GoblinThrower/Blueprints/BP_MON_GoblinThrower
```

Parent :

```text
AGridMonsterActor
```

Ajouter les mêmes composants gameplay génériques nécessaires que pour le Rat Géant :

```text
GridMonsterMovementComponent   -> composant MonsterMovement
GridMonsterBehaviorComponent   -> composant MonsterBehavior
```

`MonsterCombat`, `MonsterDeath`, audio, VFX et le SkeletalMeshComponent sont déjà fournis nativement par `AGridMonsterActor`.

Ne pas coder de comportement Gobelin dans l'Event Graph. `RangedKeeper` restera une donnée de `DA_MON_GoblinThrower` et son planner sera traité en MON17.4.

## 8. Raccordement de DA_MON_GoblinThrower

Dans `DA_MON_GoblinThrower`, renseigner :

```text
MonsterActorClass = BP_MON_GoblinThrower
SkeletalMesh      = SK_GoblinThrower
AnimationClass    = ABP_MON_GoblinThrower_C
VisualScale       = (à ajuster visuellement)
VisualOffset      = (à ajuster visuellement)
```

Ne modifier ni `MonsterId`, ni les statistiques MON17.1, ni `PrimaryAIProfile=RangedKeeper` pendant cette étape.

## 9. Test automatisé MON17.2

Filtre :

```text
Grimrock.Monsters.MON17.2
```

Le premier test `PresentationBridgeContract` utilise les assets Rat Géant déjà validés comme référence de production et exige :

- chargement d'un Skeletal Mesh réel ;
- Skeleton non nul ;
- chargement de l'Animation Blueprint réel ;
- parent `UGridMonsterAnimInstance` ;
- `IAnimClassInterface` disponible ;
- Skeleton cible de l'AnimBP non nul ;
- compatibilité entre Skeleton du mesh et Skeleton de l'AnimBP.

### Validation UE5.5.4 — 18 août 2026

Résultat fourni après compilation locale :

```text
Grimrock.Monsters.MON17.2.PresentationBridgeContract  Success
```

Bilan automatisé MON17.2 actuel : **1/1 Success**.

Ce test protège le **contrat générique**. Un test strict sur les assets Gobelin sera ajouté/activé lorsque les `.uasset` Gobelin auront réellement été créés et versionnés ; MON17.2 ne simule pas des assets binaires inexistants.

## 10. Validation manuelle UE5.5.4

### Aperçu éditeur

Après affectation du mesh dans `DA_MON_GoblinThrower`, utiliser `Reload Current` ou reconstruire l'aperçu.

Résultats attendus :

- plus aucun `Reason=MissingSkeletalMesh` pour le SpawnId du Gobelin ;
- le Gobelin est visible hors PIE ;
- il est centré dans sa case après ajustement `VisualOffset` ;
- sa taille est cohérente avec la grille après ajustement `VisualScale` ;
- son `InitialFacing` est visuellement respecté ;
- sélection et survol continuent à fonctionner.

### PIE

Lancer le niveau de test et rechercher `[GridMonsterSpawn]`.

Résultats attendus :

```text
DefinitionId=MON_GoblinThrower
Class=BP_MON_GoblinThrower_C
Spawned Monsters=... Failures=0
```

Contrôler dans le World Outliner pendant PIE :

- un seul Actor pour le SpawnId ;
- `MonsterDefinition = DA_MON_GoblinThrower` ;
- `SkeletalMeshComponent = SK_GoblinThrower` ;
- Anim Instance de classe `ABP_MON_GoblinThrower_C` ;
- présence de `MonsterMovement`, `MonsterBehavior` et `MonsterCombat` ;
- pas de `PresentationWarning` concernant ce Gobelin.

Le déplacement peut encore utiliser la stratégie actuelle ; le maintien de distance spécifique à `RangedKeeper` n'est pas attendu avant MON17.4.

## 11. Critères de clôture MON17.2

MON17.2 pourra être marqué CLOS lorsque :

- le test `Grimrock.Monsters.MON17.2` est vert sous UE5.5.4 ;
- `SK_GoblinThrower` et `SKEL_GoblinThrower` existent ;
- `ABP_MON_GoblinThrower` cible le bon Skeleton et dérive de `UGridMonsterAnimInstance` ;
- `BP_MON_GoblinThrower` contient les composants gameplay génériques requis ;
- `DA_MON_GoblinThrower` référence les trois assets de production ;
- le preview éditeur est visible sans `MissingSkeletalMesh` ;
- le Gobelin apparaît correctement en PIE sans `PresentationWarning` ;
- aucun comportement d'attaque projectile ou de kiting spécifique n'a été introduit prématurément.

## 12. Hors périmètre

- animation/montage `Attack_ThrowKnife` : MON17.3 ;
- projectile couteau et impact : MON17.3 ;
- choix d'attaque à distance et cooldown runtime : MON17.3 ;
- planner de maintien de distance / repositionnement : MON17.4 ;
- patrouille, perception et alarme de production : MON17.5 ;
- loot / XP final : MON17.6 ;
- équilibrage final : MON17.7.
