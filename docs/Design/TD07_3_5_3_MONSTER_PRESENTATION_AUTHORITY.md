# TD07.3.5.3 — Monster Presentation Authority

Date : 28 août 2026
Projet : GrimrockPrototype — Unreal Engine 5.5.4
Parent : TD07.3.5 — Combat Data Schema Reset
Statut : ASSET REPAIR PREPARED — RATGIANT / GOBLINTHROWER LFS REPAIR REQUIRED

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
