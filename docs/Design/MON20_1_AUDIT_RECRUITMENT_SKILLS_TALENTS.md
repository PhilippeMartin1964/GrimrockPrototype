# MON20.1 — Audit Recruitment / Skills / Talents

Statut : **AUDIT TERMINÉ — contrat d’implémentation défini**  
Date : **23 août 2026**  
Jalon parent : **MON20 — Recruitment / Skills / Talents**

## 1. Objectif

Identifier ce qui existe réellement avant d’ajouter recrutement, compétences et talents, afin d’éviter une architecture parallèle aux systèmes Character Creation, Party Inventory, MON12, MON15, MON16 et MON18.

## 2. Constat principal

MON20 ne part pas de zéro.

### 2.1 Recrutement déjà conçu dans CC7

Le document :

```text
docs/Design/CHARACTER_CREATION_CC7_WIZARD_RECRUITMENT.md
```

avait déjà défini :

```text
MainHero
CustomRecruit
StoryCompanion
TemporaryGuest
```

ainsi que :

- recrutement de compagnon scénarisé ;
- recrue personnalisable ;
- contexte de création ;
- écran de recrutement ;
- future réserve.

MON20 doit reprendre ce cadrage au lieu d’introduire un second modèle.

### 2.2 Le wizard de création existe réellement

`URPGCharacterCreationWizardWidget` implémente déjà un wizard C++/UMG avec les étapes :

```text
Race
Class
Attributes
Identity
Summary
```

Il valide race, classe, allocation d’attributs et nom avant création.

MON20 ne doit donc pas reconstruire le wizard ; il devra le rendre réutilisable pour les contextes de recrutement personnalisable lorsque cette tranche sera abordée.

### 2.3 Le groupe supporte déjà plusieurs personnages actifs

`UGridPartyInventoryComponent` expose :

```text
DefaultMaxActiveCharacters = 6
GetActiveCharacterCount()
GetMaxActiveCharacters()
GetSelectedCharacterIndex()
SetSelectedCharacterIndex()
GetCharacterSummary()
```

L’état par personnage est déjà très riche dans `FGridCharacterInventoryState` :

```text
CharacterId
DisplayName
RaceId / RaceDisplayName
ClassId / ClassDisplayName / ClassDefinition
Level / Experience
Attributes / DerivedStats
StatusEffects
Portrait / ClassIcon
InventorySlots
CombatHotbarSlots
```

Conclusion : **ne pas créer un nouveau `PartyMemberState` parallèle** pour la première tranche. Le recrutement doit ajouter un `FGridCharacterInventoryState` correctement initialisé dans l’autorité existante.

### 2.4 Lacune réelle : aucune transaction publique de recrutement

L’API publique possède `CreateInitialCharacter(...)`, mais pas de contrat générique du type :

```text
CanRecruitPartyMember
TryRecruitPartyMember
Remove/DismissPartyMember
```

C’est la première lacune fonctionnelle à combler.

### 2.5 Les talents possèdent déjà un socle MON15

`URPGClassAsset` contient :

```cpp
FRPGClassProgressionLevelGrant
FRPGClassProgressionChoiceDefinition
```

Une `FRPGClassProgressionChoiceDefinition` possède déjà :

```text
ChoiceId
DisplayName
Description
MinimumLevel
PointCost
PrerequisiteChoiceIds
GrantedRequirementIds
```

`FRPGClassProgressionTransactionService` fournit déjà :

```text
TryGetSelectedChoiceIds
TryGetChoicePointBalance
TryCommitChoices
AppendRuntimeSatisfiedRequirements
CapturePersistentState
RestorePersistentState
```

Décision MON20 : **ne pas créer immédiatement un second système `TalentId/TalentPoints`**. Les talents de classe devront d’abord être modélisés comme une extension/présentation des `ProgressionChoices` existants, sauf preuve qu’un besoin de gameplay ne peut pas être exprimé par ce contrat.

### 2.6 Les compétences n’existent pas comme domaine dédié

Aucun contrat `SkillId` / `TalentId` autonome n’est actuellement présent dans le code de production.

Il existe cependant déjà le mécanisme générique de `GrantedRequirementIds`, consommé par le catalogue d’actions MON12. Ce mécanisme constitue un bon point d’intégration futur pour les compétences/talents sans coupler le combat à MON20.

## 3. Décisions architecturales MON20

### D1 — Autorité du groupe

L’autorité reste :

```text
UGridPartyInventoryComponent
    -> FGridPartyInventoryState
        -> FGridCharacterInventoryState[]
```

Pas de second registre de personnages actifs.

### D2 — Identité stable

`CharacterId` reste l’identité persistante primaire du membre du groupe.

### D3 — Nature du membre

MON20 introduira la notion déjà prévue par CC7 :

```text
MainHero
CustomRecruit
StoryCompanion
TemporaryGuest
```

Cette information doit être stockée avec l’état du personnage, pas déduite de son nom/portrait/classe.

### D4 — Recrutement transactionnel

Un recrutement doit être atomique :

```text
Definition / Request
    -> validation
    -> capacité du groupe
    -> construction complète du CharacterState
    -> initialisation classe/race/stats/inventaire/hotbar
    -> commit unique
    -> notification
```

Aucune mutation partielle si une validation échoue.

### D5 — Pas de réserve dans la première tranche

Le groupe actif accepte actuellement jusqu’à 6 personnages.

MON20.2 refusera proprement le recrutement lorsque le groupe est plein. La réserve sera introduite dans une tranche ultérieure, après validation du recrutement actif.

### D6 — Talents = progression existante d’abord

Les talents de classe doivent réutiliser `FRPGClassProgressionChoiceDefinition` et `FRPGClassProgressionTransactionService` avant toute nouvelle abstraction.

### D7 — Skills séparés seulement si nécessaire

Un modèle Skill dédié sera introduit uniquement pour les compétences qui nécessitent :

- rang numérique ;
- progression indépendante de la classe ;
- tests de compétence hors combat ;
- effets/passifs non exprimables proprement par les RequirementIds existants.

## 4. Roadmap MON20 proposée

```text
MON20.1 — Audit & Architecture Contract                  TERMINÉ
MON20.2 — Active Party Recruitment Foundation           PROCHAIN
MON20.3 — Story Companion Definition / Recruitment UI
MON20.4 — Custom Recruit / Wizard Context Reuse
MON20.5 — Skills Data Model & Runtime
MON20.6 — Talents / Progression Choice Integration
MON20.7 — Cross-System Requirements / Actions / UI
MON20.8 — Persistence / Migration / Reserve
MON20.9 — Balance / Regression / Closure
```

Cette séquence pourra être ajustée si l’implémentation révèle qu’une tranche doit être divisée.

## 5. Contrat MON20.2 — prochaine étape

La première implémentation doit rester petite.

### Scope

Introduire :

1. `ERPGPartyMemberKind` ;
2. stockage de `PartyMemberKind` dans `FGridCharacterInventoryState` ;
3. une requête de recrutement C++ minimale pour un personnage déjà défini ;
4. une transaction `TryRecruitPartyMember(...)` sur l’autorité existante ;
5. validation du maximum de 6 personnages actifs ;
6. génération/validation d’un `CharacterId` stable ;
7. initialisation par les mêmes règles que la création initiale ;
8. notification `OnPartyInventoryChanged` après commit seulement ;
9. tests Automation dédiés.

### Hors scope MON20.2

Ne pas encore ajouter :

- UI de recrutement ;
- dialogue PNJ ;
- coût en or ;
- réserve ;
- renvoi d’un compagnon ;
- compétences ;
- nouveaux talents ;
- nouvelle migration SaveGame si elle n’est pas indispensable à cette tranche de modèle/runtime.

## 6. Critères de sortie MON20.2

```text
recrutement valide                 -> ajoute exactement un membre
CharacterId                        -> valide et unique
classe/race/stats                  -> initialisés par les règles existantes
inventaire/hotbar                  -> initialisés correctement
party count                        -> incrémenté de 1
party pleine                       -> rejet sans mutation
requête invalide                   -> rejet sans mutation
notification                       -> une fois après commit
ancien CreateInitialCharacter      -> non régressé
```

## 7. Conclusion

La première tâche de MON20 n’est pas de construire un grand « système de talents ».

Le socle le plus manquant et le moins risqué est le **recrutement transactionnel d’un nouveau membre dans `UGridPartyInventoryComponent`**, en réutilisant les données de personnage et les règles déjà en production.
