# MON20.7.3 — Level Up Talent Presentation Contract

Statut : **VALIDÉ UE5.5.4 — 16/16 AUTOMATION SUCCESS**  
Date : **24 août 2026**  
Jalon parent : **MON20.7 — Talents / Progression Choice Integration**

---

## 1. Objectif

Faire évoluer la présentation du Level Up MON15 pour rendre explicite le vocabulaire **Talent** sans créer un second widget, un second workflow de sélection ou une seconde transaction.

Le contrat métier reste strictement :

```text
Talent de classe = FRPGClassProgressionChoiceDefinition
TalentId sémantique = ChoiceId
Points de talent = ChoicePoints
Acquisition = FRPGClassProgressionTransactionService::TryCommitChoices
```

---

## 2. Décision d'architecture

MON20.7.3 réutilise :

```text
URPGLevelUpWidget
FRPGLevelUpView
FRPGLevelUpChoiceView
PendingChoiceIds
StageOrUnstageChoice
ConfirmSelection
CancelSelection
```

Aucun `URPGTalentWidget`, aucun `TalentPendingState` et aucune transaction parallèle ne sont ajoutés.

Les noms C++ historiques `ChoiceId`, `GrantedChoicePoints`, `SpentChoicePoints` et `RemainingChoicePoints` restent inchangés afin de préserver compatibilité MON15 et SaveGame.

Le vocabulaire Talent est une couche de présentation.

---

## 3. Contrat de présentation

`FRPGLevelUpView` expose une sous-vue :

```text
FRPGLevelUpPresentationView
```

avec :

```text
Title
TalentSectionTitle
TalentPointsLabel
EmptyTalentsMessage
SelectionPrompt
```

Valeurs MON20.7.3 :

```text
Title               = NIVEAU SUPÉRIEUR
TalentSectionTitle  = TALENTS DE CLASSE
TalentPointsLabel   = Points de talent
EmptyTalentsMessage = Aucun talent de classe à sélectionner pour ce niveau.
SelectionPrompt     = Sélectionnez un talent, ou annulez pour le reporter.
```

Cette sous-vue est `BlueprintReadOnly`, de sorte qu'un WBP puisse reprendre exactement le même contrat sans reconstruire la logique.

---

## 4. Slate natif

Le fallback Slate existant est conservé.

Il utilise désormais le contrat de présentation pour :

- le titre de la modal ;
- l'en-tête `TALENTS DE CLASSE` ;
- la ligne de points ;
- le message lorsqu'aucun talent n'est disponible dans la classe ;
- l'invite demandant de sélectionner un talent.

Les boutons de choix continuent à porter le `ChoiceId` existant et appellent toujours :

```text
StageOrUnstageChoice(ChoiceId)
```

---

## 5. Sémantique des états

La présentation ne change pas les états MON15 :

```text
bCommitted = talent acquis
bPending   = talent sélectionné dans la transaction staged
bAvailable = talent actuellement sélectionnable
```

Les raisons d'indisponibilité restent :

```text
AlreadySelected
LevelTooLow
MissingPrerequisite
InsufficientChoicePoints
...
```

Les textes utilisateur emploient désormais le vocabulaire Talent lorsque cela est pertinent, mais les enums et règles restent ceux de MON15.

---

## 6. Invariants

```text
ChoiceId avant MON20.7.3 == ChoiceId après MON20.7.3
transaction de confirmation inchangée
annulation sans mutation inchangée
budget calculé par MON15 inchangé
prérequis calculés par MON15 inchangés
projection RequirementIds inchangée
SaveGame inchangé
```

---

## 7. Automation — VALIDÉ

Filtre cumulatif :

```text
Grimrock.MON20.7.Talents
```

Tests MON20.7.3 :

```text
PresentationVocabulary
PresentationChoiceIdentity
PresentationAvailableTalent
PresentationPendingTalent
PresentationCommittedTalent
PresentationPointBudget
PresentationConfirmTransaction
PresentationCancelNoMutation
```

Validation fournie le **24 août 2026** :

```text
16 / 16 Success
0 Fail
0 Error
```

Les 8 tests MON20.7.2 restent verts et les 8 nouveaux tests MON20.7.3 passent tous.

La validation confirme notamment :

- vocabulaire Talent exposé par la vue ;
- identité `ChoiceId` inchangée ;
- talent disponible correctement présenté ;
- talent staged correctement présenté ;
- talent acquis correctement présenté ;
- budget de points identique à MON15 ;
- confirmation via la transaction MON15 ;
- annulation sans mutation.

Aucun PIE n'était requis pour cette tranche native/contractuelle.

---

## 8. Hors scope

MON20.7.3 ne fait pas :

- refonte graphique complète du WBP Level Up ;
- nouvel arbre visuel de talents ;
- drag & drop ;
- nouveaux talents de production dans les `.uasset` de classe ;
- bonus passifs numériques génériques ;
- RequirementIds transversaux supplémentaires ;
- migration SaveGame.

---

## 9. Fichiers implémentés

```text
Source/GrimrockPrototype/Public/UI/RPGLevelUpWidget.h
Source/GrimrockPrototype/Private/UI/RPGLevelUpWidget.cpp
Source/GrimrockPrototype/Private/UI/RPGLevelUpWidgetSlate.cpp
Source/GrimrockPrototype/Private/Tests/RPGMON2073TalentPresentationTests.cpp
docs/Design/MON20_7_3_LEVEL_UP_TALENT_PRESENTATION_CONTRACT.md
```

Aucun `.uasset` / `.umap` n'est requis.

---

## 10. Suite

```text
MON20.7.4 — Requirement Projection / Persistence Regression
```

MON20.7.4 doit vérifier que les talents restent correctement projetés vers `SatisfiedRequirements` et persistent/reviennent exclusivement via le snapshot MON15.6 existant, sans nouvelle donnée Talent.
