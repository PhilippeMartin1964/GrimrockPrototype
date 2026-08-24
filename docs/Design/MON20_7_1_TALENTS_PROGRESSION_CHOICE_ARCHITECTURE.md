# MON20.7.1 — Talents / Progression Choice Integration — Audit & Architecture Contract

Statut : **TERMINÉ — contrat d’implémentation défini**  
Date : **24 août 2026**  
Jalon parent : **MON20.7 — Talents / Progression Choice Integration**

---

## 1. Objectif

Définir comment les talents doivent s’intégrer au système de progression existant sans introduire un second arbre, une seconde monnaie ou un second registre de choix.

MON20.7 part du principe déjà fixé par MON20.1 et MON20.6 :

```text
Talent de classe = ProgressionChoice existant
```

sauf preuve qu’un besoin de gameplay ne peut pas être exprimé par le contrat MON15.

---

## 2. Audit de l’existant

### 2.1 Définition de choix déjà complète

`URPGClassAsset` contient :

```text
FRPGClassProgressionChoiceDefinition
    ChoiceId
    DisplayName
    Description
    MinimumLevel
    PointCost
    PrerequisiteChoiceIds
    GrantedRequirementIds
```

Ce contrat couvre déjà :

- identité stable ;
- coût ;
- niveau minimum ;
- dépendances / arbre ;
- projection vers les systèmes consommateurs.

Décision : **ne pas créer `TalentId`**. `ChoiceId` est l’identité stable d’un talent de classe.

### 2.2 Monnaie déjà existante

`FRPGClassProgressionLevelGrant::ChoicePointsGranted` fournit déjà la monnaie de progression.

`FRPGClassProgressionService` calcule :

```text
GrantedPoints
SpentPoints
RemainingPoints
```

Décision : **ne pas créer `TalentPoints`**. Les talents dépensent les ChoicePoints existants.

### 2.3 Transaction déjà atomique

`FRPGClassProgressionTransactionService` expose déjà :

```text
TryGetSelectedChoiceIds
TryGetChoicePointBalance
TryCommitChoices
AppendRuntimeSatisfiedRequirements
CapturePersistentState
RestorePersistentState
```

`TryCommitChoices()` valide avant mutation :

- personnage ;
- classe ;
- état courant ;
- choix demandé ;
- doublons ;
- niveau ;
- prérequis ;
- budget.

Décision : MON20.7 **réutilise cette transaction**. Aucun `FRPGTalentTransactionService` parallèle n’est justifié.

### 2.4 Persistance déjà disponible

MON15.6 persiste les choix par `CharacterId` via le snapshot de progression.

Décision : les talents héritent de cette persistance. MON20.7 ne change pas la version SaveGame et n’ajoute pas un snapshot Talent séparé.

### 2.5 UI Level Up déjà compatible

`URPGLevelUpWidget` possède déjà :

```text
FRPGLevelUpChoiceView
    ChoiceId
    DisplayName
    Description
    PointCost
    MinimumLevel
    bCommitted
    bPending
    bAvailable
    StatusText
```

La sélection est staged puis confirmée atomiquement.

Décision : MON20.7 doit réutiliser cette UI et son `View`. Une future présentation « Talents » est une évolution de vocabulaire/présentation, pas un nouveau workflow transactionnel.

---

## 3. Contrat sémantique retenu

Pour MON20.7 :

```text
ProgressionChoice == Talent de classe sélectionnable
ChoiceId          == TalentId sémantique
ChoicePoints      == points de talent/progression
PrerequisiteChoiceIds == dépendances du talent
GrantedRequirementIds == effets génériques exposés aux systèmes consommateurs
```

Le code métier continue à employer les noms MON15 (`ChoiceId`, `ProgressionChoices`) afin de préserver compatibilité et persistance.

Le terme **Talent** est une couche de domaine / présentation, pas une nouvelle autorité.

---

## 4. Effets de talents

MON20.7 ne doit pas inventer un moteur d’effets passifs parallèle.

Le mécanisme déjà disponible est :

```text
Talent choisi
    -> ChoiceId satisfait
    -> GrantedRequirementIds satisfaits
    -> AppendRuntimeSatisfiedRequirements
    -> consommateurs MON12 / MON20.8
```

Exemples futurs :

```text
Talent_WeaponMastery
    -> Req_WeaponMastery

Talent_ArcaneAdept
    -> Req_ArcaneAdept

Talent_ImprovedLockpicking
    -> Req_ImprovedLockpicking
```

MON20.8 décidera comment ces RequirementIds modifient actions, UI ou autres systèmes.

Les bonus numériques passifs qui ne peuvent pas être représentés proprement par un RequirementId devront faire l’objet d’un contrat séparé fondé sur un besoin concret ; ils ne sont pas introduits préventivement dans MON20.7.1.

---

## 5. Runtime API MON20.7

Le système autoritaire reste `FRPGClassProgressionTransactionService`.

MON20.7 peut ajouter une façade **sans état** uniquement si elle apporte une API de lecture utile aux consommateurs/UI, par exemple :

```text
HasTalent(CharacterIndex, ChoiceId)
TryGetSelectedTalents(CharacterIndex)
TryGetAvailableTalents(CharacterIndex)
TryGetTalentPointBalance(CharacterIndex)
```

Cette façade doit déléguer au système MON15 et ne jamais maintenir sa propre sélection.

La mutation continue à passer par :

```text
TryCommitChoices(...)
```

---

## 6. Sélection et personnage actif

Comme MON20.6, les variantes destinées à l’UI peuvent résoudre :

```text
CharacterIndex explicite
ou
UGridPartyInventoryComponent::GetSelectedCharacterIndex()
```

Il n’existe pas de sélection Talent indépendante.

---

## 7. Invariants

```text
un ChoiceId = une identité de talent stable
un talent ne peut être acquis qu’une fois
niveau minimum respecté
prérequis déjà acquis ou staged dans le même lot
budget ChoicePoints respecté
transaction invalide -> aucune mutation
transaction valide -> notification unique
projection RequirementIds immédiatement rafraîchie
persistance -> snapshot progression MON15.6 existant
```

---

## 8. Hors scope MON20.7

Ne pas ajouter dans ce jalon :

- `URPGTalentAsset` parallèle ;
- `TalentId` parallèle à `ChoiceId` ;
- `TalentPoints` parallèle aux ChoicePoints ;
- second arbre de dépendances ;
- nouveau SaveGame Talent ;
- migration SaveGame ;
- système générique de buffs passifs ;
- intégration Skill -> RequirementIds ;
- lockpicking ;
- refonte complète de `URPGLevelUpWidget`.

---

## 9. Découpage proposé MON20.7

```text
MON20.7.1 — Audit & Architecture Contract                    TERMINÉ
MON20.7.2 — Talent Runtime Read Model / Selected Character
MON20.7.3 — Level Up Talent Presentation Contract
MON20.7.4 — Requirement Projection / Persistence Regression
MON20.7.5 — Automation Regression / Closure
```

MON20.7.2 doit rester petit : une façade de lecture sans état au-dessus de MON15, avec tests Automation et aucune nouvelle donnée persistante.

---

## 10. Critères de sortie MON20.7.2

```text
personnage explicite valide -> talents sélectionnés lisibles
personnage sélectionné      -> même source autoritaire
HasTalent connu acquis       -> true
HasTalent non acquis         -> false
balance points               -> identique au service MON15
choix disponibles            -> calculés par règles MON15
index invalide               -> rejet sans mutation
aucun second état Talent     -> confirmé
aucune migration SaveGame    -> confirmé
```

---

## 11. Conclusion

MON20.7 n’a pas besoin d’un nouveau système de talents.

L’architecture retenue est :

```text
URPGClassAsset.ProgressionChoices
        +
FRPGClassProgressionService
        +
FRPGClassProgressionTransactionService
        +
URPGLevelUpWidget
        =
Talents de classe
```

Le prochain travail autoritaire est :

```text
MON20.7.2 — Talent Runtime Read Model / Selected Character
```
