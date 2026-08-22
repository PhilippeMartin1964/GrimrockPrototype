# MON17.8.1 — Goblin Thrower Animation & Death Polish — Audit / Animation Contract

Statut : **AUDIT TERMINÉ — aucune modification C++ ni asset binaire dans cette étape**

Base auditée :

```text
master
fc059add961c7e9a8f079f125eccb34060df72c1
Finalise MON18 assets
```

Le commit `fc059add` est descendant de `ec14c2c` (`Close MON18 magic and spellbook`) et ne modifie que les deux assets UI Spellbook. Il ne change aucun pipeline monstre.

## 1. Conclusion d’architecture

MON17.8 ne doit créer aucun Actor Gobelin ni second système d’animation.

Le pipeline existant est le bon :

```text
DA_MON_GoblinThrower
    -> BP_MON_GoblinThrower / AGridMonsterActor
    -> UGridMonsterAnimInstance
    -> UGridMonsterMovementComponent
    -> UGridMonsterCombatComponent
    -> UGridMonsterDeathComponent
    -> ABP_MON_GoblinThrower
    -> SK_GoblinThrower / SKEL_GoblinThrower
```

La grille reste autoritaire. Les animations et matériaux restent une présentation du gameplay déjà décidé.

Les principaux problèmes MON17.8 sont des trous de présentation :

- Walk joue pendant le déplacement mais n’est pas encore synchronisé précisément avec la progression réelle d’une case ;
- aucune animation Gobelin dédiée Turn/Hurt/Death/Alert n’est actuellement versionnée ;
- le pipeline générique DeathMontage existe déjà mais `DA_MON_GoblinThrower` ne possède pas encore l’animation de mort Mixamo retargetée ;
- après `DeathExpectedDuration`, le code ne conserve ni explicitement la pose finale ni ne lance une dissolution ;
- un mort restauré par MON9 est actuellement forcé visible ;
- aucun contrat de dissolution n’existe dans `UGridMonsterDefinitionAsset` ;
- le test MON17.2 PresentationBridgeContract charge le Rat géant, pas le Gobelin lanceur.

## 2. Pipeline animation actuel exact

### 2.1 Pont natif

`UGridMonsterAnimInstance` reflète depuis `AGridMonsterActor` :

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

L’AnimInstance ne détermine aucune cellule, aucun pathfinding, PA, Hit/Miss, dégâts ou mort.

### 2.2 Mouvement

`UGridMonsterMovementComponent` :

- réserve la cellule cible ;
- interpole l’Actor de centre à centre ;
- utilise `MoveDuration` depuis le MonsterDefinition ;
- appelle `SetMovementAnimationState(true, MoveAlpha)` pendant l’interpolation ;
- commit `CurrentCell` seulement en fin de déplacement ;
- resnappe exactement l’Actor sur le centre de la destination ;
- n’utilise aucun Root Motion ;
- Tick uniquement pendant Move/Turn.

Avec `bUseEaseInOut=true`, la position monde utilise actuellement un alpha visuel EaseInOut, tandis que `MoveAlpha` transmis à l’animation est l’alpha linéaire de temps. Cette différence est à prendre en compte pour la synchronisation des pieds avec la distance effectivement parcourue.

### 2.3 Combat / attaque

`UGridMonsterCombatComponent::StartAttackPresentation()` :

- passe `MonsterState` à `Attacking` ;
- joue audio/VFX ;
- charge `Attack.AttackMontage` ;
- appelle `Montage_Play` ;
- programme séparément la présentation projectile.

Pour `Attack_ThrowKnife`, le contrat validé reste :

```text
A_GoblinThrower_ThrowKnife
AM_GoblinThrower_ThrowKnife
ProjectileSource
ExpectedDuration          = 2.20 s
ImpactTimeSeconds         = 1.00 s
ProjectileTravelDuration  = 0.20 s
LaunchDelay               = 0.80 s
```

Ce chemin ne doit pas être modifié par le polish de locomotion ou de mort.

### 2.4 Blessure

`ApplyAttackResult()` :

- applique les dégâts ;
- si le monstre survit à un dommage réel, passe `MonsterState=Hurt` ;
- déclenche HurtAudio et HurtVFX.

Il n’existe actuellement ni `HurtMontage` générique dans `UGridMonsterDefinitionAsset`, ni animation Hurt GoblinThrower versionnée. Il ne faut donc pas inventer une animation Hurt dans MON17.8 tant qu’un asset exploitable n’est pas fourni.

### 2.5 Mort

Le contrat générique existant est déjà correct côté gameplay :

```text
HP = 0
-> MonsterState = Dead
-> attaque annulée
-> mouvement/réservation annulés
-> occupation libérée
-> collision désactivée
-> loot généré et placé
-> XP attribuée
-> liens MonsterDied exécutés
-> OnMonsterDied diffusé
-> Encounter notifié
-> DeathMontage présentationnel lancé en dernier
```

`DeathMontage` et `DeathExpectedDuration` existent déjà dans `UGridMonsterDefinitionAsset`.

`StartDeathPresentation()` joue le montage si possible puis utilise un timer one-shot. `NotifyDeathPresentationComplete()` ne fait actuellement que remettre `bDeathPresentationActive=false`.

Il n’existe actuellement :

- aucun maintien générique explicite de la pose finale ;
- aucune dissolution ;
- aucun masquage final du Skeletal Mesh.

## 3. Matrice animations / états

| État / événement | Animation attendue | Asset actuel Gobelin | Branché actuellement | Contrat MON17.8 |
|---|---|---|---|---|
| Idle | Idle de base | `A_GoblinThrower_Idle` | Oui, ABP | Conserver |
| Walk | Marche in-place | `A_GoblinThrower_Walk` | Oui, ABP via `bIsMoving` | Synchroniser la phase avec un déplacement de case |
| Turn | Rotation visuelle | Aucun asset dédié | Signal `bIsTurning` / `TurnDirection` existe ; MON3 autorise une pose neutre | Ne pas inventer d’asset ; rotation C++ reste autoritaire |
| Dormant | Repos | Aucun asset distinct | Peut réutiliser Idle | Réutiliser Idle |
| Alert | Réaction d’alerte | Aucun asset distinct | État + Audio/VFX génériques | Pas d’animation distincte sans asset |
| Pursuing | Locomotion de poursuite | Aucun asset distinct | Walk quand `bIsMoving` | Réutiliser locomotion |
| Repositioning | Locomotion tactique | Aucun asset distinct | Walk quand `bIsMoving` | Réutiliser locomotion |
| Attack_ThrowKnife | Lancer | `A_GoblinThrower_ThrowKnife` + `AM_GoblinThrower_ThrowKnife` | Oui | Ne pas modifier le timing validé |
| Hurt | Réaction de blessure | Aucun asset | État + Audio/VFX seulement | Attendre un asset ; extension générique uniquement si nécessaire |
| Death | Chute au sol | Aucun asset versionné actuellement | Pipeline `DeathMontage` déjà présent | Retargeter l’animation Mixamo et l’assigner au contrat existant |
| Idle Variations | Variations ponctuelles | Aucun asset Gobelin versionné | Infrastructure générique MON10 disponible | Ne rien ajouter tant qu’aucun asset n’existe |

## 4. Assets GoblinThrower réellement présents sur master

### Animation

```text
ABP_MON_GoblinThrower
A_GoblinThrower_Idle
A_GoblinThrower_Walk
A_GoblinThrower_ThrowKnife
AM_GoblinThrower_ThrowKnife
```

### Meshes

```text
SK_GoblinThrower
SKEL_GoblinThrower
PHYS_GoblinThrower
SM_Bomb
```

### Blueprint / Data

```text
BP_MON_GoblinThrower
DA_MON_GoblinThrower
```

### Matériaux versionnés

```text
M_Cloth_Bomber
M_Goblin_Bomber
M_Hair_Bomber
```

Les `.uasset` sont stockés via Git LFS. L’audit Git peut confirmer leur présence et leurs noms, mais pas inspecter de manière fiable leur graphe Material ni les paramètres scalaires internes. La présence d’un paramètre de dissolution devra donc être vérifiée dans UE5.5.4 avant toute décision matérielle.

## 5. Écarts et bugs constatés

### 5.1 Walk

Le code de déplacement n’explique pas un drift de grille : début, destination et snap final sont corrects.

Le trou est entre la progression de la translation et la phase du cycle de marche. L’ABP MON17.2 ne documente actuellement qu’un simple `Idle <-> Walk` piloté par `bIsMoving`.

Point important : `MoveAlpha` existe déjà. Il est préférable d’exploiter ce signal avant d’ajouter un nouveau système de locomotion.

### 5.2 Turn

Le signal natif existe mais aucun asset Gobelin Turn n’existe. Ce n’est pas un bug gameplay. MON3 prévoyait explicitement qu’un Turn puisse rester en pose neutre pendant que le C++ interpole les 90 degrés.

### 5.3 Hurt

L’état, l’audio et le VFX existent. Aucun montage/asset visuel Gobelin n’existe. Le manque est donc un asset/contrat de présentation, pas une absence de détection du dommage.

### 5.4 Death

Le gameplay de mort est déjà correctement commité avant la présentation.

Le Gobelin reste debout aujourd’hui parce qu’aucun DeathMontage Gobelin n’est encore présent/assigné. Une fois le montage assigné, le pipeline existant le lira déjà.

Il reste cependant un vrai trou générique après le montage : aucune pose de cadavre durable ni dissolution.

### 5.5 Persistence

Le restore actuel fait d’abord :

```text
SetActorHiddenInGame(false)
SkeletalMeshComponent->SetVisibility(true)
```

puis, pour un mort :

```text
RestoreCommittedDeathState(RestoredCell, true)
```

Le booléen `true` impose encore une présentation visible du cadavre. C’est incompatible avec le contrat MON17.8 souhaité où un mort restauré ne doit ni rejouer sa chute ni réapparaître après dissolution.

Le restore ne rejoue déjà pas `DeathMontage`, ce qui est correct.

### 5.6 Tests

Le test `Grimrock.Monsters.MON17.2.PresentationBridgeContract` charge le Mesh/AnimBP du Rat géant. Il ne protège pas directement `SK_GoblinThrower` et `ABP_MON_GoblinThrower`.

MON9 protège déjà qu’un mort restauré :

- reste mort ;
- garde `bDeathCommitted=true` ;
- garde `bLootGenerated=true` ;
- ne recrée aucun loot ;
- ne réémet aucune mort ;
- n’occupe aucune cellule.

Il ne vérifie pas encore que le mesh restauré est caché.

### 5.7 Footsteps

Aucun système audio générique de footsteps monstre n’a été trouvé dans le contrat MON10 actuel. L’audio générique couvre Alert, Attack, ImpactHit, ImpactMiss, Hurt, Death et Idle.

MON17.8 ne doit donc pas créer un second système audio uniquement pour le Gobelin. Le besoin Walk prioritaire reste visuel.

## 6. Architecture proposée

### 6.1 Walk Synchronization

Ordre recommandé :

1. conserver l’interpolation grid-authoritative actuelle ;
2. conserver Root Motion désactivé ;
3. utiliser `MoveAlpha` comme signal de phase de déplacement déjà normalisé par case ;
4. dans `ABP_MON_GoblinThrower`, préférer une évaluation contrôlée de `A_GoblinThrower_Walk` par `MoveAlpha` plutôt qu’un Sequence Player libre dont le PlayRate est indépendant du déplacement ;
5. vérifier dans UE5.5.4 la durée exacte et la pose début/fin de `A_GoblinThrower_Walk` ;
6. si la synchronisation spatiale exige de suivre l’EaseInOut, faire correspondre le `MoveAlpha` transmis à la progression réellement utilisée par la translation, plutôt que d’ajouter une autorité animation.

Le premier choix d’architecture est donc **MoveAlpha-driven**, pas `MoveDuration`-driven. `MoveDuration` reste la durée du déplacement. Il ne faut exposer un nouveau `MoveDuration`/`WalkPlayRate` à l’AnimInstance que si l’essai avec `MoveAlpha` démontre une insuffisance réelle.

### 6.2 Hurt

Ne pas ajouter de système dans MON17.8.2.

Si une animation Hurt exploitable est fournie ensuite, étendre le contrat de présentation de manière générique, par exemple avec un montage optionnel dans `UGridMonsterDefinitionAsset`. Ne jamais placer le résultat du dommage dans un Notify.

### 6.3 Death

Réutiliser exactement :

```text
DeathMontage
DeathExpectedDuration
UGridMonsterDeathComponent::StartDeathPresentation()
```

Procédure asset :

```text
Mixamo Death
-> IK_Mixamo
-> IK_GoblinThrower
-> RTG_Mixamo_To_GoblinThrower
-> Goblin_Mixamo_TPose
-> A_GoblinThrower_Death
-> AM_GoblinThrower_Death
-> DA_MON_GoblinThrower.DeathMontage
```

Le montage de mort doit rester presentation-only. Recommandation UE : `Enable Auto Blend Out = false` pour conserver la dernière pose au sol jusqu’au début/à la fin de la dissolution. Cette option devra être vérifiée visuellement en PIE.

### 6.4 Dissolve

Aucun contrat existant n’est disponible. L’extension minimale générique proposée, uniquement après validation de DeathMontage, est :

```text
bEnableDeathDissolve
DeathDissolveDelay
DeathDissolveDuration
DeathDissolveParameterName
```

Nom recommandé du paramètre matériau :

```text
DissolveAmount
0.0 = visible
1.0 = dissous
```

Le runtime doit :

- créer des Dynamic Material Instances uniquement au début du dissolve ;
- utiliser un timer temporaire pendant la dissolution, jamais un Tick permanent ;
- mettre progressivement `DissolveAmount` de 0 à 1 ;
- cacher le SkeletalMesh à la fin ;
- conserver l’Actor runtime mort pour la persistance ;
- ne jamais détruire, regénérer ou déplacer le loot à cause de la dissolution.

### 6.5 Restore

Contrat MON17.8 proposé :

```text
mort fraîche
  -> death gameplay immédiate
  -> DeathMontage
  -> corpse hold
  -> dissolve
  -> SkeletalMesh hidden

mort restaurée depuis SaveGame
  -> Dead restauré directement
  -> aucune occupation
  -> loot restauré séparément
  -> aucun DeathMontage
  -> aucun son/VFX de mort
  -> SkeletalMesh hidden immédiatement
```

Aucune donnée de progression de dissolve ne doit être ajoutée au SaveGame pour ce jalon. Un mort restauré est considéré comme ayant terminé sa présentation de mort.

## 7. Découpage final MON17.8

```text
MON17.8.1 — Audit / Animation Contract
    état réel, matrice, assets, trous, contrat et documentation

MON17.8.2 — Walk Synchronization
    phase Walk pilotée par progression de case
    tests signaux locomotion
    réglage ABP manuel

MON17.8.3 — Goblin Presentation State Integration
    validation Idle/Turn/Alert/Pursuing/Repositioning
    conservation ThrowKnife
    Hurt uniquement si un asset exploitable existe
    test direct Mesh/Skeleton/AnimBP Goblin

MON17.8.4 — Death Animation
    import/retarget Mixamo manuel
    A_GoblinThrower_Death / AM_GoblinThrower_Death
    DA DeathMontage / DeathExpectedDuration
    maintien de pose finale
    régressions MON8/MON17.3.3

MON17.8.5 — Generic Corpse Dissolve
    contrat data-driven
    Dynamic Material Instances
    delay + timer temporaire + hide final
    aucune modification du loot gameplay

MON17.8.6 — Save / Restore + Regression
    mort restaurée cachée
    aucune reprise DeathMontage/dissolve/audio/VFX
    loot et occupation inchangés
    extension tests MON9

MON17.8.7 — PIE Validation / Closure
    exploration, combat, death, dissolve, loot, continue
    tests ciblés puis régressions MON8/MON9/MON10/MON13/MON14/MON17
    campagne Grimrock complète seulement après validation ciblée
```

## 8. Fichiers C++ susceptibles d’être modifiés

### Probables

```text
Source/GrimrockPrototype/Public/Runtime/Monsters/GridMonsterDefinitionAsset.h
Source/GrimrockPrototype/Private/Runtime/Monsters/GridMonsterDefinitionAsset.cpp
Source/GrimrockPrototype/Public/Runtime/Monsters/GridMonsterDeathComponent.h
Source/GrimrockPrototype/Private/Runtime/Monsters/GridMonsterDeathComponent.cpp
Source/GrimrockPrototype/Private/Runtime/Monsters/GridMonsterActor.cpp
```

### Éventuel pour Walk seulement si nécessaire après essai ABP

```text
Source/GrimrockPrototype/Private/Runtime/Monsters/GridMonsterMovementComponent.cpp
Source/GrimrockPrototype/Public/Runtime/Monsters/GridMonsterActor.h
```

Le premier essai Walk doit réutiliser `MoveAlpha` afin d’éviter une extension C++ inutile.

### Tests

Prévoir un nouveau fichier ciblé MON17.8, ou compléter les suites existantes sans les dénaturer :

```text
Source/GrimrockPrototype/Private/Tests/GridMonsterMON178AnimationTests.cpp
Source/GrimrockPrototype/Private/Tests/GridMonsterMON9Tests.cpp
```

`GridMonsterCombatComponent.cpp` ne doit pas être modifié sauf découverte de régression : le timing ThrowKnife est déjà validé.

## 9. Opérations UE5.5.4 manuelles prévues

### Walk

- ouvrir `A_GoblinThrower_Walk` ;
- confirmer `Enable Root Motion=false` ;
- relever `Sequence Length` ;
- vérifier qu’un cycle complet possède une pose début/fin raccordable ;
- ouvrir `ABP_MON_GoblinThrower` ;
- conserver le Slot utilisé par `AM_GoblinThrower_ThrowKnife` ;
- remplacer/adapter la lecture Walk afin que sa phase suive `MoveAlpha` ;
- vérifier Idle <-> Walk et les blends ;
- PIE : déplacement isolé, patrouille, poursuite, mouvements successifs.

### Death

- importer l’animation Mixamo Death ;
- réutiliser `IK_Mixamo`, `IK_GoblinThrower`, `RTG_Mixamo_To_GoblinThrower`, `Goblin_Mixamo_TPose` ;
- créer `A_GoblinThrower_Death` ;
- créer `AM_GoblinThrower_Death` ;
- utiliser le Slot compatible avec l’AnimGraph ;
- régler le montage pour conserver la dernière pose, avec `Enable Auto Blend Out=false` comme premier choix ;
- assigner `DA_MON_GoblinThrower.DeathMontage` ;
- régler `DeathExpectedDuration` sur la durée réellement utile ;
- PIE : vérifier chute complète sans modifier le moment de mort gameplay.

### Materials / dissolve

Avant toute modification :

- ouvrir `SK_GoblinThrower` ;
- relever tous les Material Slots et leurs matériaux réels ;
- inspecter `M_Cloth_Bomber`, `M_Goblin_Bomber`, `M_Hair_Bomber` ;
- rechercher un paramètre de dissolve déjà existant.

Si aucun n’existe, la procédure détaillée de modification Material sera définie en MON17.8.5 après le support C++ et après vérification des Blend Modes réels. Aucun `.uasset` Material ne doit être modifié automatiquement.

## 10. Tests et garde-fous à ajouter

Minimum MON17.8 :

- contrat direct Goblin Mesh/Skeleton/AnimBP ;
- `MoveAlpha` borné, réinitialisé et cohérent avec un mouvement par case ;
- mort logique indépendante du montage ;
- dissolution n’ajoute ni loot, ni XP, ni MonsterDied ;
- restored dead caché et sans occupation ;
- restored dead sans nouveau montage/audio/VFX ;
- ThrowKnife conserve montage/socket/timing ;
- idle variations restent interrompues par Move/Hurt/Death.

Régressions ciblées avant clôture :

```text
Grimrock.Monsters.MON8
Grimrock.Monsters.MON9
Grimrock.Monsters.MON10
Grimrock.Monsters.MON13
Grimrock.Monsters.MON14
Grimrock.Monsters.MON17
```

Aucune réussite Automation ou PIE nouvelle n’est déclarée par cet audit. Les futures validations restent à exécuter sous UE5.5.4.
