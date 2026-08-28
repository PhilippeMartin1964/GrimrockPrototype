# MON10.2 — Infrastructure audio des monstres et du combat

## 1. Objectif

MON10.2 ajoute une infrastructure audio C++ modulaire, orientée données et
entièrement optionnelle. Elle couvre l’alerte, l’attaque, l’impact réussi ou
manqué, la blessure, la mort et l’ambiance Idle des monstres.

L’audio reste une présentation du résultat déjà décidé par le gameplay. Il ne
choisit aucune action, ne lance aucun jet, ne modifie aucun dégât, aucun point
de vie et aucune graine de combat. Aucun asset sous `Content/` n’est créé ou
modifié par ce jalon.

## 2. Architecture

Les principaux fichiers sont :

- `GridMonsterAudioTypes.h` : événements, définitions orientées données,
  requête Blueprint et sélection déterministe ;
- `GridMonsterAudioComponent.h/.cpp` : planification, cooldowns, requêtes,
  delegate, logs et lecture native ;
- `GridMonsterTypes.h` : données audio courantes des attaques via
  `AttackAudio`, `ImpactHitAudio` et `ImpactMissAudio` ;
- `GridMonsterDefinitionAsset` : données d’alerte, blessure, mort et ambiance ;
- `GridMonsterMON10AudioTests.cpp` : sept scénarios automatisés.

Le flux est unidirectionnel :

```text
Événement de gameplay déjà validé
    → UGridMonsterAudioComponent
    → FGridMonsterAudioPlaybackRequest
    → OnAudioPlaybackRequested
    → UGameplayStatics::PlaySoundAtLocation (optionnel)
```

`bNativePlaybackEnabled=false` conserve la requête, le delegate, les compteurs
et le log, mais évite tout appel natif au périphérique audio.

## 3. Événements audio

`EGridMonsterAudioEvent` expose sept événements :

1. `Alert` ;
2. `Attack` ;
3. `ImpactHit` ;
4. `ImpactMiss` ;
5. `Hurt` ;
6. `Death` ;
7. `Idle`.

Un événement sans son configuré est accepté par les données et ne produit
simplement aucune requête.

## 4. Données dans le MonsterDefinition

`UGridMonsterDefinitionAsset` contient :

- `AlertAudio` ;
- `HurtAudio` ;
- `DeathAudio` ;
- `IdleAudio` ;
- `IdleAudioMinDelay` et `IdleAudioMaxDelay` ;
- `bEnableIdleAudio`.

Chaque `FGridMonsterAudioEventDefinition` contient une liste de références
souples `TSoftObjectPtr<USoundBase>`, un volume, un intervalle de pitch et un
cooldown. Une définition vide est valide. La validation vérifie les nombres
finis, les intervalles et l’absence d’entrée explicitement vide sans charger
les assets.

## 5. Données dans les attaques

`FGridMonsterAttackDefinition` ajoute :

- `AttackAudio` ;
- `ImpactHitAudio` ;
- `ImpactMissAudio`.

Ces trois définitions sont facultatives. Leur validation ne rend aucun son
obligatoire et conserve les règles de combat existantes.

## 6. Autorité audio d'attaque

Depuis TD07.3.5.3, `AttackSound` est supprimé.

L'unique autorité pour le son de départ d'une attaque monster est :

```cpp
FGridMonsterAudioEventDefinition AttackAudio;
```

Une définition vide signifie simplement qu'aucun son de départ n'est joué.
Aucun fallback legacy n'est exécuté.

RatGiant et GoblinThrower ont été convertis vers `AttackAudio.Sounds` avant
suppression du champ historique.

## 7. Sélection déterministe

`FGridMonsterAudioSelector` construit une graine de présentation à partir de :

- `ResolvePersistenceId()` ;
- `MonsterId` ;
- l’événement audio ;
- son numéro d’occurrence transitoire.

Cette graine sélectionne l’index de variante et le pitch. Les mêmes entrées
produisent les mêmes résultats. Les compteurs sont uniquement runtime.

## 8. Séparation avec CombatRandomStream

La sélection audio crée son propre `FRandomStream` local. Elle ne lit et ne
modifie jamais :

- `CombatRandomStream` ;
- `EncounterRandomSeed` ;
- le résolveur de combat ;
- une structure sauvegardée ;
- `FMath::Rand`.

L’ajout ou l’absence d’un son ne peut donc pas changer une attaque, une cible ou
un résultat de combat.

## 9. Lecture spatiale

Les one-shots utilisent `UGameplayStatics::PlaySoundAtLocation` :

- alerte, attaque, blessure, mort et Idle : position du monstre ;
- impact : position du `PartyPawn`, ou position du monstre en repli.

Le son sélectionné est le seul asset chargé, au moment de la requête, avec
`LoadSynchronous`. Un chargement asynchrone pourra remplacer ce premier chemin
ultérieurement.

L’atténuation 3D et la concurrence doivent être configurées dans les assets
audio ou leurs réglages. Aucun asset `SoundAttenuation` ou `SoundConcurrency`
n’est créé par MON10.2.

## 10. Ambiance Idle par timer

L’ambiance utilise un `FTimerHandle`, jamais Tick. Elle est planifiée uniquement
si :

- l’audio et l’ambiance sont activés ;
- au moins un son Idle est configuré ;
- le monstre est vivant, activé et dans le niveau runtime actif ;
- son état est `Idle` ou `Dormant`.

Le délai déterministe reste compris entre `IdleAudioMinDelay` et
`IdleAudioMaxDelay`. À l’expiration, les conditions sont revérifiées, un son au
maximum est demandé, puis le prochain délai est planifié.

Le timer est arrêté lors des états actifs, de la blessure, de la mort, de la
désactivation du niveau et de `EndPlay`.

## 11. Cooldowns

Chaque événement possède son cooldown par monstre. Un refus est purement
présentationnel et peut être journalisé en `Verbose`. Le cooldown ne garantit
pas l’unicité logique :

- `bActiveAttackImpactCommitted` reste l’autorité de l’impact ;
- `bDeathCommitted` reste l’autorité de la mort.

## 12. Delegates et Blueprint

`UGridMonsterAudioComponent` expose :

- les fonctions manuelles pour chaque événement ;
- `OnAudioPlaybackRequested` ;
- `LastPlaybackRequest` ;
- `PlaybackRequestCount` ;
- le nombre de requêtes par événement ;
- l’état du timer Idle ;
- `bAudioEnabled` ;
- `bNativePlaybackEnabled`.

`FGridMonsterAudioPlaybackRequest` fournit la séquence, l’événement, les
identifiants, le son résolu, la position, le volume et le pitch. Le composant ne
dépend d’aucun widget.

## 13. Absence de persistance

La requête, les compteurs, les cooldowns et le timer sont transitoires. Aucun
champ audio n’est ajouté à :

- `FGridRuntimeMonsterState` ;
- `FGridLevelRuntimeState` ;
- `UGridDungeonRuntimeState` ;
- la sauvegarde du groupe.

Une restauration MON9 réinitialise l’historique audio transitoire. Un monstre
mort restauré ne rejoue ni `Death`, ni `Hurt`, ni impact et ne planifie pas
d’ambiance. Le loot et l’événement logique de mort ne sont pas rejoués.

## 14. Tests

La suite `Grimrock.Monsters.MON10.Audio` contient sept scénarios :

- `AudioDefinitionValidation` ;
- `AudioDeterministicVariantSelection` ;
- `AudioCombatStartAlertExactlyOnce` ;
- `AudioAttackAndImpactExactlyOnce` ;
- `AudioHurtDeathExclusivity` ;
- `AudioRestoreDeadSilent` ;
- `AudioIdleTimerLifecycle`.

Ils utilisent `bNativePlaybackEnabled=false` et des `USoundWave` transitoires.
Ils ne nécessitent ni asset `Content/`, ni périphérique audio.

La régression complète reste :

```text
Grimrock.Monsters.MON
Grimrock.CharacterCreation.CC5
```

## 15. Configuration manuelle du Rat géant

Après validation et intégration du code, les noms suivants sont recommandés :

```text
Content/GrimrockPrototype/Audio/Monsters/RatGiant/
    S_Rat_Alert_01
    S_Rat_Attack_01
    S_Rat_ImpactHit_01
    S_Rat_Miss_01
    S_Rat_Hurt_01
    S_Rat_Death_01
    S_Rat_Idle_01
    S_Rat_Idle_02
```

Ce sont uniquement des recommandations. Les sons peuvent être des `SoundWave`,
des `SoundCue` ou des `MetaSound Source`, dès lors qu’ils sont assignables comme
`USoundBase`.

Dans `DA_MON_RatGiant` :

1. assigner l’alerte, la blessure, la mort et les variantes Idle ;
2. activer `bEnableIdleAudio` si l’ambiance est souhaitée ;
3. régler les délais, volumes, pitches et cooldowns ;
4. dans `Attack_Bite`, assigner l’attaque et les impacts réussi/manqué ;
5. configurer l’atténuation 3D dans les sons ou leurs réglages audio.

## 16. Procédure PIE

1. Importer et assigner manuellement les sons.
2. Lancer une nouvelle partie.
3. Approcher un Rat et vérifier une seule alerte au début du combat.
4. Terminer la phase joueur.
5. Vérifier le son d’attaque même si aucun Montage n’est assigné.
6. Vérifier un seul impact réussi ou manqué.
7. Infliger des dégâts non mortels et vérifier `Hurt`.
8. Tuer le Rat et vérifier `Death` sans `Hurt` supplémentaire.
9. Vérifier que l’ambiance Idle s’arrête pendant le combat et après la mort.
10. Sauvegarder puis recharger un Rat mort et vérifier son silence.
11. Appeler `LogMonsterAudioState` et observer les requêtes Blueprint.
12. Tester `bNativePlaybackEnabled=false` sans perdre les delegates.

## 17. Limites du jalon

MON10.2 n’importe aucun son et ne crée ni `SoundCue`, ni `MetaSound`, ni asset
d’atténuation ou de concurrence. Il n’ajoute pas de musique, sons de
victoire/défaite, voix, options de volume, VFX, Niagara, nombres flottants,
variations d’animation Idle, attaques du joueur ou chargement asynchrone
complet.

`EncounterRandomSeed` et toutes les règles de gameplay restent inchangés.
