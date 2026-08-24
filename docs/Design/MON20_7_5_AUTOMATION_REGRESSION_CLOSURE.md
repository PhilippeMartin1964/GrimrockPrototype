# MON20.7.5 — Automation Regression / Closure

Statut : **VALIDÉ UE5.5.4 — CLOS**  
Date : **24 août 2026**  
Jalon parent : **MON20.7 — Talents / Progression Choice Integration**

---

## 1. Objet de la clôture

MON20.7.5 clôt le chantier Talent après la validation cumulative des tranches MON20.7.2, MON20.7.3 et MON20.7.4.

Aucun code runtime supplémentaire n'est requis dans cette tranche : la passe Automation cumulative constitue la régression finale du jalon.

---

## 2. Architecture finale

MON20.7 réutilise intégralement le système de progression MON15 :

```text
URPGClassAsset.ProgressionChoices
    -> FRPGClassProgressionChoiceDefinition
       -> ChoiceId
       -> PointCost
       -> MinimumLevel
       -> PrerequisiteChoiceIds
       -> GrantedRequirementIds

FRPGClassProgressionService
    -> disponibilité / budget / requirements

FRPGClassProgressionTransactionService
    -> sélection autoritaire
    -> transaction atomique
    -> projection RequirementIds
    -> capture / restore MON15.6

FRPGTalentRuntimeService
    -> façade read-only Talent

URPGLevelUpWidget
    -> même workflow de sélection MON15
    -> vocabulaire de présentation Talent
```

Le domaine Talent n'introduit donc aucun état parallèle.

---

## 3. Invariants confirmés

```text
Talent métier                   == ProgressionChoice MON15
identité Talent                 == ChoiceId
points Talent                   == ChoicePoints MON15
acquisition                     == TryCommitChoices()
persistance                     == FRPGCharacterProgressionSaveState
clé persistante                 == CharacterId
projection inter-systèmes       == RequirementIds
SaveGame version                == 7
```

Absences intentionnelles :

```text
pas de TalentId parallèle
pas de TalentPoints parallèle
pas de TalentState parallèle
pas de TalentSaveGame parallèle
pas de second arbre de talents
pas de second widget de progression
```

---

## 4. Validation Automation finale

Filtre :

```text
Grimrock.MON20.7.Talents
```

Résultat utilisateur sous UE5.5.4 :

```text
24 / 24 Success
0 Fail
0 Error
```

### MON20.7.2 — Runtime Read Model

```text
AvailableAfterPrerequisite
AvailableBeforeSelection
ExplicitSelectedTalents
HasTalent
InvalidIndexNoMutation
PointBalance
SelectedCharacterAuthority
SelectedCharacterFacade
```

### MON20.7.3 — Level Up Talent Presentation

```text
PresentationAvailableTalent
PresentationCancelNoMutation
PresentationChoiceIdentity
PresentationCommittedTalent
PresentationConfirmTransaction
PresentationPendingTalent
PresentationPointBudget
PresentationVocabulary
```

### MON20.7.4 — Requirement Projection / Persistence

```text
CaptureUsesMON15Snapshot
InvalidRestoreAtomic
RequirementAfterTalent
RequirementBeforeTalent
RequirementCharacterIsolation
RestoreByCharacterId
RestoreDetachedRequirements
RestoreTalentReadModel
```

---

## 5. Contrats régressés

La passe finale confirme :

- lecture Talent par personnage explicite et sélectionné ;
- isolation par `CharacterId` ;
- budget et prérequis MON15 inchangés ;
- identité `ChoiceId` inchangée dans la présentation ;
- confirmation via transaction MON15 ;
- annulation sans mutation ;
- projection immédiate de `ChoiceId` et `GrantedRequirementIds` ;
- capture dans `ClassProgressionStates` ;
- restore du read model Talent ;
- restore détaché des RequirementIds ;
- atomicité d'un restore invalide ;
- restore indépendant de l'ordre des snapshots ;
- `UGrimrockPartySaveGame::CurrentSaveVersion == 7`.

---

## 6. Fichiers de référence

```text
Source/GrimrockPrototype/Public/RPG/RPGTalentRuntimeService.h
Source/GrimrockPrototype/Private/RPG/RPGTalentRuntimeService.cpp
Source/GrimrockPrototype/Public/UI/RPGLevelUpWidget.h
Source/GrimrockPrototype/Private/UI/RPGLevelUpWidget.cpp
Source/GrimrockPrototype/Private/UI/RPGLevelUpWidgetSlate.cpp
Source/GrimrockPrototype/Private/Tests/RPGMON2072TalentRuntimeReadModelTests.cpp
Source/GrimrockPrototype/Private/Tests/RPGMON2073TalentPresentationTests.cpp
Source/GrimrockPrototype/Private/Tests/RPGMON2074TalentPersistenceRegressionTests.cpp
```

Documents :

```text
docs/Design/MON20_7_1_TALENTS_PROGRESSION_CHOICE_ARCHITECTURE.md
docs/Design/MON20_7_2_TALENT_RUNTIME_READ_MODEL.md
docs/Design/MON20_7_3_LEVEL_UP_TALENT_PRESENTATION_CONTRACT.md
docs/Design/MON20_7_4_REQUIREMENT_PROJECTION_PERSISTENCE_REGRESSION.md
docs/Design/MON20_7_5_AUTOMATION_REGRESSION_CLOSURE.md
```

---

## 7. Conclusion

```text
MON20.7 — Talents / Progression Choice Integration
VALIDÉ UE5.5.4 — CLOS — 24/24
```

Le prochain travail autoritaire est :

```text
MON20.8.1 — Cross-System Requirements / Actions / UI — Audit & Contract
```
