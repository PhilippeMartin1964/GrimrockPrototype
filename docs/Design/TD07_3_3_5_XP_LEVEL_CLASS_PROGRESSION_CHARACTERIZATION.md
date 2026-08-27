# TD07.3.3.5 — XP / Level / Class Progression Characterization

Date : **27 août 2026**  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**  
Parent : **TD07.3.3 — Character State Normalization**  
Baseline : `4112a0b2daae24e29f3c12319f6cbac3a190230a`  
Statut : **CHARACTERIZATION VALIDÉE — NORMALIZATION ACTIVE**

## 1. Objet

TD07.3.3.5 doit normaliser trois éléments qui ne possèdent pas aujourd'hui la même nature :

```text
Experience
Level
SelectedChoiceIds
```

Le but du gate est de déterminer explicitement lesquels sont des autorités durables et lesquels sont des projections reconstructibles.

## 2. Experience -> Level

`URPGCharacterRulesLibrary` définit déjà la relation complète :

```text
Level 1  -> 0 XP
Level 2  -> 1000 XP
Level 3  -> 3000 XP
Level 4  -> 6000 XP
...
Level 20 -> cap
```

et expose :

```cpp
GetLevelForExperience()
IsLevelExperienceConsistent()
NormalizeExperience()
```

Le SaveGame courant refuse un personnage lorsque :

```text
Character.Level != GetLevelForExperience(Character.Experience)
```

Cela indique que `Level` est déjà conceptuellement reconstructible depuis `Experience`.

## 3. Synchronisation Level actuelle

`FRPGExperienceRewardService` :

1. écrit d'abord `Character.Experience` ;
2. appelle `FRPGLevelUpService::ApplyPendingLevelUp()` ;
3. le service calcule `TargetLevel = GetLevelForExperience(Experience)` ;
4. il écrit ensuite `Character.Level = TargetLevel`.

Le runtime possède donc actuellement une fenêtre de synchronisation entre deux représentations du même fait.

Le service supporte :

```text
multi-level gain
    level 1 + 3000 XP
    -> level 3

demotion
    stored level 3 + XP correspondant au level 2
    -> rejet
    -> aucun rollback de XP vers le niveau stocké
```

Ce comportement doit être caractérisé avant suppression éventuelle de `Level`.

## 4. Class progression choices

Les choix de progression sont de nature différente.

```text
Choice_A
Choice_B
Choice_C
...
```

sont des décisions explicites du joueur et ne peuvent pas être reconstruites depuis XP, Level ou ClassId.

Ils doivent donc rester durables.

## 5. Double représentation actuelle

Pendant le jeu :

```text
FRPGClassProgressionTransactionService
    static RuntimeStates
        CharacterId
        ClassId
        CharacterLevel
        SelectedChoiceIds
        SatisfiedRequirements
```

Pendant la sauvegarde :

```text
UGrimrockPartySaveGame
    ClassProgressionStates[]
        CharacterId
        SelectedChoiceIds
```

Le `FGridCharacterInventoryState` ordinaire ne contient pas les choix.

Conséquence caractérisée :

```text
commit Choice_A
    -> RuntimeStates contient Choice_A

copie FGridPartyInventoryState
    -> ne contient pas Choice_A

ResetRuntimeState()
    -> Choice_A perdu

RestorePersistentState(ClassProgressionStates)
    -> Choice_A revient
```

Il existe donc une duplication runtime/save autour d'un état métier qui devrait avoir une autorité durable unique.

## 6. Projection après level-up

Un level-up appelle :

```cpp
FRPGClassProgressionTransactionService::RefreshCharacterProjection()
```

Le cache runtime conserve alors les choix déjà acquis tout en recalculant :

```text
CharacterLevel
choice points granted
remaining points
automatic requirement grants
availability des nouveaux choix
```

Exemple caractérisé :

```text
level 2
    Choice_A acquis
    1 point accordé / 1 dépensé

XP -> 3000
level -> 3

projection refresh
    Choice_A conservé
    2 points accordés
    1 point restant
    Choice_B devient sélectionnable
```

## 7. Tests ajoutés

Filtre :

```text
Grimrock.TechnicalDebt.TD07_3_3_5.Characterization
```

Tests :

```text
ExperienceLevelContract
StoredLevelSynchronization
ProgressionMirrorBoundary
LevelUpProjectionRefresh
```

Attendu :

```text
Succeeded              : 4
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0
```

## 8. Direction de normalisation envisagée

Après validation du gate, la cible privilégiée sera évaluée ainsi :

```text
FGridCharacterInventoryState
    Experience               autorité durable
    Level                    candidat suppression / projection
    SelectedChoiceIds        candidat autorité durable

FRPGClassProgressionTransactionService
    SatisfiedRequirements    projection/cache reconstructible
    CharacterLevel           projection
    SelectedChoiceIds        ne devrait plus être une autorité parallèle

UGrimrockPartySaveGame
    ClassProgressionStates   candidat suppression si SelectedChoiceIds
                             vit directement dans le character state
```

La décision finale sera prise après validation UE du gate.

## 9. Invariants à préserver

La normalisation ne doit pas changer :

```text
courbe XP
niveau maximum
règles multi-level
bonus HP/Mana du level-up
budget de choix de classe
prérequis des choix
ordre normalisé des ChoiceIds
projection des requirement tags
atomicité des commits de choix
```

## 10. Hors périmètre

Cette phase ne modifie pas :

```text
SaveGame v13
Pending Level Up notifications
Skills
Spellbook
Status Effects
Talent presentation
DataAssets
Blueprints
maps
```

Les notifications Level Up sont réservées à TD07.3.3.9.

## 11. Validation

À exécuter :

```powershell
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.TechnicalDebt.TD07_3_3_5.Characterization"
```

Le build doit être exécuté : un nouveau fichier C++ de tests est ajouté.

## 12. Stop condition du gate

- [x] relation Experience -> Level documentée ;
- [x] synchronisation stored Level documentée ;
- [x] refus de démotion documenté ;
- [x] choix métier distingués des projections ;
- [x] duplication RuntimeStates / ClassProgressionStates documentée ;
- [x] projection après level-up documentée ;
- [x] 4 tests de caractérisation ajoutés ;
- [x] compilation UE5.5.4 verte ;
- [x] 4 tests exécutés sans échec.

Validation locale du 27 août 2026 :

```text
Filter                  : Grimrock.TechnicalDebt.TD07_3_3_5.Characterization
Succeeded               : 3
Succeeded with warnings : 1
Failed                  : 0
Not run                 : 0
Process exit code        : 0
```

Le warning provient du scénario volontaire `WouldDemote`. Il est désormais déclaré via `AddExpectedError` afin que la prochaine régression du gate soit silencieuse.

Le gate est atteint. La phase B de normalisation peut commencer.
