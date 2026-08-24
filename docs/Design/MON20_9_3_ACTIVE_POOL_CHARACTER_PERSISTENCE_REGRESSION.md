# MON20.9.3 — Active / Pool Character Persistence Regression

Date : **24 août 2026**  
Statut : **VALIDÉ UE5.5.4 — 8/8 SUCCESS**

## 1. Objectif

Verrouiller la cohérence entre le recrutement MON20.2 et la persistance Skill MON20.9.2.

Le contrat essentiel est :

```text
Skill snapshot
    -> keyed by CharacterId
    -> indépendant de l'index
    -> indépendant de SelectedCharacterIndex
    -> indépendant du conteneur ActiveCharacters / CharacterPool
```

Un personnage peut donc être sauvegardé en réserve, être recruté ensuite, puis recevoir correctement son état Skill lors d'un restore grâce à son `CharacterId` stable.

## 2. Pas de nouveau runtime

MON20.9.3 n'ajoute aucune abstraction de production.

Il réutilise directement :

```text
FRPGPartyRecruitmentService::TryRecruitFromPool()
FRPGSkillPersistence::CapturePartySkills()
FRPGSkillPersistence::ValidateSavedPartySkills()
FRPGSkillPersistence::RestorePartySkills()
FRPGSkillService::GetSkillRank()
```

Aucun nouvel état de personnage, aucune nouvelle persistance Talent et aucun `.uasset/.umap` ne sont introduits.

## 3. Invariant identité

La persistance Skill ne dépend jamais de :

```text
CharacterIndex
SelectedCharacterIndex
position dans CharacterPool
position dans ActiveCharacters
```

La seule clé durable est :

```text
FGuid CharacterId
```

Cela permet notamment :

```text
capture alors que CharacterId est dans CharacterPool
    -> recrutement pool -> active
    -> restore
    -> le rang revient sur le personnage actif correspondant
```

et inversement :

```text
capture alors que CharacterId est actif
    -> déplacement en réserve
    -> restore
    -> le rang revient sur l'identité maintenant en réserve
```

## 4. Régression recrutement

`FRPGPartyRecruitmentService` copie déjà le candidat du pool avant de l'ajouter aux personnages actifs.

MON20.9.3 verrouille que cette transaction préserve `SkillRanks` et qu'un rejet atomique de recrutement laisse également les rangs du candidat en réserve inchangés.

Le système ne doit donc jamais nécessiter de synchronisation parallèle des Skills pendant le recrutement.

## 5. Tests Automation

Filtre :

```text
Grimrock.MON20.9.ActivePoolPersistence
```

Tests :

```text
RecruitmentPreservesPoolSkill
PoolSnapshotRestoresAfterRecruitment
ActiveSnapshotRestoresAfterReserveMove
MixedActivePoolIsolation
SelectedCharacterIndependent
RecruitmentRejectPreservesPoolSkill
PoolReorderRestoresByIdentity
SnapshotValidAfterRecruitmentMove
```

### Couverture

`RecruitmentPreservesPoolSkill`
- un candidat entraîné est déplacé du pool vers les actifs ;
- son rang runtime reste intact.

`PoolSnapshotRestoresAfterRecruitment`
- le snapshot est capturé alors que le personnage est en réserve ;
- le personnage est recruté ;
- le restore retrouve l'identité active par `CharacterId`.

`ActiveSnapshotRestoresAfterReserveMove`
- snapshot capturé quand le personnage est actif ;
- l'identité est ensuite placée dans le pool ;
- le restore rattache le rang à la réserve.

`MixedActivePoolIsolation`
- actif et réserve utilisent le même Skill avec des rangs différents ;
- chaque rang revient sur le bon `CharacterId`.

`SelectedCharacterIndependent`
- la sélection change entre capture et restore ;
- aucun rang n'est attaché à la sélection ;
- `SelectedCharacterIndex` n'est pas modifié par le restore Skill.

`RecruitmentRejectPreservesPoolSkill`
- une party pleine refuse le recrutement ;
- le candidat et ses Skills restent intacts dans le pool.

`PoolReorderRestoresByIdentity`
- l'ordre des personnages de réserve change ;
- les rangs restent attachés à leurs identités.

`SnapshotValidAfterRecruitmentMove`
- un snapshot capturé en réserve reste structurellement valide après le recrutement de la même identité.

## 6. Critères de sortie

```text
Skill du candidat pool -> recrutement               conservé
snapshot pool -> personnage devenu actif            restauré
snapshot actif -> personnage devenu réserve          restauré
rangs actifs/réserve différents                      isolés
changement SelectedCharacterIndex                    sans effet
recrutement rejeté                                   Skill du pool intact
réordonnancement du pool                             restore par CharacterId
snapshot avant recrutement                           valide après déplacement d'identité
```

## 7. Validation UE5.5.4 — VALIDÉ

Validation fournie le **24 août 2026** après correction de la collision de helpers en build Unity.

Filtre ciblé :

```text
Grimrock.MON20.9.ActivePoolPersistence
```

Résultat :

```text
ActiveSnapshotRestoresAfterReserveMove  Success
MixedActivePoolIsolation                 Success
PoolReorderRestoresByIdentity            Success
PoolSnapshotRestoresAfterRecruitment     Success
RecruitmentPreservesPoolSkill            Success
RecruitmentRejectPreservesPoolSkill      Success
SelectedCharacterIndependent             Success
SnapshotValidAfterRecruitmentMove        Success

8 / 8 Success
0 Fail
0 Error
```

Campagne cumulative :

```text
Grimrock.MON20.9

SkillPersistence       8/8
ActivePoolPersistence  8/8
--------------------------
TOTAL                 16/16 Success
0 Fail
0 Error
```

La validation détaillée est également consignée dans :

```text
docs/Design/MON20_9_3_AUTOMATION_VALIDATION.md
```

Aucun PIE n'est requis pour MON20.9.3 : aucun asset ni comportement visuel n'est modifié.

## 8. Suite

```text
MON20.9.4 — Skill Projection / Skills Page Restore Regression
            IMPLÉMENTÉ — VALIDATION UE5.5.4 À FAIRE
```

Cette étape vérifie que les rangs restaurés redeviennent immédiatement consommables par la projection `RequirementIds`, les actions et la page Compétences.
