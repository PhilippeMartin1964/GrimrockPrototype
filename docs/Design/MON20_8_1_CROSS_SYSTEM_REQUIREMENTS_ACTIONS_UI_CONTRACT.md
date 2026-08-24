# MON20.8.1 — Cross-System Requirements / Actions / UI — Audit & Architecture Contract

Statut : **TERMINÉ — contrat d’implémentation défini**  
Date : **24 août 2026**  
Jalon parent : **MON20.8 — Cross-System Requirements / Actions / UI**

---

## 1. Objectif

Relier proprement les compétences MON20.6 et les talents MON20.7 aux systèmes transversaux déjà présents :

- `RequirementIds` ;
- catalogue d’actions MON12 ;
- hotbar / palette d’actions ;
- page `Compétences` du menu joueur ;
- diagnostics UI de disponibilité.

La contrainte principale est de **réutiliser les contrats existants**. MON20.8 ne doit créer ni second catalogue d’actions, ni second système de requirements, ni seconde hotbar, ni second registre de personnages.

---

## 2. Audit du pipeline Requirement -> Action existant

Le projet possède déjà un pipeline fonctionnel :

```text
Sources d’actions
    ├── URPGClassAsset.CombatActions
    ├── UGridItemDefinitionAsset.CombatActions
    ├── attaques legacy équipement
    ├── action universelle Unarmed
    ├── QuickItems
    └── Spellbook
            ↓
UGridTurnManagerComponent::BuildPlayerCombatActionContributions()
            ↓
UGridTurnManagerComponent::GetAvailableCombatActions()
            ↓
FGridCombatActionCatalogContext
    SatisfiedRequirements
            ↓
FGridCombatActionCatalog::Build()
            ↓
FGridCombatActionDefinition::Requirements[]
            ↓
FGridAvailableCombatAction
    bEnabled
    AvailabilityReason
    DisabledReason
            ↓
Combat HUD / palette / hotbar
```

Le catalogue MON12 effectue déjà la comparaison :

```text
pour chaque Definition.Requirements
    si absent de Context.SatisfiedRequirements
        -> MissingRequirement
```

Conclusion : **MON20.8 doit alimenter le pipeline existant, pas le remplacer**.

---

## 3. Producteurs de requirements déjà présents

### 3.1 Classe

`UGridTurnManagerComponent::GetAvailableCombatActions()` ajoute déjà :

```text
Character.ClassId
```

à `Context.SatisfiedRequirements`.

Une action peut donc être conditionnée directement par un `ClassId`.

### 3.2 Équipement

Les `ItemTags` des objets équipés en MainHand / OffHand sont déjà projetés dans :

```text
Context.SatisfiedRequirements
```

Une action peut donc dépendre d’un tag d’équipement sans système additionnel.

### 3.3 Niveau / talents MON15-MON20.7

`FGridCombatActionCatalog::Build()` appelle déjà :

```text
FRPGClassProgressionTransactionService::AppendRuntimeSatisfiedRequirements(
    CharacterId,
    SatisfiedRequirements)
```

MON20.7 a verrouillé les règles suivantes :

```text
ProgressionLevelGrant.GrantedRequirementIds
    -> RequirementIds satisfaits

Talent acquis / ProgressionChoice
    -> ChoiceId satisfait
    -> GrantedRequirementIds satisfaits
```

Donc **les talents peuvent déjà verrouiller/déverrouiller des actions de combat** via `FGridCombatActionDefinition::Requirements`.

Aucune nouvelle intégration Talent -> CombatAction n’est nécessaire.

### 3.4 Status Effects

MON16 applique certaines interdictions après construction du catalogue, par exemple :

```text
bBlockSpellActions
    -> action Spell désactivée
    -> AvailabilityReason = MissingRequirement
    -> DisabledReason spécifique
```

Cette logique représente une **restriction temporaire de contrôle**, pas une progression permanente.

Décision : ne pas convertir les Status Effects en RequirementIds. Le système MON16 reste orthogonal.

---

## 4. Lacune principale : Skills -> RequirementIds

MON20.6 a créé :

```text
URPGSkillAsset
FRPGSkillRank
FRPGSkillService
FRPGSkillCheckService
FRPGSkillRuntimeService
```

mais aucun de ces éléments n’alimente encore :

```text
TSet<FName> SatisfiedRequirements
```

C’était explicitement reporté à MON20.8 dans le contrat MON20.6.1.

Conséquence actuelle :

```text
SkillRank = 3
```

peut être lu et utilisé dans un jet de compétence, mais ne peut pas encore rendre automatiquement disponible une action dont `Requirements` dépend de cette compétence.

---

## 5. Identité canonique des Skill definitions

`URPGSkillAsset` est déjà un `UPrimaryDataAsset`, mais il ne possède actuellement ni :

```text
GetPrimaryAssetId() explicite basé sur SkillId
```

ni résolveur canonique :

```text
SkillId -> URPGSkillAsset
```

C’est gênant pour deux besoins MON20.8 :

1. reconstruire les RequirementIds dérivés de tous les rangs d’un personnage ;
2. construire la page UI Compétences à partir des définitions canoniques.

### Décision

MON20.8.2 introduira une identité PrimaryAsset explicite :

```text
PrimaryAssetType = RPGSkill
PrimaryAssetName = SkillId
```

et un résolveur/catalogue léger utilisant `UAssetManager`, sur le même principe que le resolver des Status Effects.

Le `SkillId` reste l’identité métier. Le nom physique du `.uasset` n’est pas l’autorité.

---

## 6. Contrat de projection des Skills

Une compétence numérique doit pouvoir produire des capabilities binaires consommables par le catalogue d’actions sans remplacer les jets de compétence.

### 6.1 Nouveau profil data-driven

MON20.8.2 ajoutera à `URPGSkillAsset` un profil de grants par rang :

```text
FRPGSkillRequirementGrant
    MinimumRank
    GrantedRequirementIds[]
```

Exemple conceptuel :

```text
SkillId = Skill_Lockpicking
Rank >= 1
    -> Skill_Lockpicking          (grant automatique du SkillId)

MinimumRank = 2
    -> Req_Lockpicking_Advanced

MinimumRank = 4
    -> Req_Lockpicking_Master
```

### 6.2 Règle automatique

Pour tout rang strictement positif :

```text
SkillId lui-même devient un RequirementId satisfait
```

Cela permet une condition simple « compétence entraînée » sans authoring supplémentaire.

### 6.3 Grants par seuil

Pour chaque `FRPGSkillRequirementGrant` :

```text
CharacterRank >= MinimumRank
    -> ajouter GrantedRequirementIds
```

Aucun parsing de nom tel que `Skill_X_Rank3` n’est imposé au runtime. Les seuils restent explicitement data-driven.

### 6.4 Service retenu

Créer un service pur/sans état :

```text
FRPGSkillRequirementProjectionService
```

Responsabilités :

```text
Resolve SkillId -> definition canonique
Validate definition/rank
CollectSatisfiedRequirements(CharacterState, OutRequirements)
```

Les RequirementIds sont **dérivés**, jamais stockés dans `FGridCharacterInventoryState` et jamais persistés directement.

---

## 7. Intégration au catalogue de combat

Le catalogue MON12 reste l’autorité de disponibilité.

MON20.8.3 doit enrichir le contexte avant évaluation :

```text
ClassId
+ ItemTags équipement
+ Skill RequirementIds
+ MON15/MON20.7 RequirementIds
        ↓
FGridCombatActionCatalog::Build()
```

Pour limiter le risque de régression, MON20.8 **ne refactorera pas** le branchement Talent déjà validé dans `FGridCombatActionCatalog::Build()`.

La projection Skill sera ajoutée au niveau du TurnManager, qui possède déjà le `FGridCharacterInventoryState` nécessaire.

### Invariant d’exécution

L’UI ne constitue jamais l’autorité :

```text
hotbar click
    -> RequestCharacterCombatAction()
    -> disponibilité recalculée par le TurnManager/catalogue
    -> exécution seulement si l’action est encore valide
```

Un changement de rang, de talent, d’équipement, de mana, de PA ou de status ne doit donc jamais contourner la validation runtime.

---

## 8. Hotbar et palette : aucun nouveau système

L’audit de `UGridCombatHudWidget` confirme que le modèle actuel est déjà adapté aux actions verrouillées :

- un binding hotbar conserve son identité ;
- `BuildHotbarActions()` le résout à nouveau contre le catalogue courant ;
- une action résolue mais indisponible reste connue ;
- `DisabledReason` est propagé ;
- le slot est atténué via `UnavailableSlotOpacity` ;
- le tooltip explique l’indisponibilité ;
- la même action devient automatiquement exécutable dès que ses requirements sont satisfaits.

Décision : **ne pas créer une hotbar Skill/Talent séparée**.

Une action verrouillée peut rester assignable/configurable, ce qui permet au joueur de préparer sa barre avant de satisfaire le requirement.

---

## 9. Diagnostics de requirements

`FGridAvailableCombatAction` expose actuellement :

```text
AvailabilityReason = MissingRequirement
DisabledReason = texte générique
```

mais pas la liste structurée des requirements manquants.

MON20.8.3 ajoutera un diagnostic transient :

```text
MissingRequirements[]
```

construit de façon déterministe depuis :

```text
Definition.Requirements - SatisfiedRequirements
```

Objectifs :

- Automation plus précise ;
- Blueprint/UI capable d’expliquer une action verrouillée ;
- diagnostics de développement ;
- aucune modification de l’identité ou de la transaction d’action.

Le texte utilisateur peut rester générique lorsqu’aucun label métier n’est disponible. Les `FName` bruts ne doivent pas être imposés comme texte final au joueur.

---

## 10. Page Compétences : état actuel

Le menu contient déjà :

```text
Page_Skills = WBP_GridSkills
```

mais côté C++ :

```text
UGrimrockMenuWidget::Page_Skills
    -> TObjectPtr<UWidget>
```

Il n’existe actuellement ni :

```text
UGridSkillsWidget
InitializeSkillsWidget()
RefreshSkills()
GetSkillsWidget()
```

Contrairement à `WBP_GridSpellbook`, la page Skills n’est donc encore reliée à aucun modèle runtime spécialisé.

---

## 11. Contrat UI retenu

MON20.8.4 créera une page native de présentation :

```text
UGridSkillsWidget
    -> WBP_GridSkills
```

avec un view model read-only :

```text
FGridSkillsPageView
    CharacterIndex
    CharacterId
    CharacterName
    Skills[]
    Talents[]
```

### Vue Skill

```text
SkillId
DisplayName
Description
GoverningAttribute
Rank
MaxRank
bTrained
```

### Vue Talent

Réutiliser `FRPGTalentRuntimeService` pour afficher :

```text
ChoiceId
DisplayName
Description
PointCost
bSelected
```

### Autorité de sélection

La page utilise exclusivement :

```text
UGridPartyInventoryComponent::SelectedCharacterIndex
```

Aucun index parallèle dans le shell.

### Mutation

MON20.8.4 reste **read-only côté progression** :

- acquisition des talents -> `URPGLevelUpWidget` MON15/MON20.7 ;
- mutation brute des SkillRanks -> APIs MON20.6 réservées au runtime/tests tant qu’aucune économie de points de compétence n’est définie ;
- aucun bouton UI ne doit inventer une monnaie de Skills.

Le menu ajoutera seulement :

```text
InitializeSkillsWidget()
RefreshSkills()
```

sur le même principe que Spellbook.

---

## 12. Skill checks hors combat restent distincts des RequirementIds

Il est important de ne pas confondre :

```text
RequirementId
    -> condition binaire / capability / unlock

FRPGSkillCheckService
    -> test numérique d20 + rank + attribut contre difficulté
```

Exemple :

```text
Skill_Lockpicking présent
    -> autorise éventuellement une action « Crocheter »

puis

Lockpicking check DC 15
    -> décide si la serrure est effectivement ouverte
```

MON20.8 ne remplace donc pas les futurs jets de crochetage, pièges, perception ou exploration par des tags binaires.

---

## 13. Persistance

MON20.8 n’incrémente pas le SaveGame.

```text
UGrimrockPartySaveGame::CurrentSaveVersion = 7
```

reste inchangé.

Les RequirementIds Skill sont toujours recalculés depuis les rangs runtime.

La persistance des `SkillRanks` reste réservée à :

```text
MON20.9 — Reserve / Persistence / Migration
```

Après chargement futur :

```text
SkillRanks restaurés
    -> RequirementIds recalculés
```

Aucun snapshot de RequirementIds n’est nécessaire.

---

## 14. Découpage retenu pour MON20.8

```text
MON20.8.1 — Audit & Architecture Contract                         TERMINÉ
MON20.8.2 — Skill Definition Identity & Requirement Projection    PROCHAIN
MON20.8.3 — Combat Action Requirement Integration & Diagnostics
MON20.8.4 — Skills/Talents Page Read Model & Menu Integration
MON20.8.5 — Automation / PIE Regression & Closure
```

### MON20.8.2

- `URPGSkillAsset::GetPrimaryAssetId()` ;
- resolver/catalogue `SkillId -> URPGSkillAsset` ;
- `FRPGSkillRequirementGrant` ;
- projection du `SkillId` et des grants par seuil ;
- tests purs.

### MON20.8.3

- ajout de la projection Skills au contexte du TurnManager ;
- requirements Skill + Talent + classe + équipement combinés ;
- `MissingRequirements[]` transient ;
- régression hotbar/action catalogue.

### MON20.8.4

- `UGridSkillsWidget` ;
- view model Skills + Talents ;
- sélection via l’inventaire autoritaire ;
- intégration `UGrimrockMenuWidget` ;
- reparent de `WBP_GridSkills` uniquement lorsque la tranche C++ est validée.

### MON20.8.5

- Automation cumulative ;
- PIE page Compétences ;
- PIE action verrouillée -> déverrouillée ;
- clôture documentaire.

---

## 15. Invariants MON20.8

```text
FGridCombatActionDefinition::Requirements reste le contrat consommateur
FGridCombatActionCatalog reste l’autorité de disponibilité
hotbar MON12 reste unique
SelectedCharacterIndex reste l’autorité UI de sélection
Talent == ProgressionChoice MON15
SkillRank reste dans FGridCharacterInventoryState
Skill check numérique != RequirementId binaire
RequirementIds dérivés ne sont pas persistés
SaveGame reste v7 pendant MON20.8
aucun système parallèle de requirements/actions/personnages
```

---

## 16. Fichiers probables des tranches suivantes

```text
Source/GrimrockPrototype/Public/RPG/RPGSkillAsset.h
Source/GrimrockPrototype/Private/RPG/RPGSkillAsset.cpp
Source/GrimrockPrototype/Public/RPG/RPGSkillRequirementProjectionService.h
Source/GrimrockPrototype/Private/RPG/RPGSkillRequirementProjectionService.cpp
Source/GrimrockPrototype/Public/Runtime/Combat/GridCombatTypes.h
Source/GrimrockPrototype/Private/Runtime/Combat/GridCombatActionCatalog.cpp
Source/GrimrockPrototype/Private/Runtime/Combat/GridTurnManagerPlayerActionCatalog.cpp
Source/GrimrockPrototype/Public/UI/GridSkillsWidget.h
Source/GrimrockPrototype/Private/UI/GridSkillsWidget.cpp
Source/GrimrockPrototype/Public/UI/GrimrockMenuWidget.h
Source/GrimrockPrototype/Private/UI/GrimrockMenuWidget.cpp
Source/GrimrockPrototype/Private/Tests/RPGMON208*.cpp
```

Aucun de ces fichiers de production n’est modifié par MON20.8.1.

---

## 17. Validation de MON20.8.1

Cette tranche est un audit/contrat uniquement.

```text
Compilation UE : non requise
Automation     : non requise
PIE            : non requis
.uasset/.umap  : aucun changement
```

Le prochain travail autoritaire est :

```text
MON20.8.2 — Skill Definition Identity & Requirement Projection
```
