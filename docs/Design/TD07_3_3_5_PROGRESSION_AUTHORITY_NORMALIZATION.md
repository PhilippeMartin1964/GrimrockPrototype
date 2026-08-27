# TD07.3.3.5 — XP / Level / Class Progression Authority Normalization

Statut : **B1 IMPLÉMENTÉ — À VALIDER**

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

## SaveGame v14

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

## B2 — stop condition intermédiaire

B2 doit :
1. supprimer `FRPGCharacterProgressionSaveState` ;
2. supprimer `ClassProgressionStates` ;
3. supprimer les helpers de test qui ne servent plus qu'à cet ancien format ;
4. confirmer qu'aucune référence C++ à cet ancien miroir ne subsiste ;
5. conserver le schéma v14 si aucune nouvelle donnée sérialisée ne change ;
6. exécuter les régressions MON15 / MON20.7 / TD07.3.2.

TD07.3.3.5 reste ouvert jusqu'à validation de B2.

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
