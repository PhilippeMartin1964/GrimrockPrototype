# MON20.8.4 — Skills/Talents Page Read Model & Menu Integration

Statut : **IMPLÉMENTÉ — VALIDATION UE5.5.4 À FAIRE**  
Date : **24 août 2026**  
Jalon parent : **MON20.8 — Cross-System Requirements / Actions / UI**

---

## 1. Objectif

Relier la page existante `WBP_GridSkills` au modèle runtime Skills/Talents sans créer de second système de progression ni de sélection de personnage.

La page est strictement **read-only** pour MON20.8 :

```text
Skills
    -> URPGSkillAsset + SkillRanks MON20.6

Talents
    -> ProgressionChoices MON15 / MON20.7

Personnage affiché
    -> UGridPartyInventoryComponent::SelectedCharacterIndex
```

Aucun point de compétence, achat de Skill ou arbre de talent parallèle n'est introduit.

---

## 2. View model

`GridSkillsUiTypes.h` ajoute :

```text
FGridSkillEntryView
    SkillId
    DisplayName
    Description
    GoverningAttribute
    Rank
    MaxRank
    bTrained

FGridTalentEntryView
    ChoiceId
    DisplayName
    Description
    MinimumLevel
    PointCost
    bSelected

FGridSkillsPageView
    CharacterIndex
    CharacterId
    CharacterName
    Skills[]
    Talents[]
    GrantedTalentPoints
    SpentTalentPoints
    RemainingTalentPoints
```

Les identités métier restent donc :

```text
Skill  -> SkillId
Talent -> ChoiceId
```

---

## 3. Service read-only

`FGridSkillsPageService` est sans état.

Il fournit :

```text
TryBuildCharacterView()
TryBuildSelectedCharacterView()
ResolveCanonicalSkillDefinitions()
```

La résolution canonique des Skills utilise :

```text
PrimaryAssetType = RPGSkill
PrimaryAssetName = SkillId
```

et charge les définitions dans un ordre déterministe par `SkillId`.

Le service :

- valide le personnage et son `CharacterId` ;
- valide l'état sparse des `SkillRanks` ;
- exige une définition canonique pour tout Skill entraîné ;
- expose également les Skills non entraînés avec `Rank = 0` ;
- réutilise `FRPGTalentRuntimeService` pour les talents acquis ;
- réutilise le solde de points MON15 ;
- ne mutile jamais le personnage, les Skills ou la progression.

Une erreur laisse `FGridSkillsPageView` invalide et vide : aucune vue partielle n'est publiée.

---

## 4. Widget natif

`UGridSkillsWidget : UUserWidget` est le bridge natif prévu pour `WBP_GridSkills`.

API :

```text
InitializeSkillsWidget(PartyPawn)
RefreshSkills()
GetSkillEntryCount()
GetSkillEntry()
GetTalentEntryCount()
GetTalentEntry()
```

État Blueprint-readable :

```text
OwningPartyPawn
InventoryComponent
View
OnSkillsRefreshed
```

Le widget écoute :

```text
UGridPartyInventoryComponent::OnPartyInventoryChanged
```

Un changement de `SelectedCharacterIndex` déclenche déjà cette notification dans l'inventaire autoritaire ; la page suit donc automatiquement le personnage sélectionné sans stocker son propre index.

---

## 5. Intégration GrimrockMenu

`UGrimrockMenuWidget` ajoute :

```text
RefreshSkills()
GetSkillsWidget()
```

et initialise `UGridSkillsWidget` avec le même `AGrimrockPartyPawn` que l'inventaire et le Spellbook.

Lorsque l'onglet Skills devient actif :

```text
SetActiveTopTab(Skills)
    -> RefreshSkills()
```

### Compatibilité WBP

`Page_Skills` reste temporairement déclaré :

```text
TObjectPtr<UWidget> Page_Skills
```

Le shell obtient le widget spécialisé par :

```text
Cast<UGridSkillsWidget>(Page_Skills)
```

Cette décision évite de casser `WBP_GrimrockMenu` tant que `WBP_GridSkills` n'a pas encore été reparenté dans l'éditeur Unreal.

Après validation C++/Automation, l'étape asset sera :

```text
WBP_GridSkills
    -> Reparent Blueprint
    -> UGridSkillsWidget
```

Aucun autre Blueprint de menu n'a besoin d'être remplacé.

---

## 6. Autorités préservées

```text
FGridPartyInventoryState reste l'autorité du groupe
SelectedCharacterIndex reste l'autorité de sélection
SkillRanks restent dans FGridCharacterInventoryState
URPGSkillAsset reste la définition Skill
Talent == ProgressionChoice MON15/MON20.7
FRPGTalentRuntimeService reste le read model Talent
URPGLevelUpWidget reste le workflow d'acquisition des talents
aucune monnaie de Skill n'est inventée
aucun SaveGame v8 dans MON20.8
```

---

## 7. Fichiers

Production :

```text
Source/GrimrockPrototype/Public/UI/GridSkillsUiTypes.h
Source/GrimrockPrototype/Public/UI/GridSkillsPageService.h
Source/GrimrockPrototype/Private/UI/GridSkillsPageService.cpp
Source/GrimrockPrototype/Public/UI/GridSkillsWidget.h
Source/GrimrockPrototype/Private/UI/GridSkillsWidget.cpp
Source/GrimrockPrototype/Public/UI/GrimrockMenuWidget.h
Source/GrimrockPrototype/Private/UI/GrimrockMenuWidget.cpp
```

Tests :

```text
Source/GrimrockPrototype/Private/Tests/RPGMON2084SkillsPageReadModelTests.cpp
```

Documentation :

```text
docs/Design/MON20_8_4_SKILLS_TALENTS_PAGE_READ_MODEL_MENU_INTEGRATION.md
```

---

## 8. Automation

Filtre :

```text
Grimrock.MON20.8.SkillsPage
```

Tests :

```text
SelectedCharacterIdentity
SkillRanks
DeterministicSkillOrder
TalentProjection
TalentPointBalance
SelectedCharacterAuthority
DuplicateDefinitionAtomic
MissingRankDefinitionAtomic
```

Attendu :

```text
8 / 8 Success
0 Fail
0 Error
```

La campagne cumulative MON20.8 attend alors :

```text
SkillRequirements  8
ActionRequirements 8
SkillsPage         8
---------------------
Total             24
```

---

## 9. Validation asset / PIE après Automation

Une fois le C++ et les 8 tests validés :

1. reparent `WBP_GridSkills` vers `UGridSkillsWidget` ;
2. compiler et sauvegarder le Blueprint ;
3. ouvrir le menu en PIE ;
4. sélectionner l'onglet Compétences ;
5. vérifier que le personnage sélectionné est affiché ;
6. changer de personnage et vérifier le refresh ;
7. vérifier Skills entraînés/non entraînés et talents acquis.

Le reparent est volontairement différé après validation C++ afin d'éviter de versionner un `.uasset` cassé si le contrat natif nécessite une correction.

---

## 10. Critères de sortie

```text
[ ] GrimrockPrototypeEditor compile sous UE5.5.4
[ ] Grimrock.MON20.8.SkillsPage = 8/8 Success
[ ] Grimrock.MON20.8 = 24/24 Success
[ ] sélection autoritaire suivie sans index parallèle
[ ] Skills entraînés et non entraînés projetés
[ ] talents acquis projetés via MON20.7
[ ] solde de points Talent identique à MON15
[ ] aucune mutation par la page
[ ] WBP_GridSkills reparenté vers UGridSkillsWidget
[ ] PIE onglet Compétences validé
```

Après validation complète : **MON20.8.5 — Automation / PIE Regression & Closure**.
