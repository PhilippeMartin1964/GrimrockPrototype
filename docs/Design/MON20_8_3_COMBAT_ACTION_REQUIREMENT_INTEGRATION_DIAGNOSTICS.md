# MON20.8.3 — Combat Action Requirement Integration & Diagnostics

Statut : **IMPLÉMENTÉ — VALIDATION UE5.5.4 À FAIRE**  
Date : **24 août 2026**  
Jalon parent : **MON20.8 — Cross-System Requirements / Actions / UI**

---

## 1. Objectif

Brancher les RequirementIds produits par les Skills MON20.8.2 sur le catalogue de combat MON12, sans créer de pipeline parallèle, et rendre les requirements manquants inspectables de façon structurée par le runtime, les tests et l'UI.

Le contrat consommateur reste :

```text
FGridCombatActionDefinition::Requirements[]
```

L'autorité de disponibilité reste :

```text
FGridCombatActionCatalog
```

---

## 2. Pipeline final

```text
FGridCharacterInventoryState
    ├── ClassId
    ├── équipement -> ItemTags
    └── SkillRanks
            ↓
FRPGSkillRequirementProjectionService
            ↓
SkillId + GrantedRequirementIds par seuil
            ↓
FGridCombatActionCatalogContext::SatisfiedRequirements
            +
FRPGClassProgressionTransactionService
    -> niveau / talents / GrantedRequirementIds
            ↓
FGridCombatActionCatalog::Build()
            ↓
Definition.Requirements[]
            ↓
FGridAvailableCombatAction
    bEnabled
    AvailabilityReason
    DisabledReason
    MissingRequirements[]
```

Aucun second catalogue, registre de personnages, système Talent ou hotbar n'est introduit.

---

## 3. Intégration Skill dans le TurnManager

`UGridTurnManagerComponent::GetAvailableCombatActions()` possédait déjà le `FGridCharacterInventoryState` autoritaire et construisait :

```text
Context.SatisfiedRequirements
```

à partir de :

```text
Character.ClassId
ItemTags des équipements MainHand / OffHand
```

MON20.8.3 ajoute ensuite :

```cpp
FRPGSkillRequirementProjectionService::AppendSatisfiedRequirements(
    Character,
    Context.SatisfiedRequirements,
    SkillRequirementError);
```

Le catalogue continue ensuite son comportement existant et ajoute les requirements MON15/MON20.7 via :

```cpp
FRPGClassProgressionTransactionService::AppendRuntimeSatisfiedRequirements(
    CharacterId,
    EffectiveContext.SatisfiedRequirements);
```

La projection Talent validée par MON20.7 n'est donc ni déplacée ni refactorée.

---

## 4. Politique d'échec de la projection Skill

`FRPGSkillRequirementProjectionService` est atomique : en cas d'erreur, le `TSet<FName>` reçu reste inchangé.

Dans le TurnManager :

```text
projection Skill valide
    -> SkillId + grants de seuil ajoutés

projection Skill invalide
    -> warning GridActionCatalog
    -> aucun RequirementId Skill partiel ne fuite
    -> ClassId / ItemTags déjà présents restent intacts
    -> requirements Talent ajoutés ensuite par le catalogue restent intacts
```

Le comportement est donc **fail-closed pour les seules capacités Skill concernées**, sans invalider les autres sources de requirements.

Diagnostic runtime :

```text
[GridActionCatalog] SkillRequirementProjectionFailed
Character=<index>
CharacterId=<guid>
Error=<raison>
```

---

## 5. Diagnostic structuré MissingRequirements

`FGridAvailableCombatAction` expose désormais :

```cpp
TArray<FName> MissingRequirements;
```

Le tableau est `Transient` et `BlueprintReadOnly`.

Il est construit par le catalogue à partir de :

```text
Definition.Requirements - EffectiveContext.SatisfiedRequirements
```

Règles :

```text
- aucune mutation de Definition.Requirements
- aucun doublon
- tri lexical déterministe
- tableau vide si tous les RequirementIds sont satisfaits
- calcul effectué indépendamment de la raison primaire d'indisponibilité
```

Ce dernier point est important. Une action peut par exemple manquer à la fois :

```text
PA insuffisants
+ Skill_Lockpicking absent
```

La raison primaire reste :

```text
InsufficientActionPoints
```

pour préserver l'ordre de décision MON12, tandis que :

```text
MissingRequirements = [Skill_Lockpicking]
```

reste disponible pour le diagnostic/UI.

---

## 6. UI / hotbar

Aucun nouveau système UI n'est créé dans MON20.8.3.

Le comportement existant reste :

```text
binding hotbar
    -> identité conservée
    -> action re-résolue contre le catalogue courant
    -> action verrouillée reste connue mais bEnabled=false
    -> clic -> RequestCharacterCombatAction()
    -> disponibilité recalculée
```

`MissingRequirements[]` enrichit le modèle déjà consommable par Blueprint/UI mais n'impose pas l'affichage de `FName` bruts au joueur.

Les libellés utilisateur détaillés appartiennent à la couche de présentation ; le fallback `DisabledReason` générique reste valide.

---

## 7. Invariants préservés

```text
FGridCombatActionDefinition::Requirements reste le contrat de gating
FGridCombatActionCatalog reste l'autorité de disponibilité
TurnManager recalcule la disponibilité à chaque requête
Talent == ProgressionChoice MON15/MON20.7
Skill RequirementId != Skill Check numérique
Status Effects restent orthogonaux aux RequirementIds
hotbar MON12 reste unique
aucune migration SaveGame
CurrentSaveVersion reste 7
aucun .uasset / .umap requis
```

Le blocage de sorts MON16 continue par exemple à utiliser son `DisabledReason` spécifique après construction du catalogue et n'est pas converti artificiellement en RequirementId.

---

## 8. Fichiers modifiés

```text
Source/GrimrockPrototype/Public/Runtime/Combat/GridCombatTypes.h
Source/GrimrockPrototype/Private/Runtime/Combat/GridCombatActionCatalog.cpp
Source/GrimrockPrototype/Private/Runtime/Combat/GridTurnManagerPlayerActionCatalog.cpp
Source/GrimrockPrototype/Private/Tests/RPGMON2083ActionRequirementIntegrationTests.cpp
```

Documentation :

```text
docs/Design/MON20_8_3_COMBAT_ACTION_REQUIREMENT_INTEGRATION_DIAGNOSTICS.md
```

Aucun asset Unreal n'est nécessaire pour cette tranche.

---

## 9. Automation

Filtre :

```text
Grimrock.MON20.8.ActionRequirements
```

Tests :

```text
MissingRequirementDiagnostic
MissingRequirementsDeterministicOrder
RequirementsSatisfied
SkillIdUnlocksAction
SkillThresholdUnlocksAction
SkillAndExistingRequirementsCompose
SkillProjectionFailureAtomic
NonRequirementFailureKeepsDiagnostic
```

Attendu :

```text
8 / 8 Success
0 Fail
0 Error
```

### Couverture

`MissingRequirementDiagnostic`
: une action verrouillée expose précisément le RequirementId absent.

`MissingRequirementsDeterministicOrder`
: doublons supprimés et ordre stable.

`RequirementsSatisfied`
: requirements satisfaits -> action active et aucun diagnostic manquant.

`SkillIdUnlocksAction`
: tout rang Skill positif peut déverrouiller une action conditionnée par `SkillId`.

`SkillThresholdUnlocksAction`
: un grant data-driven de rang atteint déverrouille l'action.

`SkillAndExistingRequirementsCompose`
: les requirements déjà présents et ceux issus des Skills se composent sans collision.

`SkillProjectionFailureAtomic`
: une projection Skill invalide ne contamine pas les requirements déjà valides et l'action Skill échoue fermée.

`NonRequirementFailureKeepsDiagnostic`
: une raison primaire antérieure, telle que les PA insuffisants, ne masque pas la liste structurée des requirements absents.

---

## 10. Critères de sortie

```text
[ ] GrimrockPrototypeEditor compile sous UE5.5.4
[ ] Grimrock.MON20.8.ActionRequirements = 8/8 Success
[ ] SkillId positif déverrouille une action Skill-gated
[ ] grant de seuil atteint déverrouille une action
[ ] requirements classe / équipement / Skill / Talent restent composables
[ ] projection Skill invalide ne fuite aucun grant partiel
[ ] MissingRequirements est dédupliqué et déterministe
[ ] raison primaire MON12 conservée en présence d'autres failures
[ ] aucune migration SaveGame
[ ] aucun .uasset/.umap
```

Après validation : **MON20.8.4 — Skills/Talents Page Read Model & Menu Integration**.
