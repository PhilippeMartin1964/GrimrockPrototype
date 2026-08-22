# MON17.8.6 — Dead Monster Save / Restore

Statut : **CONTRAT / IMPLÉMENTATION / TESTS AJOUTÉS — compilation et validation UE5.5.4 requises**

## 1. Objectif

Finaliser la persistance visuelle introduite par MON17.8.4/5 sans sérialiser les détails transitoires de présentation.

Contrat :

```text
monstre mort restauré
    -> Dead restauré immédiatement
    -> aucune occupation
    -> aucune collision
    -> aucun DeathMontage
    -> aucun DeathAudio / DeathVFX
    -> aucun dissolve rejoué
    -> SkeletalMesh caché immédiatement
    -> Actor runtime mort conservé
    -> loot restauré séparément par le pipeline Items
```

Un monstre mort restauré est considéré comme ayant déjà terminé toute sa présentation de mort.

## 2. Décision de persistance

MON17.8.6 ne sauvegarde pas :

```text
position courante dans DeathMontage
temps restant de corpse hold
DeathDissolveAlpha
temps restant de dissolve
Dynamic Material Instances
timers de présentation
```

Ces données sont purement transitoires.

Que la sauvegarde soit prise pendant la chute, pendant le corpse hold ou pendant la dissolution, le chargement reprend dans un état canonique unique :

```text
Dead + mesh caché
```

Cela évite de créer une machine d'état de présentation dans le SaveGame et conserve MON9 comme source de vérité pour l'état logique du monstre.

## 3. État MON17.8.5 de référence

Le GoblinThrower a été validé en PIE avec :

```text
DeathExpectedDuration = 3.6333333 s
DeathDissolveDelay    = 2.0 s
DeathDissolveDuration = 1.5 s
Parameter             = DissolveAmount
```

Le matériau générique utilise `MF_MonsterDeathDissolveMask`.

L'authoring final validé corrige les cas limites du Noise par :

```text
Noise
 -> Clamp(0.01, 0.99)
 -> If
```

avec :

```text
A = NoiseSafe
B = DissolveAmount
A > B  -> 1
A == B -> 1
A < B  -> 0
```

Ainsi :

```text
DissolveAmount = 0 -> aucune perforation parasite sur le monstre vivant
DissolveAmount 0..1 -> disparition progressive
DissolveAmount = 1 -> disparition complète
```

Validation PIE observée : le Gobelin vivant n'est plus partiellement dissous et le cadavre se dissout progressivement après la mort.

## 4. Modification runtime MON17.8.6

### AGridMonsterActor::RestoreRuntimeMonsterState

Le chemin mort doit appeler :

```text
RestoreCommittedDeathState(RestoredCell, false)
```

`false` signifie que la pose de cadavre n'est pas restaurée visuellement : le mesh est caché immédiatement.

Le chemin reste indépendant de `bEnableDeathDissolve` : même un ancien monstre sans dissolve est caché lorsqu'il est restauré mort. C'est une règle de persistance générique, pas une règle de matériau.

### UGridMonsterDeathComponent::RestoreCommittedDeathState

La restauration morte :

1. arrête le timer de DeathPresentation ;
2. remet à zéro tout état/timer/MID de dissolve ;
3. annule attaque et mouvement ;
4. arrête audio/VFX ;
5. libère occupation ;
6. restaure `bDeathCommitted=true` et `bLootGenerated=true` ;
7. désactive collision ;
8. laisse l'Actor runtime présent ;
9. cache le SkeletalMesh lorsque `bRestorePresentationPose=false`.

Elle n'appelle jamais `StartDeathPresentation()` ni `ScheduleDeathDissolve()`.

### UGridMonsterDeathComponent::RestoreLivingState

La restauration vivante remet aussi à zéro tout état de dissolve et restaure les matériaux/visibilité d'origine afin qu'aucun MID ou alpha transitoire issu d'une ancienne présentation ne puisse survivre à un restore vivant.

## 5. Loot / XP / événements

Invariants conservés :

```text
bDeathCommitted = true
bLootGenerated = true
LogicalDeathEventCount inchangé
LinkExecutionAttemptCount inchangé
aucun Award XP
aucune nouvelle génération de loot
aucun OnMonsterDied
aucun MonsterDied link
```

Les items déjà persistés restent gérés par `FGridLevelRuntimeState::Items` et ne dépendent pas de la visibilité du monstre.

## 6. Tests MON17.8.6

Nouveau fichier :

```text
Source/GrimrockPrototype/Private/Tests/
    GridMonsterMON178PersistenceTests.cpp
```

Tests :

```text
Grimrock.Monsters.MON17.8.DeadRestorePresentationContract
Grimrock.Monsters.MON17.8.LivingRestoreClearsDeathPresentation
```

### DeadRestorePresentationContract

Vérifie notamment :

- état `Dead` et HP=0 ;
- `bDeathCommitted=true` ;
- `bLootGenerated=true` ;
- mesh caché ;
- Actor conservé ;
- collision désactivée ;
- aucune occupation ;
- aucune présentation Death active ;
- aucun dissolve actif ;
- aucun événement/lien/loot généré par le restore.

### LivingRestoreClearsDeathPresentation

Vérifie qu'après un état mort restauré/caché, un restore vivant canonique :

- rend le mesh visible ;
- efface le commit de mort ;
- efface l'état transitoire de dissolve ;
- restaure une occupation normale si le monstre est activé.

## 7. Régressions demandées

Après compilation UE5.5.4 :

```text
Grimrock.Monsters.MON17.8
Grimrock.Monsters.MON9
Grimrock.Monsters.MON8
Grimrock.Monsters.MON10
Grimrock.Monsters.MON17.3.3
```

Le filtre MON17.8 doit désormais contenir **8 tests**.

Aucun résultat n'est déclaré validé avant retour local UE5.5.4.

## 8. Validation PIE demandée

### Scénario A — mort complètement dissoute

```text
1. tuer un GoblinThrower
2. attendre la dissolution complète
3. sauvegarder
4. recharger
```

Attendu :

```text
Gobelin invisible
aucun replay de mort
aucun dissolve
loot toujours présent
aucune occupation
```

### Scénario B — sauvegarde pendant la présentation

```text
1. tuer un GoblinThrower
2. sauvegarder pendant la chute, le corpse hold ou le dissolve
3. recharger
```

Attendu identique :

```text
état canonique Dead + mesh caché immédiatement
```

### Scénario C — plusieurs morts

Plusieurs monstres morts avant sauvegarde doivent rester tous cachés après chargement, sans duplication de loot ou d'événements.

## 9. Frontière MON17.8.7

MON17.8.7 effectuera la validation finale/closure :

- compilation ;
- automatisation complète ;
- PIE GoblinThrower ;
- non-régression RatGiant ;
- synthèse du contrat générique Walk / Death / Dissolve / Persistence.
