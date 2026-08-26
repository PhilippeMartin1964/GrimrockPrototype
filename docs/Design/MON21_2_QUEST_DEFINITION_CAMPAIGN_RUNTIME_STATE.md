# MON21.2 — Quest Definition + Campaign Runtime State

Date : 26 août 2026  
Baseline : `1912f2e7b6d3401f7aa161351a0dd051ee7ed6db` — TD05 RuntimeActor stop condition

## 1. Objet

MON21.2 introduit la fondation métier Quest prévue par MON21.1 :

```text
QuestDefinition data-driven
    -> identité stable QuestId

Quest runtime state unique par QuestId
    -> campagne / groupe
```

Le jalon ne branche volontairement ni Event -> Command, ni SaveGame, ni Journal UMG. Ces responsabilités appartiennent respectivement à MON21.3, MON21.4 et MON21.5.

## 2. Autorité runtime

L’autorité de campagne est :

```text
UGridQuestSubsystem : UGameInstanceSubsystem
```

Ce choix respecte le contrat MON21.1 :

- aucune classe Actor manager ;
- une seule autorité de quête par GameInstance/session ;
- survit naturellement aux changements de map ;
- aucun Tick permanent ;
- indépendant de `UGridLevelAsset` et de `FGridDungeonRuntimeState` ;
- n’ajoute aucune responsabilité au `AGrimrockPartyPawn` après la campagne TD02/TD05.

La persistance reste absente en MON21.2. Le subsystem porte uniquement l’état runtime transient de la campagne.

## 3. Définition data-driven

```text
UGridQuestDefinitionAsset : UPrimaryDataAsset
```

Identité :

```text
QuestId : FName
```

Présentation :

```text
DisplayName
Description
```

Objectifs ordonnés :

```text
FGridQuestObjectiveDefinition
    ObjectiveId : FName
    DisplayName
    Description
```

Le tableau `Objectives` définit l’ordre séquentiel canonique.

Validation :

- `QuestId != NAME_None` ;
- titre de quête non vide ;
- `ObjectiveId != NAME_None` ;
- `ObjectiveId` unique dans la quête ;
- titre d’objectif non vide.

Une quête sans objectif est autorisée afin de permettre un contrat direct `StartQuest -> CompleteQuest`.

## 4. État runtime

Statut de quête :

```text
Inactive
Active
Completed
Failed
```

Statut d’objectif :

```text
Inactive
Active
Completed
Failed
```

Structures :

```text
FGridQuestObjectiveRuntimeState
FGridQuestRuntimeState
FGridCampaignQuestRuntimeState
```

L’état runtime ne stocke aucun pointeur de définition comme source de vérité. Il conserve uniquement :

- `QuestId` ;
- `ObjectiveId` ;
- statuts runtime.

Cela prépare un snapshot SaveGame stable par IDs en MON21.4.

## 5. Progression séquentielle

`StartQuest` :

- crée au plus un runtime state par `QuestId` ;
- passe la quête à `Active` ;
- active le premier objectif ;
- laisse les objectifs suivants `Inactive`.

`CompleteObjective` :

- exige que la quête soit `Active` ;
- exige que l’objectif ciblé soit l’objectif `Active` ;
- marque cet objectif `Completed` ;
- active l’objectif suivant ;
- complète automatiquement la quête après le dernier objectif.

`CompleteQuest` :

- permet la complétion explicite d’une quête active ;
- marque ses objectifs `Completed`.

`FailQuest` :

- échoue uniquement une quête active ;
- marque l’objectif courant `Failed` lorsqu’il existe.

Les transitions terminales ne sont pas réouvertes en MON21.2.

## 6. Registre des définitions

Le subsystem expose un registre transient :

```text
QuestId -> UGridQuestDefinitionAsset
```

`RegisterQuestDefinition` est idempotent pour le même asset et refuse deux définitions différentes revendiquant le même `QuestId`.

Le chargement/catalogage d’assets de production sera branché dans une tranche ultérieure ; aucun `.uasset` n’est requis pour cette fondation C++.

## 7. Notification

```text
OnQuestStateChanged(FName QuestId)
```

Cette notification ne transporte aucune copie de l’état autoritaire. Les futurs read models Journal devront relire le subsystem.

`QuestId == NAME_None` signifie un reset global de la campagne.

## 8. Validation runtime

`ValidateRuntimeState` protège :

- unicité des `QuestId` ;
- présence d’une définition enregistrée ;
- cohérence ordre/nombre des objectifs ;
- cohérence entre statut de quête et statuts d’objectifs ;
- progression séquentielle.

## 9. Automation

Filtres :

```text
Grimrock.Quests.MON21_2.DefinitionValidation
Grimrock.Quests.MON21_2.CampaignRuntimeState
```

Couverture :

- contrat `UPrimaryDataAsset` ;
- contrat `UGameInstanceSubsystem` ;
- IDs invalides/dupliqués ;
- collision de définition par `QuestId` ;
- état absent avant démarrage ;
- démarrage idempotent ;
- ordre d’objectifs ;
- refus d’une complétion hors séquence ;
- progression et complétion automatique ;
- complétion directe d’une quête sans objectif ;
- échec ;
- validation du runtime ;
- reset de campagne conservant les définitions.

## 10. Hors périmètre

MON21.2 ne modifie pas :

- `EGridObjectCommand` ;
- `UGridActivationComponent` ;
- Lua ;
- `UGrimrockPartySaveGame` / SaveVersion 9 ;
- `FGridDungeonRuntimeState` ;
- UMG / Journal ;
- Map ;
- Codex ;
- `.uasset` / `.umap`.

## 11. Suite

Après validation UE5.5.4 de MON21.2 :

```text
MON21.3 — Quest Event/Command Integration
```

MON21.3 devra adapter le bus Event -> Command existant pour demander des mutations au `UGridQuestSubsystem`, sans dupliquer son état ni introduire une seconde autorité Quest.
