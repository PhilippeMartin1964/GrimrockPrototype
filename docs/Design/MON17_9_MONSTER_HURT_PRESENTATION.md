# MON17.9 — Generic Monster Hurt / Hit-Reaction Presentation

Statut : **C++ IMPLÉMENTÉ — COMPILATION, TESTS ET PIE À VALIDER**

## 1. Objectif

Ajouter une présentation générique de réaction aux dégâts non mortels sans modifier l'autorité du combat, de la grille ou de la persistance.

Contrat :

```text
Damage
  |
  +-- fatal
  |     -> Dead
  |     -> Death presentation
  |
  +-- non fatal + actual applied damage
        -> Hurt
        -> HurtAudio
        -> HurtVFX
        -> HurtMontage optionnel
```

Un coup fatal ne passe jamais par une présentation Hurt supplémentaire.

## 2. Existant conservé

`AGridMonsterActor::ApplyAttackResult()` possédait déjà le contrat logique suivant :

```text
CurrentHealth <= 0
    -> MarkDead()
else if GetTotalAppliedDamage() > 0
    -> SetMonsterState(Hurt)
    -> PlayHurt()
    -> PlayHurtVFX()
```

MON17.9 ajoute uniquement la présentation de montage au chemin non fatal.

Le test historique `Grimrock.Monsters.MON10.Audio.AudioHurtDeathExclusivity` reste la protection de référence pour l'exclusivité Hurt/Death audio.

## 3. Contrat de données

`UGridMonsterDefinitionAsset` expose désormais :

```cpp
TSoftObjectPtr<UAnimMontage> HurtMontage;
```

Caractéristiques :

- optionnel ;
- purement présentationnel ;
- aucune validation n'impose sa présence ;
- aucun monstre existant n'est obligé de fournir un montage ;
- `RatGiant` peut donc conserver `HurtMontage = None`.

### Pas de HurtExpectedDuration

MON17.9 n'ajoute volontairement aucun `HurtExpectedDuration`.

Le gameplay ne doit jamais attendre la fin du montage Hurt. Le retour visuel vers le graphe d'animation normal repose sur le montage lui-même, notamment `Enable Auto Blend Out = true` pour le GoblinThrower.

## 4. API générique

`AGridMonsterActor` expose :

```cpp
bool StartHurtPresentation();
void StopHurtPresentation(float BlendOutTime = 0.10f);
```

### StartHurtPresentation

La fonction :

- refuse un monstre mort ;
- accepte `HurtMontage = None` comme no-op sûr ;
- refuse de remplacer une présentation Attack active ;
- refuse de jouer pendant une mort engagée ou sa présentation ;
- charge le montage uniquement lorsqu'une lecture est réellement demandée ;
- redémarre le même montage Hurt lorsqu'un nouveau hit efficace arrive ;
- utilise `Montage_Play(..., bStopAllMontages=false)` ;
- ne crée aucun timer ;
- ne modifie ni HP, armures, état de combat, grille ou initiative.

### StopHurtPresentation

La fonction cible uniquement le montage Hurt configuré.

Pour les nettoyages de mort/restauration, elle utilise la référence déjà chargée (`TSoftObjectPtr::Get`) et ne provoque donc pas de chargement synchrone inutile.

## 5. Intégration Damage

Le chemin non fatal devient :

```text
ApplyAttackResult
    -> apply armor/health damage
    -> if HP <= 0
           MarkDead
       else if actual damage > 0
           SetMonsterState(Hurt)
           HurtAudio
           HurtVFX
           StartHurtPresentation
```

L'ordre du test fatal est inchangé et reste prioritaire.

## 6. Priorités de présentation

Politique MON17.9 :

```text
Death > Attack > Hurt > IdleVariation
```

### Death

Toujours prioritaire. `MarkDead()` appelle déjà `ResetAnimationSignals()` ; ce nettoyage arrête désormais le montage Hurt ciblé avant la suite de la mort.

### Attack

Si `bAttackPresentationActive` est vrai, la logique de dégâts, l'état Hurt, HurtAudio et HurtVFX restent applicables mais le montage Hurt ne remplace pas l'AttackMontage.

### IdleVariation

Aucune logique parallèle n'est ajoutée. `SetMonsterState(Hurt)` déclenche déjà le rafraîchissement des idle variations, et celles-ci ne sont autorisées que pour `Idle`/`Dormant`.

### Move / Turn

Le montage Hurt ne bloque pas le mouvement ou la rotation autoritaires de grille.

## 7. Mort et restauration

`ResetAnimationSignals()` arrête maintenant la présentation Hurt avant de remettre les signaux Move/Turn à zéro.

Cette fonction était déjà appelée par :

- l'initialisation ;
- `MarkDead()` ;
- `RestoreRuntimeMonsterState()`.

MON17.9 réutilise donc les points de nettoyage existants au lieu d'ajouter un nouveau cycle de vie.

### Save / Load

La politique existante reste :

```text
Saved Hurt + last-known party cell
    -> Alert

Saved Hurt + no last-known party cell
    -> Idle
```

Aucun temps de montage, booléen de présentation ou état d'avancement Hurt n'est persisté.

## 8. Asset GoblinThrower authoré dans UE5.5.4

Source Mixamo :

```text
Pain Gesture.fbx
```

Chaîne utilisée :

```text
Pain Gesture
  -> goblin_d_shareyko_Skeleton
  -> IK_Mixamo
  -> IK_GoblinThrower
  -> RTG_Mixamo_To_GoblinThrower
  -> Goblin_Mixamo_TPose
  -> A_GoblinThrower_Hurt
  -> AM_GoblinThrower_Hurt
```

Observations validées manuellement pendant l'authoring :

```text
A_GoblinThrower_Hurt
- Skeleton : SKEL_GoblinThrower
- Root Motion : disabled
- retarget visuellement correct

AM_GoblinThrower_Hurt
- Slot : DefaultGroup.DefaultSlot
- Enable Auto Blend Out : true
- Blend In : 0.25 s
- Blend Out : 0.25 s
```

Les `.uasset` restent gérés depuis Unreal Editor par l'utilisateur et ne sont pas écrits par cette étape C++.

## 9. Configuration à effectuer après compilation

Dans `DA_MON_GoblinThrower` :

```text
Monster | Animation
Hurt Montage = AM_GoblinThrower_Hurt
```

Pour les monstres sans animation de réaction, notamment le RatGiant tant qu'aucun asset n'est authoré :

```text
Hurt Montage = None
```

## 10. Tests automatisés ajoutés

Fichier :

```text
Source/GrimrockPrototype/Private/Tests/GridMonsterMON179HurtPresentationTests.cpp
```

Tests :

```text
Grimrock.Monsters.MON17.9.HurtDefinitionContract
Grimrock.Monsters.MON17.9.HurtPresentationApiContract
Grimrock.Monsters.MON17.9.NonFatalDamageRequestsHurt
Grimrock.Monsters.MON17.9.FatalDamageBypassesHurt
Grimrock.Monsters.MON17.9.MissingHurtMontageIsSafe
Grimrock.Monsters.MON17.9.HurtRestoreNormalization
```

Ils protègent :

- le caractère optionnel de `HurtMontage` ;
- l'API générique ;
- le chemin non fatal vers Hurt ;
- le bypass Hurt lors d'un coup fatal ;
- la compatibilité des monstres sans montage ;
- la normalisation Save/Load de l'état Hurt.

## 11. Validation encore requise

Ne pas considérer MON17.9 comme fermé avant résultats réels.

À exécuter :

```text
Grimrock.Monsters.MON17.9
Grimrock.Monsters.MON17.8
Grimrock.Monsters.MON10
Grimrock.Monsters.MON9
Grimrock.Monsters.MON8
Grimrock.Monsters.MON17.3.3
```

Puis en PIE :

1. coup non fatal GoblinThrower -> Hurt montage, audio/VFX, retour normal ;
2. second coup non fatal -> Hurt rejoué proprement ;
3. coup fatal -> Death immédiatement, aucune réaction Hurt supplémentaire ;
4. RatGiant sans HurtMontage -> aucun crash/régression ;
5. sauvegarde/restauration -> aucun montage Hurt repris.

## 12. Non-objectifs

MON17.9 n'introduit pas :

```text
UGridMonsterHurtComponent
HurtExpectedDuration
Hurt gameplay timer
AnimNotify obligatoire
Tick permanent
nouvel EGridMonsterState
C++ GoblinThrower spécifique
sauvegarde de progression de montage
```

Le principe reste celui de MON17.8 : **gameplay autoritaire, présentation optionnelle et data-driven**.
