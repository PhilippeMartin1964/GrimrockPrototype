# MON20.7.2 — Talent Runtime Read Model / Selected Character

Statut : **VALIDÉ UE5.5.4 — 8/8 AUTOMATION SUCCESS**  
Date : **24 août 2026**  
Jalon parent : **MON20.7 — Talents / Progression Choice Integration**

---

## 1. Objectif

Exposer une lecture métier « Talent » au-dessus du système MON15 existant, sans créer de nouvel état, de nouvelle monnaie ni de nouveau workflow transactionnel.

MON20.7.2 permet aux futurs consommateurs/UI de demander :

```text
quels talents ce personnage possède ?
possède-t-il tel talent ?
combien de points de talent/progression lui restent ?
quels talents peut-il sélectionner maintenant ?
```

pour :

```text
un CharacterIndex explicite
ou le personnage actuellement sélectionné
```

---

## 2. Autorité inchangée

Le contrat reste :

```text
URPGClassAsset.ProgressionChoices
        ↓
FRPGClassProgressionService
        ↓
FRPGClassProgressionTransactionService
        ↓
runtime state MON15 keyed by CharacterId
        ↓
SaveGame progression MON15.6
```

MON20.7.2 n'ajoute aucun :

```text
TalentId parallèle
TalentPoints parallèle
TalentState parallèle
TalentSaveState parallèle
```

Le terme `Talent` est uniquement une façade de domaine / présentation.

---

## 3. Service ajouté

Nouveau service sans état :

```text
FRPGTalentRuntimeService
```

Il ne possède aucune donnée membre et ne propose aucune mutation.

Toute acquisition continue à passer par :

```text
FRPGClassProgressionTransactionService::TryCommitChoices(...)
```

### 3.1 Talents acquis

API :

```text
TryGetSelectedTalents
TryGetSelectedCharacterTalents
```

La source autoritaire est :

```text
FRPGClassProgressionTransactionService::TryGetSelectedChoiceIds
```

La façade reconstruit ensuite les vues dans l'ordre data-driven de :

```text
URPGClassAsset::ProgressionChoices
```

### 3.2 HasTalent

API :

```text
HasTalent
HasSelectedCharacterTalent
```

Un `ChoiceId` connu et acquis renvoie `true`.
Un `ChoiceId` connu mais non acquis renvoie `false`.
Un identifiant vide, une classe invalide ou un index invalide rejette la requête.

### 3.3 Solde de points

API :

```text
TryGetTalentPointBalance
TryGetSelectedCharacterTalentPointBalance
```

Le résultat :

```text
FRPGTalentPointBalance
    GrantedPoints
    SpentPoints
    RemainingPoints
```

est un alias de lecture du calcul MON15 :

```text
FRPGClassProgressionTransactionService::TryGetChoicePointBalance
```

Aucun calcul de points n'est dupliqué.

### 3.4 Talents disponibles

API :

```text
TryGetAvailableTalents
TryGetSelectedCharacterAvailableTalents
```

Chaque `ProgressionChoice` est évalué via :

```text
FRPGClassProgressionService::GetChoiceAvailability
```

Seuls les choix avec :

```text
AvailabilityReason == None
```

sont exposés comme disponibles.

Donc niveau minimum, prérequis, sélection déjà acquise et budget restent exclusivement gouvernés par MON15.

---

## 4. Read model

`FRPGTalentRuntimeView` expose :

```text
ChoiceId
DisplayName
Description
MinimumLevel
PointCost
bSelected
AvailabilityReason
```

Il ne contient aucune copie persistante du talent : il est reconstruit à la demande depuis la classe et la sélection MON15.

Pour un talent acquis, `AvailabilityReason` reflète naturellement :

```text
AlreadySelected
```

Pour un talent disponible :

```text
None
```

---

## 5. Personnage sélectionné

Les variantes `SelectedCharacter` utilisent uniquement :

```text
UGridPartyInventoryComponent::GetSelectedCharacterIndex()
```

Il n'existe aucune sélection Talent indépendante.

Changer le personnage sélectionné via l'API inventaire existante change immédiatement la source des lectures Talent.

---

## 6. Atomicité / absence de mutation

MON20.7.2 est entièrement read-only.

Les appels peuvent demander au service MON15 de rafraîchir son cache dérivé, mais ils ne modifient jamais :

- la liste autoritaire des choix acquis ;
- les points accordés ;
- le niveau ;
- la classe ;
- l'inventaire ;
- le SaveGame.

Un index invalide :

```text
retourne false
réinitialise les sorties
ne modifie aucun choix acquis
```

---

## 7. Fichiers

Ajoutés :

```text
Source/GrimrockPrototype/Public/RPG/RPGTalentRuntimeService.h
Source/GrimrockPrototype/Private/RPG/RPGTalentRuntimeService.cpp
Source/GrimrockPrototype/Private/Tests/RPGMON2072TalentRuntimeReadModelTests.cpp
```

Documentation :

```text
docs/Design/MON20_7_2_TALENT_RUNTIME_READ_MODEL.md
```

Aucun `.uasset`, `.umap` ou changement SaveGame.

---

## 8. Validation UE5.5.4

Campagne Automation fournie le **24 août 2026** :

```text
Grimrock.MON20.7.Talents.AvailableAfterPrerequisite  Success
Grimrock.MON20.7.Talents.AvailableBeforeSelection    Success
Grimrock.MON20.7.Talents.ExplicitSelectedTalents     Success
Grimrock.MON20.7.Talents.HasTalent                   Success
Grimrock.MON20.7.Talents.InvalidIndexNoMutation      Success
Grimrock.MON20.7.Talents.PointBalance                Success
Grimrock.MON20.7.Talents.SelectedCharacterAuthority  Success
Grimrock.MON20.7.Talents.SelectedCharacterFacade     Success
```

Résultat final :

```text
8 / 8 Success
0 Fail
0 Error
```

Le log confirme notamment :

- acquisition et lecture cohérentes via le runtime MON15 ;
- prérequis correctement recalculés après acquisition ;
- budget `Granted / Spent / Remaining` cohérent ;
- changement de `SelectedCharacterIndex` respecté par la façade ;
- isolation par `CharacterId` ;
- index invalide rejeté sans mutation.

Aucun PIE n'est requis : MON20.7.2 n'introduit ni UI ni interaction monde.

---

## 9. Résultat

MON20.7.2 est **VALIDÉ UE5.5.4 — CLOS**.

Aucun second état Talent, aucune monnaie parallèle et aucune migration SaveGame n'ont été introduits.

---

## 10. Suite

```text
MON20.7.3 — Level Up Talent Presentation Contract
```

Cette tranche fait évoluer la présentation du Level Up existant pour rendre explicite le vocabulaire Talent sans créer un second workflow de sélection.