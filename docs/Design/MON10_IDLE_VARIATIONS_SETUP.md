# MON10.4 — Variations d’animation Idle des monstres

## 1. Objectif

MON10.4 ajoute une infrastructure C++ modulaire, orientée données et
entièrement optionnelle pour jouer ponctuellement une animation Idle secondaire
d’un monstre réellement inactif. Le jalon ne crée, n’importe et ne modifie
aucun asset sous `Content/`.

Les animations visées peuvent montrer un monstre qui renifle, regarde autour
de lui, se gratte, bouge la tête ou adopte une courte pose d’attente.

## 2. Architecture

Les fichiers principaux sont :

- `GridMonsterIdleVariationTypes.h` : définition, requête et sélecteur pur ;
- `GridMonsterIdleVariationComponent.h/.cpp` : éligibilité, timers, chargement,
  delegate, lecture native et interruption ;
- `GridMonsterDefinitionAsset.h/.cpp` : configuration orientée données ;
- `GridMonsterActor.h/.cpp` : intégration aux transitions de présentation ;
- `GridMonsterMON10IdleVariationTests.cpp` : huit scénarios automatisés.

Le flux reste unidirectionnel :

```text
Monstre réellement inactif
    → délai déterministe one-shot
    → variation déterministe
    → FGridMonsterIdleVariationPlaybackRequest
    → delegate Blueprint
    → Dynamic Montage optionnel
    → timer de durée one-shot
    → nouveau délai
```

## 3. Données d’une variation

`FGridMonsterIdleVariationDefinition` contient :

- `VariationId`, unique dans le DataAsset ;
- `Animation`, référence souple vers `UAnimSequenceBase` ;
- `PlayRate`, fini et strictement positif ;
- `ExpectedDuration`, durée de repli finie et strictement positive.

Une entrée existante doit être complète. La liste `IdleVariations` peut en
revanche être vide et ne rend pas le monstre invalide. La validation utilise
`TSoftObjectPtr::IsNull()` et ne charge aucune animation.

## 4. Sélection déterministe

`FGridMonsterIdleVariationSelector::BuildPresentationSeed` combine uniquement :

- `AGridMonsterActor::ResolvePersistenceId()` ;
- `MonsterId` ;
- le numéro d’occurrence transitoire.

Un salt propre au choix de variation est ensuite appliqué à un
`FRandomStream` local. Les mêmes entrées produisent toujours le même index.
Le sélecteur ne lit et ne modifie ni `CombatRandomStream`, ni
`EncounterRandomSeed`, ni une donnée sauvegardée et n’utilise pas
`FMath::Rand`.

## 5. Évitement des répétitions

Lorsque `bAvoidImmediateIdleVariationRepeat=true`, que plusieurs variations
existent et que l’index précédent est valide, le sélecteur choisit dans une
plage compacte de `N - 1` valeurs puis saute l’ancien index. Le résultat reste
déterministe et ne peut pas répéter immédiatement la variation précédente.

Avec une seule variation, l’index `0` reste autorisé.

## 6. Délai déterministe

`SelectDelay` applique un salt distinct de celui du choix de variation. Le
délai reste compris entre `IdleVariationMinDelay` et
`IdleVariationMaxDelay`. Si les bornes sont identiques, cette valeur est
retournée exactement. Une entrée invalide reçue par le sélecteur pur retourne
un délai positif sûr ; le DataAsset refuse néanmoins une plage invalide.

Ce délai est l’attente avant la prochaine variation, jamais la durée de
l’animation.

## 7. Conditions d’éligibilité

Une planification ou une lecture exige :

- le composant initialisé et `bIdleVariationsEnabled=true` ;
- un `MonsterDefinition` valide avec `bEnableIdleVariations=true` ;
- au moins une variation et un Slot non `None` ;
- un monstre vivant et `bMonsterEnabled=true` ;
- un niveau runtime actif et un Actor non caché ;
- un état `Idle` ou `Dormant` ;
- aucun déplacement ni rotation ;
- aucune présentation d’attaque ou de mort ;
- aucune variation Idle déjà active pour une nouvelle lecture.

Un refus ordinaire lié à ces conditions ne produit aucun warning.

## 8. Timer de délai

Le composant utilise un `FTimerHandle` one-shot pour l’attente. La planification
est idempotente : un `RefreshIdleVariationScheduling` répété conserve le timer
et le numéro d’occurrence déjà préparés. Il ne repousse donc pas indéfiniment
la prochaine variation.

L’animation n’est sélectionnée et chargée qu’à l’expiration du délai.

## 9. Timer de durée

Un second timer one-shot représente la durée active. `EffectiveDuration`
utilise en priorité :

```text
Animation->GetPlayLength() / PlayRate
```

si le résultat est fini et strictement positif. Sinon,
`ExpectedDuration` sert de repli. À l’expiration, la référence au Dynamic
Montage est nettoyée, l’état redevient inactif et un nouveau délai est
planifié si le monstre reste éligible.

## 10. Lecture dynamique par Slot

Lorsque `bNativePlaybackEnabled=true`, le composant appelle la surcharge
UE 5.5.4 suivante :

```cpp
UAnimMontage* UAnimInstance::PlaySlotAnimationAsDynamicMontage(
    UAnimSequenceBase* Asset,
    FName SlotNodeName,
    float BlendInTime,
    float BlendOutTime,
    float InPlayRate,
    int32 LoopCount,
    float BlendOutTriggerTime,
    float InTimeToStartMontageAt);
```

`LoopCount` vaut toujours `1`. Le composant ne conserve que le
`UAnimMontage*` dynamique qu’il a créé. L’absence d’AnimInstance ou un échec de
lecture ne supprime ni la requête Blueprint ni son timer de durée.

Avec `bNativePlaybackEnabled=false`, aucun Montage n’est créé, mais la requête,
le delegate, les compteurs et le cycle des timers restent actifs.

## 11. Interruption par déplacement

`SetMovementAnimationState` détecte uniquement les transitions réelles de
`bIsMoving`. Le début d’un déplacement annule le délai et arrête la variation
active. La fin du déplacement replanifie seulement si toutes les autres
conditions restent vraies. Les mises à jour de `MoveAlpha` ne recréent aucun
timer.

## 12. Interruption par rotation

`SetTurnAnimationState` applique la même règle à `bIsTurning`. Le début de la
rotation interrompt immédiatement l’Idle ; la fin peut créer un nouveau délai.

## 13. Interruption par combat

`SetMonsterState` rafraîchit le composant. Les états `Alert`, `Pursuing`,
`Attacking` et `Repositioning` annulent immédiatement les timers et la
variation active. `StartAttackPresentation` reste autoritaire : le passage à
`Attacking` interrompt l’Idle avant le lancement du montage d’attaque.

`StopIdleVariations` appelle uniquement :

```cpp
AnimInstance->Montage_Stop(BlendOutTime, ActiveIdleDynamicMontage);
```

Le montage suivi est toujours précisé. Le composant n’utilise jamais
`StopAllMontages` et ne peut donc pas arrêter le montage d’attaque.

## 14. Interruption par blessure et mort

L’état `Hurt` interrompt l’Idle. `MarkDead`, `CommitDeath`, la restauration
d’une mort validée et la désactivation du niveau arrêtent également les deux
timers et le seul montage Idle suivi. La mort logique n’attend jamais la fin
de cette présentation.

Le montage de mort n’est ni lu ni arrêté par le composant Idle.

## 15. Séparation du gameplay

Le système ne choisit aucune action tactique, ne consomme aucun point d’action,
ne déclenche aucun déplacement, ne change aucune phase, ne simule pas une
animation dans `MonsterState`, ne lance aucun jet et ne calcule ou n’applique
aucun dégât.

La variation est une conséquence visuelle d’un état inactif déjà établi.

## 16. Séparation Audio/VFX

Le composant ne référence aucun son et aucun Niagara System. Une variation Idle
ne déclenche pas `IdleAudio`, et l’ambiance audio Idle ne déclenche pas une
animation. Les composants Audio et VFX conservent leurs propres sélecteurs,
occurrences, requêtes et cycles de vie.

## 17. Delegate Blueprint

`OnIdleVariationPlaybackRequested` reçoit une
`FGridMonsterIdleVariationPlaybackRequest` par variation acceptée. La requête
expose séquence, occurrence, monstre, variation, index, animation résolue,
Slot, vitesse, durée effective et temps de blend.

Blueprint peut aussi lire `LastPlaybackRequest`, `CurrentVariationIndex`,
`bIdleVariationActive`, les compteurs et l’état du timer, ou appeler
`PlayIdleVariationNow`.

## 18. Absence de Tick

`UGridMonsterIdleVariationComponent` fixe :

```cpp
PrimaryComponentTick.bCanEverTick = false;
PrimaryComponentTick.bStartWithTickEnabled = false;
```

Les seules activités différées sont les deux timers one-shot.

## 19. Absence de persistance

Occurrences, index, timers, Dynamic Montage, dernière requête et compteurs
restent transitoires. Aucun champ n’est ajouté à `FGridRuntimeMonsterState`,
`FGridLevelRuntimeState`, `UGridDungeonRuntimeState` ou la sauvegarde du
groupe.

Une restauration MON9 réinitialise l’ensemble :

- mort : aucun timer et aucune variation ;
- vivant Idle : nouveau délai seulement ;
- vivant Alert/Pursuing : aucune planification avant le retour Idle.

Une ancienne variation n’est jamais rejouée.

## 20. Tests

La suite `Grimrock.Monsters.MON10.IdleVariations` ajoute huit scénarios :

1. `IdleVariationDefinitionValidation` ;
2. `IdleVariationDeterministicSelection` ;
3. `IdleVariationNoImmediateRepeat` ;
4. `IdleVariationDeterministicDelay` ;
5. `IdleVariationSchedulingLifecycle` ;
6. `IdleVariationRequestExactlyOnce` ;
7. `IdleVariationRestoreSilent` ;
8. `IdleVariationNoTickNoPersistenceAndIsolation`.

Ils utilisent des `UAnimSequence` transitoires et
`bNativePlaybackEnabled=false`. Aucun rendu, squelette, asset `Content/`, GPU
ou périphérique audio n’est requis.

## 21. Configuration manuelle future du Rat géant

Créer ou importer manuellement, après validation du code :

```text
Content/GrimrockPrototype/Monsters/RatGiant/Animation/
    A_MON_RatGiant_Idle_Sniff
    A_MON_RatGiant_Idle_Scratch
    A_MON_RatGiant_Idle_LookAround
```

Chaque animation doit être non bouclante, revenir proprement à la pose Idle,
utiliser le même Skeleton que le Rat géant, ne produire aucun déplacement
racine de gameplay et ne contenir aucun Notify de logique de combat.

Dans `DA_MON_RatGiant`, configurer manuellement :

```text
bEnableIdleVariations = true

IdleVariations[0]
    VariationId      = Idle_Sniff
    Animation        = A_MON_RatGiant_Idle_Sniff
    PlayRate         = 1.0
    ExpectedDuration = durée approximative

IdleVariations[1]
    VariationId      = Idle_Scratch
    Animation        = A_MON_RatGiant_Idle_Scratch
    PlayRate         = 1.0
    ExpectedDuration = durée approximative

IdleVariations[2]
    VariationId      = Idle_LookAround
    Animation        = A_MON_RatGiant_Idle_LookAround
    PlayRate         = 1.0
    ExpectedDuration = durée approximative

IdleVariationMinDelay = 5.0
IdleVariationMaxDelay = 12.0
bAvoidImmediateIdleVariationRepeat = true
IdleVariationSlotName = DefaultSlot
IdleVariationBlendInTime = 0.15
IdleVariationBlendOutTime = 0.15
```

## 22. Configuration future de l’Animation Blueprint

Dans `ABP_MON_RatGiant` :

1. ouvrir l’Animation Blueprint puis l’AnimGraph ;
2. conserver la State Machine de locomotion existante ;
3. placer un nœud Slot après sa sortie ;
4. utiliser le même Slot que `IdleVariationSlotName` ;
5. relier le Slot à `Output Pose` ;
6. vérifier la compatibilité du montage d’attaque ;
7. compiler et sauvegarder.

`DefaultSlot` est le repli de compatibilité. Un Slot dédié, par exemple
`MonsterIdleVariation`, peut mieux séparer les variations Idle des montages
d’attaque, à condition de le créer sur le Skeleton et d’utiliser exactement le
même nom dans le DataAsset et l’AnimGraph. Les animations secondaires ne
doivent pas boucler.

## 23. Procédure PIE

1. Importer au moins deux animations Idle.
2. Configurer le Slot dans `ABP_MON_RatGiant`.
3. Configurer `DA_MON_RatGiant`.
4. Régler temporairement les délais à 2–4 secondes.
5. Démarrer une nouvelle partie.
6. Rester hors combat devant un Rat.
7. Vérifier qu’une variation se joue après le délai.
8. Vérifier qu’elle ne boucle pas.
9. Vérifier le retour au Base Idle.
10. Attendre la variation suivante.
11. Vérifier l’absence de répétition immédiate.
12. Faire déplacer le Rat et vérifier l’interruption immédiate.
13. Déclencher le combat.
14. Vérifier qu’aucune variation Idle ne recouvre l’attaque.
15. Blesser le Rat et vérifier l’arrêt de l’Idle.
16. Tuer le Rat et vérifier qu’aucune variation ne reprend.
17. Sauvegarder et recharger.
18. Vérifier qu’aucune ancienne variation n’est rejouée.
19. Désactiver le niveau runtime et vérifier l’absence de timer actif.
20. Filtrer l’Output Log sur `GridMonsterIdle`.
21. Vérifier les séquences et l’absence de doublons.

## 24. Limites du jalon

MON10.4 ne crée ou ne modifie aucune animation, aucun DataAsset, aucun
Animation Blueprint, aucun Blueprint de monstre et aucune carte. Il n’ajoute
ni synchronisation automatique avec `IdleAudio`, ni Idle VFX, ni Root Motion
gameplay, ni montage d’attaque/blessure/mort, ni BlendSpace, Control Rig,
Motion Matching, StateTree ou Behavior Tree.

Le chargement est synchrone uniquement pour la variation sélectionnée. Le
chargement asynchrone et l’optimisation générale restent des sujets MON10.5.
