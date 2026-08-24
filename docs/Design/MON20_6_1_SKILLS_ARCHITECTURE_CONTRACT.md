# MON20.6.1 — Skills Data Model & Runtime — Audit & Architecture Contract

Statut : **TERMINÉ — contrat d’implémentation défini**  
Date : **24 août 2026**  
Jalon parent : **MON20.6 — Skills Data Model & Runtime**

---

## 1. Objectif

Définir le plus petit modèle de compétences compatible avec l’architecture existante, avant d’ajouter du code runtime.

MON20.6 doit répondre à trois besoins qui ne sont pas couverts par les `ProgressionChoices` de MON15 :

- un **rang numérique** par compétence ;
- une progression pouvant être indépendante d’un talent de classe ;
- des **tests de compétence hors combat** utilisables par serrures, pièges, exploration, scripts et futures quêtes.

Le système doit rester orienté données et ne doit pas créer un second registre de personnages.

---

## 2. Audit de l’existant

### 2.1 Autorité du personnage

L’état actif d’un personnage reste dans :

```text
UGridPartyInventoryComponent
    -> FGridPartyInventoryState
        -> FGridCharacterInventoryState[]
```

`FGridCharacterInventoryState` contient déjà l’identité stable `CharacterId`, la classe, la race, le niveau et les six attributs RPG.

Décision : les compétences appartiennent au personnage existant. Aucun `SkillCharacterState` parallèle global ne sera créé.

### 2.2 Attributs disponibles

Le projet possède déjà :

```text
Strength
Dexterity
Constitution
Intelligence
Wisdom
Charisma
```

Il n’existe pas encore d’enum générique pour désigner un attribut. MON20.6 introduira un enum local au domaine Skill afin d’éviter un refactor transversal prématuré.

### 2.3 Talents / ProgressionChoices déjà existants

`URPGClassAsset` possède déjà :

```text
FRPGClassProgressionLevelGrant
FRPGClassProgressionChoiceDefinition
```

avec notamment :

```text
ChoiceId
MinimumLevel
PointCost
PrerequisiteChoiceIds
GrantedRequirementIds
```

`FRPGClassProgressionTransactionService` gère déjà la sélection transactionnelle, le solde de points, la projection des RequirementIds et la persistance.

Décision : **MON20.6 ne réimplémente pas les talents**. Ils restent réservés à MON20.7 et réutiliseront ce système.

### 2.4 RequirementIds existants

Le catalogue MON12 consomme déjà un ensemble :

```text
TSet<FName> SatisfiedRequirements
```

et MON15 sait y projeter ses grants.

Décision : le modèle Skill doit pouvoir produire plus tard des RequirementIds, mais cette intégration croisée sera traitée dans MON20.8. MON20.6 reste centré sur définition, rangs et résolution des tests.

### 2.5 SaveGame

`UGrimrockPartySaveGame` est actuellement en version 7 et possède des snapshots spécialisés pour progression, status effects et spellbook.

La roadmap réserve explicitement persistance/migration à MON20.9.

Décision : **MON20.6 n’incrémente pas la version de sauvegarde**. Les rangs de compétences sont runtime pour cette tranche. Leur capture/restauration sera ajoutée dans MON20.9 avec un snapshot keyed by `CharacterId`.

### 2.6 Serrures et exploration

Le design des serrures prévoit déjà un besoin concret :

```text
LockpickDifficulty
compétence de crochetage
outil de crochetage
```

mais aucun domaine `SkillId` n’existe actuellement en production.

Le premier consommateur naturel d’un test de compétence sera donc le futur crochetage, sans coupler directement le modèle Skill au `WallLockActor` dans MON20.6.

---

## 3. Contrat de données retenu

### 3.1 Definition asset

Créer :

```text
URPGSkillAsset : UPrimaryDataAsset
```

Champs minimaux :

```text
SkillId
DisplayName
Description
GoverningAttribute
MaxRank
bAllowUntrainedChecks
```

`SkillId` est l’identité stable data-driven.

### 3.2 Attribut directeur

Créer :

```text
ERPGSkillGoverningAttribute
    None
    Strength
    Dexterity
    Constitution
    Intelligence
    Wisdom
    Charisma
```

`None` permet une compétence purement basée sur son rang lorsque nécessaire.

### 3.3 État runtime d’un personnage

Créer une structure légère :

```text
FRPGSkillRank
    SkillId
    Rank
```

et une collection runtime dans `FGridCharacterInventoryState` :

```text
SkillRanks : TArray<FRPGSkillRank>
```

La propriété sera `Transient` dans MON20.6 afin de ne pas modifier le contrat SaveGame avant MON20.9.

Pourquoi dans `FGridCharacterInventoryState` :

- l’état reste attaché à l’autorité du personnage ;
- le recrutement et la sélection de personnage utilisent déjà cette structure ;
- pas de map statique globale cachée ;
- pas de second registre CharacterId -> Skills.

### 3.4 Invariants

Pour un personnage :

```text
SkillId != None
Rank >= 0
Rank <= Definition.MaxRank
un seul enregistrement par SkillId
rang 0 = compétence non entraînée
```

Une entrée de rang 0 n’a pas besoin d’être stockée physiquement.

---

## 4. Runtime API retenue

Créer un service sans UObject métier :

```text
FRPGSkillService
```

API minimale de MON20.6.2 :

```text
GetSkillRank(CharacterState, SkillId)
TrySetSkillRank(CharacterState, SkillDefinition, NewRank)
TryIncreaseSkillRank(CharacterState, SkillDefinition, Delta)
ValidateSkillState(CharacterState)
```

Règles :

- échec = aucune mutation ;
- `NewRank == 0` supprime l’entrée sparse ;
- aucun doublon de SkillId ;
- aucune mutation de classe, niveau, XP ou ProgressionChoices ;
- aucun broadcast inventaire dans le service pur lui-même.

Une couche transactionnelle/notifiante pourra être ajoutée uniquement lorsqu’un vrai flux de dépense de points sera défini.

---

## 5. Contrat des tests de compétence

MON20.6.3 introduira un resolver séparé :

```text
FRPGSkillCheckService
```

Entrées prévues :

```text
SkillDefinition
CharacterState
Difficulty
FRandomStream optionnel/déterministe
```

Sortie prévue :

```text
FRPGSkillCheckResult
    SkillId
    Rank
    AttributeValue
    AttributeModifier
    Roll
    Total
    Difficulty
    Success
```

La formule exacte sera figée dans MON20.6.3 avec tests Automation. Elle ne sera pas cachée dans les acteurs de serrure ou de trigger.

---

## 6. Hors scope MON20.6

Ne pas ajouter maintenant :

- arbre de talents ;
- `TalentId` parallèle ;
- nouvelle monnaie de talents ;
- SaveGame v8 ;
- UI complète de dépense de points ;
- modification des `.uasset` de classes/races ;
- couplage direct à `AGridWallLockActor` ;
- formules de pièges, dialogues ou quêtes ;
- refactor global de `FRPGAttributes`.

---

## 7. Découpage MON20.6

```text
MON20.6.1 — Audit & Architecture Contract              TERMINÉ
MON20.6.2 — Skill Definition + Character Runtime Ranks PROCHAIN
MON20.6.3 — Deterministic Skill Check Resolution
MON20.6.4 — Runtime Access / Character Selection API
MON20.6.5 — Automation Regression / Closure
```

Les intégrations suivantes restent dans leurs jalons dédiés :

```text
MON20.7 — Talents / Progression Choice Integration
MON20.8 — Requirements / Actions / UI
MON20.9 — Persistence / Migration / Reserve
```

---

## 8. Critères de sortie MON20.6.2

```text
Skill definition valide                 -> acceptée
SkillId None                            -> rejeté
MaxRank invalide                        -> rejeté
rang absent                             -> 0
set rang valide                         -> une seule entrée
augmentation valide                     -> rang augmenté
rang > MaxRank                          -> rejet sans mutation
rang < 0                                -> rejet sans mutation
set rang 0                              -> entrée supprimée
SkillId dupliqué dans CharacterState    -> état détecté invalide
aucune migration SaveGame               -> confirmé
```

---

## 9. Conclusion

MON20.6 a bien besoin d’un domaine Skill dédié : les `ProgressionChoices` sont adaptés aux talents binaires, mais pas aux rangs numériques ni aux tests hors combat.

Le contrat retenu reste volontairement petit :

```text
URPGSkillAsset
    + FRPGSkillRank dans le CharacterState existant
    + FRPGSkillService pur
    + FRPGSkillCheckService ensuite
```

Aucun second système de personnage, aucune nouvelle sauvegarde et aucun couplage prématuré aux acteurs du donjon.
