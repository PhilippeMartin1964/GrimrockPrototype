# MON17.9 — Generic Monster Hurt / Hit-Reaction Presentation

Statut : **VALIDÉ ET CLOS**

## 1. Objectif

Ajouter une présentation générique de réaction aux dégâts non mortels sans modifier l'autorité du combat, de la grille ou de la persistance.

Contrat final :

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
- `RatGiant` peut conserver `HurtMontage = None`.

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

La politique reste :

```text
Saved Hurt + last-known party cell
    -> Alert

Saved Hurt + no last-known party cell
    -> Idle
```

Aucun temps de montage, booléen de présentation ou état d'avancement Hurt n'est persisté.

## 8. Assets GoblinThrower validés dans UE5.5.4

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

Configuration validée :

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

Configuration du Data Asset validée :

```text
DA_MON_GoblinThrower
Hurt Montage = AM_GoblinThrower_Hurt
Death Montage = AM_GoblinThrower_Death
Death Expected Duration = 3.633333 s
```

Les `.uasset` restent authorés et enregistrés depuis Unreal Editor.

## 9. Compilation

Compilation C++ UE5.5.4 confirmée par l'utilisateur le 23 août 2026.

Résultat :

```text
Compilation : SUCCESS
```

## 10. Tests automatisés MON17.9

Fichier :

```text
Source/GrimrockPrototype/Private/Tests/GridMonsterMON179HurtPresentationTests.cpp
```

Résultat confirmé :

```text
Grimrock.Monsters.MON17.9.FatalDamageBypassesHurt        Success
Grimrock.Monsters.MON17.9.HurtDefinitionContract         Success
Grimrock.Monsters.MON17.9.HurtPresentationApiContract    Success
Grimrock.Monsters.MON17.9.HurtRestoreNormalization       Success
Grimrock.Monsters.MON17.9.MissingHurtMontageIsSafe       Success
Grimrock.Monsters.MON17.9.NonFatalDamageRequestsHurt     Success

MON17.9 : 6/6 Success
```

`HurtRestoreNormalization` confirme les deux sorties prévues :

```text
Hurt + no last-known party cell -> Idle
Hurt + last-known party cell    -> Alert
```

## 11. Régressions automatisées

Les suites de régression demandées ont été exécutées et les tests affichés terminent en `Success` :

```text
Grimrock.Monsters.MON17.8
Grimrock.Monsters.MON10
Grimrock.Monsters.MON9
Grimrock.Monsters.MON8
Grimrock.Monsters.MON17.3.3
```

Le test MON10 `AudioHurtDeathExclusivity` confirme notamment l'exclusivité attendue entre Hurt et Death.

Les warnings observés dans certaines fixtures (`MissingMonsterMovement`, absence de Party, PresentationWarning de fixtures sans mesh) ne correspondent pas à des échecs MON17.9 et les tests concernés terminent en `Success`.

## 12. Validation PIE finale

Validation visuelle utilisateur confirmée le 23 août 2026.

### Cas fatal

Un premier GoblinThrower a reçu un coup critique :

```text
Attack_Unarmed
10 dégâts
10 -> 0 PV
```

Résultat observé :

```text
Dead directement
Death presentation
aucune réaction Hurt supplémentaire
```

Le pipeline historique Death reste fonctionnel : loot, XP, MonsterDied, corpse/dissolve et occupancy release sont exécutés.

### Cas non fatal répété

Un second GoblinThrower a reçu deux coups non mortels :

```text
Premier impact : 4 dégâts, 10 -> 6 PV
Deuxième impact : 4 dégâts, 6 -> 2 PV
```

Résultat visuel confirmé : le Gobelin se « tortille de douleur » après chaque coup au but, puis revient correctement à sa présentation normale.

Cela valide :

```text
nonfatal damage
-> Hurt
-> AM_GoblinThrower_Hurt
-> Auto Blend Out
-> retour normal
```

Le monstre reste sur sa case et conserve ensuite son comportement de combat.

## 13. Non-objectifs confirmés

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

## 14. Verdict de clôture

MON17.9 est **VALIDÉ ET CLOS**.

Le contrat final reste celui de MON17.8 : **gameplay autoritaire, présentation optionnelle et data-driven**.
