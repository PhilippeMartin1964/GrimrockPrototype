# TD07.3.3.5 — XP / Level / Class Progression Authority Normalization

Statut : **VALIDÉ — STOP CONDITION ATTEINTE**

## Objectif

Supprimer les autorités concurrentes identifiées par la caractérisation TD07.3.3.5 sans modifier la courbe XP, le budget de choix de classe, les prérequis ni les effets d'un level-up.

## Décision d'autorité

### Experience

`FGridCharacterInventoryState::Experience` est l'autorité durable de progression de niveau.

`Level` reste disponible aux consommateurs runtime afin d'éviter un refactor artificiellement massif, mais devient une projection `Transient` reconstruite par :

```cpp
URPGCharacterRulesLibrary::GetLevelForExperience(Character.Experience)
```

Les flux normaux de gain d'XP continuent d'écrire `Experience` puis d'appeler `FRPGLevelUpService::ApplyPendingLevelUp`, qui synchronise la projection `Level` et recalcule les projections dérivées.

### Choix de progression de classe

La nouvelle autorité durable est :

```cpp
FGridCharacterInventoryState::SelectedClassProgressionChoiceIds
```

`FRPGClassProgressionTransactionService::RuntimeStates` ne stocke plus les ChoiceIds. Il ne contient que des informations reconstructibles telles que les requirements satisfaits.

Un reset du cache runtime ne doit donc plus supprimer les choix acquis.

## SaveGame v14 — B1

La suppression de `Level` du schéma durable ouvre le schéma prototype exact-match **v14**.

Contrat :

```text
SaveVersion == 14 -> validation / chargement
SaveVersion != 14 -> rejet
aucune migration
aucune réécriture
```

Au chargement, `Level` est reconstruit depuis `Experience` avant validation. `Experience` n'est jamais corrigé silencieusement : une valeur hors contrat reste rejetée par la validation stricte.

## B1 et résidu transitoire

B1 conserve temporairement :

```cpp
FRPGCharacterProgressionSaveState
UGrimrockPartySaveGame::ClassProgressionStates
```

Ce tableau n'est plus une autorité :
- les nouvelles sauvegardes le remettent à vide ;
- la validation ne le consulte plus ;
- le chargement ne le restaure plus ;
- les projections runtime sont reconstruites depuis le personnage.

Il est conservé uniquement pour minimiser le bruit de compilation dans quelques tests historiques pendant B1.

## B2 — miroir Save supprimé

B2 supprime physiquement :

```text
FRPGCharacterProgressionSaveState
UGrimrockPartySaveGame::ClassProgressionStates
```

Le serializer ne vide plus de tableau legacy et MON18.8 ne fabrique plus de snapshots de progression vides.

Comme ce retrait modifie le layout sérialisé, B2 ouvre :

```text
CurrentSaveVersion = 15
v14 et antérieures -> rejet sans migration
```

Il ne reste plus qu'une autorité durable pour les choix :

```text
FGridCharacterInventoryState::SelectedClassProgressionChoiceIds
```

et une projection runtime reconstructible :

```text
RuntimeStates.SatisfiedRequirements
```

TD07.3.3.5 reste ouvert jusqu'à validation de B2 et du Shipping Win64.

## Validation B1

Filtre principal :

```text
Grimrock.TechnicalDebt.TD07_3_3_5.NormalizationB1
```

Tests :
- `SchemaAuthority`
- `CharacterChoiceAuthority`
- `TransientLevelRoundTrip`
- `SaveSchemaVersion`

Régressions ciblées :
- `Grimrock.TechnicalDebt.TD07_3_3_5.Characterization`
- `Grimrock.RPG.MON15.2`
- `Grimrock.RPG.MON15.3`
- `Grimrock.RPG.MON15.5`
- `Grimrock.MON20.7.Talents`
- `Grimrock.TechnicalDebt.TD07_3_2`


## Validation B2

Filtre principal :

```text
Grimrock.TechnicalDebt.TD07_3_3_5.NormalizationB2
```

Tests :

```text
SchemaPurge
CharacterChoiceRoundTrip
ProjectionRebuild
SaveSchemaVersion
```

Régressions :

```text
Grimrock.TechnicalDebt.TD07_3_3_5.NormalizationB1
Grimrock.TechnicalDebt.TD07_3_3_5.Characterization
Grimrock.RPG.MON15.2
Grimrock.RPG.MON15.3
Grimrock.RPG.MON15.5
Grimrock.MON20.7.Talents
Grimrock.Magic.MON18.8
Grimrock.TechnicalDebt.TD07_3_2
Grimrock.Monsters.MON9
Grimrock.RPG.MON16.7
Grimrock.RPG.MON16.8
```

Puis Win64 Shipping.

### Stop condition B2

- [x] autorité durable Experience ;
- [x] Level transient ;
- [x] choix de classe durables sur le personnage ;
- [x] RuntimeStates réduit au read-model requirements ;
- [x] FRPGCharacterProgressionSaveState supprimé ;
- [x] ClassProgressionStates supprimé ;
- [x] helper MON18.8 legacy supprimé ;
- [x] SaveGame v15 exact-match ;
- [x] test B2 ajouté ;
- [x] build UE5.5.4 vert ;
- [x] NormalizationB2 4/4 ;
- [x] régressions B1 / MON15 / MON20.7 / MON18.8 / Save vertes ;
- [x] Shipping Win64 vert.


## Validation de clôture — 27 août 2026

```text
NormalizationB2             4 success / 0 warning / 0 failed
NormalizationB1             4 success / 0 warning / 0 failed
Characterization            4 success / 0 warning / 0 failed

MON15.2                      1 success / 4 warnings / 0 failed
MON15.3                      5 success / 1 warning / 0 failed
MON15.5                      8 success / 0 warning / 0 failed

MON20.7 Talents             24 success / 0 warning / 0 failed
MON18.8 Spellbook           11 success / 0 warning / 0 failed
TD07.3.2                     6 success / 0 warning / 0 failed
MON9                         9 success / 4 warnings / 0 failed
MON16.7                     10 success / 0 warning / 0 failed
MON16.8                     10 success / 0 warning / 0 failed

Win64 Shipping               COOK / PACKAGE VALIDATED
```

Tous les filtres bloquants ont terminé sans Failed ni Not run. Les warnings MON15.2, MON15.3 et MON9 restent non bloquants et appartiennent aux baselines déjà connues.

**TD07.3.3.5 est clos et validé.**
