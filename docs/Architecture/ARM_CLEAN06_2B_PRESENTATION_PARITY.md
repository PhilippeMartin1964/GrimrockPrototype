# ARM-CLEAN06.2B — Contrat de présentation audio et Niagara sans perte

## Décision de migration

ARM-CLEAN06.2A a constaté via Unreal **0 écart sur 1 640 profils ARM** et identifié
les quatre définitions Sword modifiées depuis la baseline, sans modifier d'asset
(issue #95). Les présentations audio Sword sont des tableaux de variantes :
la structure historique FGridPlayerAttackPresentationProfile porte des sons
multiples, une plage de pitch, un volume, des timings, les offsets d'animation
et plusieurs systèmes Niagara. Le premier prototype canonique ARM-CLEAN06.1
n'offrait qu'un SoundBase par événement : il était donc **inapte à une copie
sans perte** des présentations existantes.

## Unique autorité cible

La classe UGridActionPresentationCatalogueAsset conserve la seule source
d'édition de la présentation. Chaque entrée porte un PresentationId stable et :

- FGridCanonicalAudioVariants : Sounds[] ordonné, VolumeMultiplier, PitchMin,
  PitchMax. Les événements Attack, ImpactHit, ImpactMiss, ImpactFlesh,
  ImpactMetal, ImpactWood, Parry, Block, Critical et Heavy ont chacun une
  configuration indépendante.
- FGridCanonicalVFXVariants : Systems[] ordonné, bAttachToHeldItem, SocketName,
  LocationOffset, RotationOffset et Scale. Canaux Attack, ImpactHit,
  ImpactMiss, Parry et Block.
- MotionStyle, bAnimateHeldItem, MotionDurationSeconds, ImpactDelaySeconds,
  RecoveryDelaySeconds, PeakLocationOffset, PeakRotationOffset et
  FeedbackDurationSeconds.

Un événement vide est autorisé et ne crée aucune action. Un élément invalide,
une plage de pitch inversée, une durée négative ou une échelle Niagara nulle
est refusé. La configuration reste dans le catalogue global, **jamais dans
DA_Weapon_xxx**. Le rattachement à la mécanique se fait exclusivement par
ItemActionBinding(ActionId, PresentationId) authoré sur l'archétype.

## Couverture des tests

Deux tests Automation en C++ :

- Grimrock.RPG.ARM.CLEAN06_2.Presentation.Variants :
  quatre chemins d'effets Sword avec ordre conservé, volume et pitch,
  attaque ratée/réussie indépendantes, timings, Niagara et SocketName.
  Les références de test sont des soft paths synthétiques et ne chargent
  aucun asset binaire.
- Grimrock.RPG.ARM.CLEAN06_2.Presentation.RejectInvalid :
  refuse une plage de pitch inversée, un son vide, un scale nul et un
  délai d'impact négatif ; vérifie aussi qu'un profil restauré est valide.

**Ce n'est pas la migration des contenus.** Aucun DA, Blueprint, niveau ni
SaveGame n'est modifié. Le comportement actuel conserve ses lecteurs historiques
jusqu'à la conversion UE5.5.4 vérifiée (ARM-CLEAN06.2C–06.4). Les références
réelles et les réglages précis du commit a60031db restent dans les binaires
UE ; ils ne doivent jamais être remplacés par les chemins synthétiques des tests.

Suite : comparer les champs sérialisés des 56 vrais DataAssets avec leurs
destinations canoniques, vérifier les class affinities, réactions, éléments
interactifs et éventuelles propriétés métiers encore non mappées. Refuser
tout effacement tant que le bilan est incomplet.
