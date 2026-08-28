# TD07.3.5.3 — Monster Presentation Authority

Date : 28 août 2026
Projet : GrimrockPrototype — Unreal Engine 5.5.4
Parent : TD07.3.5 — Combat Data Schema Reset
Statut : VALIDÉ — CLOS

## 1. Cible

Monster attack presentation doit utiliser uniquement :

Audio :
- AttackAudio
- ImpactHitAudio
- ImpactMissAudio

VFX :
- AttackVFXDefinition
- ImpactHitVFXDefinition
- ImpactMissVFXDefinition

À supprimer après réparation authoring :
- AttackSound
- ImpactVFX
- fallback AttackAudio -> AttackSound
- fallback ImpactHitVFXDefinition -> ImpactVFX

## 2. Assets concernés

L'audit TD07.3.1 remonte exactement deux MONSTER.LEGACY_ATTACK_SOUND :

- DA_MON_RatGiant
- DA_MON_GoblinThrower

Aucun MONSTER.LEGACY_IMPACT_VFX n'est remonté par la baseline.

Les deux DataAssets sont stockés via Git LFS.

RatGiant :
oid sha256:1669e9e61f4591d9ba84a4656f27e0fc4faf19605596c72f1859d7e799b69442
size 10369

GoblinThrower :
oid sha256:f9cf698365660ea84c6cbadd854b48d609c03583c02f15a4727f5177f9e69d2e
size 11486

## 3. Conversion

Pour chaque attack :

AttackSound non vide + AttackAudio vide
-> AttackAudio.Sounds += AttackSound
-> AttackSound = null

Si AttackAudio est déjà configuré avec une valeur différente, la réparation échoue comme authoring ambigu au lieu d'inventer une fusion.

Même politique défensive pour ImpactVFX :
- si présent et ImpactHitVFXDefinition vide : copie vers Systems
- si les deux sont incompatibles : erreur
- puis ImpactVFX est vidé

## 4. Outil one-shot

Automation :
Grimrock.TechnicalDebt.TD07_3_5_3.AssetRepair.MonsterPresentationAssets

Elle charge les deux vrais DataAssets LFS, effectue la conversion, valide les définitions, sauvegarde les packages et vérifie que les champs legacy sont vides.

## 5. Script local

Scripts/RepairTD07353MonsterPresentation.ps1

Le script :
- exige master
- exige Git LFS
- refuse des modifications locales préexistantes sur les deux assets
- lance l'Automation
- stage uniquement les DataAssets réellement modifiés
- commit uniquement ces fichiers
- push origin/master

## 6. Séquence

repair current monster LFS assets
-> version repaired assets
-> remove AttackSound / ImpactVFX from C++
-> remove runtime fallbacks
-> update MON10 tests + TD07.3.1 audit
-> normalization gate
-> continue TD07.3.5.4 range schema

Aucun shim PostLoad/runtime de compatibilité n'est introduit.


## 7. Réparation LFS validée

Le 28 août 2026 :

```text
Grimrock.TechnicalDebt.TD07_3_5_3.AssetRepair
Succeeded              : 1
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0
Report                 : Saved/Automation/TD04/TD04-20260828-090239
```

Commit LFS :

```text
645775c5296531d422abf9cc020cd75109eae2d0
Repair monster presentation authoring
```

Pointeurs après réparation :

```text
DA_MON_RatGiant
oid sha256:4c36c75dcff8d6b6aeb95d7ad56f315e8391d51b55a0a94cd044fdf594ba9068
size 11241

DA_MON_GoblinThrower
oid sha256:ba9109bb061ae35ccc58855243e5fe3fdce35e2c3e9c38a9ab73abd73f7b085e
size 11490
```

## 8. Normalisation C++ appliquée

Supprimés de `FGridMonsterAttackDefinition` :

```text
AttackSound
ImpactVFX
```

Supprimés du runtime :

```text
AttackAudio -> AttackSound fallback
ImpactHitVFXDefinition -> ImpactVFX fallback
temporary LegacyDefinition adapters
```

Autorité restante :

```text
AttackAudio
ImpactHitAudio
ImpactMissAudio

AttackVFXDefinition
ImpactHitVFXDefinition
ImpactMissVFXDefinition
```

`RangeCells` reste volontairement inchangé jusqu'à TD07.3.5.4.

L'outil one-shot et son script sont supprimés après usage.

## 9. Gate de normalisation

Filtre :

```text
Grimrock.TechnicalDebt.TD07_3_5_3.Normalization
```

Tests :

```text
SchemaAuthority
RepairedAssets
CurrentPresentationDefinitions
RuntimeFallbackRemoval
```

Attendu : 4/4, zéro warning.


## 10. Validation finale

Gate local validé le 28 août 2026 :

```text
Grimrock.TechnicalDebt.TD07_3_5_3.Normalization   4/4
Grimrock.TechnicalDebt.TD07_3_5.Characterization 4/4
Grimrock.Monsters.MON10.Audio                     7/7
Grimrock.Monsters.MON10.VFX                       8/8
Warnings                                           0
Failures                                           0
```

Reports :

```text
Saved/Automation/TD04/TD04-20260828-091855
Saved/Automation/TD04/TD04-20260828-091908
Saved/Automation/TD04/TD04-20260828-091921
Saved/Automation/TD04/TD04-20260828-091934
```

TD07.3.5.3 est clos. La tranche active devient TD07.3.5.4 — Monster Range Schema.
